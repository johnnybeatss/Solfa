#pragma once

#include "../Parameters.h"
#include "../dsp/EqEngine.h"

#include <array>

namespace solfa::ui
{
    // UI-side copy of the EQ response, designed from the current parameter values with the same
    // design functions the audio engine uses (so the drawn curve is the real response).
    class CurveModel
    {
    public:
        void update (const params::Handles& handles, double sampleRate, double literalFocusHz);

        const dsp::BandTarget& getTarget (int band) const noexcept { return targets[(size_t) band]; }
        bool isEnabled (int band) const noexcept { return targets[(size_t) band].enabled; }

        double bandDb (int band, double frequencyHz) const noexcept;
        double totalDb (double frequencyHz) const noexcept; // all bands + Literal
        // Dynamic bands: the response at full Range (static + range), for drawing the swing.
        double bandDynamicDb (int band, double frequencyHz) const noexcept;

        double getSampleRate() const noexcept { return sampleRate; }

    private:
        double sampleRate = 48000.0;
        std::array<dsp::BandTarget, dsp::numBands> targets {};
        std::array<dsp::CascadeCoefficients, dsp::numBands> cascades {}, dynamicCascades {};
        dsp::CascadeCoefficients literal {};
    };

    // Mapping helpers shared by the display and its tests.
    struct Mapping
    {
        static constexpr double minHz = 20.0, maxHz = 20000.0;
        float width = 1.0f, height = 1.0f;
        double rangeDb = 18.0;

        float xForFrequency (double f) const noexcept;
        double frequencyForX (float x) const noexcept;
        float yForDb (double db) const noexcept;
        double dbForY (float y) const noexcept;
    };
}
