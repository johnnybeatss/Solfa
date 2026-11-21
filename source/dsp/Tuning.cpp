#include "Tuning.h"

#include <cmath>
#include <limits>

namespace solfa::dsp
{
    namespace
    {
        constexpr std::array<bool, 12> majorSteps { true, false, true, false, true, true, false, true, false, true, false, true };
        constexpr std::array<bool, 12> minorSteps { true, false, true, true, false, true, false, true, true, false, true, false };

        bool inScale (int pitchClassFromRoot, Scale scale) noexcept
        {
            switch (scale)
            {
                case Scale::chromatic: return true;
                case Scale::major: return majorSteps[(size_t) pitchClassFromRoot];
                case Scale::minor: return minorSteps[(size_t) pitchClassFromRoot];
            }
            return true;
        }
    }

    double snapToScale (double frequencyHz, const Tuning& t) noexcept
    {
        if (frequencyHz <= 0.0)
            return frequencyHz;

        // MIDI-style note number relative to A4 = 69, as a real number.
        const auto note = 69.0 + 12.0 * std::log2 (frequencyHz / t.referenceA4);
        const auto centre = (int) std::lround (note);

        double best = note;
        double bestDistance = std::numeric_limits<double>::max();
        for (int candidate = centre - 6; candidate <= centre + 6; ++candidate)
        {
            const auto pitchClass = ((candidate % 12) + 12) % 12;         // 0 = C
            const auto fromRoot = ((pitchClass - t.keyRoot) % 12 + 12) % 12;
            if (! inScale (fromRoot, t.scale))
                continue;
            const auto distance = std::abs (candidate - note);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                best = candidate;
            }
        }

        return t.referenceA4 * std::exp2 ((best - 69.0) / 12.0);
    }

    double centsFromNearestNote (double frequencyHz, double referenceA4) noexcept
    {
        const auto note = 69.0 + 12.0 * std::log2 (frequencyHz / referenceA4);
        return 100.0 * (note - std::round (note));
    }
}
