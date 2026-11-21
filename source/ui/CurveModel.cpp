#include "CurveModel.h"

#include <cmath>

namespace solfa::ui
{
    namespace
    {
        dsp::CascadeCoefficients designFor (const dsp::BandTarget& t, double gainDb, double fs)
        {
            dsp::BandSettings s;
            s.type = t.type;
            s.frequencyHz = t.frequencyHz;
            s.gainDb = gainDb;
            s.q = t.q;
            return dsp::designCascade (s, t.slopeDbPerOct, fs);
        }

        double db (const dsp::CascadeCoefficients& c, double f, double fs)
        {
            return 10.0 * std::log10 (dsp::cascadePowerGain (c, f, fs));
        }
    }

    void CurveModel::update (const params::Handles& handles, double fs, double literalFocusHz)
    {
        sampleRate = fs > 0.0 ? fs : 48000.0;
        const auto amount = handles.amount->load() / 100.0;

        for (int b = 0; b < dsp::numBands; ++b)
        {
            auto& t = targets[(size_t) b];
            t = handles.bands[(size_t) b].read();
            if (! t.enabled)
                continue;
            const auto gainType = dsp::isGainTypeFor (t.type);
            cascades[(size_t) b] = designFor (t, gainType ? dsp::snapToUnity (t.gainDb * amount) : 0.0, sampleRate);
            if (gainType && t.dynamic)
                dynamicCascades[(size_t) b] = designFor (t, dsp::snapToUnity ((t.gainDb + t.dynRangeDb) * amount), sampleRate);
        }

        dsp::BandSettings lit { dsp::FilterType::bell, literalFocusHz, dsp::snapToUnity (handles.literal->load() * amount), 6.0 };
        literal = dsp::designCascade (lit, 12, sampleRate);
    }

    double CurveModel::bandDb (int band, double f) const noexcept
    {
        return isEnabled (band) ? db (cascades[(size_t) band], f, sampleRate) : 0.0;
    }

    double CurveModel::bandDynamicDb (int band, double f) const noexcept
    {
        const auto& t = targets[(size_t) band];
        if (! t.enabled || ! t.dynamic || ! dsp::isGainTypeFor (t.type))
            return bandDb (band, f);
        return db (dynamicCascades[(size_t) band], f, sampleRate);
    }

    double CurveModel::totalDb (double f) const noexcept
    {
        double sum = db (literal, f, sampleRate);
        for (int b = 0; b < dsp::numBands; ++b)
            sum += bandDb (b, f);
        return sum;
    }

    float Mapping::xForFrequency (double f) const noexcept
    {
        return (float) (std::log (f / minHz) / std::log (maxHz / minHz)) * width;
    }

    double Mapping::frequencyForX (float x) const noexcept
    {
        return minHz * std::pow (maxHz / minHz, (double) (x / width));
    }

    float Mapping::yForDb (double d) const noexcept
    {
        return (float) (0.5 - 0.5 * d / rangeDb) * height;
    }

    double Mapping::dbForY (float y) const noexcept
    {
        return (0.5 - (double) (y / height)) * 2.0 * rangeDb;
    }
}
