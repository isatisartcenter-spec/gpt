# LFO Drag & Drop onto Knobs

## Already in tree
- `Source/UI/ModDropSupport.h` — `ModDropSlider`, `ModSourceDragButton`, `paramIdToDest`
- Editor is already `juce::DragAndDropContainer`
- LFO pills: `modSrcLfo1/2/3`

## Wire-up (PluginEditor.h)

1. Include:
```cpp
#include "UI/ModDropSupport.h"
```

2. Change Knob slider type:
```cpp
struct Knob {
    salek::ModDropSlider s;  // was juce::Slider
    juce::Label name;
    juce::String paramId;
};
```

3. Before `#include "PluginEditorAddKnob.inl"` define:
```cpp
#define SALEK_MOD_DROP_SLIDER 1
```

4. LFO source buttons — use `ModSourceDragButton` or in existing onClick/mouseDrag:
```cpp
// Example for LFO1 pill
modSrcLfo1.onClick = [this] { armedModSource = 0; };
// Prefer drag: subclass or
// container->startDragging (salek::modSourceDragDescription (0), &modSrcLfo1);
```

Replace `juce::TextButton modSrcLfo1` with `salek::ModSourceDragButton modSrcLfo1` and:
```cpp
modSrcLfo1.setSourceIndex (0);
modSrcLfo2.setSourceIndex (1);
modSrcLfo3.setSourceIndex (2);
```

## Usage
1. Drag **LFO1 / LFO2 / LFO3** pill
2. Drop on any mapped knob (cutoff, reso, osc levels, warp, fold, FM, delay/reverb/chorus mix, …)
3. Route appears in **MOD ROUTES** panel (depth adjustable)

## Mapped destinations
See `paramIdToDest()` in `ModDropSupport.h`.

Unmapped knobs still work as normal controls; drop is ignored.
