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
        mixIn.reset (sampleRate, smoothingSeconds);
        firstTarget = true;
        reset();
    }

    void EqBand::reset()
    {
        resetFilters();
        coefficientsDirty = true;
    }

    void EqBand::resetFilters() noexcept
    {
        for (auto& channel : filters)
            for (auto& f : channel)
                f.reset();
    }

    bool EqBand::filtersSilent() const noexcept
    {
        for (const auto& channel : filters)
            for (int i = 0; i < cascade.numSections; ++i)
                if (! channel[(size_t) i].isSilent())
                    return false;
        return true;
    }

    bool EqBand::isGainType() const noexcept
    {
        return type == FilterType::bell || type == FilterType::lowShelf
            || type == FilterType::highShelf || type == FilterType::tilt;
    }

    void EqBand::setTarget (const BandTarget& t, double amount) noexcept
    {
        if (t.type != type || t.slopeDbPerOct != slopeDbPerOct)
        {
            type = t.type;
            slopeDbPerOct = t.slopeDbPerOct;
            coefficientsDirty = true;
        }

        if (t.placement != placement)
        {
            placement = t.placement;
            resetFilters(); // the filters now see a different signal
        }

        enabled = t.enabled;

        log2Freq.setTargetValue (std::log2 (std::max (t.frequencyHz, 1.0)));
        log2Q.setTargetValue (std::log2 (std::max (t.q, 0.01)));
        gainDb.setTargetValue (enabled && isGainType() ? snapToUnity (t.gainDb * amount) : 0.0);
        mixIn.setTargetValue (enabled && ! isGainType() ? 1.0 : 0.0);

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
        mixIn.setCurrentAndTargetValue (mixIn.getTargetValue());
        coefficientsDirty = true;
    }

    void EqBand::advance (int numSamples) noexcept
    {
        if (log2Freq.isSmoothing() || log2Q.isSmoothing() || gainDb.isSmoothing())
        {
            log2Freq.skip (numSamples);
            log2Q.skip (numSamples);
            gainDb.skip (numSamples);
            coefficientsDirty = true;
        }

        if (coefficientsDirty)
            updateCoefficients();

        bool nowActive;
        if (isGainType())
            nowActive = ! (cascade.isIdentity() && ! gainDb.isSmoothing() && filtersSilent());
        else
            nowActive = mixIn.isSmoothing() || mixIn.getTargetValue() > 0.0;

        if (active && ! nowActive)
            resetFilters();
        active = nowActive;
    }

    void EqBand::updateCoefficients() noexcept
    {
        BandSettings s;
        s.type = type;
        s.frequencyHz = std::exp2 (log2Freq.getCurrentValue());
        s.q = std::exp2 (log2Q.getCurrentValue());
        s.gainDb = gainDb.getCurrentValue();

        cascade = designCascade (s, slopeDbPerOct, sampleRate);
        for (auto& channel : filters)
            for (int i = 0; i < cascade.numSections; ++i)
                channel[(size_t) i].setCoefficients (cascade.sections[(size_t) i]);

        coefficientsDirty = false;
        ++version;
    }

    bool EqBand::contributesToCurve() const noexcept
    {
        if (! active)
            return false;
        return isGainType() || mixIn.getTargetValue() > 0.0;
    }

    double EqBand::runChain (int channel, double x) noexcept
    {
        auto& chain = filters[(size_t) channel];
        for (int i = 0; i < cascade.numSections; ++i)
            x = chain[(size_t) i].process (x);
        return x;
    }

    void EqBand::process (double* const* ch, int numChannels, int numSamples) noexcept
    {
        if (! active)
            return;

        const bool crossfading = ! isGainType() && mixIn.isSmoothing();
        const auto steadyMix = isGainType() ? 1.0 : mixIn.getCurrentValue();

        // Mono: Left/Right/Mid behave like Stereo; Side of a mono signal is silence (no effect).
        auto route = placement;
        if (numChannels < 2)
        {
            if (route == StereoPlacement::side)
                return;
            route = StereoPlacement::stereo;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const auto mix = crossfading ? mixIn.getNextValue() : steadyMix;
            auto apply = [&] (int filterChannel, double x) {
                const auto wet = runChain (filterChannel, x);
                return mix == 1.0 ? wet : x + mix * (wet - x);
            };

            switch (route)
            {
                case StereoPlacement::stereo:
                    for (int c = 0; c < numChannels; ++c)
                        ch[c][i] = apply (c, ch[c][i]);
                    break;

                case StereoPlacement::left:
                    ch[0][i] = apply (0, ch[0][i]);
                    break;

                case StereoPlacement::right:
                    ch[1][i] = apply (1, ch[1][i]);
                    break;

                case StereoPlacement::mid:
                case StereoPlacement::side:
                {
                    auto m = 0.5 * (ch[0][i] + ch[1][i]);
                    auto s = 0.5 * (ch[0][i] - ch[1][i]);
                    if (route == StereoPlacement::mid)
                        m = apply (0, m);
                    else
                        s = apply (0, s);
                    ch[0][i] = m + s;
                    ch[1][i] = m - s;
                    break;
                }
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
        {
            if (! b.contributesToCurve() || b.getCascade().isIdentity())
                continue;
            const auto p = cascadePowerGain (b.getCascade(), frequencyHz, sampleRate);
            power *= b.getPlacement() == StereoPlacement::stereo ? p : 0.5 * (1.0 + p);
        }
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
        bands[(size_t) numBands].setTarget (literalTarget, amount);

        if (snapRequested.exchange (false))
            for (auto& b : bands)
                b.snapToTarget();

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
