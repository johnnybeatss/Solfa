#pragma once

#include "dsp/EqEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs are stored in DAW sessions: never rename one after release.
// Bump the version number in juce::ParameterID instead.
namespace solfa::params
{
    inline constexpr auto outputGain = "outputGain";
    inline constexpr auto amount = "amount";
    inline constexpr auto autoGain = "autoGain"; // static loudness compensation from the curve
    inline constexpr auto compare = "compare";   // loudness-matched bypass for honest A/B

    // Per-band IDs: "b1_on", "b1_type", "b1_freq", "b1_gain", "b1_q" ... "b8_q"
    juce::String bandId (int bandIndex, const char* suffix);

    inline constexpr auto on = "on";
    inline constexpr auto type = "type";
    inline constexpr auto freq = "freq";
    inline constexpr auto gain = "gain";
    inline constexpr auto q = "q";
    inline constexpr auto slope = "slope"; // low/high cut steepness
    inline constexpr auto place = "place"; // Stereo / Left / Right / Mid / Side

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Lock-free handles to every parameter, read on the audio thread.
    struct BandHandles
    {
        std::atomic<float>* on = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* slope = nullptr;
        std::atomic<float>* place = nullptr;

        dsp::BandTarget read() const noexcept;
    };

    struct Handles
    {
        explicit Handles (juce::AudioProcessorValueTreeState& state);

        std::atomic<float>* outputGain = nullptr;
        std::atomic<float>* amount = nullptr;
        std::atomic<float>* autoGain = nullptr;
        std::atomic<float>* compare = nullptr;
        std::array<BandHandles, dsp::numBands> bands;
    };
}
