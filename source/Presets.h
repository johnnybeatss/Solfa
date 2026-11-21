#pragma once

#include "dsp/FilterDesign.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// Factory presets. Each is named after a solfeggio / 432 Hz number and shapes timbre with broad,
// gentle moves placed on that number's octaves (see docs/research/report.md and docs/presets.md).
// Descriptions describe sound character only: no health or therapeutic claims.
namespace solfa::presets
{
    struct Band
    {
        dsp::FilterType type;
        double frequencyHz, gainDb, q;
    };

    struct Preset
    {
        juce::String name;
        juce::String character;      // what it does to the sound
        double focusHz;              // the number the preset is named after
        std::vector<Band> bands;
        double resonancePercent;     // 0 = off
        double decaySeconds;
        int referenceIndex = -1;     // tuning reference to select (index into referenceOptionsHz), -1 = keep
    };

    // Index 0 is "Init" (everything at its default).
    const std::vector<Preset>& factory();

    // Sets every affected parameter; call on the message thread.
    void apply (const Preset& preset, juce::AudioProcessorValueTreeState& state);

    // User presets: XML files (*.solfapreset) in Documents/Solfa/Presets.
    inline constexpr auto userPresetExtension = ".solfapreset";
    juce::File userPresetFolder();
    juce::Array<juce::File> userPresetFiles();
    bool writeUserPreset (const juce::File& file, const juce::XmlElement& state);
    std::unique_ptr<juce::XmlElement> readUserPreset (const juce::File& file);
}
