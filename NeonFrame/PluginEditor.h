#pragma once
#include "BackgroundsData.h"
#include "SpaceScene.h"
#include "UiImagesData.h"
#include "UpdateConfig.h"
#include "StorePanel.h"
#include "CustomWaveEditor.h"
#include "GitiLog.h"
#include "GroupFrames.h"
#include <JuceHeader.h>
#include <algorithm>
#include "PluginProcessor.h"
#include "UI/ModMatrixPanel.h"
#include "UI/SonicCoreGL.h"
#include "UI/MagicPad.h"
#include "UI/OscShapeMonitor.h"
#include "UI/MomentaryLatchButton.h"
#include "Synth/SynthVoice.h"
#include "GitiGenesis.h"
#include "UI/ModDropSupport.h"
#include "GitiCrystalAssets.h"

#include "PluginEditorLookAndFeel.inl"
#include "PluginEditorFxLfo.inl"

class WavetableDisplay : public juce::Component, private juce::Timer {
public:
    explicit WavetableDisplay (juce::AudioProcessorValueTreeState& s) : apvts (s) { startTimerHz (30); }
    void paint (juce::Graphics& g) override;
    void timerCallback() override { if (isShowing()) repaint(); }
private:
    juce::AudioProcessorValueTreeState& apvts;
};
inline void WavetableDisplay::paint (juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (SalekLookAndFeel::panel());
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (SalekLookAndFeel::border());
    g.drawRoundedRectangle (bounds, 6.0f, 1.f);

    auto getValue = [&] (const char* id, float fallback)
    {
        if (auto* p = apvts.getRawParameterValue (id)) return p->load();
        return fallback;
    };
    const float table = juce::jlimit (0.f, 1.f, getValue ("osc1_table", 0.f));
    const float warp  = juce::jlimit (0.f, 1.f, getValue ("osc1_warp", 0.f));
    const float fold  = juce::jlimit (0.f, 1.f, getValue ("osc1_fold", 0.f));
    const float drive = juce::jlimit (0.f, 1.f, getValue ("osc1_drive", 0.f));
    const float phaseOffset = juce::jlimit (0.f, 1.f, getValue ("osc1_phase", 0.f));

    auto plot = bounds.reduced (8.f, 5.f).withTrimmedTop (13.f);
    if (plot.getWidth() < 2.f || plot.getHeight() < 2.f) return;

    // Beveled plot, soft center line, and depth grid keep the compact header scope legible.
    g.setColour (juce::Colours::black.withAlpha (0.24f));
    g.fillRoundedRectangle (plot.translated (0.f, 1.5f), 3.f);
    g.setColour (SalekLookAndFeel::teal().withAlpha (0.10f));
    for (int i = 1; i < 4; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / 4.f;
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
    }
    g.setColour (juce::Colours::white.withAlpha (0.17f));
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

    juce::Path wave, fill;
    constexpr int sampleCount = 160;
    const auto& wavetable = salek::SynthVoice::getSharedWavetable();
    for (int i = 0; i < sampleCount; ++i)
    {
        const float t = (float) i / (float) (sampleCount - 1);
        float phase = std::fmod (t + phaseOffset, 1.f);
        if (warp > 1.0e-4f)
        {
            phase = std::pow (juce::jmax (1.0e-6f, phase), 1.f + warp * 3.4f);
            if (warp > 0.42f)
            {
                const float mirror = juce::jlimit (0.f, 1.f, (warp - 0.42f) * 1.4f);
                if (phase > 0.5f) phase = 0.5f + (phase - 0.5f) * (1.f - mirror * 0.55f);
                if (warp > 0.75f) phase = std::fmod (phase * (1.f + (warp - 0.75f) * 3.2f), 1.f);
            }
            phase = juce::jlimit (0.f, 0.99999f, phase);
        }

        float sample = wavetable.getSample (table, phase);
        if (fold > 1.0e-4f)
        {
            const float f = fold * fold;
            const float x = sample * (1.f + f * 7.5f);
            float hard = x;
            for (int k = 0; k < 3; ++k)
            {
                if (hard > 1.f) hard = 2.f - hard;
                else if (hard < -1.f) hard = -2.f - hard;
                else break;
            }
            const float folded = std::sin (x * juce::MathConstants<float>::halfPi * (0.8f + f * 0.9f));
            sample = (folded * (1.f - f * 0.45f) + hard * (f * 0.45f)) / (1.f + f * 0.9f);
        }
        if (drive > 1.0e-4f)
        {
            const float gain = 1.f + drive * 5.8f;
            float shaped = sample * gain;
            shaped = std::tanh (shaped * (1.f + drive * 0.28f)) - 0.07f * drive * shaped * shaped;
            sample = shaped / (0.88f + 0.12f * std::tanh (gain));
        }

        const float x = plot.getX() + t * plot.getWidth();
        const float y = plot.getCentreY() - juce::jlimit (-1.15f, 1.15f, sample) * plot.getHeight() * 0.42f;
        if (i == 0)
        {
            wave.startNewSubPath (x, y);
            fill.startNewSubPath (x, plot.getBottom());
            fill.lineTo (x, y);
        }
        else
        {
            wave.lineTo (x, y);
            fill.lineTo (x, y);
        }
    }
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.closeSubPath();
    const auto waveColour = SalekLookAndFeel::teal();
    g.setColour (waveColour.withAlpha (0.11f));
    g.fillPath (fill);
    g.setColour (waveColour.withAlpha (0.20f));
    g.strokePath (wave, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (waveColour.brighter (0.28f));
    g.strokePath (wave, juce::PathStrokeType (1.25f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const int frame = juce::roundToInt (table * (float) (wavetable.getNumFrames() - 1));
    g.setFont (juce::FontOptions (8.f, juce::Font::bold));
    g.setColour (SalekLookAndFeel::text());
    g.drawText ("WT1  " + juce::String (frame + 1).paddedLeft ('0', 3) + " "
                    + salek::Wavetable::getFrameName (frame) + "  / " + juce::String (wavetable.getNumFrames()),
                bounds.toNearestInt().removeFromTop (13).reduced (7, 0), juce::Justification::centredLeft);
}

#include "PluginEditorVisualizers.inl"
#include "PluginEditorAboutPerformance.inl"

class AdsrDisplay : public juce::Component, private juce::Timer {
public:
    explicit AdsrDisplay (juce::AudioProcessorValueTreeState& s) : apvts (s) { startTimerHz (30); }
    void paint (juce::Graphics& g) override;
    void timerCallback() override { if (isShowing()) repaint(); }
private:
    juce::AudioProcessorValueTreeState& apvts;
};
inline void AdsrDisplay::paint (juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced (2.f);
    g.setColour (SalekLookAndFeel::panel());
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (SalekLookAndFeel::border());
    g.drawRoundedRectangle (r, 6.f, 1.f);

    auto gval = [&] (const char* id, float fallback) {
        if (auto* p = apvts.getRawParameterValue (id)) return p->load();
        return fallback;
    };
    const float a = gval ("amp_attack", 0.01f);
    const float d = gval ("amp_decay", 0.2f);
    const float s = gval ("amp_sustain", 0.7f);
    const float rel = gval ("amp_release", 0.3f);
    auto mapT = [] (float t) { return std::pow (juce::jlimit (0.001f, 1.f, t / 2.f), 0.55f); };
    const float wa = mapT (a), wd = mapT (d), wr = mapT (rel), ws = 0.28f;
    const float sum = wa + wd + ws + wr;
    auto plot = r.reduced (8.f, 5.f).withTrimmedTop (9.f);
    g.setFont (juce::FontOptions (8.f, juce::Font::bold));
    g.setColour (SalekLookAndFeel::mutedText());
    g.drawText ("AMP ENVELOPE", r.toNearestInt().removeFromTop (12).reduced (7, 0), juce::Justification::centredLeft);

    for (int i = 1; i < 4; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / 4.f;
        g.setColour (SalekLookAndFeel::border().withAlpha (0.48f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
    }
    g.setColour (SalekLookAndFeel::border().withAlpha (0.60f));
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

    const float x0 = plot.getX(), y0 = plot.getBottom();
    const float x1 = x0 + plot.getWidth() * (wa / sum), y1 = plot.getY();
    const float x2 = x1 + plot.getWidth() * (wd / sum), y2 = plot.getY() + plot.getHeight() * (1.f - s);
    const float x3 = x2 + plot.getWidth() * (ws / sum), y3 = y2;
    const float x4 = plot.getRight(), y4 = plot.getBottom();
    juce::Path env;
    env.startNewSubPath (x0, y0); env.lineTo (x1, y1); env.lineTo (x2, y2); env.lineTo (x3, y3); env.lineTo (x4, y4);
    auto fill = env;
    fill.lineTo (x4, y0);
    fill.closeSubPath();
    g.setColour (SalekLookAndFeel::teal().withAlpha (0.10f));
    g.fillPath (fill);
    g.setColour (SalekLookAndFeel::teal());
    g.strokePath (env, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float nodesX[] = { x1, x2, x3 };
    const float nodesY[] = { y1, y2, y3 };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (SalekLookAndFeel::panel());
        g.fillEllipse (nodesX[i] - 2.3f, nodesY[i] - 2.3f, 4.6f, 4.6f);
        g.setColour (SalekLookAndFeel::champagne());
        g.fillEllipse (nodesX[i] - 1.25f, nodesY[i] - 1.25f, 2.5f, 2.5f);
    }
}

class FilterCurveDisplay : public juce::Component, private juce::Timer {
public:
    explicit FilterCurveDisplay (juce::AudioProcessorValueTreeState& s) : apvts (s) { startTimerHz (30); }
    void paint (juce::Graphics& g) override;
    void paintContent (juce::Graphics& g);
    void timerCallback() override { if (isShowing()) repaint(); }
    void setFilterIndex (int i) { filterIdx = i; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override { dragSet (e); }
    void mouseDrag (const juce::MouseEvent& e) override { dragSet (e); }
    void dragSet (const juce::MouseEvent& e)
    {
        auto plot = getLocalBounds().toFloat().reduced (10.f, 9.f).withTrimmedTop (11.f);
        const float tx = juce::jlimit (0.f, 1.f, (e.position.x - plot.getX()) / juce::jmax (1.f, plot.getWidth()));
        const float ty = juce::jlimit (0.f, 1.f, (e.position.y - plot.getY()) / juce::jmax (1.f, plot.getHeight()));
        auto setP = [&] (const char* id, float v) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
        setP (filterIdx == 0 ? "filter_cutoff" : "filter2_cutoff", 20.f * std::pow (900.f, tx));
        setP (filterIdx == 0 ? "filter_reso" : "filter2_reso", 1.f - ty);
    }
private:
    juce::AudioProcessorValueTreeState& apvts;
    int filterIdx = 0;
};
inline void FilterCurveDisplay::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    salek::drawNeonFrame (g, area, 0);
    const auto screen = salek::neonScreen (area, 0);
    if (screen.isEmpty()) { paintContent (g); return; }
    g.saveState();
    g.reduceClipRegion (screen.getSmallestIntegerContainer());
    paintContent (g);
    g.restoreState();
}

inline void FilterCurveDisplay::paintContent (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.f);
    g.setColour (juce::Colour (0xff060812)); g.fillRoundedRectangle (r, 6.f);
    const juce::Colour col = filterIdx == 0 ? SalekLookAndFeel::teal() : SalekLookAndFeel::rose();
    g.setColour (col.withAlpha (0.5f)); g.drawRoundedRectangle (r, 6.f, 1.f);
    auto gval = [&] (const char* id, float d) { if (auto* p = apvts.getRawParameterValue (id)) return p->load(); return d; };
    const float cutHz = juce::jlimit (20.f, 20000.f, gval (filterIdx == 0 ? "filter_cutoff" : "filter2_cutoff", 1000.f));
    const float reso  = juce::jlimit (0.f, 1.f, gval (filterIdx == 0 ? "filter_reso" : "filter2_reso", 0.3f));
    const int fmode = (int) gval (filterIdx == 0 ? "filter_mode" : "filter2_mode", 0.f);
    static const char* names[] = { "LP 12","LP 24","HP 12","HP 24","Band","Notch","Peak","AllPass","Acid LP","Ladder","Comb","Formant","BandRej","LoShelf","HiShelf","Phaser","Diode LP","Sallen","Vocal","RingMod","BP-Q","LP+HP","Notch 2","Wide" };
    const float Q = 0.55f + reso * reso * 14.f;
    auto sq = [] (float v) { return v * v; };
    auto lp2 = [&] (float x, float q) { return 1.f / std::sqrt (sq (1.f - x * x) + sq (x / q)); };
    auto bp2 = [&] (float x, float q) { return (x / q) / std::sqrt (sq (1.f - x * x) + sq (x / q)); };
    auto mag = [&] (float x) -> float   // linear magnitude at f/fc = x
    {
        const float x2 = x * x;
        switch (fmode)
        {
            case 0: return lp2 (x, Q);
            case 1: return lp2 (x, 0.6f) * lp2 (x, Q);
            case 2: return x2 * lp2 (x, Q);
            case 3: return x2 * x2 * lp2 (x, 0.6f) * lp2 (x, Q);
            case 4: return bp2 (x, Q * 1.2f);
            case 5: case 12: return std::abs (1.f - x2) / std::sqrt (sq (1.f - x2) + sq (x / (Q * (fmode == 12 ? 0.4f : 1.f))));
            case 6: { const float g2 = 1.f + (Q - 0.5f) * 0.9f; return std::sqrt (sq (1.f - x2) + sq (x * g2 / Q)) / std::sqrt (sq (1.f - x2) + sq (x / Q)) * 0.6f + 0.4f; }
            case 7: return 1.f - 0.45f * std::exp (-sq (std::log (x) * 3.f));
            case 8: return lp2 (x, 0.7f) * lp2 (x, Q * 1.4f) * 0.9f;
            case 9: return lp2 (x, 0.5f) * lp2 (x, 0.6f + reso * 8.f) / (1.f + reso * 0.9f * lp2 (0.05f, 1.f));
            case 10: return 0.15f + 0.85f * std::abs (std::cos (x * 9.f)) * (0.5f + reso * 0.5f) + 0.1f;
            case 11: case 18: return 0.08f + 1.2f * std::exp (-sq (std::log (x) * (4.f + reso * 8.f))) + 0.8f * std::exp (-sq ((std::log (x) - 0.7f) * (4.f + reso * 8.f)));
            case 13: return 1.f + 3.f / (1.f + sq (x * 1.4f));
            case 14: return 1.f + 3.f * sq (x) / (1.f + sq (x));
            case 15: return std::abs (1.f - x2) / std::sqrt (sq (1.f - x2) + sq (x / Q)) * std::abs (1.f - sq (x * 1.7f)) / std::sqrt (sq (1.f - sq (x * 1.7f)) + sq (x * 1.7f / Q));
            case 16: return lp2 (x, 0.5f) / std::sqrt (1.f + sq (x) * x2 * 0.0f + x2 * 0.5f) * (1.f + reso * 0.6f * std::exp (-sq (std::log (x) * 4.f)));
            case 17: return lp2 (x, 0.5f + reso * 2.5f);
            case 19: return 0.5f + 0.45f * std::sin (std::log (x) * 14.f + reso * 4.f);
            case 20: return bp2 (x, Q * 3.f);
            case 21: return 0.5f * (lp2 (x, Q) + x2 * lp2 (x, Q));
            case 22: return std::abs (1.f - x2) / std::sqrt (sq (1.f - x2) + sq (x / (Q * 4.f)));
            case 23: return x < 1.f ? 1.6f : 0.55f;
            default: return lp2 (x, Q);
        }
    };
    auto plot = r.reduced (8.f, 5.f).withTrimmedTop (11.f).withTrimmedBottom (9.f);
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.setColour (col);
    g.drawText ("F" + juce::String (filterIdx + 1) + "  " + juce::String (names[juce::jlimit (0, 23, fmode)]) + "  " + juce::String (cutHz, 0) + " Hz",
                r.toNearestInt().removeFromTop (12).reduced (7, 0), juce::Justification::centredLeft);
    g.setColour (SalekLookAndFeel::mutedText());
    g.drawText ("RES " + juce::String (juce::roundToInt (reso * 100.f)) + "%", r.toNearestInt().removeFromTop (12).reduced (7, 0), juce::Justification::centredRight);
    const float dbMin = -54.f, dbMax = 24.f;
    auto yOfDb = [&] (float db) { return plot.getBottom() - (juce::jlimit (dbMin, dbMax, db) - dbMin) / (dbMax - dbMin) * plot.getHeight(); };
    auto xOfHz = [&] (float hz) { return plot.getX() + (std::log (hz) - std::log (20.f)) / (std::log (20000.f) - std::log (20.f)) * plot.getWidth(); };
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (float hz : { 100.f, 1000.f, 10000.f }) g.drawVerticalLine ((int) xOfHz (hz), plot.getY(), plot.getBottom());
    for (float db : { -36.f, -18.f, 0.f, 12.f }) g.drawHorizontalLine ((int) yOfDb (db), plot.getX(), plot.getRight());
    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.setFont (juce::FontOptions (7.f));
    const char* lbl[] = { "100", "1k", "10k" }; const float lh[] = { 100.f, 1000.f, 10000.f };
    for (int i = 0; i < 3; ++i) g.drawText (lbl[i], juce::Rectangle<float> (xOfHz (lh[i]) - 14.f, plot.getBottom() + 1.f, 28.f, 8.f).toNearestInt(), juce::Justification::centred);
    g.drawText ("0dB", juce::Rectangle<float> (plot.getX() + 1.f, yOfDb (0.f) - 8.f, 22.f, 8.f).toNearestInt(), juce::Justification::centredLeft);
    juce::Path curve;
    for (int i = 0; i < 160; ++i)
    {
        const float t = (float) i / 159.f;
        const float hz = std::exp (std::log (20.f) + t * (std::log (20000.f) - std::log (20.f)));
        const float db = 20.f * std::log10 (juce::jmax (1.0e-4f, mag (hz / cutHz)));
        const float x = plot.getX() + t * plot.getWidth(), y = yOfDb (db);
        if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
    }
    auto fill = curve; fill.lineTo (plot.getRight(), plot.getBottom()); fill.lineTo (plot.getX(), plot.getBottom()); fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (col.withAlpha (0.35f), 0, plot.getY(), col.withAlpha (0.02f), 0, plot.getBottom(), false));
    g.fillPath (fill);
    g.setColour (col); g.strokePath (curve, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float mx = xOfHz (cutHz);
    const float my = yOfDb (20.f * std::log10 (juce::jmax (1.0e-4f, mag (1.f))));
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawVerticalLine ((int) mx, plot.getY(), plot.getBottom());
    g.setColour (juce::Colours::white); g.fillEllipse (mx - 4.f, my - 4.f, 8.f, 8.f);
    g.setColour (col); g.drawEllipse (mx - 4.f, my - 4.f, 8.f, 8.f, 1.4f);
}

class LfoDisplay : public juce::Component, private juce::Timer {
public:
    explicit LfoDisplay (juce::AudioProcessorValueTreeState& s) : apvts (s) { startTimerHz (30); }
    void setSources (salek::LFO* a, salek::LFO* b, salek::LFO* c) { src[0] = a; src[1] = b; src[2] = c; }
    void paint (juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced (2.f);
        g.setColour (SalekLookAndFeel::panel()); g.fillRoundedRectangle (bounds, 8.f);
        g.setColour (SalekLookAndFeel::border()); g.drawRoundedRectangle (bounds, 8.f, 1.f);
        auto gval=[&](const char* id,float d){if(auto*p=apvts.getRawParameterValue(id))return p->load();return d;};
        const char* rates[] = {"lfo_rate","lfo2_rate","lfo3_rate"};
        const char* amts[]  = {"lfo_amount","lfo2_amount","lfo3_amount"};
        const char* waves[] = {"lfo_wave","lfo2_wave","lfo3_wave"};
        juce::Colour cols[3] = { SalekLookAndFeel::teal(), SalekLookAndFeel::rose(), juce::Colour (0xffa8c895) };
        const char* labels[3] = { "LFO1", "LFO2", "LFO3" };
        float w = bounds.getWidth() / 3.f;
        for (int L = 0; L < 3; ++L)
        {
            auto r = bounds.withX (bounds.getX() + L * w).withWidth (w).reduced (4.f, 3.f);
            g.setColour (L == selected ? cols[L] : SalekLookAndFeel::border().withAlpha (0.72f)); g.drawRoundedRectangle (r, 6.f, L == selected ? 2.4f : 1.f);
            float rate = gval (rates[L], 1.f); int wave = (int) gval (waves[L], 0.f); float amt = gval (amts[L], 0.5f);
            g.setColour (SalekLookAndFeel::mutedText()); g.setFont (juce::FontOptions (9.f, juce::Font::bold));
            g.drawText (juce::String (labels[L]) + "  " + juce::String (rate, 2) + "Hz",
                        r.removeFromTop (14).toNearestInt(), juce::Justification::centred);
            auto plot = r.reduced (2.f, 2.f);
            g.setColour (SalekLookAndFeel::border().withAlpha (0.48f));
            g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());
            juce::Path curve; const int N = 64;
            float spd = juce::jmax (0.05f, rate) * 0.15f;
            for (int i = 0; i < N; ++i)
            {
                float t = (float) i / (N - 1);
                const float cycles = t * 2.f + phase * spd * (1.f + L * 0.3f) / juce::MathConstants<float>::twoPi;
                const bool custom = (wave == 5) && (src[L] != nullptr);
                float y = custom ? src[L]->previewCustom (cycles) : salek::LFO::previewShape (wave, cycles);
                y *= (0.25f + 0.75f * amt);
                float px = plot.getX() + t * plot.getWidth();
                float py = plot.getCentreY() - y * plot.getHeight() * 0.4f;
                if (i == 0) curve.startNewSubPath (px, py); else curve.lineTo (px, py);
            }
            g.setColour (cols[L]); g.strokePath (curve, juce::PathStrokeType (1.7f));
        }
    }
    void timerCallback() override { if (! isShowing()) return; phase += 0.12f; repaint(); }
    std::function<void (int)> onSelect;
    int selected = 0;
    void setSelected (int i) { selected = i; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, 2, (int) (e.position.x / juce::jmax (1.f, (float) getWidth() / 3.f)));
        selected = i; repaint();
        if (onSelect) onSelect (i);
    }
private:
    juce::AudioProcessorValueTreeState& apvts; float phase = 0.f;
    salek::LFO* src[3] { nullptr, nullptr, nullptr };
};

#include "PluginEditorStepGrid.inl"

// Big animated centre visualizer (reference: holographic lotus over a perspective floor), reacts to output level
struct GitiVisualizer : public juce::Component, private juce::Timer
{
    std::function<float()> level;
    float lvl = 0.f, t = 0.f;
    explicit GitiVisualizer (std::function<float()> f) : level (std::move (f))
    { startTimerHz (30); setInterceptsMouseClicks (false, false); }

    void timerCallback() override
    {
        const float l = juce::jlimit (0.f, 1.f, level ? level() : 0.f);
        lvl = l > lvl ? l : lvl * 0.9f;
        t += 0.033f * (1.f + lvl * 2.5f);
        if (isShowing()) repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using juce::Colour;
        const Colour cyan = giti::primaryColour(), purple = giti::tertiaryColour(), magenta = giti::secondaryColour(), gold = giti::accentColour();
        const auto b = getLocalBounds().toFloat();
        const float W = b.getWidth(), H = b.getHeight();
        SalekLookAndFeel::drawOrnatePanel (g, b.reduced (3.f), cyan, "HOLOGRAPHIC VISUALIZER",
                                           juce::String::fromUTF8 ("تجلی صوت"), lvl);
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (b.reduced (5.f).withTrimmedTop (18.f).toNearestInt());
        const float cx = W * 0.5f, cy = H * 0.47f, R = juce::jmin (W * 0.5f, H * 0.52f) * (0.86f + lvl * 0.06f);

        // bloom
        g.setGradientFill (juce::ColourGradient (purple.withAlpha (0.30f + 0.35f * lvl), cx, cy, Colour (0x00000000), cx + R * 1.6f, cy, true));
        g.fillEllipse (cx - R * 1.6f, cy - R * 1.6f, R * 3.2f, R * 3.2f);

        // perspective floor grid (3D depth), scrolls toward the viewer
        const float hz = H * 0.70f;
        for (int i = -16; i <= 16; ++i)
        {
            g.setColour ((i % 2 ? cyan : magenta).withAlpha (0.10f));
            g.drawLine (cx + (float) i * 6.f, hz, cx + (float) i * W * 0.085f, H, 1.f);
        }
        for (int k = 0; k < 12; ++k)
        {
            const float z = std::fmod ((float) k + t * 0.9f, 12.f) / 12.f;
            const float y = hz + (H - hz) * std::pow (z, 2.3f);
            g.setColour (cyan.withAlpha (0.05f + 0.22f * z));
            g.drawLine (b.getX() + 6.f, y, b.getRight() - 6.f, y, 1.f);
        }

        // lotus layers (3 counter-rotating rings + gold inner) with bloom pass
        for (int layer = 0; layer < 4; ++layer)
        {
            const int petals = layer == 3 ? 8 : 16 - layer * 4;
            const float L = R * (1.0f - 0.24f * (float) layer) * (1.f + 0.04f * std::sin (t * 1.3f + (float) layer));
            const float w = L * (layer == 3 ? 0.30f : 0.22f);
            juce::Path petal;
            petal.startNewSubPath (0.f, 0.f);
            petal.cubicTo (w, -L * 0.3f, w, -L * 0.7f, 0.f, -L);
            petal.cubicTo (-w, -L * 0.7f, -w, -L * 0.3f, 0.f, 0.f);
            const Colour cc = layer == 0 ? cyan : (layer == 1 ? purple.brighter (0.3f) : (layer == 2 ? magenta : gold));
            for (int i = 0; i < petals; ++i)
            {
                const float a = t * 0.22f * (layer % 2 ? -1.f : 1.f) + (float) i * juce::MathConstants<float>::twoPi / (float) petals;
                const auto tr = juce::AffineTransform::rotation (a).translated (cx, cy);
                g.setColour (cc.withAlpha (0.10f + 0.12f * lvl));
                g.strokePath (petal, juce::PathStrokeType (4.5f), tr);
                g.setColour (cc.withAlpha (0.70f));
                g.strokePath (petal, juce::PathStrokeType (1.3f), tr);
            }
        }

        // tilted orbit rings with travelling beads (fake 3D)
        for (int k = 0; k < 3; ++k)
        {
            const float sy = 0.40f + 0.28f * std::sin (t * 0.35f + (float) k * 2.1f);
            const float rot = t * 0.18f * (float) (k + 1) + (float) k * 1.05f;
            const Colour oc = k == 0 ? gold : (k == 1 ? cyan : magenta);
            juce::Path o;
            o.addEllipse (-R * 1.05f, -R * 1.05f * sy, R * 2.1f, R * 2.1f * sy);
            const auto tr = juce::AffineTransform::rotation (rot).translated (cx, cy);
            g.setColour (oc.withAlpha (0.12f));
            g.strokePath (o, juce::PathStrokeType (7.f), tr);
            g.setColour (oc.withAlpha (0.65f));
            g.strokePath (o, juce::PathStrokeType (1.6f), tr);
            const float p = t * (0.9f + 0.4f * (float) k);
            juce::Point<float> pt (R * 1.05f * std::cos (p), R * 1.05f * sy * std::sin (p));
            pt = pt.transformedBy (tr);
            g.setColour (oc.withAlpha (0.35f));  g.fillEllipse (pt.x - 8.f, pt.y - 8.f, 16.f, 16.f);
            g.setColour (juce::Colours::white);  g.fillEllipse (pt.x - 2.5f, pt.y - 2.5f, 5.f, 5.f);
        }

        // depth particles (parallax)
        for (int i = 0; i < 110; ++i)
        {
            const float f1 = std::fmod ((float) i * 0.6180339f, 1.f), f2 = std::fmod ((float) i * 0.3819660f + 0.17f, 1.f);
            const float z = 0.25f + f2 * 0.75f;
            const float ang = f1 * juce::MathConstants<float>::twoPi + t * (0.15f + 0.5f * z) * ((i & 1) ? 1.f : -1.f);
            const float rr = R * (0.25f + 1.25f * f2);
            const float px = cx + std::cos (ang) * rr, py = cy + std::sin (ang) * rr * 0.62f;
            g.setColour ((i % 3 == 0 ? gold : (i % 3 == 1 ? cyan : magenta)).withAlpha (0.18f + 0.5f * z * (0.6f + 0.4f * std::sin (t * 2.f + (float) i))));
            const float sz = 1.0f + 2.6f * z;
            g.fillEllipse (px - sz * 0.5f, py - sz * 0.5f, sz, sz);
        }

        // floating crystals orbiting the lotus (rotate + bob)
        {
            const GitiCrystal::Id gem[5] = { GitiCrystal::Diamond, GitiCrystal::HexSwirl, GitiCrystal::Octagon, GitiCrystal::Arabic, GitiCrystal::WaveDisc };
            for (int i = 0; i < 5; ++i)
            {
                const auto& img = GitiCrystal::get (gem[i]);
                if (! img.isValid()) continue;
                const float a = t * (0.35f + 0.07f * (float) i) + (float) i * juce::MathConstants<float>::twoPi / 5.f;
                const float rr = R * (0.62f + 0.18f * std::sin (t * 0.8f + (float) i));
                const float gx = cx + std::cos (a) * rr, gy = cy + std::sin (a) * rr * 0.62f + 6.f * std::sin (t * 1.7f + (float) i);
                const float sz = 26.f + 6.f * std::sin (t * 2.f + (float) i) + 8.f * lvl;
                juce::Graphics::ScopedSaveState gs (g);
                g.addTransform (juce::AffineTransform::rotation (t * 0.9f * (i % 2 ? -1.f : 1.f), gx, gy));
                g.setOpacity (0.9f);
                g.drawImage (img, juce::Rectangle<float> (sz, sz).withCentre ({ gx, gy }), juce::RectanglePlacement::centred);
            }
        }

        // core
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.95f), cx, cy, gold.withAlpha (0.f), cx + R * 0.22f, cy, true));
        g.fillEllipse (cx - R * 0.22f, cy - R * 0.22f, R * 0.44f, R * 0.44f);
        juce::Path star;
        star.addStar ({ cx, cy }, 8, R * 0.07f, R * 0.17f, t * 0.6f);
        g.setColour (gold);
        g.strokePath (star, juce::PathStrokeType (1.6f));
    }
};


// Animated ambient background for FX / ABOUT pages (24 fps, light on CPU): aurora ribbons, warp stars / dust,
// rotating mandala, drifting crystals, radar sweep.
struct GitiAmbient : public juce::Component, private juce::Timer
{
    int mode = 0;   // 0 = FX (dark field + drifting particles), 1 = ABOUT (cosmic temple)
    float t = 0.f;

    // FX mode: the picture is scaled once (on resize) and stays fixed. Only a few dust motes move,
    // and only their small areas are repainted, so the CPU cost stays very low.
    struct Mote { float x, y, r, speed, phase; };
    std::array<Mote, 60> motes {};
    juce::Random rng { 1967 };

    explicit GitiAmbient (int m) : mode (m)
    {
        startTimerHz (24);
        setInterceptsMouseClicks (false, false);
        for (auto& mo : motes) respawn (mo, true);
    }

    void respawn (Mote& mo, bool anywhere)
    {
        mo.x = rng.nextFloat();
        mo.y = anywhere ? rng.nextFloat() : 1.02f;
        mo.r = 0.9f + rng.nextFloat() * 1.6f;
        mo.speed = 0.0012f + rng.nextFloat() * 0.0022f;   // fraction of height per tick
        mo.phase = rng.nextFloat() * 6.2831f;
    }

    juce::Rectangle<int> moteArea (const Mote& mo) const
    {
        const float px = mo.x * (float) getWidth() + 5.f * std::sin (t * 0.7f + mo.phase);
        const float py = mo.y * (float) getHeight();
        const int rr = (int) std::ceil (mo.r * 2.f) + 2;
        return { (int) px - rr, (int) py - rr, 2 * rr, 2 * rr };
    }

    void timerCallback() override
    {
        t += 0.04f;
        if (! isShowing()) return;
        for (auto& mo : motes)
        {
            mo.y -= mo.speed;
            if (mo.y < -0.02f) respawn (mo, false);
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using juce::Colour;
        const Colour c1 = giti::primaryColour(), c2 = giti::secondaryColour(), c3 = giti::accentColour(), c4 = giti::tertiaryColour();
        const auto b = getLocalBounds().toFloat();
        const float W = b.getWidth(), H = b.getHeight();
        if (W < 40.f || H < 40.f) return;

        if (mode == 0)
        {
            // FX page: plain dark background with drifting particles (no picture, cheap to repaint)
            g.fillAll (Colour (0xff04050c));

            int idx = 0;
            for (const auto& mo : motes)
            {
                const float px = mo.x * W + 5.f * std::sin (t * 0.7f + mo.phase);
                const float py = mo.y * H;
                const float a = 0.35f + 0.25f * std::sin (t * 1.3f + mo.phase);
                const Colour pc = (idx % 3 == 0) ? c1 : ((idx % 3 == 1) ? c3 : c2);
                ++idx;
                g.setColour (pc.withAlpha (juce::jlimit (0.04f, 0.16f, a * 0.25f)));
                g.fillEllipse (px - mo.r * 2.4f, py - mo.r * 2.4f, mo.r * 4.8f, mo.r * 4.8f);   // soft halo
                g.setColour (pc.withAlpha (juce::jlimit (0.2f, 0.9f, a)));
                g.fillEllipse (px - mo.r, py - mo.r, mo.r * 2.f, mo.r * 2.f);                  // core
            }
            return;
        }
        g.setGradientFill (juce::ColourGradient (Colour (0xff070a1e), 0.f, 0.f, Colour (0xff120a30), W, H, false));
        g.fillAll();
        const float cx = W * 0.5f, cy = H * 0.5f, R = juce::jmin (W, H) * 0.46f;

        // nebula glows
        g.setGradientFill (juce::ColourGradient (c4.withAlpha (0.30f), W * (0.25f + 0.1f * std::sin (t * 0.3f)), H * 0.3f, Colour (0x00000000), W * 0.25f + R, H * 0.3f, true));
        g.fillAll();
        g.setGradientFill (juce::ColourGradient (c1.withAlpha (0.20f), W * (0.8f - 0.1f * std::sin (t * 0.2f)), H * 0.75f, Colour (0x00000000), W * 0.8f - R, H * 0.75f, true));
        g.fillAll();

        // aurora ribbons
        for (int k = 0; k < 3; ++k)
        {
            juce::Path p;
            const float base = H * (0.30f + 0.20f * (float) k);
            for (int x = 0; x <= (int) W; x += 14)
            {
                const float xf = (float) x;
                const float y = base + 26.f * std::sin (xf * 0.012f + t * (0.7f + 0.25f * (float) k) + (float) k * 2.f)
                                     + 12.f * std::sin (xf * 0.031f - t * 0.9f);
                if (x == 0) p.startNewSubPath (xf, y); else p.lineTo (xf, y);
            }
            const Colour rc = k == 0 ? c1 : (k == 1 ? c2 : c3);
            g.setColour (rc.withAlpha (0.05f));  g.strokePath (p, juce::PathStrokeType (26.f));
            g.setColour (rc.withAlpha (0.10f));  g.strokePath (p, juce::PathStrokeType (9.f));
            g.setColour (rc.withAlpha (0.45f));  g.strokePath (p, juce::PathStrokeType (1.4f));
        }

        // rotating mandala
        for (int layer = 0; layer < 3; ++layer)
        {
            juce::Path star;
            star.addStar ({ cx, cy }, 8 + layer * 4, R * (0.52f - 0.12f * (float) layer), R * (0.92f - 0.18f * (float) layer),
                          t * 0.12f * (layer % 2 ? -1.f : 1.f));
            g.setColour ((layer == 0 ? c3 : (layer == 1 ? c1 : c2)).withAlpha (0.20f));
            g.strokePath (star, juce::PathStrokeType (1.2f));
        }
        for (int r = 1; r <= 4; ++r)
        {
            g.setColour (c1.withAlpha (0.06f + 0.01f * (float) r));
            g.drawEllipse (cx - R * (float) r / 4.f, cy - R * (float) r / 4.f, R * (float) r / 2.f, R * (float) r / 2.f, 1.f);
        }

        if (mode == 0)
        {
            // warp stars flying out of the centre
            for (int i = 0; i < 90; ++i)
            {
                const float ang = (float) i * 2.3999632f;
                const float z = std::fmod (t * 0.22f + std::fmod ((float) i * 0.6180339f, 1.f), 1.f);
                const float z0 = juce::jmax (0.f, z - 0.06f);
                const float r0 = R * 1.9f * z0 * z0, r1 = R * 1.9f * z * z;
                const float dx = std::cos (ang), dy = std::sin (ang) * 0.75f;
                g.setColour ((i % 3 == 0 ? c3 : (i % 3 == 1 ? c1 : c2)).withAlpha (juce::jlimit (0.f, 0.8f, z * 1.2f)));
                g.drawLine (cx + dx * r0, cy + dy * r0, cx + dx * r1, cy + dy * r1, 0.8f + 1.4f * z);
            }
            // radar sweep
            const float a = t * 0.9f;
            juce::Path sweep;
            sweep.startNewSubPath (cx, cy);
            sweep.addCentredArc (cx, cy, R, R, 0.f, a - 0.55f, a, false);
            sweep.closeSubPath();
            g.setGradientFill (juce::ColourGradient (c1.withAlpha (0.02f), cx, cy, c1.withAlpha (0.20f), cx + std::sin (a) * R, cy - std::cos (a) * R, false));
            g.fillPath (sweep);
            g.setColour (c1.withAlpha (0.6f));
            g.drawLine (cx, cy, cx + std::sin (a) * R, cy - std::cos (a) * R, 1.4f);
            // scanner line
            const float sy = std::fmod (t * 40.f, H);
            g.setColour (c2.withAlpha (0.10f));  g.fillRect (0.f, sy - 6.f, W, 12.f);
            g.setColour (c2.withAlpha (0.35f));  g.drawLine (0.f, sy, W, sy, 1.f);
        }
        else
        {
            // drifting golden dust
            for (int i = 0; i < 120; ++i)
            {
                const float f1 = std::fmod ((float) i * 0.6180339f, 1.f), f2 = std::fmod ((float) i * 0.7548777f + 0.3f, 1.f);
                const float y = H - std::fmod (f2 * H + t * (8.f + 22.f * f1), H);
                const float x = f1 * W + 14.f * std::sin (t * 0.8f + (float) i);
                g.setColour ((i % 2 ? c3 : c1).withAlpha (0.15f + 0.45f * f1));
                const float s = 1.f + 2.4f * f1;
                g.fillEllipse (x, y, s, s);
            }
        }

        // drifting crystals (the GITI artwork floats around)
        const GitiCrystal::Id ids[6] = { GitiCrystal::Diamond, GitiCrystal::HexSwirl, GitiCrystal::Octagon,
                                          GitiCrystal::WaveDisc, GitiCrystal::Arabic, GitiCrystal::Vortex };
        const int n = mode == 0 ? 4 : 6;
        for (int i = 0; i < n; ++i)
        {
            const auto& img = GitiCrystal::get (ids[i]);
            if (! img.isValid()) continue;
            const float px = cx + (W * 0.38f) * std::sin (t * (0.13f + 0.03f * (float) i) + (float) i * 1.7f);
            const float py = cy + (H * 0.34f) * std::sin (t * (0.17f + 0.02f * (float) i) + (float) i * 2.4f);
            const float sz = 46.f + 16.f * (float) (i % 3) + 6.f * std::sin (t + (float) i);
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::rotation (t * 0.35f * (i % 2 ? -1.f : 1.f), px, py));
            g.setOpacity (0.55f);
            g.drawImage (img, juce::Rectangle<float> (sz, sz).withCentre ({ px, py }), juce::RectanglePlacement::centred);
        }
    }
};

class SalekHightechAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  public juce::DragAndDropContainer,
                                          public juce::Timer,
                                          public juce::ListBoxModel
{
public:
    explicit SalekHightechAudioProcessorEditor (SalekHightechAudioProcessor&);
    ~SalekHightechAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress& key) override;
    bool keyStateChanged (bool isKeyDown) override;
private:
    SalekHightechAudioProcessor& processor;
    SalekLookAndFeel lnf;
    ScopeDisplay scope;
    SpectrumDisplay spectrum;
    std::unique_ptr<WavetableDisplay> wtDisplay;
    std::unique_ptr<AdsrDisplay> adsrDisplay;
    std::unique_ptr<FilterCurveDisplay> filterDisplay;
    std::unique_ptr<LfoDisplay> lfoDisplay;
    std::unique_ptr<ModMatrixPanel> matrixPanel;
    std::unique_ptr<SonicCoreGL> sonicCore;
    std::unique_ptr<OscShapeMonitor> oscMon1, oscMon2, oscMon3;
    juce::VBlankAttachment vblank;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    struct Knob { salek::ModDropSlider s; juce::Label name; juce::String paramId; };
    std::vector<std::unique_ptr<Knob>> knobs;
    using SAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<SAtt>> atts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAtts;
    juce::Component mainTab, oscTab, filterTab, envTab, modTab, lfoTab, fxTab, magicTab, seqTab, speedTab, aboutTab, presetTab, modularTab;
    juce::ComboBox themeBox;
    juce::ComboBox presetFilterBox;
    juce::TextEditor presetSearchBox;
    juce::ComboBox filterMode, lfoWave;
    juce::ComboBox filterRouteBox, voiceModeBox;
    juce::ComboBox seqMagicTargetBox;
    juce::ComboBox distModeBox, reverbModeBox, delayModeBox, compModeBox;
    juce::Rectangle<int> fxModRect[8];   // FX module cells in fxTab coordinates (set by the layout)
    juce::Label buildLabel;              // "build 0.9.1": shows which build is running
    juce::ToggleButton arpOn { "ARP ON" }, seqOn { "SEQ ON" };
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, initBtn { "INIT" }, savePresetBtn { "SAVE" }, quickSaveBtn { "QSAVE" }, loadPresetBtn { "LOAD" }, bankBtn { "BANK" };
    juce::Label presetLabel, title, tagline;
    juce::ListBox presetList { "presets", this };
    int presetHoverRow = -1;
    struct PresetHoverListener : public juce::MouseListener
    {
        juce::ListBox* list = nullptr; int* row = nullptr;
        void mouseMove  (const juce::MouseEvent& e) override { update (e); }
        void mouseEnter (const juce::MouseEvent& e) override { update (e); }
        void mouseExit  (const juce::MouseEvent&) override { if (row && *row != -1) { *row = -1; if (list) list->repaint(); } }
        void update (const juce::MouseEvent& e)
        {
            if (list == nullptr || row == nullptr) return;
            const auto p = e.getEventRelativeTo (list).getPosition();
            const int r = list->getRowContainingPosition (p.x, p.y);
            if (r != *row) { *row = r; list->repaint(); }
        }
    };
    PresetHoverListener presetHoverListener;
    juce::TextButton filterStyleBtn { "FADERS" };
    juce::TextButton oscSelBtn[3];
    // FM ONLY toggles (OSC2 / OSC3). Declared after the buttons so attachments die first.
    juce::TextButton fmMuteBtn[2];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fmMuteAtt[2];
    // FX chain order: POS selector in each effect module header (1..8)
    juce::ComboBox fxPosBox[salek::fxchain::Count];
    int selectedOsc = 0;
    // ---- new layout state ----
    juce::TextButton nsSelBtn[2], advBtn[2], filterTabBtn[2];
    juce::TextButton filterLed[5];
    juce::ComboBox filter2Mode;
    bool advOpen[2] = { true, true };
    int sel23 = 0, selNS = 0, filterSel = 0;
    juce::Rectangle<int> oscFrameRect[3], envFrameRect[2], nsScopeRect;
    GitiAmbient fxAmbient { 0 }, aboutAmbient { 1 };
    void syncFilterLamps();
    void updateModBadges();
    GitiVisualizer visualizer { [this] { return processor.getOutputPeak(); } };
    bool filterFader = false;
    std::unique_ptr<StepGridComponent> stepGrid;
    std::unique_ptr<AudioSpeedMonitor> speedMonitor;
    std::unique_ptr<AboutInfoPanel> aboutInfo;
    juce::OwnedArray<juce::Label> fxSectionLabels;
    juce::OwnedArray<juce::ImageComponent> fxSectionIcons;
    juce::OwnedArray<juce::ImageComponent> tabSectionIcons; // MAIN/LFO/MOD badges
    juce::OwnedArray<juce::TextButton> seqMagicBtns;
    juce::TextButton seqMagicOff;
    juce::OwnedArray<FxMonitor> fxMonitors;
    juce::ToggleButton fxBypass[8];
    juce::TextButton fxAddBtn[8];          // "+ ADD <effect>" tile shown in an empty slot
    juce::TextButton presetToggle { "<<" }; // collapses preset list only
    juce::TextButton bgSwapBtn { "BG" }; // swap background art
    juce::TextButton inspireBtn { "INSPIRE" };
    juce::TextButton charCycleBtn { "CHAR" }; // cycle character overlay PNGs
    juce::TextButton modSrcLfo1 { "LFO1" }, modSrcLfo2 { "LFO2" }, modSrcLfo3 { "LFO3" };
    int armedModSource = -1; // 0/1/2 = LFO1/2/3 for click-to-assign
    bool isModDragging = false; // true while dragging from LFO pill
    int layoutRecoveryTicks = 0;
    bool editorWasShowing = false;
    int restoreLayoutDelay = 1;
    juce::Slider masterGainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAtt;
    juce::TextButton langToggle { "EN" };   // EN <-> FA
    int charPortraitIdx = 5;
    juce::Image charImgL, charImgR; // cached portraits
    juce::Rectangle<int> charSlotBounds; // left under preset list
    juce::Image cachedBg;           // pre-scaled hero (rebuild on resize/theme only)
    int cachedBgW = 0, cachedBgH = 0, cachedBgTheme = -1;
    bool uiLangFa = false;
    LfoShapeEditor lfoShapeEditor;
    juce::ComboBox lfo1WaveBox, lfo2WaveBox, lfo3WaveBox;
    juce::TextButton lfoSyncBtn[3], lfoBipBtn[3], lfoRetrigBtn[3];
    juce::Rectangle<int> lfoColBox[3];
    juce::Rectangle<int> modBox[6];
    std::unique_ptr<GroupFrames> lfoFrames, modFrames;   // framed groups on the LFO and MOD tabs      // MOD tab frames: FM, AM, PM, RM, MACRO built-in, MACRO user   // frame around each LFO block (for the paint)
    juce::ComboBox lfoDivBox[3];
    juce::TextButton sizeBtn { "100%" };
    juce::ComboBox noiseTypeBox;
    bool holdOn[9] = {};
    juce::TextButton onBtn[5];
    juce::TextButton fmInvBtn { "INV" };
    juce::TextButton waveQuick[3][6];   // SIN TRI SQR SAW BSAW PLS: one-click basic waveforms per oscillator   // OSC1, OSC2, OSC3, NOISE, SUB power buttons
    int uiScalePct = 100;
    juce::ComboBox lfoShapeBank; // 32 preset shapes
    juce::ComboBox lfoPointsBox; // 8 / 16 / 32 editable vertices
    juce::TextButton lfoPresetSine { "SIN" }, lfoPresetTri { "TRI" }, lfoPresetSaw { "SAW" },
                     lfoPresetSqr { "SQR" }, lfoPresetPulse { "PLS" }, lfoPresetCustom { "DRAW" },
                     lfoPresetExp { "EXP" }, lfoPresetLog { "LOG" }, lfoPresetBell { "BELL" },
                     lfoPresetWob { "WOB" }, lfoPresetChaos { "CHAOS" }, lfoPresetGate { "GATE" };
    // Copy custom shape across LFOs + 3 reusable slots
    juce::TextButton lfoCopyTo1 { "->L1" }, lfoCopyTo2 { "->L2" }, lfoCopyTo3 { "->L3" },
                     lfoSaveA { "SAVE A" }, lfoLoadA { "LOAD A" },
                     lfoSaveB { "SAVE B" }, lfoLoadB { "LOAD B" },
                     lfoSaveC { "SAVE C" }, lfoLoadC { "LOAD C" };
    juce::ComboBox osc1ShapeBox, osc2ShapeBox, osc3ShapeBox;
    int lfoShapeTarget = 0;
    std::unique_ptr<MagicPad> magicPad;
    juce::TextButton magicLoopBtn, magicGlitchBtn, magicFlangeBtn, magicPsychBtn, magicMasterBtn;
    juce::TextButton likeBtn, dislikeBtn;   // SPARK: LIKE saves the sound / NOPE rolls the same style again
    int lastSparkStyle = -1;
    juce::String lastSparkCat;
    std::function<int ()> currentMagicMask;               // effective mask (magic_mask, or magic_mode if -1)
    std::function<void (int)> setMagicMask;               // writes magic_mask + engine, refreshes the LED buttons
    salek::MomentaryLatchButton magicHold { };
    juce::Label magicHint;
    juce::MidiKeyboardComponent keyboard;
    int computerKeyboardBase = 36; // computer-keyboard C2; convenient bass range
    juce::Array<int> computerKeyboardHeldNotes;
    juce::Array<int> computerKeyboardHeldKeys;
    float phaseLights = 0.0f;
    float animPhase = 0.0f;
    juce::Colour themeAccent { 0xff00e8ff }, themeAccent2 { 0xffffd700 }, themePanelBg { 0xff0a0614 };
    juce::Image logoImg, logoAltImg, heroImg, faceImg, lianImg, cyanImg;
    struct LogoOverlayComp : public juce::Component
    {
        juce::Image img, cache;   // legacy hero art (unused now)
        void paint (juce::Graphics& g) override
        {
            const auto& logo = GitiCrystal::get (GitiCrystal::Logo);
            if (! logo.isValid()) return;
            const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
            const auto b = getLocalBounds().toFloat().reduced (2.f);
            const float sc = juce::jmin (b.getWidth() / (float) logo.getWidth(), b.getHeight() / (float) logo.getHeight());
            const auto dest = juce::Rectangle<float> ((float) logo.getWidth() * sc, (float) logo.getHeight() * sc).withCentre (b.getCentre());
            const auto c = dest.getCentre();
            const float breathe = 0.5f + 0.5f * (float) std::sin (now * 1.4);
            g.setGradientFill (juce::ColourGradient (giti::primaryColour().withAlpha (0.10f + 0.12f * breathe), c.x, c.y,
                                                     juce::Colour (0x00000000), c.x + dest.getWidth() * 0.62f, c.y, true));
            g.fillEllipse (c.x - dest.getWidth() * 0.62f, c.y - dest.getHeight() * 0.7f, dest.getWidth() * 1.24f, dest.getHeight() * 1.4f);
            g.setOpacity (1.f);
            g.drawImage (logo, dest, juce::RectanglePlacement::centred);
            // twinkling sparkles along the wings / letters
            for (int i = 0; i < 7; ++i)
            {
                const float k = std::fmod ((float) now * 0.45f + (float) i * 0.143f, 1.f);
                const float px = dest.getX() + dest.getWidth() * (0.14f + 0.72f * std::fmod ((float) i * 0.37f + 0.11f, 1.f));
                const float py = dest.getY() + dest.getHeight() * (0.16f + 0.7f * std::fmod ((float) i * 0.61f + 0.23f, 1.f));
                const float a = std::sin (k * juce::MathConstants<float>::pi);
                g.setColour (juce::Colours::white.withAlpha (0.85f * a));
                g.drawLine (px - 5.f * a, py, px + 5.f * a, py, 1.2f);
                g.drawLine (px, py - 5.f * a, px, py + 5.f * a, 1.2f);
            }
        }
    };
    LogoOverlayComp logoOverlay;
    struct RandomZonesComp : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
    {
        static constexpr int kZones = 6;
        std::function<void (const juce::String&)> onPick;
        static const char* name (int i) { static const char* n[] = { "BASS", "PAD", "LEAD", "PLUCK", "FX", "FM" }; return n[juce::jlimit (0, kZones - 1, i)]; }
        static juce::Colour col (int i)
        {
            static const juce::uint32 c[] = { 0xffe0243c, 0xff2ecf68, 0xff3a86ff, 0xffa24bff, 0xffffb000, 0xffff4fa3 };
            return juce::Colour (c[juce::jlimit (0, kZones - 1, i)]);
        }
        int hover = -1, pressed = -1, flashZone = -1;
        float flash = 0.f;

        int zoneAt (float x) const { return juce::jlimit (0, kZones - 1, (int) (x / juce::jmax (1.f, (float) getWidth() / (float) kZones))); }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            const float w = r.getWidth() / (float) kZones;
            for (int i = 0; i < kZones; ++i)
            {
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.f);
                const bool isDown = (i == pressed), isHover = (i == hover);
                if (isDown) cell = cell.reduced (2.f);                                  // pressed: the button sinks
                g.setColour ((isDown ? col (i).darker (0.25f) : col (i)).withAlpha (isHover || isDown ? 0.97f : 0.78f));
                g.fillRoundedRectangle (cell, 4.f);
                if (isHover || isDown)
                {
                    g.setColour (juce::Colours::white.withAlpha (isDown ? 0.9f : 0.55f));
                    g.drawRoundedRectangle (cell, 4.f, 1.5f);
                }
                if (i == flashZone && flash > 0.f)                                      // after the click: a short flash
                {
                    g.setColour (juce::Colours::white.withAlpha (flash * 0.6f));
                    g.fillRoundedRectangle (cell, 4.f);
                }
                g.setColour (juce::Colours::white);
                const bool wide = cell.getWidth() > 40.f;
                g.setFont (juce::FontOptions (wide ? 13.f : 8.f, juce::Font::bold));
                g.drawText (wide ? juce::String (name (i)) : juce::String (name (i)).substring (0, 1), cell.toNearestInt(), juce::Justification::centred);
            }
        }
        void mouseMove (const juce::MouseEvent& e) override { const int z = zoneAt (e.position.x); if (z != hover) { hover = z; repaint(); } }
        void mouseExit (const juce::MouseEvent&) override  { hover = -1; pressed = -1; repaint(); }
        void mouseDown (const juce::MouseEvent& e) override { pressed = zoneAt (e.position.x); hover = pressed; repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            const int z = zoneAt (e.position.x);
            const bool fire = pressed >= 0 && z == pressed && getLocalBounds().contains (e.getPosition());
            pressed = -1;
            if (fire)
            {
                flashZone = z; flash = 1.f; startTimerHz (60);
                if (onPick) onPick (name (z));
            }
            repaint();
        }
        void timerCallback() override
        {
            flash -= 0.09f;
            if (flash <= 0.f) { flash = 0.f; stopTimer(); }
            repaint();
        }
    };
    RandomZonesComp randomZones;
    ModStagePanel modStage;
    SpaceScene spaceScene;
    std::unique_ptr<QuickLfoPanel> quickLfo[3];
    std::vector<juce::Image> bgImages;
    int bgIndex = 0;
    struct ImageOverlay : public juce::Component
    {
        juce::Image img;
        ImageOverlay() { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics& g) override
        {
            if (img.isValid()) g.drawImage (img, getLocalBounds().toFloat(), juce::RectanglePlacement::centred);
        }
    };
    ImageOverlay stageLogo;
    juce::Image uiLogo;
    juce::TextButton updateBtn { "UPDATE" };
    juce::TextButton storeBtn { "STORE" };
    juce::TextButton undoBtn { "UNDO" }, redoBtn { "REDO" }, drawWaveBtn { "DRAW" };
    juce::TextButton magicRndBtn { "RND X/Y" };
    juce::TextButton ecoBtn { "ECO" }, phoneBtn { "PHONE" };
    salek::MomentaryLatchButton holdBtn[4];          // MAGIC: per-editor state (was static)
    int activeMode = 0;
    juce::TextButton magicCombo[14];
    bool combosInited = false;
    std::shared_ptr<bool> editorAlive = std::make_shared<bool> (true);   // false once the editor is destroyed (async callbacks check it)   // Performance: Eco = no oversampling (less CPU)   // random position for the MAGIC X/Y pad
    juce::Image proceduralBg (int mode, int w, int h);      // logo shown in the middle of the big MAIN-tab visualizer
    bool stageOnMain = false;
    int stageTopEd = 0;
    void refreshCharCache();
    int heroIndex = 0;
    bool presetCollapsed = false;
    void applyHeroFromTheme();
    void cycleHero();
    void applyUiLanguage();
    struct PresetRow { bool isHeader = false; juce::String label; int programIndex = -1; juce::String category; int itemCount = -1; };
    juce::StringArray collapsedCats; // category names that are folded
    juce::Array<PresetRow> presetRows;
    juce::String getPresetCategory (const juce::String& programName) const;
    void rebuildPresetRows();
    Knob& addKnob(juce::Component& parent, const char* id, const char* label, juce::Colour c);
    void assignModToParam (const juce::String& paramId, float amount = 0.5f);
    void assignModSourceToParam (int lfoIndex, const juce::String& paramId, float amount);   // 0..2 = LFO1..3
    bool resolveModDest (const juce::String& paramId, salek::ModMatrix::Dest& dest);
    void showModMenu (const juce::String& paramId);   // right-click on a knob: list / remove its modulation
    void tryAssignModAt (juce::Point<int> editorPos, float amount, const juce::ModifierKeys& mods);
    juce::Point<int> currentMousePositionInEditor() const;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    std::vector<std::unique_ptr<juce::MouseListener>> modHookListeners;
    struct KnobDropTarget : public juce::Component, public juce::DragAndDropTarget
    {
        SalekHightechAudioProcessorEditor* ed = nullptr;
        juce::String paramId;
        bool hot = false;
        bool isInterestedInDragSource (const SourceDetails& d) override
        {
            return salek::parseModSourceDrag (d.description.toString()) >= 0;
        }
        void itemDragEnter (const SourceDetails&) override { hot = true;  repaint(); }
        void itemDragExit  (const SourceDetails&) override { hot = false; repaint(); }
        void itemDropped (const SourceDetails& d) override
        {
            hot = false;
            const int src = salek::parseModSourceDrag (d.description.toString());   // 0..2 = LFO1..3
            if (ed != nullptr && src >= 0)
            {
                float amt = 0.5f;
                if (juce::ModifierKeys::currentModifiers.isShiftDown()) amt = 1.0f;
                if (juce::ModifierKeys::currentModifiers.isAltDown())   amt = -0.5f;
                ed->assignModSourceToParam (src, paramId, amt);   // exactly one route, one undo step
            }
            repaint();
        }
        void paint (juce::Graphics& g) override
        {
            if (! hot) return;
            g.setColour (juce::Colour (0xff00e8ff).withAlpha (0.4f));
            g.fillEllipse (getLocalBounds().toFloat().reduced (2.f));
            g.setColour (juce::Colour (0xffff2d9b).withAlpha (0.95f));
            g.drawEllipse (getLocalBounds().toFloat().reduced (2.f), 2.5f);
        }
        bool shouldDrawDragImageWhenOver() override { return false; }
        // Critical: only intercept hits while a drag is active so slider stays usable
        bool hitTest (int, int) override
        {
            return ed != nullptr && ed->isDragAndDropActive();
        }
    };
    std::vector<std::unique_ptr<KnobDropTarget>> knobDropTargets;
    // bottom model strip
    juce::OwnedArray<juce::ImageComponent> modelStrip;
    juce::Label modelStripTitle;
    juce::Rectangle<int> modelStripBounds;
    void addCombo(juce::Component& parent, juce::ComboBox& box, const char* id, juce::StringArray items);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SalekHightechAudioProcessorEditor)
};
