#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

/** CURRENT-style circular terrain XY pad — 3D-ish heightfield + dual orbital cursors */
class MagicPad : public juce::Component, private juce::Timer
{
public:
    std::function<void(float,float,bool)> onChange; // x,y,active

    MagicPad() { startTimerHz (10); }

    void setPosition (float nx, float ny, bool act)
    {
        x = juce::jlimit (0.f, 1.f, nx);
        y = juce::jlimit (0.f, 1.f, ny);
        touching = act;
        repaint();
    }
    void setModeColour (juce::Colour c) { accent = c; repaint(); }
    void setModeName (const juce::String& n) { modeName = n; repaint(); }
    void setAudioPeak (float p) noexcept { peak = juce::jlimit (0.f, 1.f, p); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.f);
        // dark glass
        g.setColour (juce::Colour (0xff0a0c14).withAlpha (0.55f));
        g.fillRoundedRectangle (bounds, 16.f);

        auto circle = getCircleArea();
        const float cx = circle.getCentreX();
        const float cy = circle.getCentreY();
        const float R  = circle.getWidth() * 0.5f;

        // outer glow ring
        g.setColour (accent.withAlpha (0.12f + peak * 0.15f));
        g.drawEllipse (circle.expanded (8.f), 3.f);
        g.setColour (accent.withAlpha (0.45f));
        g.drawEllipse (circle, 2.2f);

        // clip to circle for terrain
        juce::Path clip;
        clip.addEllipse (circle);
        g.saveState();
        g.reduceClipRegion (clip);

        // procedural terrain (CURRENT-style height strips)
        // Keep the depth mesh crisp while reducing per-repaint path work by ~35%.
        const int cols = 40;
        const int rows = 28;
        for (int row = 0; row < rows; ++row)
        {
            const float v = (float) row / (float) (rows - 1); // 0 top .. 1 bottom
            juce::Path strip;
            bool first = true;
            for (int col = 0; col <= cols; ++col)
            {
                const float u = (float) col / (float) cols;
                // height field: multi-octave noise + XY morph + audio pulse
                float h = terrain (u, v, x, y, anim, peak);
                // project to circle with perspective tilt
                float px = circle.getX() + u * circle.getWidth();
                float baseY = circle.getY() + v * circle.getHeight();
                float py = baseY - h * R * 0.42f;
                // only draw inside circle roughly
                float dx = px - cx, dy = py - cy;
                if (dx * dx + dy * dy > R * R * 1.05f)
                    continue;
                if (first) { strip.startNewSubPath (px, py); first = false; }
                else strip.lineTo (px, py);
            }
            if (! first)
            {
                // depth colour: cyan → purple
                auto col = accent.interpolatedWith (juce::Colour (0xffb6a5d7), v);
                col = col.interpolatedWith (juce::Colour (0xff73c9c5), 0.3f + 0.4f * (1.f - v));
                g.setColour (col.withAlpha (0.25f + 0.35f * (1.f - v) + peak * 0.2f));
                g.strokePath (strip, juce::PathStrokeType (1.4f));
            }
        }

        // secondary denser mesh for “crystal” detail near cursor
        {
            const float focusU = x, focusV = 1.f - y;
            for (int i = 0; i < 18; ++i)
            {
                float ang = anim * 0.4f + (float) i / 18.f * juce::MathConstants<float>::twoPi;
                float rad = R * (0.15f + 0.55f * ((float) i / 18.f));
                float px = cx + std::cos (ang) * rad * (0.6f + 0.4f * focusU);
                float py = cy + std::sin (ang) * rad * (0.6f + 0.4f * focusV);
                float h = terrain (focusU, focusV, x, y, anim + i, peak);
                py -= h * 12.f;
                g.setColour (juce::Colours::white.withAlpha (0.08f + peak * 0.1f));
                g.fillEllipse (px - 1.5f, py - 1.5f, 3.f, 3.f);
            }
        }

        g.restoreState();

        // orbital rings (parameter rings like CURRENT)
        for (int ring = 1; ring <= 2; ++ring)
        {
            float rr = R * (0.35f + ring * 0.22f);
            g.setColour (juce::Colours::white.withAlpha (0.12f + (touching ? 0.08f : 0.f)));
            g.drawEllipse (cx - rr, cy - rr, rr * 2.f, rr * 2.f, 1.2f);
        }

        // dual cursors mapped to X/Y on rings
        auto drawCursor = [&] (float ang, float rad, float size)
        {
            float px = cx + std::cos (ang) * rad;
            float py = cy + std::sin (ang) * rad;
            g.setColour (juce::Colours::white.withAlpha (0.2f));
            g.drawEllipse (px - size * 1.6f, py - size * 1.6f, size * 3.2f, size * 3.2f, 1.5f);
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillEllipse (px - size * 0.45f, py - size * 0.45f, size * 0.9f, size * 0.9f);
            g.setColour (accent.withAlpha (0.7f));
            g.drawEllipse (px - size, py - size, size * 2.f, size * 2.f, 2.f);
        };
        // X → angle on outer ring, Y → angle on inner ring
        float angX = x * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
        float angY = y * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
        drawCursor (angX, R * 0.57f, 14.f);
        drawCursor (angY, R * 0.35f, 11.f);

        // free drag cursor (main XY)
        {
            float px = circle.getX() + x * circle.getWidth();
            float py = circle.getY() + (1.f - y) * circle.getHeight();
            // clamp to circle
            float dx = px - cx, dy = py - cy;
            float d = std::sqrt (dx * dx + dy * dy);
            if (d > R * 0.92f && d > 1.f)
            {
                px = cx + dx / d * R * 0.92f;
                py = cy + dy / d * R * 0.92f;
            }
            if (touching)
            {
                g.setColour (accent.withAlpha (0.2f));
                g.fillEllipse (px - 26.f, py - 26.f, 52.f, 52.f);
            }
            g.setColour (juce::Colours::white);
            g.fillEllipse (px - 6.f, py - 6.f, 12.f, 12.f);
            g.setColour (accent);
            g.drawEllipse (px - 10.f, py - 10.f, 20.f, 20.f, 2.f);
        }

        // CURRENT-style labels + connector lines to orbital cursors
        {
            float angX = x * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
            float angY = y * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
            float ox = cx + std::cos (angX) * (R * 0.57f);
            float oy = cy + std::sin (angX) * (R * 0.57f);
            float ix = cx + std::cos (angY) * (R * 0.35f);
            float iy = cy + std::sin (angY) * (R * 0.35f);

            float labelX = bounds.getX() + 14.f;
            float morphY = bounds.getY() + bounds.getHeight() * 0.28f;
            float depthY = bounds.getY() + bounds.getHeight() * 0.48f;

            // connector lines
            g.setColour (accent.withAlpha (0.45f));
            g.drawLine (labelX + 70.f, morphY + 8.f, ox, oy, 1.2f);
            g.setColour (juce::Colour (0xffb6a5d7).withAlpha (0.45f));
            g.drawLine (labelX + 70.f, depthY + 8.f, ix, iy, 1.2f);
            // dots at ends
            g.setColour (accent.withAlpha (0.8f));
            g.fillEllipse (ox - 3.f, oy - 3.f, 6.f, 6.f);
            g.setColour (juce::Colour (0xffb6a5d7).withAlpha (0.8f));
            g.fillEllipse (ix - 3.f, iy - 3.f, 6.f, 6.f);

            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.setFont (juce::FontOptions (12.f, juce::Font::bold));
            g.drawText ("Morph", labelX, morphY, 70.f, 16.f, juce::Justification::centredLeft);
            g.setColour (accent);
            g.setFont (juce::FontOptions (11.f));
            g.drawText (juce::String ((int) std::round (x * 100.f)) + "%", labelX, morphY + 16.f, 70.f, 14.f, juce::Justification::centredLeft);

            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.setFont (juce::FontOptions (12.f, juce::Font::bold));
            g.drawText ("Depth", labelX, depthY, 70.f, 16.f, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xffb6a5d7));
            g.setFont (juce::FontOptions (11.f));
            g.drawText (juce::String ((int) std::round (y * 100.f)) + "%", labelX, depthY + 16.f, 70.f, 14.f, juce::Justification::centredLeft);

            // Mode / Delay-style secondary readouts
            float botY = bounds.getY() + bounds.getHeight() * 0.68f;
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.setFont (juce::FontOptions (11.f, juce::Font::bold));
            g.drawText ("Mode", labelX, botY, 70.f, 14.f, juce::Justification::centredLeft);
            g.setColour (accent.withAlpha (0.85f));
            g.drawText (modeName, labelX, botY + 14.f, 90.f, 14.f, juce::Justification::centredLeft);
        }

        // title
        g.setColour (accent);
        g.setFont (juce::FontOptions (14.f, juce::Font::bold));
        g.drawText ("MAGIC  |  " + modeName,
                    getLocalBounds().removeFromTop (22).reduced (12, 0),
                    juce::Justification::centredLeft);

        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.setFont (juce::FontOptions (10.f));
        g.drawText (touching ? "ACTIVE — drag terrain" : "click & drag  ·  HOLD to latch",
                    getLocalBounds().removeFromBottom (18).reduced (10, 0),
                    juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& e) override { updateFromPos (e.position, true); }
    void mouseDrag (const juce::MouseEvent& e) override { updateFromPos (e.position, true); }
    void mouseUp (const juce::MouseEvent&) override
    {
        touching = false;
        if (onChange) onChange (x, y, false);
        repaint();
    }

    void timerCallback() override
    {
        if (! isShowing()) return;
        anim += 0.045f;
        repaint();
    }

private:
    float x = 0.5f, y = 0.5f, anim = 0.f, peak = 0.f;
    bool touching = false;
    juce::Colour accent { 0xff73c9c5 };
    juce::String modeName { "LOOP" };

    juce::Rectangle<float> getCircleArea() const
    {
        auto r = getLocalBounds().toFloat().reduced (18.f);
        // leave side gutters for labels
        r.removeFromLeft (r.getWidth() * 0.18f);
        r.removeFromRight (r.getWidth() * 0.08f);
        float s = juce::jmin (r.getWidth(), r.getHeight()) * 0.92f;
        return { r.getCentreX() - s * 0.5f, r.getCentreY() - s * 0.5f, s, s };
    }

    static float terrain (float u, float v, float mx, float my, float t, float pk) noexcept
    {
        // layered “spectrum mountain”
        float n1 = std::sin ((u * 9.f + mx * 4.f) + t * 0.7f) * std::cos ((v * 7.f - my * 3.f) + t * 0.5f);
        float n2 = std::sin ((u * 17.f - t) * (0.8f + mx)) * 0.45f;
        float n3 = std::sin ((v * 13.f + u * 5.f + t * 1.2f)) * 0.3f;
        float ridge = 1.f - std::abs (std::sin ((u + v + mx) * 6.f + t * 0.3f));
        float h = 0.35f * n1 + 0.25f * n2 + 0.2f * n3 + 0.25f * ridge;
        h *= 0.55f + 0.45f * my + pk * 0.35f;
        return juce::jlimit (-0.2f, 1.2f, h * 0.5f + 0.35f);
    }

    void updateFromPos (juce::Point<float> p, bool act)
    {
        auto c = getCircleArea();
        float nx = (p.x - c.getX()) / juce::jmax (1.f, c.getWidth());
        float ny = 1.f - (p.y - c.getY()) / juce::jmax (1.f, c.getHeight());
        x = juce::jlimit (0.f, 1.f, nx);
        y = juce::jlimit (0.f, 1.f, ny);
        touching = act;
        if (onChange) onChange (x, y, act);
        repaint();
    }
};
