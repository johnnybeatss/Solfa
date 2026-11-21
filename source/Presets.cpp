#include "Presets.h"
#include "Parameters.h"

namespace solfa::presets
{
    namespace
    {
        using T = dsp::FilterType;

        void set (juce::AudioProcessorValueTreeState& state, const juce::String& id, float value)
        {
            if (auto* p = state.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (value));
        }

        void setDefault (juce::AudioProcessorValueTreeState& state, const juce::String& id)
        {
            if (auto* p = state.getParameter (id))
                p->setValueNotifyingHost (p->getDefaultValue());
        }
    }

    const std::vector<Preset>& factory()
    {
        // Starting curves derived from the research (broad, +/-0.5-4 dB, Q 0.3-1.2, octaves of the
        // named number). They are hypotheses to refine by ear with level-matched listening.
        static const std::vector<Preset> presets {
            { "Init", "Flat. Everything at its default.", 528.0, {}, 0.0, 1.5 },

            { "Grounding 174", "Weight in the lows, less mud, a softer top.", 174.0,
                { { T::lowShelf, 87.0, 2.0, 0.7 }, { T::bell, 348.0, -1.5, 1.0 }, { T::highShelf, 8000.0, -1.0, 0.7 } },
                15.0, 1.5 },

            { "Restore 285", "Warm and rounded: lows up, highs eased.", 285.0,
                { { T::tilt, 1140.0, -2.0, 0.5 }, { T::highShelf, 9120.0, -1.5, 0.7 } },
                15.0, 1.5 },

            { "Release 396", "Clears the low-mid cloud for a more open sound.", 396.0,
                { { T::bell, 396.0, -2.0, 1.0 }, { T::tilt, 1584.0, 1.0, 0.5 } },
                10.0, 1.2 },

            { "Change 417", "Less boxiness, a little more presence.", 417.0,
                { { T::bell, 417.0, -1.5, 1.2 }, { T::bell, 3336.0, 1.5, 0.8 } },
                15.0, 1.5 },

            { "Heart 528", "Closer and more intimate: presence up, boom down, top softened.", 528.0,
                { { T::bell, 4224.0, 1.5, 0.8 }, { T::bell, 264.0, -1.0, 1.0 }, { T::highShelf, 10000.0, -1.0, 0.7 } },
                20.0, 1.8 },

            { "Connect 639", "Open and wide: less low-mid, more air.", 639.0,
                { { T::bell, 319.5, -1.5, 1.0 }, { T::highShelf, 10224.0, 1.5, 0.7 } },
                15.0, 1.5 },

            { "Clarity 741", "Clear and present, without harshness.", 741.0,
                { { T::bell, 370.5, -1.5, 1.0 }, { T::bell, 2964.0, 1.0, 0.7 } },
                10.0, 1.2 },

            { "Intuition 852", "Airy and light: extra air, less low end.", 852.0,
                { { T::highShelf, 13632.0, 2.5, 0.7 }, { T::lowShelf, 106.5, -1.5, 0.7 } },
                20.0, 2.5 },

            { "Crown 963", "Ethereal: bright air on top, weight taken out underneath.", 963.0,
                { { T::highShelf, 7704.0, 3.0, 0.7 }, { T::lowShelf, 120.4, -2.5, 0.7 } },
                20.0, 3.0 },

            { "Natural 432", "No EQ: selects A = 432 Hz tuning for Key Snap and rings gently at 432.", 432.0,
                {}, 15.0, 2.0, 1 },
        };
        return presets;
    }

    void apply (const Preset& preset, juce::AudioProcessorValueTreeState& state)
    {
        // Start from defaults for every band, then write the preset's bands into the first slots.
        for (int b = 0; b < dsp::numBands; ++b)
            for (auto* suffix : { params::on, params::type, params::freq, params::gain, params::q, params::slope, params::place })
                setDefault (state, params::bandId (b, suffix));

        for (size_t i = 0; i < preset.bands.size() && i < (size_t) dsp::numBands; ++i)
        {
            const auto& band = preset.bands[i];
            const auto b = (int) i;
            set (state, params::bandId (b, params::type), (float) band.type);
            set (state, params::bandId (b, params::freq), (float) band.frequencyHz);
            set (state, params::bandId (b, params::gain), (float) band.gainDb);
            set (state, params::bandId (b, params::q), (float) band.q);
            set (state, params::bandId (b, params::on), 1.0f);
        }

        setDefault (state, params::amount);
        setDefault (state, params::outputGain);
        setDefault (state, params::literal);
        setDefault (state, params::compare);
        set (state, params::focus, (float) preset.focusHz);
        set (state, params::resAmount, (float) preset.resonancePercent);
        set (state, params::resDecay, (float) preset.decaySeconds);

        if (preset.referenceIndex >= 0)
            set (state, params::tuneRef, (float) preset.referenceIndex);
    }
}
