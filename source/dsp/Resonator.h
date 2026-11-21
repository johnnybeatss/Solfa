#pragma once

#include "Biquad.h"

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>

namespace solfa::dsp
{
    // Resonance: tuned "sympathetic strings". A small bank of very narrow band-pass resonators at
    // the focus frequency and its octaves is excited by the incoming audio and mixed in, so the
    // focus frequencies ring with the music and decay when it stops. Each resonator peaks at
    // 0 dB, so it can only sustain energy already present near its frequency.
    class Resonator
    {
    public:
        static constexpr int numVoices = 4;
        static constexpr std::array<double, numVoices> octaveRatios { 0.5, 1.0, 2.0, 4.0 };
        static constexpr std::array<double, numVoices> voiceWeights { 0.5, 1.0, 0.7, 0.4 };

        void prepare (double sampleRate, int numChannels);
        void reset() noexcept;

        // Set once per block from the audio thread.
        void setFocus (double frequencyHz) noexcept;
        void setDecaySeconds (double t60) noexcept;
        void setAmount (double linearGain) noexcept;

        // Adds the resonance to the buffer in place. Does nothing (bit-exact) while silent.
        void process (juce::AudioBuffer<float>& buffer) noexcept;

        bool isActive() const noexcept { return active; }

        // Q giving a ring time of t60 seconds at frequencyHz: Q = pi f T60 / ln(1000).
        static double qForDecay (double frequencyHz, double t60) noexcept;

    private:
        void updateCoefficients() noexcept;

        static constexpr int updateInterval = 16;
        double sampleRate = 48000.0;
        int channels = 2;
        bool active = false;
        double targetFocus = 528.0, targetDecay = 1.5;
        juce::SmoothedValue<double> log2Focus { std::log2 (528.0) };
        juce::SmoothedValue<double> log2Decay { std::log2 (1.5) };
        juce::SmoothedValue<double> amount { 0.0 };
        bool dirty = true;

        std::array<std::array<Biquad, numVoices>, 2> voices;
    };
}
