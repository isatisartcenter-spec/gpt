#pragma once
#include <JuceHeader.h>
#include "../Modulation/ModMatrix.h"

namespace salek
{

/** Map APVTS parameter id → ModMatrix destination (for LFO drop targets). */
inline bool paramIdToDest (const juce::String& id, ModMatrix::Dest& out) noexcept
{
    if (id == "filter_cutoff") { out = ModMatrix::Dest::FilterCutoff; return true; }
    if (id == "filter_reso" || id == "filter_res") { out = ModMatrix::Dest::FilterReso; return true; }
    if (id == "filter_env") { out = ModMatrix::Dest::FilterEnv; return true; }
    if (id == "osc1_level") { out = ModMatrix::Dest::Osc1Level; return true; }
    if (id == "osc2_level") { out = ModMatrix::Dest::Osc2Level; return true; }
    if (id == "osc3_level") { out = ModMatrix::Dest::Osc3Level; return true; }
    if (id == "osc1_table" || id == "osc1_wtpos") { out = ModMatrix::Dest::Osc1Table; return true; }
    if (id == "osc2_table" || id == "osc2_wtpos") { out = ModMatrix::Dest::Osc2Table; return true; }
    if (id == "osc3_table" || id == "osc3_wtpos") { out = ModMatrix::Dest::Osc3Table; return true; }
    if (id == "osc1_warp") { out = ModMatrix::Dest::Osc1Warp; return true; }
    if (id == "osc2_warp") { out = ModMatrix::Dest::Osc2Warp; return true; }
    if (id == "osc3_warp") { out = ModMatrix::Dest::Osc3Warp; return true; }
    if (id == "osc1_fold") { out = ModMatrix::Dest::Osc1Fold; return true; }
    if (id == "osc2_fold") { out = ModMatrix::Dest::Osc2Fold; return true; }
    if (id == "osc3_fold") { out = ModMatrix::Dest::Osc3Fold; return true; }
    if (id == "osc1_drive") { out = ModMatrix::Dest::Osc1Drive; return true; }
    if (id == "osc2_drive") { out = ModMatrix::Dest::Osc2Drive; return true; }
    if (id == "osc3_drive") { out = ModMatrix::Dest::Osc3Drive; return true; }
    if (id == "osc1_pan") { out = ModMatrix::Dest::Osc1Pan; return true; }
    if (id == "osc2_pan") { out = ModMatrix::Dest::Osc2Pan; return true; }
    if (id == "osc3_pan") { out = ModMatrix::Dest::Osc3Pan; return true; }
    if (id == "fm_2to1" || id == "osc1_fm") { out = ModMatrix::Dest::Fm2to1; return true; }
    if (id == "fm_3to1") { out = ModMatrix::Dest::Fm3to1; return true; }
    if (id == "amp_level" || id == "master_gain") { out = ModMatrix::Dest::Amp; return true; }
    if (id == "delay_mix") { out = ModMatrix::Dest::DelayMix; return true; }
    if (id == "reverb_mix") { out = ModMatrix::Dest::ReverbMix; return true; }
    if (id == "chorus_mix") { out = ModMatrix::Dest::ChorusMix; return true; }
    if (id == "dist_drive") { out = ModMatrix::Dest::DistDrive; return true; }
    if (id == "dist_mix") { out = ModMatrix::Dest::DistMix; return true; }
    if (id == "master_drive") { out = ModMatrix::Dest::MasterDrive; return true; }
    if (id == "phaser_mix") { out = ModMatrix::Dest::PhaserMix; return true; }
    if (id == "bassify") { out = ModMatrix::Dest::Bassify; return true; }
    if (id == "magic_x") { out = ModMatrix::Dest::MagicX; return true; }
    if (id == "magic_y") { out = ModMatrix::Dest::MagicY; return true; }
    // Every pitch knob has its OWN destination (dropping an LFO on OCT does not touch SEMI / FINE / ...)
    {
        using D = ModMatrix::Dest;
        struct Row { const char* id; D d; };
        static const Row rows[] = {
            { "osc1_octave", D::Osc1Oct }, { "osc2_octave", D::Osc2Oct }, { "osc3_octave", D::Osc3Oct },
            { "osc1_semi", D::Osc1Semi }, { "osc2_semi", D::Osc2Semi }, { "osc3_semi", D::Osc3Semi },
            { "osc1_coarse", D::Osc1Crs }, { "osc2_coarse", D::Osc2Crs }, { "osc3_coarse", D::Osc3Crs },
            { "osc1_fine", D::Osc1FineP }, { "osc2_fine", D::Osc2FineP }, { "osc3_fine", D::Osc3FineP },
            { "osc1_detune", D::Osc1DetP }, { "osc2_detune", D::Osc2DetP }, { "osc3_detune", D::Osc3DetP },
            { "sub_octave", D::SubOct }, { "sub_semi", D::SubSemi },
            { "lfo_rate", D::Lfo1Rate }, { "lfo2_rate", D::Lfo2Rate }, { "lfo3_rate", D::Lfo3Rate },
            { "lfo_amount", D::Lfo1Amt }, { "lfo2_amount", D::Lfo2Amt }, { "lfo3_amount", D::Lfo3Amt },
            { "osc1_udet", D::Osc1UDet }, { "osc2_udet", D::Osc2UDet }, { "osc3_udet", D::Osc3UDet },
            { "delay_fb", D::DelayFb }, { "reverb_size", D::ReverbSize }, { "delay_time", D::DelayTime },
            { "chorus_rate", D::ChorusRate }, { "chorus_depth", D::ChorusDepth }, { "phaser_rate", D::PhaserRate },
            { "phaser_depth", D::PhaserDepth }, { "reverb_decay", D::ReverbDecay },
            { "amp_attack", D::AmpAttack }, { "amp_decay", D::AmpDecay }, { "amp_sustain", D::AmpSustain }, { "amp_release", D::AmpRelease },
            { "macro1", D::MacroMod1 }, { "macro2", D::MacroMod2 }, { "macro3", D::MacroMod3 }, { "macro4", D::MacroMod4 },
            { "macro5", D::MacroMod5 }, { "macro6", D::MacroMod6 }, { "macro7", D::MacroMod7 }, { "macro8", D::MacroMod8 },
            { "comp_mix", D::CompMix }, { "comp_threshold", D::CompThreshold }, { "comp_gain", D::CompGain }, { "comp_depth", D::CompDepth } };
        for (auto& r : rows) if (id == r.id) { out = r.d; return true; }
    }
    if (id == "noise_level") { out = ModMatrix::Dest::NoiseLevel; return true; }
    if (id == "filter_drive") { out = ModMatrix::Dest::FilterDrive; return true; }
    if (id == "filter2_cutoff") { out = ModMatrix::Dest::Filter2Cutoff; return true; }
    if (id == "filter2_reso") { out = ModMatrix::Dest::Filter2Reso; return true; }
    if (id == "filter2_drive") { out = ModMatrix::Dest::Filter2Drive; return true; }
    if (id == "filter_shape")  { out = ModMatrix::Dest::FilterShape;  return true; }
    if (id == "filter2_shape") { out = ModMatrix::Dest::Filter2Shape; return true; }
    if (id == "filter1_mix") { out = ModMatrix::Dest::Filter1Mix; return true; }
    if (id == "filter2_mix") { out = ModMatrix::Dest::Filter2Mix; return true; }
    if (id == "sub_level") { out = ModMatrix::Dest::SubLevel; return true; }
    return false;
}

/** Drag payload: "SHAE_MOD_SRC:<sourceIndex>" */
inline juce::String modSourceDragDescription (int sourceIndex) 
{
    return "SHAE_MOD_SRC:" + juce::String (sourceIndex);
}

inline int parseModSourceDrag (const juce::String& desc) noexcept
{
    if (! desc.startsWith ("SHAE_MOD_SRC:")) return -1;
    return desc.fromFirstOccurrenceOf (":", false, false).getIntValue();
}

/** Rotary knob with a Serum/Vital-style modulation ring.
    - When an LFO is routed into this knob, a coloured arc shows how far it moves the value.
    - Drag vertically ON that ring to change the LFO amount for this knob only.
    - Hovering shows the route, e.g. "LFO1 +0.34".
    Dropping an LFO onto a knob is handled by the editor's drop target (one drop = one route). */
class ModDropSlider : public juce::Slider
{
public:
    std::function<void()> onModGestureStart;   // called once before a ring drag (undo point)
    std::function<void()> onModChanged;        // lets the editor refresh the matrix view

    void setModContext (ModMatrix* m, juce::String parameterId)
    {
        matrix = m;
        paramId = std::move (parameterId);
    }

    /** First LFO (0..2) routed into this knob. Returns -1 when none; amountOut is its amount. */
    int getLfoRoute (float& amountOut) const noexcept
    {
        ModMatrix::Dest dest;
        if (! routeDest (dest)) return -1;
        for (int i = 0; i < 3; ++i)
            if (matrix->getRouteAmount (lfoSource (i), dest, amountOut)) return i;
        return -1;
    }

    void paint (juce::Graphics& g) override
    {
        juce::Slider::paint (g);

        // hover or drag: a glowing cyan ring around the knob, so the selected knob is obvious
        if (isMouseOver (true) || ringDragging)
        {
            const auto cc = knobCentre();
            const float rr = juce::jmin (knobRadius() + 4.0f, juce::jmin (getWidth(), getHeight()) * 0.5f - 1.5f);
            juce::Path halo;
            halo.addEllipse (cc.x - rr, cc.y - rr, rr * 2.0f, rr * 2.0f);
            // soft outer glow (wide, faint), then a bright cyan band and a white shine line
            for (int layer = 4; layer >= 1; --layer)
            {
                g.setColour (juce::Colour (0xffb0ff3c).withAlpha (0.11f * (float) (5 - layer)));
                g.strokePath (halo, juce::PathStrokeType ((float) layer * 2.4f));
            }
            g.setColour (juce::Colour (0xffeaffc4).withAlpha (0.95f));
            g.strokePath (halo, juce::PathStrokeType (1.5f));
        }

        float amt = 0.f;
        const int src = getLfoRoute (amt);
        if (src < 0) return;

        const auto rp = getRotaryParameters();
        const auto centre = knobCentre();
        const float radius = knobRadius() + 3.0f;
        const float p = (float) valueToProportionOfLength (getValue());
        const float q = juce::jlimit (0.0f, 1.0f, p + amt);
        const float a0 = rp.startAngleRadians + p * (rp.endAngleRadians - rp.startAngleRadians);
        const float a1 = rp.startAngleRadians + q * (rp.endAngleRadians - rp.startAngleRadians);

        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                           juce::jmin (a0, a1), juce::jmax (a0, a1), true);
        g.setColour (ringColour (src).withAlpha (0.95f));
        g.strokePath (arc, juce::PathStrokeType (2.4f));

        if (isMouseOver (true) || ringDragging)
        {
            g.setColour (ringColour (src));
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText ("LFO" + juce::String (src + 1) + "  " + (amt >= 0.f ? "+" : "")
                            + juce::String (amt, 2),
                        getLocalBounds().removeFromTop (12), juce::Justification::centredTop, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        float amt = 0.f;
        const int src = getLfoRoute (amt);
        if (src >= 0 && ! e.mods.isPopupMenu() && (e.mods.isAltDown() || isOnRingExact (e.position)))
        {
            ringDragging = true;
            ringSrc = src;
            ringStartAmt = amt;
            ringStartY = e.position.y;
            if (onModGestureStart) onModGestureStart();
            return;
        }
        juce::Slider::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! ringDragging) { juce::Slider::mouseDrag (e); return; }
        ModMatrix::Dest dest;
        if (matrix == nullptr || ! routeDest (dest)) return;
        // 100 px of drag = full range; never 0 so the route (and the ring) survive the drag
        float amt = juce::jlimit (-1.0f, 1.0f, ringStartAmt + (ringStartY - e.position.y) * 0.005f);   // 200 px = full range
        if (std::abs (amt) < 0.01f) amt = amt < 0.0f ? -0.01f : 0.01f;
        matrix->addRoute (lfoSource (ringSrc), dest, amt);
        if (onModChanged) onModChanged();
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (ringDragging) { ringDragging = false; repaint(); return; }
        juce::Slider::mouseUp (e);
    }

    void mouseEnter (const juce::MouseEvent& e) override { juce::Slider::mouseEnter (e); repaint(); }
    void mouseExit  (const juce::MouseEvent& e) override { juce::Slider::mouseExit (e);  repaint(); }

private:
    static ModMatrix::Source lfoSource (int i) noexcept
    {
        return i == 0 ? ModMatrix::Source::LFO1 : (i == 1 ? ModMatrix::Source::LFO2 : ModMatrix::Source::LFO3);
    }
    static juce::Colour ringColour (int lfo) noexcept
    {
        return lfo == 0 ? juce::Colour (0xff00e8ff) : (lfo == 1 ? juce::Colour (0xffff2d9b) : juce::Colour (0xffffc94a));
    }
    // Same geometry as SalekLookAndFeel::drawRotarySlider, so the ring and the hit area sit exactly on the drawn knob
    juce::Point<float> knobCentre() const
    {
        auto b = getLocalBounds().toFloat().reduced (3.0f);
        return { b.getCentreX(), b.getCentreY() - 1.0f };
    }
    float knobRadius() const
    {
        auto b = getLocalBounds().toFloat().reduced (3.0f);
        return juce::jmin (b.getWidth(), b.getHeight()) * (getProperties().contains ("big_knob") ? 0.47f : 0.40f);
    }

    bool routeDest (ModMatrix::Dest& dest) const noexcept
    {
        return matrix != nullptr && paramIdToDest (paramId, dest);
    }
    // ring = the band just outside the knob body (where the modulation arc is drawn)
    bool isOnRing (juce::Point<float> pos)
    {
        const float radius = knobRadius() + 3.0f;
        const float d = pos.getDistanceFrom (knobCentre());
        return d >= radius - 6.0f && d <= radius + 10.0f;   // generous band around the arc
    }

    bool isOnRingExact (juce::Point<float> pos) const
    {
        const float radius = knobRadius() + 3.0f;
        const float d = pos.getDistanceFrom (knobCentre());
        return d >= radius - 3.0f && d <= radius + 3.0f;   // only right on the arc; the knob body stays a normal knob
    }

    ModMatrix* matrix = nullptr;
    juce::String paramId;
    bool ringDragging = false;
    int ringSrc = 0;
    float ringStartAmt = 0.f;
    float ringStartY = 0.f;
};

/** Text button that starts a mod-source drag (LFO1/2/3 …). */
class ModSourceDragButton : public juce::TextButton
{
public:
    void setSourceIndex (int idx) noexcept { sourceIndex = idx; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        juce::TextButton::mouseDrag (e);
        if (e.getDistanceFromDragStart() > 6)
        {
            if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
            {
                container->startDragging (modSourceDragDescription (sourceIndex), this);
            }
        }
    }

private:
    int sourceIndex = 0;
};

} // namespace salek
