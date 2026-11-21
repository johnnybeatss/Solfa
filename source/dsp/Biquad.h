#pragma once

#include "FilterDesign.h"
#include <cmath>

namespace solfa::dsp
{
    // Transposed Direct Form II biquad in double precision (EarLevel's recommended form for
    // floating point). Coefficients can change between samples; state is kept.
    class Biquad
    {
    public:
        void setCoefficients (const BiquadCoefficients& c) noexcept { coeffs = c; }
        const BiquadCoefficients& getCoefficients() const noexcept { return coeffs; }

        void reset() noexcept { s1 = s2 = 0.0; }

        double process (double x) noexcept
        {
            const auto y = coeffs.b0 * x + s1;
            s1 = coeffs.b1 * x - coeffs.a1 * y + s2;
            s2 = coeffs.b2 * x - coeffs.a2 * y;
            return y;
        }

        // True once the internal state has decayed to (numerically) nothing.
        bool isSilent (double threshold = 1.0e-10) const noexcept
        {
            return std::abs (s1) < threshold && std::abs (s2) < threshold;
        }

    private:
        BiquadCoefficients coeffs;
        double s1 = 0.0, s2 = 0.0;
    };
}
