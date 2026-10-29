#pragma once
#include <JuceHeader.h>

// Simple free preset system — users can save/load everything
class PresetManager
{
public:
    PresetManager (juce::AudioProcessorValueTreeState& vts) : apvts (vts)
    {
        presetDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                        .getChildFile ("SALEK_HIGHTECH")
                        .getChildFile ("Presets");
        presetDir.createDirectory();
    }

    juce::StringArray getPresetList() const
    {
        juce::StringArray list;
        for (auto& f : presetDir.findChildFiles (juce::File::findFiles, false, "*.salek"))
            list.add (f.getFileNameWithoutExtension());
        list.sort (true);
        return list;
    }

    bool savePreset (const juce::String& name)
    {
        auto file = presetDir.getChildFile (name + ".salek");
        auto state = apvts.copyState();
        if (auto xml = state.createXml())
            return xml->writeTo (file);
        return false;
    }

    bool loadPreset (const juce::String& name)
    {
        auto file = presetDir.getChildFile (name + ".salek");
        if (!file.existsAsFile()) return false;
        if (auto xml = juce::parseXML (file))
        {
            if (xml->hasTagName (apvts.state.getType()))
            {
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
                return true;
            }
        }
        return false;
    }

    juce::File getPresetDir() const { return presetDir; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::File presetDir;
};
