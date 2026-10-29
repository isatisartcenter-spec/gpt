#pragma once
#include <JuceHeader.h>
#include <functional>

namespace salek
{

/** Touch-first button for the MAGIC hold controls.
    - Touch (press + release quickly): ON while the finger / mouse is down, CUT on release.
    - Press and hold for about 0.35 s: stays ON (latched) after release.
    - Tap a latched button: it cuts again.
    onPressChange(on) fires whenever the effective state changes; getToggleState() mirrors it. */
class MomentaryLatchButton : public juce::TextButton,
                             private juce::Timer
{
public:
    static constexpr int latchAfterMs = 350;

    MomentaryLatchButton()
    {
        setClickingTogglesState (false);
    }

    std::function<void (bool on)> onPressChange;

    bool isLatched() const noexcept { return latched; }

    void setLatched (bool shouldLatch)
    {
        latched = shouldLatch;
        setToggleState (latched, juce::dontSendNotification);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        pressed = true;
        wasLatchedAtDown = latched;
        pressStartMs = juce::Time::getMillisecondCounter();
        setToggleState (true, juce::dontSendNotification);
        if (onPressChange) onPressChange (true);
        startTimer (30);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! pressed) return;
        pressed = false;
        stopTimer();

        if (wasLatchedAtDown)
            latched = false;          // tapping a latched button cuts it
        // a quick touch leaves latched == false -> cut; a long press already set latched in timerCallback

        const bool on = latched;
        setToggleState (on, juce::dontSendNotification);
        if (onPressChange) onPressChange (on);
        repaint();
    }

private:
    void timerCallback() override
    {
        if (pressed && ! wasLatchedAtDown && ! latched
            && juce::Time::getMillisecondCounter() - pressStartMs >= (juce::uint32) latchAfterMs)
        {
            latched = true;           // held long enough: stays on after release
            stopTimer();
        }
    }

    bool latched = false;
    bool pressed = false;
    bool wasLatchedAtDown = false;
    juce::uint32 pressStartMs = 0;
};

} // namespace salek
