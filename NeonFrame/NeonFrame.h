#pragma once
#include <JuceHeader.h>
#include "../AssetsData.h"

// Neon screen frames for the monitors. The frame is drawn FIRST; the monitor content is then clipped to the
// screen rectangle (the inner area measured from the neon edges), so corners and centre ornaments never cover it.
namespace salek
{
struct NeonFrameSpec { const char* file; const char* binary; float l, t, r, b; };   // screen rectangle as fractions

inline const NeonFrameSpec& neonFrameSpec (int index) noexcept
{
    static const NeonFrameSpec specs[3] = {
        { "frame_thin.png",  "frame_thin_png",  0.101f, 0.172f, 0.899f, 0.826f },   // 0: thin neon, corner rings
        { "frame_synth.png", "frame_synth_png", 0.134f, 0.249f, 0.866f, 0.712f },   // 1: SYNTH console
        { "frame_wide.png",  "frame_wide_png",  0.217f, 0.209f, 0.784f, 0.764f } }; // 2: wide neon, centre bars
    return specs[juce::jlimit (0, 2, index)];
}

inline juce::Image& neonFrameImage (int index)
{
    static juce::Image images[3];
    static bool loaded[3] {};
    const int i = juce::jlimit (0, 2, index);
    if (! loaded[i])
    {
        loaded[i] = true;
#if SALEK_HAS_BINARY_DATA
        images[i] = SalekAssets::fromBinaryName (neonFrameSpec (i).binary);   // embedded: works in VST3 and Standalone
#endif
        if (! images[i].isValid())                                           // fallback: loose folder (dev builds)
        {
            auto file = SalekAssets::findAssetsDir().getChildFile (neonFrameSpec (i).file);
            if (file.existsAsFile()) images[i] = juce::ImageFileFormat::loadFrom (file);
        }
    }
    return images[i];
}

// The frame keeps its aspect ratio and is centred in the area
inline juce::Rectangle<float> neonFrameBounds (juce::Rectangle<float> area, const juce::Image& img)
{
    // stretched to the whole monitor area: the frames are square art, and fitting them by aspect made wide monitors tiny
    (void) img;
    return area;
}

// Screen rectangle inside `area`; empty when the frame art is missing (then the monitor draws as before)
inline juce::Rectangle<float> neonScreen (juce::Rectangle<float> area, int index)
{
    const auto& img = neonFrameImage (index);
    if (! img.isValid()) return {};
    const auto fb = neonFrameBounds (area, img);
    const auto sp = neonFrameSpec (index);
    return { fb.getX() + sp.l * fb.getWidth(), fb.getY() + sp.t * fb.getHeight(),
             (sp.r - sp.l) * fb.getWidth(), (sp.b - sp.t) * fb.getHeight() };
}

inline void drawNeonFrame (juce::Graphics& g, juce::Rectangle<float> area, int index)
{
    const auto& img = neonFrameImage (index);
    if (img.isValid()) g.drawImage (img, neonFrameBounds (area, img));
}
} // namespace salek
