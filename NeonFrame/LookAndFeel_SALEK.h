#pragma once
#include <JuceHeader.h>

// SALEK HIGHTECH custom LookAndFeel
// Neon / acid / cyberpunk / psychedelic identity
class LookAndFeel_SALEK : public juce::LookAndFeel_V4
{
public:
    LookAndFeel_SALEK()
    {
        // Core palette
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff0a0015)); // deep void purple-black
        setColour (juce::Slider::thumbColourId, juce::Colour (0xff00ffcc)); // cyan
        setColour (juce::Slider::trackColourId, juce::Colour (0xff220033));
        setColour (juce::Slider::backgroundColourId, juce::Colour (0xff110022));
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff00aa)); // magenta
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff330055));
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1a0033));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffff00cc));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xff00ffee));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff000000));
        setColour (juce::Label::textColourId, juce::Colour (0xffee00ff));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Outer glow ring
        g.setColour (juce::Colour (0xff00ffcc).withAlpha (0.25f));
        g.fillEllipse (bounds.expanded (3.0f));

        // Background disc
        juce::ColourGradient grad (juce::Colour (0xff1a0030), centre.x - radius, centre.y,
                                   juce::Colour (0xff0a0018), centre.x + radius, centre.y + radius, true);
        g.setGradientFill (grad);
        g.fillEllipse (bounds);

        // Neon arc
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius * 0.85f, radius * 0.85f, 0.0f,
                           rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xffff00aa));
        g.strokePath (arc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Inner cyan accent
        g.setColour (juce::Colour (0xff00ffee).withAlpha (0.7f));
        g.drawEllipse (bounds.reduced (radius * 0.25f), 1.5f);

        // Thumb / pointer
        juce::Path pointer;
        auto pointerLen = radius * 0.7f;
        auto pointerThick = 3.0f;
        pointer.addRectangle (-pointerThick * 0.5f, -radius * 0.8f, pointerThick, pointerLen);
        g.setColour (juce::Colour (0xffffffff));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        auto base = shouldDrawButtonAsDown ? juce::Colour (0xffff00cc)
                                           : (shouldDrawButtonAsHighlighted ? juce::Colour (0xff330055)
                                                                            : juce::Colour (0xff1a0033));

        // Angled / cyber shape feel
        juce::Path p;
        p.addRoundedRectangle (bounds, 3.0f);
        g.setColour (base);
        g.fillPath (p);

        // Neon border
        g.setColour (juce::Colour (0xff00ffcc).withAlpha (shouldDrawButtonAsHighlighted ? 0.9f : 0.5f));
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return juce::Font (juce::FontOptions (juce::jmin (14.0f, (float) buttonHeight * 0.6f)).withStyle ("Bold"));
    }
};