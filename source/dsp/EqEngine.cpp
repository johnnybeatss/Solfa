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

    bool EqBand::isGainType() const noexcept { return isGainTypeFor (type); }

    double EqBand::levelToOffsetDb() const noexcept
    {
        const auto levelDb = 10.0 * std::log10 (meanSquare + 1.0e-30);
        const auto amountOver = std::clamp ((levelDb - thresholdDb) / dynamicKneeSpanDb, 0.0, 1.0);
        return rangeDb * amountOver;
    }

    void EqBand::updateSidechain() noexcept
    {
        // The detector listens to the band's own region: band-pass for bells and tilt,
        // low-pass for low shelves, high-pass for high shelves.
        BandSettings s;
        s.frequencyHz = std::exp2 (log2Freq.getCurrentValue());
        s.q = std::clamp (std::exp2 (log2Q.getCurrentValue()), 0.3, 18.0);
        s.type = type == FilterType::lowShelf ? FilterType::highCut
               : type == FilterType::highShelf ? FilterType::lowCut
                                                : FilterType::bandPass;
        if (s.type != FilterType::bandPass)
            s.q = 0.7071;
        const auto c = designBilinear (sanitise (s, sampleRate), sampleRate);
        for (auto& f : sidechain)
            f.setCoefficients (c);

        // RMS averaging over about two periods of the band frequency (1-20 ms), so the detector
        // reads the average level rather than riding each waveform peak.
        const auto rmsSeconds = std::clamp (2.0 / s.frequencyHz, 0.001, 0.02);
        rmsCoeff = 1.0 - std::exp (-1.0 / (rmsSeconds * sampleRate));
    }

    double EqBand::detectorPower (double* const* ch, int numChannels, int i) noexcept
    {
        if (numChannels < 2)
        {
            const auto y = sidechain[0].process (ch[0][i]);
            return y * y;
        }
        switch (placement)
        {
            case StereoPlacement::stereo:
            {
                const auto l = sidechain[0].process (ch[0][i]);
                const auto r = sidechain[1].process (ch[1][i]);
                return 0.5 * (l * l + r * r);
            }
            case StereoPlacement::left: { const auto y = sidechain[0].process (ch[0][i]); return y * y; }
            case StereoPlacement::right: { const auto y = sidechain[1].process (ch[1][i]); return y * y; }
            case StereoPlacement::mid: { const auto y = sidechain[0].process (0.5 * (ch[0][i] + ch[1][i])); return y * y; }
            case StereoPlacement::side: { const auto y = sidechain[0].process (0.5 * (ch[0][i] - ch[1][i])); return y * y; }
        }
        return 0.0;
    }

    void EqBand::setTarget (const BandTarget& t, double amount) noexcept
    {
        const bool wantDynamic = t.enabled && t.dynamic && isGainTypeFor (t.type);
        if (wantDynamic != dynamic)
        {
            dynamic = wantDynamic;
            meanSquare = smoothedOffsetDb = 0.0;
            for (auto& s : sidechain)
                s.reset();
        }
        thresholdDb = t.dynThresholdDb;
        rangeDb = t.dynRangeDb * amount;
        attackMs = std::max (t.dynAttackMs, 0.1);
        releaseMs = std::max (t.dynReleaseMs, 1.0);

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

        // Attack/release act on the gain change itself (in dB), like a compressor's gain smoothing:
        // attack while the band moves towards its range, release while it returns.
        double newOffset = 0.0;
        if (dynamic)
        {
            const auto target = levelToOffsetDb();
            const bool engaging = std::abs (target) > std::abs (smoothedOffsetDb);
            const auto timeMs = engaging ? attackMs : releaseMs;
            const auto coeff = 1.0 - std::exp (-numSamples / (timeMs * 1.0e-3 * sampleRate));
            smoothedOffsetDb += coeff * (target - smoothedOffsetDb);
            newOffset = snapToUnity (smoothedOffsetDb);
        }
        const bool dynamicMoved = std::abs (newOffset - dynOffsetDb) > 0.01
                               || (newOffset == 0.0 && dynOffsetDb != 0.0);
        if (dynamicMoved)
            dynOffsetDb = newOffset;

        if (coefficientsDirty)
            updateCoefficients (true);
        else if (dynamicMoved)
            updateCoefficients (false);

        bool nowActive;
        if (isGainType())
            nowActive = ! (cascade.isIdentity() && ! gainDb.isSmoothing() && filtersSilent());
        else
            nowActive = mixIn.isSmoothing() || mixIn.getTargetValue() > 0.0;

        if (active && ! nowActive)
            resetFilters();
        active = nowActive;
    }

    void EqBand::updateCoefficients (bool shapeChanged) noexcept
    {
        BandSettings s;
        s.type = type;
        s.frequencyHz = std::exp2 (log2Freq.getCurrentValue());
        s.q = std::exp2 (log2Q.getCurrentValue());
        s.gainDb = gainDb.getCurrentValue();

        if (shapeChanged)
        {
            if (isGainType())
            {
                // Choose the matched fit once per shape; dynamic gain changes reuse it (fast path).
                matchCandidate = chooseMatchCandidate (s, sampleRate);
                staticCascade = {};
                staticCascade.sections[0] = designWithCandidate (s, sampleRate, matchCandidate);
                updateSidechain();
            }
            else
            {
                staticCascade = designCascade (s, slopeDbPerOct, sampleRate);
            }
            ++version; // auto-gain follows the static curve only
        }

        if (isGainType() && dynOffsetDb != 0.0)
        {
            auto moving = s;
            moving.gainDb = s.gainDb + dynOffsetDb;
            cascade = {};
            cascade.sections[0] = designWithCandidate (moving, sampleRate, matchCandidate);
        }
        else
        {
            cascade = staticCascade;
        }

        for (auto& channel : filters)
            for (int i = 0; i < cascade.numSections; ++i)
                channel[(size_t) i].setCoefficients (cascade.sections[(size_t) i]);

        coefficientsDirty = false;
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
        // The detector hears the band's input (before this band filters it). It runs even while
        // the band is momentarily flat, so the band can wake up when its region gets loud.
        if (dynamic)
            for (int i = 0; i < numSamples; ++i)
            {
                meanSquare += rmsCoeff * (detectorPower (ch, numChannels, i) - meanSquare);
            }

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
            if (! b.contributesToCurve() || b.getStaticCascade().isIdentity())
                continue;
            const auto p = cascadePowerGain (b.getStaticCascade(), frequencyHz, sampleRate);
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
                anyActive = anyActive || b.needsAudio();
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
