#include "Resonator.h"
#include "FilterDesign.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace solfa::dsp
{
    double Resonator::qForDecay (double frequencyHz, double t60) noexcept
    {
        return std::numbers::pi * frequencyHz * t60 / std::log (1000.0);
    }

    void Resonator::prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, 2);
        log2Focus.reset (sampleRate, 0.05);
        log2Decay.reset (sampleRate, 0.05);
        amount.reset (sampleRate, 0.05);
        log2Focus.setCurrentAndTargetValue (std::log2 (targetFocus));
        log2Decay.setCurrentAndTargetValue (std::log2 (targetDecay));
        reset();
    }

    void Resonator::reset() noexcept
    {
        for (auto& ch : voices)
            for (auto& v : ch)
                v.reset();
        active = false;
        dirty = true;
    }

    void Resonator::setFocus (double f) noexcept
    {
        targetFocus = f;
        log2Focus.setTargetValue (std::log2 (std::max (f, 10.0)));
    }

    void Resonator::setDecaySeconds (double t60) noexcept
    {
        targetDecay = t60;
        log2Decay.setTargetValue (std::log2 (std::max (t60, 0.05)));
    }

    void Resonator::setAmount (double g) noexcept { amount.setTargetValue (std::max (g, 0.0)); }

    void Resonator::updateCoefficients() noexcept
    {
        const auto focus = std::exp2 (log2Focus.getCurrentValue());
        const auto t60 = std::exp2 (log2Decay.getCurrentValue());

        for (int v = 0; v < numVoices; ++v)
        {
            // Bilinear (RBJ) band-pass: exact 0 dB peak at the prewarped centre frequency. Resonators
            // need far higher Q than an EQ band allows, so this bypasses design()'s Q limit.
            const auto f = std::min (focus * octaveRatios[(size_t) v], maxFrequencyRatio * sampleRate);
            const auto w0 = 2.0 * std::numbers::pi * f / sampleRate;
            const auto alpha = std::sin (w0) / (2.0 * qForDecay (f, t60));
            const auto a0 = 1.0 + alpha;
            const BiquadCoefficients c { alpha / a0, 0.0, -alpha / a0, -2.0 * std::cos (w0) / a0, (1.0 - alpha) / a0 };
            for (auto& ch : voices)
                ch[(size_t) v].setCoefficients (c);
        }
        dirty = false;
    }

    void Resonator::process (juce::AudioBuffer<float>& buffer) noexcept
    {
        const auto numSamples = buffer.getNumSamples();
        const auto numChannels = std::min (buffer.getNumChannels(), channels);

        const bool wanted = amount.getTargetValue() > 0.0 || amount.isSmoothing();
        if (! wanted)
        {
            bool silent = true;
            for (auto& ch : voices)
                for (auto& v : ch)
                    silent = silent && v.isSilent();
            if (silent || ! active)
            {
                if (active)
                    reset();
                return;
            }
        }
        active = true;

        for (int start = 0; start < numSamples; start += updateInterval)
        {
            const auto len = std::min (updateInterval, numSamples - start);
            if (log2Focus.isSmoothing() || log2Decay.isSmoothing())
            {
                log2Focus.skip (len);
                log2Decay.skip (len);
                dirty = true;
            }
            if (dirty)
                updateCoefficients();

            for (int i = start; i < start + len; ++i)
            {
                const auto g = amount.getNextValue();
                for (int c = 0; c < numChannels; ++c)
                {
                    auto* x = buffer.getWritePointer (c);
                    const auto in = (double) x[i];
                    double ring = 0.0;
                    for (int v = 0; v < numVoices; ++v)
                        ring += voiceWeights[(size_t) v] * voices[(size_t) c][(size_t) v].process (in);
                    x[i] = (float) (in + g * ring);
                }
            }
        }
    }
}
