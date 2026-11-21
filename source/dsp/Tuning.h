#pragma once

#include <array>

namespace solfa::dsp
{
    // Key Snap: moves a frequency onto the nearest note of a scale in 12-tone equal temperament,
    // relative to a chosen A4 reference (440 / 432 / 444 Hz).
    enum class Scale
    {
        chromatic = 0,
        major,
        minor // natural minor
        // Append only: stored in DAW sessions.
    };

    inline constexpr int numScales = 3;
    inline constexpr std::array<double, 3> referenceOptionsHz { 440.0, 432.0, 444.0 };

    struct Tuning
    {
        double referenceA4 = 440.0;
        int keyRoot = 0; // 0 = C, 1 = C#, ... 11 = B
        Scale scale = Scale::chromatic;
    };

    // Nearest in-scale frequency (measured in cents, so it is symmetric in pitch).
    double snapToScale (double frequencyHz, const Tuning& tuning) noexcept;

    // Signed distance in cents from frequencyHz to the nearest note of the chromatic scale.
    double centsFromNearestNote (double frequencyHz, double referenceA4) noexcept;
}
