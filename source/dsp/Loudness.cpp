#include "Loudness.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

namespace solfa::dsp
{
    namespace
    {
        double biquadPower (const BiquadCoefficients& c, double frequencyHz, double sampleRate) noexcept
        {
            const auto z1 = std::polar (1.0, -2.0 * std::numbers::pi * frequencyHz / sampleRate);
            const auto z2 = z1 * z1;
            return std::norm ((c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2));
        }
    }

    KWeighting KWeighting::design (double sampleRate) noexcept
    {
        KWeighting k;
        {
            constexpr double f0 = 1681.974450955533, gainDb = 3.999843853973347, q = 0.7071752369554196;
            const auto K = std::tan (std::numbers::pi * f0 / sampleRate);
            const auto vh = std::pow (10.0, gainDb / 20.0);
            const auto vb = std::pow (vh, 0.4996667741545416);
            const auto a0 = 1.0 + K / q + K * K;
            k.shelf = { (vh + vb * K / q + K * K) / a0, 2.0 * (K * K - vh) / a0, (vh - vb * K / q + K * K) / a0,
                2.0 * (K * K - 1.0) / a0, (1.0 - K / q + K * K) / a0 };
        }
        {
            constexpr double f0 = 38.13547087602444, q = 0.5003270373238773;
            const auto K = std::tan (std::numbers::pi * f0 / sampleRate);
            const auto a0 = 1.0 + K / q + K * K;
            k.highPass = { 1.0, -2.0, 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / q + K * K) / a0 };
        }
        return k;
    }

    double KWeighting::powerAt (double frequencyHz, double sampleRate) const noexcept
    {
        return biquadPower (shelf, frequencyHz, sampleRate) * biquadPower (highPass, frequencyHz, sampleRate);
    }

    //==============================================================================
    void LoudnessMeter::prepare (double sampleRate, int, double integrationSeconds)
    {
        const auto k = KWeighting::design (sampleRate);
        for (int ch = 0; ch < maxChannels; ++ch)
        {
            shelf[(size_t) ch].setCoefficients (k.shelf);
            highPass[(size_t) ch].setCoefficients (k.highPass);
        }
        alpha = 1.0 - std::exp (-1.0 / (integrationSeconds * sampleRate));
        reset();
    }

    void LoudnessMeter::reset() noexcept
    {
        for (int ch = 0; ch < maxChannels; ++ch)
        {
            shelf[(size_t) ch].reset();
            highPass[(size_t) ch].reset();
        }
        meanSquare = 0.0;
    }

    void LoudnessMeter::process (const float* const* channels, int numChannels, int numSamples) noexcept
    {
        numChannels = std::min (numChannels, maxChannels);
        for (int i = 0; i < numSamples; ++i)
        {
            double sum = 0.0;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const auto y = highPass[(size_t) ch].process (shelf[(size_t) ch].process ((double) channels[ch][i]));
                sum += y * y;
            }
            meanSquare += alpha * (sum - meanSquare);
        }
    }

    //==============================================================================
    void AutoGainEstimator::prepare (double sampleRate)
    {
        // Pink noise has equal power per octave, so a log-spaced grid with equal weights
        // represents it; each point is then K-weighted.
        constexpr int numPoints = 64;
        const auto top = std::min (20000.0, 0.49 * sampleRate);
        const auto k = KWeighting::design (sampleRate);

        frequencies.resize (numPoints);
        weights.resize (numPoints);
        weightSum = 0.0;
        for (int i = 0; i < numPoints; ++i)
        {
            const auto f = 20.0 * std::pow (top / 20.0, i / (numPoints - 1.0));
            frequencies[(size_t) i] = f;
            weights[(size_t) i] = k.powerAt (f, sampleRate);
            weightSum += weights[(size_t) i];
        }
    }
}
