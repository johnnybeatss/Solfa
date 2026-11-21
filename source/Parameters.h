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

    // Phase 5: focus frequency, Key Snap, Literal and Resonance
    inline constexpr auto focus = "focus";         // the preset's named number, in Hz
    inline constexpr auto pure = "pure";           // true: use focus exactly; false: Key Snap it
    inline constexpr auto tuneRef = "tuneRef";     // A4 reference: 440 / 432 / 444
    inline constexpr auto key = "key";             // C .. B
    inline constexpr auto scale = "scale";         // Chromatic / Major / Minor
    inline constexpr auto literal = "literal";     // narrow boost at the focus frequency, 0-3 dB
    inline constexpr auto resAmount = "resAmount"; // Resonance level, %
    inline constexpr auto resDecay = "resDecay";   // Resonance ring time, s

    // Per-band IDs: "b1_on", "b1_type", "b1_freq", "b1_gain", "b1_q" ... "b8_q"
    juce::String bandId (int bandIndex, const char* suffix);

    inline constexpr auto on = "on";
    inline constexpr auto type = "type";
    inline constexpr auto freq = "freq";
    inline constexpr auto gain = "gain";
    inline constexpr auto q = "q";
    inline constexpr auto slope = "slope"; // low/high cut steepness
    inline constexpr auto place = "place"; // Stereo / Left / Right / Mid / Side
    inline constexpr auto dyn = "dyn";     // dynamic EQ on/off
    inline constexpr auto thr = "thr";     // dynamic threshold, dBFS
    inline constexpr auto range = "range"; // dynamic range, dB
    inline constexpr auto atk = "atk";     // dynamic attack, ms
    inline constexpr auto rel = "rel";     // dynamic release, ms

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
        std::atomic<float>* dyn = nullptr;
        std::atomic<float>* thr = nullptr;
        std::atomic<float>* range = nullptr;
        std::atomic<float>* atk = nullptr;
        std::atomic<float>* rel = nullptr;

        dsp::BandTarget read() const noexcept;
    };

    struct Handles
    {
        explicit Handles (juce::AudioProcessorValueTreeState& state);

        std::atomic<float>* outputGain = nullptr;
        std::atomic<float>* amount = nullptr;
        std::atomic<float>* autoGain = nullptr;
        std::atomic<float>* compare = nullptr;
        std::atomic<float>* focus = nullptr;
        std::atomic<float>* pure = nullptr;
        std::atomic<float>* tuneRef = nullptr;
        std::atomic<float>* key = nullptr;
        std::atomic<float>* scale = nullptr;
        std::atomic<float>* literal = nullptr;
        std::atomic<float>* resAmount = nullptr;
        std::atomic<float>* resDecay = nullptr;
        std::array<BandHandles, dsp::numBands> bands;
    };
}
