#pragma once

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
        highCut, // 12 dB/oct low-pass
        tilt     // -g/2 below the pivot, +g/2 above
    };

    inline constexpr int numFilterTypes = 6;

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

    // Magnitude response in dB of a digital biquad at frequencyHz.
    double magnitudeDb (const BiquadCoefficients& c, double frequencyHz, double sampleRate) noexcept;

    // Magnitude response in dB of the analog prototype for these settings.
    double analogMagnitudeDb (const BandSettings& settings, double frequencyHz) noexcept;
}
