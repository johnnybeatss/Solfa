#include "EqEngine.h"

#include <algorithm>
#include <cmath>

namespace solfa::dsp
{
    //==============================================================================
    void EqBand::prepare (double newSampleRate, double smoothingSeconds)
    {
        sampleRate = newSampleRate;
        log2Freq.reset (sampleRate, smoothingSeconds);
        gainDb.reset (sampleRate, smoothingSeconds);
        log2Q.reset (sampleRate, smoothingSeconds);
        cutMix.reset (sampleRate, smoothingSeconds);
        firstTarget = true;
        reset();
    }

    void EqBand::reset()
    {
        for (auto& f : filters)
            f.reset();
        coefficientsDirty = true;
    }

    bool EqBand::isGainType() const noexcept
    {
        return type != FilterType::lowCut && type != FilterType::highCut;
    }

    void EqBand::setTarget (const BandTarget& t, double amount) noexcept
    {
        if (t.type != type)
        {
            type = t.type;
            coefficientsDirty = true;
        }

        enabled = t.enabled;

        log2Freq.setTargetValue (std::log2 (std::max (t.frequencyHz, 1.0)));
        log2Q.setTargetValue (std::log2 (std::max (t.q, 0.01)));
        gainDb.setTargetValue (enabled && isGainType() ? snapToUnity (t.gainDb * amount) : 0.0);
        cutMix.setTargetValue (enabled && ! isGainType() ? 1.0 : 0.0);

        if (firstTarget)
        {
            snapToTarget();
            firstTarget = false;
        }
    }

    void EqBand::snapToTarget() noexcept
    {
        log2Freq.setCurrentAndTargetValue (log2Freq.getTargetValue());
        log2Q.setCurrentAndTargetValue (log2Q.getTargetValue());
        gainDb.setCurrentAndTargetValue (gainDb.getTargetValue());
        cutMix.setCurrentAndTargetValue (cutMix.getTargetValue());
        coefficientsDirty = true;
    }

    void EqBand::advance (int numSamples) noexcept
    {
        const bool moving = log2Freq.isSmoothing() || log2Q.isSmoothing() || gainDb.isSmoothing();

        if (moving)
        {
            log2Freq.skip (numSamples);
            log2Q.skip (numSamples);
            gainDb.skip (numSamples);
            coefficientsDirty = true;
        }

        if (coefficientsDirty)
            updateCoefficients();

        if (isGainType())
        {
            const bool identity = getCoefficients().isIdentity() && ! gainDb.isSmoothing();
            if (identity && filters[0].isSilent() && filters[1].isSilent())
            {
                if (active)
                    for (auto& f : filters)
                        f.reset();
                active = false;
            }
            else
            {
                active = true;
            }
        }
        else
        {
            const bool off = ! cutMix.isSmoothing() && cutMix.getTargetValue() == 0.0;
            if (off && active)
                for (auto& f : filters)
                    f.reset();
            active = ! off;
        }
    }

    void EqBand::updateCoefficients() noexcept
    {
        BandSettings s;
        s.type = type;
        s.frequencyHz = std::exp2 (log2Freq.getCurrentValue());
        s.q = std::exp2 (log2Q.getCurrentValue());
        s.gainDb = gainDb.getCurrentValue();

        const auto c = design (s, sampleRate);
        for (auto& f : filters)
            f.setCoefficients (c);

        coefficientsDirty = false;
        ++version;
    }

    bool EqBand::contributesToCurve() const noexcept
    {
        if (! active)
            return false;
        return isGainType() || cutMix.getTargetValue() > 0.0;
    }

    void EqBand::process (double* const* channels, int numChannels, int numSamples) noexcept
    {
        if (! active)
            return;

        if (isGainType())
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& f = filters[(size_t) ch];
                auto* x = channels[ch];
                for (int i = 0; i < numSamples; ++i)
                    x[i] = f.process (x[i]);
            }
            return;
        }

        // Cut filters: crossfade in/out on enable/disable (only while the mix is moving).
        if (! cutMix.isSmoothing() && cutMix.getCurrentValue() == 1.0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& f = filters[(size_t) ch];
                auto* x = channels[ch];
                for (int i = 0; i < numSamples; ++i)
                    x[i] = f.process (x[i]);
            }
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const auto mix = cutMix.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& x = channels[ch][i];
                const auto wet = filters[(size_t) ch].process (x);
                x += mix * (wet - x);
            }
        }
    }

    //==============================================================================
    void EqEngine::prepare (double newSampleRate, int /*maxBlockSize*/, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        constexpr double smoothingSeconds = 0.03; // within the 20-50 ms range from the research

        for (auto& b : bands)
            b.prepare (sampleRate, smoothingSeconds);
    }

    void EqEngine::reset()
    {
        for (auto& b : bands)
            b.reset();
    }

    double EqEngine::curvePowerAt (double frequencyHz) const noexcept
    {
        double power = 1.0;
        for (const auto& b : bands)
            if (b.contributesToCurve() && ! b.getCoefficients().isIdentity())
                power *= powerGain (b.getCoefficients(), frequencyHz, sampleRate);
        return power;
    }

    unsigned EqEngine::getVersion() const noexcept
    {
        unsigned v = 17;
        for (const auto& b : bands)
            v = v * 1000003u + b.getVersion() * 2u + (b.contributesToCurve() ? 1u : 0u);
        return v;
    }

    bool EqEngine::isBypassed() const noexcept
    {
        return std::none_of (bands.begin(), bands.end(), [] (const EqBand& b) { return b.isActive(); });
    }

    void EqEngine::process (juce::AudioBuffer<float>& buffer) noexcept
    {
        const auto numChannels = std::min ({ buffer.getNumChannels(), channels, maxChannels });
        const auto numSamples = buffer.getNumSamples();

        for (int i = 0; i < numBands; ++i)
            bands[(size_t) i].setTarget (targets[(size_t) i], amount);

        double* ptrs[maxChannels] = { scratch[0].data(), scratch[1].data() };

        for (int start = 0; start < numSamples; start += updateInterval)
        {
            const auto len = std::min (updateInterval, numSamples - start);

            bool anyActive = false;
            for (auto& b : bands)
            {
                b.advance (len);
                anyActive = anyActive || b.isActive();
            }

            if (! anyActive)
                continue; // leave this chunk untouched (bit-exact)

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const auto* src = buffer.getReadPointer (ch, start);
                for (int i = 0; i < len; ++i)
                    scratch[(size_t) ch][(size_t) i] = (double) src[i];
            }

            for (auto& b : bands)
                b.process (ptrs, numChannels, len);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* dst = buffer.getWritePointer (ch, start);
                for (int i = 0; i < len; ++i)
                    dst[i] = (float) scratch[(size_t) ch][(size_t) i];
            }
        }
    }
}
