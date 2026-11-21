#pragma once

#include "Biquad.h"

#include <array>
#include <cmath>
#include <vector>

namespace solfa::dsp
{
    // ITU-R BS.1770 K-weighting (stage 1: high shelf, stage 2: RLB high-pass), designed for any
    // sample rate with the parameters used by libebur128. At 48 kHz it reproduces the
    // coefficients published in BS.1770 (verified in tests).
    struct KWeighting
    {
        BiquadCoefficients shelf, highPass;
        static KWeighting design (double sampleRate) noexcept;

        // |K(f)|^2
        double powerAt (double frequencyHz, double sampleRate) const noexcept;
    };

    // K-weighted mean-square meter with exponential integration (about BS.1770 "short-term":
    // 3 s). Channels are summed with weight 1 (BS.1770 weights for L/R).
    class LoudnessMeter
    {
    public:
        void prepare (double sampleRate, int numChannels, double integrationSeconds = 3.0);
        void reset() noexcept;

        void process (const float* const* channels, int numChannels, int numSamples) noexcept;

        double getMeanSquare() const noexcept { return meanSquare; }

    private:
        static constexpr int maxChannels = 2;
        std::array<Biquad, maxChannels> shelf, highPass;
        double meanSquare = 0.0;
        double alpha = 0.0;
    };

    // Static auto-gain: the loudness change of the EQ curve for pink noise, K-weighted.
    // Evaluates the curve on a fixed log grid; no allocation after prepare().
    class AutoGainEstimator
    {
    public:
        void prepare (double sampleRate);

        // Power gain of the curve at each grid frequency is supplied by `curvePowerAt`.
        template <typename CurvePower>
        double estimateDb (CurvePower&& curvePowerAt) const noexcept
        {
            double weighted = 0.0;
            for (size_t i = 0; i < frequencies.size(); ++i)
                weighted += weights[i] * curvePowerAt (frequencies[i]);
            return 10.0 * std::log10 (weighted / weightSum);
        }

        const std::vector<double>& getFrequencies() const noexcept { return frequencies; }

    private:
        std::vector<double> frequencies, weights;
        double weightSum = 1.0;
    };
}
