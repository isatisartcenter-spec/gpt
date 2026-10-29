#pragma once
#include <JuceHeader.h>
#include "../Modulation/ModMatrix.h"

/** Compact route list inspired by the HTML reference: LFO activity, routes,
    draggable depth bars, and explicit add/clear controls. */
class ModMatrixPanel : public juce::Component, private juce::Timer
{
public:
    explicit ModMatrixPanel (salek::ModMatrix& m) : matrix (m)
    {
        for (int i = 0; i < (int) salek::ModMatrix::Source::NumSources; ++i)
            sourceBox.addItem (salek::ModMatrix::sourceName ((salek::ModMatrix::Source) i), i + 1);
        for (int i = 0; i < (int) salek::ModMatrix::Dest::NumDests; ++i)
            destBox.addItem (salek::ModMatrix::destName ((salek::ModMatrix::Dest) i), i + 1);
        sourceBox.setSelectedId (1);
        destBox.setSelectedId (1);
        addButton.setButtonText ("+ ADD");
        clearButton.setButtonText ("CLR");
        for (auto* b : { &addButton, &clearButton })
        {
            b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff111925));
            b->setColour (juce::TextButton::textColourOffId, juce::Colour (0xff73c9c5));
            addAndMakeVisible (*b);
        }
        sourceBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111925));
        sourceBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff354451));
        sourceBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xff73c9c5));
        destBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111925));
        destBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff354451));
        destBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xffe2e8ed));
        addAndMakeVisible (sourceBox);
        addAndMakeVisible (destBox);
        addButton.onClick = [this]
        {
            const auto src = (salek::ModMatrix::Source) juce::jmax (0, sourceBox.getSelectedId() - 1);
            const auto dst = (salek::ModMatrix::Dest) juce::jmax (0, destBox.getSelectedId() - 1);
            matrix.addRoute (src, dst, 0.5f);
            selectedSource = (int) src;
            selectedDest = (int) dst;
            repaint();
        };
        clearButton.onClick = [this] { matrix.clear(); selectedSource = selectedDest = -1; repaint(); };
        startTimerHz (12);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 3);
        r.removeFromTop (53);
        auto controls = r.removeFromTop (25);
        const int w = controls.getWidth();
        sourceBox.setBounds (controls.removeFromLeft (juce::jmin (w / 3, 180)).reduced (1));
        destBox.setBounds (controls.removeFromLeft (juce::jmin (w / 2, 220)).reduced (1));
        clearButton.setBounds (controls.removeFromRight (46).reduced (1));
        addButton.setBounds (controls.removeFromRight (62).reduced (1));
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff111925).withAlpha (0.96f));
        g.fillRoundedRectangle (r, 8.f);
        g.setColour (juce::Colour (0xff354451));
        g.drawRoundedRectangle (r, 8.f, 1.f);
        g.setFont (juce::FontOptions (10.f, juce::Font::bold));
        g.setColour (juce::Colour (0xffd5bd88));
        g.drawText ("MOD ROUTES", 10, 3, 140, 17, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff9aa9b5));
        g.setFont (juce::FontOptions (9.f));
        g.drawText ("drag a bar to set depth  |  wheel = fine adjust  |  X = remove",
                    130, 3, juce::jmax (80, getWidth() - 140), 17, juce::Justification::centredLeft);

        const juce::Colour cols[] = { juce::Colour (0xff73c9c5), juce::Colour (0xffd49aaf), juce::Colour (0xffa8c895) };
        auto srcArea = getLocalBounds().reduced (8, 0).withY (20).withHeight (27).toFloat();
        const float sw = srcArea.getWidth() / 3.f;
        for (int i = 0; i < 3; ++i)
        {
            auto cell = srcArea.withX (srcArea.getX() + i * sw).withWidth (sw - 4.f);
            g.setColour (juce::Colour (0xff192330));
            g.fillRoundedRectangle (cell, 4.f);
            const float value = juce::jlimit (-1.f, 1.f, matrix.getSourceValue ((salek::ModMatrix::Source) i));
            auto track = cell.reduced (5.f, 5.f);
            g.setColour (cols[i].withAlpha (0.8f));
            const float half = track.getCentreX();
            const float edge = half + value * track.getWidth() * 0.5f;
            g.fillRoundedRectangle (juce::Rectangle<float> (juce::jmin (half, edge), track.getY(),
                                      juce::jmax (1.f, std::abs (edge - half)), track.getHeight()), 3.f);
            g.setColour (cols[i]);
            g.setFont (juce::FontOptions (9.f, juce::Font::bold));
            g.drawText (juce::String ("LFO") + juce::String (i + 1), cell.toNearestInt().withTrimmedRight (cell.getWidth() * 0.72f), juce::Justification::centredLeft);
        }

        const auto& routes = matrix.getRoutes();
        const int rowTop = 82;
        const int rowH = 25;
        int shown = 0;
        for (int slot = 0; slot < salek::ModMatrix::MaxRoutes; ++slot)
        {
            const auto& route = routes[(size_t) slot];
            if (! route.active.load (std::memory_order_acquire)) continue;
            const auto source = route.source.load (std::memory_order_relaxed);
            const auto dest = route.dest.load (std::memory_order_relaxed);
            const float amount = route.amount.load (std::memory_order_relaxed);
            if (shown++ < routeOffset) continue;
            const int y = rowTop + (shown - routeOffset - 1) * rowH;
            if (y + rowH > getHeight() - 2) break;
            auto row = juce::Rectangle<float> (8.f, (float) y, getWidth() - 16.f, rowH - 2.f);
            const int src = (int) source;
            const int dst = (int) dest;
            const bool selected = src == selectedSource && dst == selectedDest;
            g.setColour (selected ? juce::Colour (0xff192330) : juce::Colour (0xff111925));
            g.fillRoundedRectangle (row, 4.f);
            const auto col = cols[src >= 0 && src < 3 ? src : 0];
            g.setColour (col);
            g.setFont (juce::FontOptions (9.f, juce::Font::bold));
            const juce::String sourceName = salek::ModMatrix::sourceName (source);
            const juce::String destName = salek::ModMatrix::destName (dest);
            g.drawText (sourceName + "  ->  " + destName, row.reduced (5.f, 0.f).withWidth (150.f).toNearestInt(), juce::Justification::centredLeft);
            {   // "x SRC" chip: pick a 2nd source that modulates this route's depth (click = next source, wheel = depth)
                const int ms = route.modSrc.load (std::memory_order_relaxed);
                auto chip = chipRect (row);
                g.setColour (ms >= 0 ? juce::Colour (0xff3a2a50) : juce::Colour (0xff1b2430));
                g.fillRoundedRectangle (chip, 4.f);
                g.setColour (ms >= 0 ? juce::Colour (0xffffc861) : juce::Colour (0xff6b7a88));
                g.setFont (juce::FontOptions (9.f, juce::Font::bold));
                g.drawText (ms >= 0 ? juce::String ("x ") + salek::ModMatrix::sourceName ((salek::ModMatrix::Source) ms) + " " + juce::String ((int) std::lround (route.modDepth.load (std::memory_order_relaxed) * 100.f)) + "%"
                                    : juce::String ("x  + MOD"), chip.toNearestInt(), juce::Justification::centred);
            }
            auto bar = row.withX (row.getX() + 158.f).withWidth (juce::jmax (25.f, row.getWidth() - 308.f)).reduced (1.f, 6.f);
            g.setColour (juce::Colour (0xff354451));
            g.fillRoundedRectangle (bar, 3.f);
            const float mid = bar.getCentreX();
            const float end = mid + juce::jlimit (-1.f, 1.f, amount) * bar.getWidth() * 0.5f;
            g.setColour (amount >= 0.f ? col : juce::Colour (0xffd49aaf));
            g.fillRoundedRectangle ({ juce::jmin (mid, end), bar.getY(), juce::jmax (1.f, std::abs (end - mid)), bar.getHeight() }, 3.f);
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions (9.f));
            g.drawText (juce::String (amount, 2), (int) row.getRight() - 47, y, 31, rowH - 2, juce::Justification::centredRight);
            g.setColour (juce::Colour (0xffd49aaf));
            g.drawText ("X", (int) row.getRight() - 22, y, 16, rowH - 2, juce::Justification::centred);
            if (selected) selectedSlot = slot;
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        int slot = routeAt (e.position.y);
        if (slot < 0) return;
        auto& route = matrix.getRoute (slot);
        selectedSlot = slot;
        selectedSource = (int) route.source.load (std::memory_order_relaxed);
        selectedDest = (int) route.dest.load (std::memory_order_relaxed);
        if (e.position.x > getWidth() - 34.f || e.mods.isRightButtonDown())
        {
            matrix.removeRoute (slot);
            if (slot == selectedSlot) selectedSlot = -1;
            repaint();
            return;
        }
        {
            auto rowR = juce::Rectangle<float> (8.f, 0.f, (float) getWidth() - 16.f, 23.f);
            if (chipRect (rowR).getX() <= e.position.x && e.position.x <= chipRect (rowR).getRight())
            {
                const int n = (int) salek::ModMatrix::Source::NumSources;
                int ms = route.modSrc.load (std::memory_order_relaxed);
                ms = e.mods.isShiftDown() ? (ms < 0 ? n - 1 : ms - 1) : (ms + 1 >= n ? -1 : ms + 1);
                route.modSrc.store (ms, std::memory_order_relaxed);
                repaint();
                return;
            }
        }
        dragging = true;
        setAmountFromX (e.position.x);
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return;
        setAmountFromX (e.position.x);
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        const int slot = routeAt (e.position.y);
        if (slot < 0) { routeOffset = juce::jmax (0, routeOffset + (wheel.deltaY < 0.f ? 1 : -1)); repaint(); return; }
        auto& route = matrix.getRoute (slot);
        selectedSlot = slot;
        selectedSource = (int) route.source.load (std::memory_order_relaxed);
        selectedDest = (int) route.dest.load (std::memory_order_relaxed);
        {
            auto rowR = juce::Rectangle<float> (8.f, 0.f, (float) getWidth() - 16.f, 23.f);
            if (chipRect (rowR).getX() <= e.position.x && e.position.x <= chipRect (rowR).getRight() && route.modSrc.load() >= 0)
            {
                route.modDepth.store (juce::jlimit (0.f, 1.f, route.modDepth.load (std::memory_order_relaxed) + wheel.deltaY * 0.1f), std::memory_order_relaxed);
                repaint();
                return;
            }
        }
        route.amount.store (juce::jlimit (-1.f, 1.f, route.amount.load (std::memory_order_relaxed) + wheel.deltaY * 0.06f),
                            std::memory_order_relaxed);
        repaint();
    }

private:
    salek::ModMatrix& matrix;
    juce::ComboBox sourceBox, destBox;
    juce::TextButton addButton, clearButton;
    int selectedSlot = -1, selectedSource = -1, selectedDest = -1, routeOffset = 0;
    bool dragging = false;

    static juce::Rectangle<float> chipRect (const juce::Rectangle<float>& row) noexcept
    {
        return { row.getRight() - 148.f, row.getY() + 3.f, 92.f, row.getHeight() - 6.f };
    }

    int routeAt (float y) const noexcept
    {
        if (y < 82.f) return -1;
        const int row = (int) ((y - 82.f) / 25.f) + routeOffset;
        int seen = 0;
        const auto& routes = matrix.getRoutes();
        for (int i = 0; i < salek::ModMatrix::MaxRoutes; ++i)
            if (routes[(size_t) i].active.load (std::memory_order_acquire) && seen++ == row) return i;
        return -1;
    }

    void setAmountFromX (float x) noexcept
    {
        if (selectedSlot < 0) return;
        auto& route = matrix.getRoute (selectedSlot);
        const float left = 174.f;
        const float right = (float) getWidth() - 162.f;
        const float amount = juce::jlimit (-1.f, 1.f, (x - (left + right) * 0.5f) / juce::jmax (1.f, (right - left) * 0.5f));
        route.amount.store (std::abs (amount) < 0.015f ? 0.f : amount, std::memory_order_relaxed);
    }

    void timerCallback() override { if (isShowing()) repaint(); }
};
