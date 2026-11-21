#include "Spectrum.h"

#include <algorithm>
#include <cmath>

namespace solfa::ui
{
    Spectrum::Spectrum()
    {
        levels.assign ((size_t) getNumBins(), floorDb);
    }

    void Spectrum::push (const float* samples, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            history[(size_t) writePos] = samples[i];
            writePos = (writePos + 1) % fftSize;
        }
    }

    void Spectrum::update (double frameSeconds) noexcept
    {
        // Oldest sample first.
        for (int i = 0; i < fftSize; ++i)
            work[(size_t) i] = history[(size_t) ((writePos + i) % fftSize)];
        std::fill (work.begin() + fftSize, work.end(), 0.0f);

        window.multiplyWithWindowingTable (work.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (work.data());

        // A Hann-windowed sine of amplitude A peaks at A * N / 4, so this reads in dBFS.
        const auto norm = 4.0 / fftSize;
        const auto fallDb = 30.0 * frameSeconds; // fall 30 dB per second
        for (int bin = 1; bin < getNumBins(); ++bin)
        {
            const auto mag = (double) work[(size_t) bin] * norm;
            auto db = 20.0 * std::log10 (mag + 1.0e-12);
            db += tiltDbPerOctave * std::log2 (binFrequency (bin) / 1000.0);
            db = std::max (db, floorDb);
            auto& level = levels[(size_t) bin];
            level = std::max (db, level - fallDb);
        }
    }

    double Spectrum::levelDbAt (double frequencyHz) const noexcept
    {
        const auto bin = frequencyHz * fftSize / sampleRate;
        const auto i = (int) std::floor (bin);
        if (i < 1 || i + 1 >= (int) levels.size())
            return floorDb;
        const auto frac = bin - i;
        return levels[(size_t) i] * (1.0 - frac) + levels[(size_t) (i + 1)] * frac;
    }

    double Spectrum::findPeakNear (double frequencyHz, double octaveRadius) const noexcept
    {
        const auto lo = std::max (1, (int) std::floor (frequencyHz * std::exp2 (-octaveRadius) * fftSize / sampleRate));
        const auto hi = std::min ((int) levels.size() - 2, (int) std::ceil (frequencyHz * std::exp2 (octaveRadius) * fftSize / sampleRate));
        if (hi <= lo)
            return frequencyHz;

        int best = lo;
        for (int i = lo; i <= hi; ++i)
            if (levels[(size_t) i] > levels[(size_t) best])
                best = i;

        // Flat region (no real peak): keep the requested frequency.
        if (levels[(size_t) best] - *std::min_element (levels.begin() + lo, levels.begin() + hi + 1) < 3.0)
            return frequencyHz;

        // Parabolic interpolation through the peak bin and its neighbours.
        const auto a = levels[(size_t) best - 1], b = levels[(size_t) best], c = levels[(size_t) best + 1];
        const auto denom = a - 2.0 * b + c;
        const auto offset = std::abs (denom) > 1.0e-12 ? 0.5 * (a - c) / denom : 0.0;
        return binFrequency (best + std::clamp (offset, -0.5, 0.5));
    }
}
