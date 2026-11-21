#include "EqMatch.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace solfa::dsp
{
    //==============================================================================
    void LongTermSpectrum::reset()
    {
        powerSum.assign ((size_t) fftSize / 2 + 1, 0.0);
        filled = 0;
        frames = 0;
    }

    void LongTermSpectrum::push (const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            frame[(size_t) filled++] = samples[i];
            if (filled == fftSize)
            {
                analyseFrame();
                // 50% overlap: keep the second half as the start of the next frame.
                std::copy (frame.begin() + fftSize / 2, frame.end(), frame.begin());
                filled = fftSize / 2;
            }
        }
    }

    void LongTermSpectrum::analyseFrame()
    {
        std::copy (frame.begin(), frame.end(), work.begin());
        std::fill (work.begin() + fftSize, work.end(), 0.0f);
        window.multiplyWithWindowingTable (work.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (work.data());
        for (size_t bin = 0; bin < powerSum.size(); ++bin)
            powerSum[bin] += (double) work[bin] * (double) work[bin];
        ++frames;
    }

    std::vector<double> LongTermSpectrum::levelsDb (const std::vector<double>& freqs, double smoothingOctaves) const
    {
        std::vector<double> out (freqs.size(), -200.0);
        if (frames == 0)
            return out;

        const auto binHz = sampleRate / fftSize;
        const auto norm = 1.0 / frames;
        const auto lastBin = (int) powerSum.size() - 1;

        for (size_t i = 0; i < freqs.size(); ++i)
        {
            const auto f = freqs[i];
            const auto lo = f * std::exp2 (-0.5 * smoothingOctaves), hi = f * std::exp2 (0.5 * smoothingOctaves);
            const auto b0 = std::max (1, (int) std::ceil (lo / binHz)), b1 = std::min (lastBin, (int) std::floor (hi / binHz));

            double power = 0.0;
            if (b1 >= b0)
            {
                for (int b = b0; b <= b1; ++b)
                    power += powerSum[(size_t) b];
                power /= (b1 - b0 + 1);
            }
            else
            {
                // Band narrower than a bin (low frequencies): interpolate between neighbours.
                const auto pos = std::clamp (f / binHz, 1.0, (double) lastBin - 1.0);
                const auto k = (int) pos;
                const auto t = pos - k;
                power = powerSum[(size_t) k] * (1.0 - t) + powerSum[(size_t) k + 1] * t;
            }
            out[i] = 10.0 * std::log10 (power * norm + 1.0e-30);
        }
        return out;
    }

    //==============================================================================
    namespace match
    {
        namespace
        {
            double weight (double f)
            {
                return (f >= 40.0 && f <= 16000.0) ? 1.0 : 0.2;
            }

            // Response of one band on the grid: designed once, evaluated at every frequency.
            std::vector<double> bandCurve (const FittedBand& b, const std::vector<double>& freqs, double fs)
            {
                const auto c = design ({ b.type, b.frequencyHz, b.gainDb, b.q }, fs);
                std::vector<double> r (freqs.size());
                for (size_t i = 0; i < freqs.size(); ++i)
                    r[i] = magnitudeDb (c, freqs[i], fs);
                return r;
            }

            // Solve the small symmetric system A x = y (Gaussian elimination with partial pivoting).
            std::vector<double> solve (std::vector<std::vector<double>> a, std::vector<double> y)
            {
                const auto n = y.size();
                for (size_t c = 0; c < n; ++c)
                {
                    size_t p = c;
                    for (size_t r = c + 1; r < n; ++r)
                        if (std::abs (a[r][c]) > std::abs (a[p][c]))
                            p = r;
                    std::swap (a[c], a[p]);
                    std::swap (y[c], y[p]);
                    if (std::abs (a[c][c]) < 1.0e-12)
                        continue;
                    for (size_t r = c + 1; r < n; ++r)
                    {
                        const auto m = a[r][c] / a[c][c];
                        for (size_t k = c; k < n; ++k)
                            a[r][k] -= m * a[c][k];
                        y[r] -= m * y[c];
                    }
                }
                std::vector<double> x (n, 0.0);
                for (size_t c = n; c-- > 0;)
                {
                    if (std::abs (a[c][c]) < 1.0e-12)
                        continue;
                    double s = y[c];
                    for (size_t k = c + 1; k < n; ++k)
                        s -= a[c][k] * x[k];
                    x[c] = s / a[c][c];
                }
                return x;
            }
        }

        std::vector<double> logGrid (int n)
        {
            std::vector<double> f ((size_t) n);
            for (int i = 0; i < n; ++i)
                f[(size_t) i] = 20.0 * std::pow (1000.0, i / (n - 1.0));
            return f;
        }

        std::vector<double> matchCurve (const std::vector<double>& freqs, const std::vector<double>& ref,
            const std::vector<double>& cur, double amount, double maxDb)
        {
            std::vector<double> d (freqs.size());
            double sum = 0.0;
            int count = 0;
            for (size_t i = 0; i < freqs.size(); ++i)
            {
                d[i] = ref[i] - cur[i];
                if (freqs[i] >= 100.0 && freqs[i] <= 10000.0)
                {
                    sum += d[i];
                    ++count;
                }
            }
            const auto mean = count > 0 ? sum / count : 0.0;
            for (auto& v : d)
                v = std::clamp (v - mean, -maxDb, maxDb) * amount;
            return d;
        }

        std::vector<double> responseDb (const std::vector<FittedBand>& bands, const std::vector<double>& freqs, double fs)
        {
            std::vector<double> r (freqs.size(), 0.0);
            for (const auto& b : bands)
            {
                const auto c = bandCurve (b, freqs, fs);
                for (size_t i = 0; i < freqs.size(); ++i)
                    r[i] += c[i];
            }
            return r;
        }

        double rmsErrorDb (const std::vector<double>& freqs, const std::vector<double>& a, const std::vector<double>& b)
        {
            double s = 0.0, w = 0.0;
            for (size_t i = 0; i < freqs.size(); ++i)
                if (freqs[i] >= 40.0 && freqs[i] <= 16000.0)
                {
                    s += (a[i] - b[i]) * (a[i] - b[i]);
                    w += 1.0;
                }
            return w > 0.0 ? std::sqrt (s / w) : 0.0;
        }

        std::vector<FittedBand> fit (const std::vector<double>& freqs, const std::vector<double>& target, int maxBands, double fs)
        {
            std::vector<FittedBand> bands;
            const auto n = freqs.size();
            auto residual = target;

            for (int iteration = 0; iteration < maxBands; ++iteration)
            {
                // 1. Largest remaining (weighted) deviation.
                size_t peak = 0;
                for (size_t i = 1; i < n; ++i)
                    if (weight (freqs[i]) * std::abs (residual[i]) > weight (freqs[peak]) * std::abs (residual[peak]))
                        peak = i;
                const auto g = residual[peak];
                if (std::abs (g) < 0.5)
                    break;

                // 2. Its width: walk out until the deviation halves or changes sign.
                size_t lo = peak, hi = peak;
                while (lo > 0 && residual[lo - 1] * g > 0 && std::abs (residual[lo - 1]) > 0.5 * std::abs (g))
                    --lo;
                while (hi + 1 < n && residual[hi + 1] * g > 0 && std::abs (residual[hi + 1]) > 0.5 * std::abs (g))
                    ++hi;

                FittedBand band;
                band.gainDb = std::clamp (g, -12.0, 12.0);
                if (lo == 0 && freqs[hi] < 2000.0)
                {
                    band.type = FilterType::lowShelf;
                    band.frequencyHz = freqs[hi];
                    band.q = 0.707;
                }
                else if (hi == n - 1 && freqs[lo] > 2000.0)
                {
                    band.type = FilterType::highShelf;
                    band.frequencyHz = freqs[lo];
                    band.q = 0.707;
                }
                else
                {
                    const auto bw = std::max (std::log2 (freqs[hi] / freqs[lo]), 0.1);
                    const auto r = std::exp2 (bw);
                    band.type = FilterType::bell;
                    band.frequencyHz = freqs[peak];
                    band.q = std::clamp (std::sqrt (r) / (r - 1.0), 0.3, 4.0);
                }
                bands.push_back (band);

                const auto curve = bandCurve (band, freqs, fs);
                for (size_t i = 0; i < n; ++i)
                    residual[i] -= curve[i];
            }

            // 3. Refine all gains together: weighted least squares on each band's shape.
            for (int pass = 0; pass < 3 && ! bands.empty(); ++pass)
            {
                const auto m = bands.size();
                std::vector<std::vector<double>> shape (m);
                std::vector<double> current (n, 0.0);
                for (size_t b = 0; b < m; ++b)
                {
                    shape[b] = bandCurve (bands[b], freqs, fs);
                    for (size_t i = 0; i < n; ++i)
                    {
                        current[i] += shape[b][i];
                        shape[b][i] /= bands[b].gainDb; // dB per dB of gain
                    }
                }
                std::vector<std::vector<double>> a (m, std::vector<double> (m, 0.0));
                std::vector<double> y (m, 0.0);
                for (size_t i = 0; i < n; ++i)
                {
                    const auto w = weight (freqs[i]);
                    const auto r = target[i] - current[i];
                    for (size_t p = 0; p < m; ++p)
                    {
                        y[p] += w * shape[p][i] * r;
                        for (size_t q = 0; q < m; ++q)
                            a[p][q] += w * shape[p][i] * shape[q][i];
                    }
                }
                const auto delta = solve (a, y);
                for (size_t b = 0; b < m; ++b)
                {
                    auto g = std::clamp (bands[b].gainDb + delta[b], -12.0, 12.0);
                    if (std::abs (g) < 0.05) // keep a sign so the shape stays defined
                        g = bands[b].gainDb > 0 ? 0.05 : -0.05;
                    bands[b].gainDb = g;
                }
            }
            return bands;
        }
    }
}
