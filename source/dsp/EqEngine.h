#pragma once

#include "Biquad.h"
#include "FilterDesign.h"

#include <array>
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

    struct BandTarget
    {
        bool enabled = false;
        FilterType type = FilterType::bell;
        double frequencyHz = 1000.0;
        double gainDb = 0.0;
        double q = 0.707;
        int slopeDbPerOct = 12;
        StereoPlacement placement = StereoPlacement::stereo;
    };

    // One EQ band: parameter smoothing, coefficient updates and per-channel filter cascades.
    class EqBand
    {
    public:
        void prepare (double sampleRate, double smoothingSeconds);
        void reset();

        // Called once per block from the audio thread.
        void setTarget (const BandTarget& target, double amount) noexcept;

        // Advances smoothing by numSamples and refreshes coefficients if anything moved.
        void advance (int numSamples) noexcept;

        // Processes in place. Returns immediately if the band is currently inactive.
        void process (double* const* channels, int numChannels, int numSamples) noexcept;

        bool isActive() const noexcept { return active; }

        // True if this band currently shapes the response (used by auto-gain).
        bool contributesToCurve() const noexcept;

        // Increments whenever the coefficients change.
        unsigned getVersion() const noexcept { return version; }

        const CascadeCoefficients& getCascade() const noexcept { return cascade; }
        StereoPlacement getPlacement() const noexcept { return placement; }

    private:
        bool isGainType() const noexcept;
        void updateCoefficients() noexcept;
        void snapToTarget() noexcept;
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

        CascadeCoefficients cascade;
        std::array<std::array<Biquad, maxSections>, maxChannels> filters;
        bool coefficientsDirty = true;
        unsigned version = 0;
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
        void setAmount (double amountFraction) noexcept { amount = amountFraction; }

        void process (juce::AudioBuffer<float>& buffer) noexcept;

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
        std::array<EqBand, numBands> bands;
        std::array<BandTarget, numBands> targets;
        double amount = 1.0;
        double sampleRate = 48000.0;
        int channels = 2;

        std::array<std::array<double, updateInterval>, maxChannels> scratch {};
    };
}
