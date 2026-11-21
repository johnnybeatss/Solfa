#pragma once

#include "Biquad.h"
#include "FilterDesign.h"

#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>

namespace solfa::dsp
{
    inline constexpr int numBands = 24;
    inline constexpr int maxChannels = 2;

    // Gains closer to 0 dB than this are treated as exactly 0 dB. Parameter snapping can turn a
    // "0 dB" default into about -5e-7 dB on CPUs that fuse multiply-add (e.g. Apple Silicon),
    // which would otherwise break bit-exact transparency.
    inline constexpr double unityToleranceDb = 1.0e-3;

    inline double snapToUnity (double gainDb) noexcept
    {
        return std::abs (gainDb) < unityToleranceDb ? 0.0 : gainDb;
    }

    // Which part of a stereo signal a band processes. Append only: stored in DAW sessions.
    enum class StereoPlacement
    {
        stereo = 0,
        left,
        right,
        mid,
        side
    };

    inline constexpr int numPlacements = 5;

    inline bool isGainTypeFor (FilterType t) noexcept
    {
        return t == FilterType::bell || t == FilterType::lowShelf || t == FilterType::highShelf || t == FilterType::tilt;
    }

    struct BandTarget
    {
        bool enabled = false;
        FilterType type = FilterType::bell;
        double frequencyHz = 1000.0;
        double gainDb = 0.0;
        double q = 0.707;
        int slopeDbPerOct = 12;
        StereoPlacement placement = StereoPlacement::stereo;

        // Dynamic EQ (bell, shelves, tilt only): as the band's own frequency region rises above
        // the threshold, the gain moves towards gain + range, reaching it 12 dB above threshold.
        bool dynamic = false;
        double dynThresholdDb = -24.0; // RMS level of the sidechain-filtered signal, dBFS
        double dynRangeDb = -6.0;      // negative: tame when loud; positive: lift when loud
        double dynAttackMs = 10.0;
        double dynReleaseMs = 150.0;
    };

    // Detector level above threshold at which a dynamic band reaches its full range.
    inline constexpr double dynamicKneeSpanDb = 12.0;

    // One EQ band: parameter smoothing, coefficient updates and per-channel filter cascades.
    class EqBand
    {
    public:
        void prepare (double sampleRate, double smoothingSeconds);
        void reset();

        // Called once per block from the audio thread.
        void setTarget (const BandTarget& target, double amount) noexcept;

        // Jumps every smoothed value to its target (preset loads).
        void snapToTarget() noexcept;

        // Advances smoothing by numSamples and refreshes coefficients if anything moved.
        void advance (int numSamples) noexcept;

        // Processes in place. Returns immediately if the band is currently inactive.
        void process (double* const* channels, int numChannels, int numSamples) noexcept;

        bool isActive() const noexcept { return active; }

        // True if process() must see the audio: the band is filtering, or its detector is listening.
        bool needsAudio() const noexcept { return active || dynamic; }

        // True if this band currently shapes the response (used by auto-gain).
        bool contributesToCurve() const noexcept;

        // Increments whenever the coefficients change.
        unsigned getVersion() const noexcept { return version; }

        const CascadeCoefficients& getCascade() const noexcept { return cascade; }

        // The response without dynamic movement (what auto-gain uses).
        const CascadeCoefficients& getStaticCascade() const noexcept { return staticCascade; }

        // Current dynamic gain offset in dB (0 when not dynamic).
        double getDynamicOffsetDb() const noexcept { return dynOffsetDb; }
        StereoPlacement getPlacement() const noexcept { return placement; }

    private:
        bool isGainType() const noexcept;
        void updateCoefficients (bool shapeChanged) noexcept;
        void updateSidechain() noexcept;
        double detectorPower (double* const* ch, int numChannels, int i) noexcept;
        double levelToOffsetDb() const noexcept;
        bool filtersSilent() const noexcept;
        void resetFilters() noexcept;
        double runChain (int channel, double x) noexcept;

        double sampleRate = 48000.0;
        FilterType type = FilterType::bell;
        int slopeDbPerOct = 12;
        StereoPlacement placement = StereoPlacement::stereo;
        bool enabled = false;
        bool active = false;
        bool firstTarget = true;

        // Smoothed in perceptual domains: frequency and Q in log2, gain in dB.
        juce::SmoothedValue<double> log2Freq { std::log2 (1000.0) };
        juce::SmoothedValue<double> gainDb { 0.0 };
        juce::SmoothedValue<double> log2Q { std::log2 (0.707) };
        // Crossfade used only when switching filters that have no gain to ramp (cuts, notch, band-pass).
        juce::SmoothedValue<double> mixIn { 0.0 };

        CascadeCoefficients cascade, staticCascade;
        std::array<std::array<Biquad, maxSections>, maxChannels> filters;
        bool coefficientsDirty = true;
        unsigned version = 0;
        int matchCandidate = 0;

        // Dynamic EQ state
        bool dynamic = false;
        double thresholdDb = -24.0, rangeDb = 0.0;
        double attackMs = -1.0, releaseMs = -1.0;
        double meanSquare = 0.0;  // short RMS average of the sidechain signal (~2 periods)
        double rmsCoeff = 0.0;
        double smoothedOffsetDb = 0.0; // attack/release applied to the gain change, in dB
        double dynOffsetDb = 0.0; // currently applied
        std::array<Biquad, maxChannels> sidechain;
    };

    // The full EQ: numBands bands in series, processed in double precision.
    class EqEngine
    {
    public:
        // Coefficients are recomputed at most every this many samples while parameters move.
        static constexpr int updateInterval = 16;

        void prepare (double sampleRate, int maxBlockSize, int numChannels);
        void reset();

        void setBand (int index, const BandTarget& target) noexcept { targets[(size_t) index] = target; }

        // Literal mode: an extra narrow band outside the 24 user bands.
        void setLiteral (const BandTarget& target) noexcept { literalTarget = target; }

        // Next process() jumps straight to the targets instead of ramping (preset loads).
        void requestSnap() noexcept { snapRequested.store (true); }
        void setAmount (double amountFraction) noexcept { amount = amountFraction; }

        void process (juce::AudioBuffer<float>& buffer) noexcept;

        // Advances smoothing and coefficients without touching audio (linear-phase mode, where
        // the FIR does the filtering but auto-gain still needs the current static curve).
        void updateOnly (int numSamples) noexcept;

        // True if no band is doing anything (lets the processor skip work and stay bit-exact).
        bool isBypassed() const noexcept;

        const EqBand& getBand (int index) const noexcept { return bands[(size_t) index]; }

        // Approximate power gain |H(f)|^2 of the whole curve, for auto-gain. Bands on one
        // channel or on Mid/Side count as affecting half the signal power.
        double curvePowerAt (double frequencyHz) const noexcept;

        // Changes whenever any band's coefficients change.
        unsigned getVersion() const noexcept;

        double getSampleRate() const noexcept { return sampleRate; }

    private:
        // bands[numBands] is the Literal band.
        std::array<EqBand, numBands + 1> bands;
        std::array<BandTarget, numBands> targets;
        BandTarget literalTarget;
        std::atomic<bool> snapRequested { false };
        double amount = 1.0;
        double sampleRate = 48000.0;
        int channels = 2;

        std::array<std::array<double, updateInterval>, maxChannels> scratch {};
    };
}
