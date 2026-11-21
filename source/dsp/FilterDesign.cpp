#include "FilterDesign.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <numbers>

namespace solfa::dsp
{
    namespace
    {
        constexpr double pi = std::numbers::pi;
        constexpr double twoPi = 2.0 * std::numbers::pi;

        bool isGainType (FilterType t) noexcept
        {
            return t == FilterType::bell || t == FilterType::lowShelf
                || t == FilterType::highShelf || t == FilterType::tilt;
        }

        // First-order low-pass, analog-matched: exact pole, magnitude fitted at DC and Nyquist.
        BiquadCoefficients matchedFirstOrderLowPass (double f, double fs) noexcept
        {
            const auto w = twoPi * f;
            const auto a1 = -std::exp (-w / fs);
            const auto wn = pi * fs;
            const auto nyquistMag = std::sqrt (w * w / (w * w + wn * wn));
            const auto sum = 1.0 + a1;                 // b0 + b1 (unity gain at DC)
            const auto diff = (1.0 - a1) * nyquistMag; // b0 - b1
            return { 0.5 * (sum + diff), 0.5 * (sum - diff), 0.0, a1, 0.0 };
        }

        // First-order high-pass, bilinear (consistent with the 12 dB/oct low-cut choice).
        BiquadCoefficients bilinearFirstOrderHighPass (double f, double fs) noexcept
        {
            const auto K = std::tan (pi * f / fs);
            const auto b0 = 1.0 / (1.0 + K);
            return { b0, -b0, 0.0, (K - 1.0) / (K + 1.0), 0.0 };
        }

        // A = 10^(g/40), the RBJ cookbook amplitude
        double amplitude (double gainDb) noexcept { return std::pow (10.0, gainDb / 40.0); }

        BiquadCoefficients invert (const BiquadCoefficients& c) noexcept
        {
            // 1/H: swap numerator and denominator, renormalise so the new a0 is 1.
            const auto n = c.b0;
            return { 1.0 / n, c.a1 / n, c.a2 / n, c.b1 / n, c.b2 / n };
        }

        bool isFinite (const BiquadCoefficients& c) noexcept
        {
            return std::isfinite (c.b0) && std::isfinite (c.b1) && std::isfinite (c.b2)
                && std::isfinite (c.a1) && std::isfinite (c.a2);
        }

        // Max |digital - analog| in dB over a fixed log-spaced grid; used to pick the best fit.
        double fitError (const BiquadCoefficients& c, const BandSettings& s, double sampleRate) noexcept
        {
            constexpr int numPoints = 16;
            const auto lo = std::log (20.0);
            const auto hi = std::log (0.49 * sampleRate);
            double worst = 0.0;

            for (int i = 0; i < numPoints; ++i)
            {
                const auto f = std::exp (lo + (hi - lo) * i / (numPoints - 1));
                const auto ref = analogMagnitudeDb (s, f);
                if (ref < -40.0)
                    continue; // deep stop-band of cut filters: not meaningful to compare
                worst = std::max (worst, std::abs (magnitudeDb (c, f, sampleRate) - ref));
            }
            return worst;
        }

        BiquadCoefficients designBestMatched (const BandSettings& s, double sampleRate) noexcept
        {
            const auto analog = makeAnalogPrototype (s);
            const auto w0 = twoPi * s.frequencyHz / sampleRate;
            const std::array<double, 2> candidates { w0, 0.5 * pi };

            BiquadCoefficients best;
            double bestError = 1.0e300;
            bool found = false;

            for (auto wm : candidates)
            {
                BiquadCoefficients c;
                if (! designMatched (analog, sampleRate, wm, c))
                    continue;

                const auto err = fitError (c, s, sampleRate);
                if (err < bestError)
                {
                    best = c;
                    bestError = err;
                    found = true;
                }
            }

            return found ? best : designBilinear (s, sampleRate);
        }
    }

    bool BiquadCoefficients::isIdentity() const noexcept
    {
        return b0 == 1.0 && b1 == 0.0 && b2 == 0.0 && a1 == 0.0 && a2 == 0.0;
    }

    double AnalogBiquad::magnitudeSquared (double w) const noexcept
    {
        const auto numRe = n0 - n2 * w * w, numIm = n1 * w;
        const auto denRe = d0 - d2 * w * w, denIm = d1 * w;
        return (numRe * numRe + numIm * numIm) / (denRe * denRe + denIm * denIm);
    }

    BandSettings sanitise (BandSettings s, double sampleRate) noexcept
    {
        s.frequencyHz = std::clamp (s.frequencyHz, minFrequencyHz, maxFrequencyRatio * sampleRate);
        s.q = std::clamp (s.q, minQ, maxQ);

        if (s.type == FilterType::lowShelf || s.type == FilterType::highShelf)
            s.q = std::min (s.q, maxShelfQ);
        else if (s.type == FilterType::tilt)
            s.q = tiltQ;

        if (! isGainType (s.type))
            s.gainDb = 0.0;

        return s;
    }

    AnalogBiquad makeAnalogPrototype (const BandSettings& s) noexcept
    {
        const auto w = twoPi * s.frequencyHz;
        const auto q = s.q;
        const auto A = amplitude (s.gainDb);
        const auto sA = std::sqrt (A);

        switch (s.type)
        {
            case FilterType::bell:
                // (s^2 + (A/Q) w s + w^2) / (s^2 + w/(A Q) s + w^2)
                return { 1.0, A / q * w, w * w, 1.0, w / (A * q), w * w };

            case FilterType::lowShelf:
                // A (s^2 + sqrt(A)/Q w s + A w^2) / (A s^2 + sqrt(A)/Q w s + w^2)
                return { A, A * sA / q * w, A * A * w * w, A, sA / q * w, w * w };

            case FilterType::highShelf:
                // A (A s^2 + sqrt(A)/Q w s + w^2) / (s^2 + sqrt(A)/Q w s + A w^2)
                return { A * A, A * sA / q * w, A * w * w, 1.0, sA / q * w, A * w * w };

            case FilterType::tilt:
            {
                // High shelf scaled by 1/A so it goes from -g/2 (low) to +g/2 (high).
                return { A, sA / q * w, w * w, 1.0, sA / q * w, A * w * w };
            }

            case FilterType::lowCut:
                return { 1.0, 0.0, 0.0, 1.0, w / q, w * w };

            case FilterType::highCut:
                return { 0.0, 0.0, w * w, 1.0, w / q, w * w };

            case FilterType::notch:
                return { 1.0, 0.0, w * w, 1.0, w / q, w * w };

            case FilterType::bandPass:
                return { 0.0, w / q, 0.0, 1.0, w / q, w * w };
        }

        return {};
    }

    BiquadCoefficients designBilinear (const BandSettings& s, double sampleRate) noexcept
    {
        const auto A = amplitude (s.gainDb);
        const auto sA = std::sqrt (A);
        const auto w0 = twoPi * s.frequencyHz / sampleRate;
        const auto c = std::cos (w0);
        const auto alpha = std::sin (w0) / (2.0 * s.q);

        double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

        switch (s.type)
        {
            case FilterType::bell:
                b0 = 1 + alpha * A; b1 = -2 * c; b2 = 1 - alpha * A;
                a0 = 1 + alpha / A; a1 = -2 * c; a2 = 1 - alpha / A;
                break;

            case FilterType::lowShelf:
                b0 = A * ((A + 1) - (A - 1) * c + 2 * sA * alpha);
                b1 = 2 * A * ((A - 1) - (A + 1) * c);
                b2 = A * ((A + 1) - (A - 1) * c - 2 * sA * alpha);
                a0 = (A + 1) + (A - 1) * c + 2 * sA * alpha;
                a1 = -2 * ((A - 1) + (A + 1) * c);
                a2 = (A + 1) + (A - 1) * c - 2 * sA * alpha;
                break;

            case FilterType::highShelf:
            case FilterType::tilt:
                b0 = A * ((A + 1) + (A - 1) * c + 2 * sA * alpha);
                b1 = -2 * A * ((A - 1) + (A + 1) * c);
                b2 = A * ((A + 1) + (A - 1) * c - 2 * sA * alpha);
                a0 = (A + 1) - (A - 1) * c + 2 * sA * alpha;
                a1 = 2 * ((A - 1) - (A + 1) * c);
                a2 = (A + 1) - (A - 1) * c - 2 * sA * alpha;
                if (s.type == FilterType::tilt)
                {
                    b0 /= A; b1 /= A; b2 /= A;
                }
                break;

            case FilterType::lowCut:
                b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2;
                a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha;
                break;

            case FilterType::highCut:
                b0 = (1 - c) / 2; b1 = 1 - c; b2 = (1 - c) / 2;
                a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha;
                break;

            case FilterType::notch:
                b0 = 1; b1 = -2 * c; b2 = 1;
                a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha;
                break;

            case FilterType::bandPass:
                b0 = alpha; b1 = 0; b2 = -alpha;
                a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha;
                break;
        }

        return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
    }

    bool designMatched (const AnalogBiquad& an, double sampleRate, double matchOmega, BiquadCoefficients& out) noexcept
    {
        if (an.d2 <= 0.0 || an.d0 <= 0.0)
            return false;

        const auto T = 1.0 / sampleRate;

        // Poles: s^2 + 2 zeta wp s + wp^2, mapped exactly with z = exp(sT).
        const auto wp = std::sqrt (an.d0 / an.d2);
        const auto zeta = (an.d1 / an.d2) / (2.0 * wp);
        const auto decay = std::exp (-zeta * wp * T);

        double a1;
        if (zeta < 1.0)
            a1 = -2.0 * decay * std::cos (wp * T * std::sqrt (1.0 - zeta * zeta));
        else
            a1 = -2.0 * decay * std::cosh (wp * T * std::sqrt (zeta * zeta - 1.0));
        const auto a2 = decay * decay;

        // |H(e^jw)|^2 = (B0 p0 + B1 p1 + B2 p2) / (A0 p0 + A1 p1 + A2 p2)
        // with p0 = cos^2(w/2), p1 = sin^2(w/2), p2 = 4 p0 p1.
        const auto A0 = (1.0 + a1 + a2) * (1.0 + a1 + a2);
        const auto A1 = (1.0 - a1 + a2) * (1.0 - a1 + a2);
        const auto A2 = -4.0 * a2;

        const auto B0 = A0 * an.magnitudeSquared (0.0);
        const auto B1 = A1 * an.magnitudeSquared (pi * sampleRate);

        const auto s = std::sin (0.5 * matchOmega);
        const auto p1 = s * s, p0 = 1.0 - p1, p2 = 4.0 * p0 * p1;
        if (p2 < 1.0e-12)
            return false;

        const auto target = an.magnitudeSquared (matchOmega * sampleRate);
        const auto B2 = (target * (A0 * p0 + A1 * p1 + A2 * p2) - B0 * p0 - B1 * p1) / p2;

        // Recover a minimum-phase numerator from B0, B1, B2.
        const auto sB0 = std::sqrt (B0), sB1 = std::sqrt (B1);
        const auto W = 0.5 * (sB0 + sB1);
        const auto disc = W * W + B2;
        if (disc < 0.0)
            return false;

        const auto b0 = 0.5 * (W + std::sqrt (disc));
        if (b0 <= 0.0)
            return false;

        out = { b0, 0.5 * (sB0 - sB1), -B2 / (4.0 * b0), a1, a2 };
        return isFinite (out);
    }

    BiquadCoefficients design (BandSettings s, double sampleRate) noexcept
    {
        s = sanitise (s, sampleRate);

        if (isGainType (s.type) && std::abs (s.gainDb) < 1.0e-9)
            return {}; // exact identity

        // Low-cut: bilinear measured more accurate where low-cuts are used. Notch: bilinear keeps
        // the zero exactly on the unit circle, so the notch is truly deep at its frequency.
        if (s.type == FilterType::lowCut || s.type == FilterType::notch)
            return designBilinear (s, sampleRate);

        if (isGainType (s.type) && s.gainDb < 0.0)
        {
            auto boost = s;
            boost.gainDb = -s.gainDb;
            return invert (designBestMatched (boost, sampleRate));
        }

        return designBestMatched (s, sampleRate);
    }

    double butterworthSectionQ (int order, int k) noexcept
    {
        // Pole-pair angle from the negative real axis: (2k+1)pi/(2n) for even n, (k+1)pi/n for odd n.
        const auto phi = (order % 2 == 0) ? pi * (2 * k + 1) / (2.0 * order) : pi * (k + 1) / (double) order;
        return 1.0 / (2.0 * std::cos (phi));
    }

    CascadeCoefficients designCascade (BandSettings s, int slopeDbPerOct, double sampleRate) noexcept
    {
        CascadeCoefficients out;

        if (s.type != FilterType::lowCut && s.type != FilterType::highCut)
        {
            out.sections[0] = design (s, sampleRate);
            return out;
        }

        s = sanitise (s, sampleRate);
        const auto order = std::clamp (slopeDbPerOct / 6, 1, 2 * maxSections);
        const auto numPairs = order / 2;
        const auto resonance = s.q * std::numbers::sqrt2; // q / 0.7071: 1 at the Butterworth default
        int n = 0;

        if (order % 2 == 1)
            out.sections[(size_t) n++] = s.type == FilterType::highCut ? matchedFirstOrderLowPass (s.frequencyHz, sampleRate)
                                                                        : bilinearFirstOrderHighPass (s.frequencyHz, sampleRate);

        for (int k = 0; k < numPairs; ++k)
        {
            auto section = s;
            section.q = butterworthSectionQ (order, k);
            if (k == numPairs - 1)
                section.q *= resonance; // the band's Q sets the corner resonance on the sharpest pair
            out.sections[(size_t) n++] = design (section, sampleRate);
        }

        out.numSections = n;
        return out;
    }

    double cascadePowerGain (const CascadeCoefficients& c, double frequencyHz, double sampleRate) noexcept
    {
        double p = 1.0;
        for (int i = 0; i < c.numSections; ++i)
            p *= powerGain (c.sections[(size_t) i], frequencyHz, sampleRate);
        return p;
    }

    double powerGain (const BiquadCoefficients& c, double frequencyHz, double sampleRate) noexcept
    {
        const auto z1 = std::polar (1.0, -twoPi * frequencyHz / sampleRate);
        const auto z2 = z1 * z1;
        return std::norm ((c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2));
    }

    double magnitudeDb (const BiquadCoefficients& c, double frequencyHz, double sampleRate) noexcept
    {
        return 10.0 * std::log10 (powerGain (c, frequencyHz, sampleRate));
    }

    double analogMagnitudeDb (const BandSettings& settings, double frequencyHz) noexcept
    {
        return 10.0 * std::log10 (makeAnalogPrototype (settings).magnitudeSquared (twoPi * frequencyHz));
    }
}
