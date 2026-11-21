#pragma once

#include <array>

// Coefficient design for Solfa's EQ bands.
//
// Policy (measured in docs/research/notes/eq_dsp_best_practices.md and the Phase 2 prototype):
//  - Bell, shelves, tilt and high-cut use an analog-matched design: the analog prototype's
//    poles are mapped exactly (impulse invariance) and the numerator is fitted so the digital
//    magnitude equals the analog magnitude at DC, Nyquist and one more frequency. This avoids
//    the bilinear transform's "cramping" near Nyquist.
//  - Cuts (negative gain) are designed as the inverse of the matching boost, which keeps the
//    fit well conditioned and makes +g followed by -g an exact identity.
//  - Low-cut uses the RBJ bilinear design, which measured more accurate for realistic
//    low-cut frequencies.
// All maths is double precision.

namespace solfa::dsp
{
    enum class FilterType
    {
        bell = 0,
        lowShelf,
        highShelf,
        lowCut,  // 12 dB/oct high-pass
        highCut, // low-pass, 6-96 dB/oct
        tilt,    // -g/2 below the pivot, +g/2 above
        notch,
        bandPass // 0 dB peak at the centre frequency
        // Append new types at the end: the index is stored in DAW sessions.
    };

    inline constexpr int numFilterTypes = 8;

    // Cut filters can be steeper than one biquad: Butterworth cascades up to 96 dB/oct.
    inline constexpr int maxSections = 8;
    inline constexpr std::array<int, 8> slopeOptionsDbPerOct { 6, 12, 18, 24, 36, 48, 72, 96 };
    inline constexpr int defaultSlopeIndex = 1; // 12 dB/oct

    // Normalised digital biquad: H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)
    struct BiquadCoefficients
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

        bool isIdentity() const noexcept;
    };

    // Analog biquad: H(s) = (n2 s^2 + n1 s + n0) / (d2 s^2 + d1 s + d0), s in rad/s
    struct AnalogBiquad
    {
        double n2 = 0.0, n1 = 0.0, n0 = 1.0;
        double d2 = 0.0, d1 = 0.0, d0 = 1.0;

        double magnitudeSquared (double omega) const noexcept;
    };

    struct BandSettings
    {
        FilterType type = FilterType::bell;
        double frequencyHz = 1000.0;
        double gainDb = 0.0;
        double q = 0.707;
    };

    // Limits applied by design(); exposed so tests and UI agree with the DSP.
    inline constexpr double minFrequencyHz = 10.0;
    inline constexpr double maxFrequencyRatio = 0.48; // of the sample rate
    inline constexpr double minQ = 0.1;
    inline constexpr double maxQ = 18.0;
    inline constexpr double maxShelfQ = 2.0; // higher shelf Q gives large, unmusical resonances
    inline constexpr double tiltQ = 0.5;

    // Clamp frequency/Q into the ranges design() actually uses.
    BandSettings sanitise (BandSettings settings, double sampleRate) noexcept;

    // The analog prototype a band is matched to (RBJ cookbook analog forms).
    AnalogBiquad makeAnalogPrototype (const BandSettings& settings) noexcept;

    // RBJ cookbook bilinear-transform design (reference and low-cut).
    BiquadCoefficients designBilinear (const BandSettings& settings, double sampleRate) noexcept;

    // Analog-matched design that fits the magnitude at DC, Nyquist and matchOmega (rad/sample).
    // Returns false if the fit is not realisable (caller should fall back).
    bool designMatched (const AnalogBiquad& analog, double sampleRate, double matchOmega, BiquadCoefficients& out) noexcept;

    // Top-level design used by the EQ engine. Safe to call on the audio thread (no allocation).
    BiquadCoefficients design (BandSettings settings, double sampleRate) noexcept;

    // design() = designWithCandidate (s, fs, chooseMatchCandidate (s, fs)). Choosing is the costly
    // part (it measures two fits), so dynamic bands choose once per shape and reuse the choice
    // while only their gain moves. -1 means "not a matched design" (bilinear).
    int chooseMatchCandidate (BandSettings settings, double sampleRate) noexcept;
    BiquadCoefficients designWithCandidate (BandSettings settings, double sampleRate, int candidate) noexcept;

    // A band's filter as a cascade of biquads (first-order sections have b2 = a2 = 0).
    struct CascadeCoefficients
    {
        std::array<BiquadCoefficients, maxSections> sections {};
        int numSections = 1;

        bool isIdentity() const noexcept { return numSections == 1 && sections[0].isIdentity(); }
    };

    // Full design for a band. slopeDbPerOct applies to low/high cut only (6..96, multiple of 6).
    CascadeCoefficients designCascade (BandSettings settings, int slopeDbPerOct, double sampleRate) noexcept;

    double cascadePowerGain (const CascadeCoefficients& c, double frequencyHz, double sampleRate) noexcept;

    // Q of each Butterworth biquad section for a filter of the given order (highest Q last).
    double butterworthSectionQ (int order, int sectionIndex) noexcept;

    // Power gain |H|^2 of a digital biquad at frequencyHz.
    double powerGain (const BiquadCoefficients& c, double frequencyHz, double sampleRate) noexcept;

    // Magnitude response in dB of a digital biquad at frequencyHz.
    double magnitudeDb (const BiquadCoefficients& c, double frequencyHz, double sampleRate) noexcept;

    // Magnitude response in dB of the analog prototype for these settings.
    double analogMagnitudeDb (const BandSettings& settings, double frequencyHz) noexcept;
}
