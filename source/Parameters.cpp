#include "Parameters.h"

namespace solfa::params
{
    namespace
    {
        // Defaults: a classic layout (shelf, six bells, shelf), all switched off so a fresh
        // instance is transparent.
        struct BandDefault
        {
            dsp::FilterType type;
            float freq;
            float q;
        };

        constexpr std::array<BandDefault, dsp::numBands> bandDefaults { {
            { dsp::FilterType::lowShelf, 80.0f, 0.707f },
            { dsp::FilterType::bell, 200.0f, 1.0f },
            { dsp::FilterType::bell, 500.0f, 1.0f },
            { dsp::FilterType::bell, 1000.0f, 1.0f },
            { dsp::FilterType::bell, 2000.0f, 1.0f },
            { dsp::FilterType::bell, 4000.0f, 1.0f },
            { dsp::FilterType::bell, 8000.0f, 1.0f },
            { dsp::FilterType::highShelf, 12000.0f, 0.707f },
        } };

        // Exact logarithmic mapping between [min, max] and [0, 1].
        juce::NormalisableRange<float> logRange (float min, float max)
        {
            return { min, max,
                [] (float lo, float hi, float t) { return lo * std::pow (hi / lo, t); },
                [] (float lo, float hi, float v) { return std::log (v / lo) / std::log (hi / lo); },
                [] (float lo, float hi, float v) { return juce::jlimit (lo, hi, v); } };
        }

        juce::String formatHz (float v, int)
        {
            return v < 1000.0f ? juce::String (v, 1) + " Hz" : juce::String (v / 1000.0f, 2) + " kHz";
        }
    }

    juce::String bandId (int bandIndex, const char* suffix)
    {
        return "b" + juce::String (bandIndex + 1) + "_" + suffix;
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        const juce::StringArray typeNames { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Tilt" };
        jassert (typeNames.size() == dsp::numFilterTypes);

        for (int b = 0; b < dsp::numBands; ++b)
        {
            const auto& d = bandDefaults[(size_t) b];
            const auto prefix = "Band " + juce::String (b + 1) + " ";
            auto group = std::make_unique<juce::AudioProcessorParameterGroup> (
                "band" + juce::String (b + 1), "Band " + juce::String (b + 1), "|");

            group->addChild (std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID { bandId (b, on), 1 }, prefix + "On", false));

            group->addChild (std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { bandId (b, type), 1 }, prefix + "Type", typeNames, (int) d.type));

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { bandId (b, freq), 1 }, prefix + "Freq", logRange (20.0f, 20000.0f), d.freq,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (formatHz)));

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { bandId (b, gain), 1 }, prefix + "Gain",
                juce::NormalisableRange<float> (-18.0f, 18.0f, 0.01f), 0.0f,
                juce::AudioParameterFloatAttributes().withLabel ("dB")));

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { bandId (b, q), 1 }, prefix + "Q", logRange (0.1f, 18.0f), d.q));

            layout.add (std::move (group));
        }

        // Scales every band's gain in dB (never a dry/wet mix: see CLAUDE.md).
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { amount, 1 }, "Amount",
            juce::NormalisableRange<float> (0.0f, 150.0f, 0.1f), 100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { outputGain, 1 }, "Output",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        return layout;
    }

    dsp::BandTarget BandHandles::read() const noexcept
    {
        dsp::BandTarget t;
        t.enabled = on->load() >= 0.5f;
        t.type = (dsp::FilterType) juce::jlimit (0, dsp::numFilterTypes - 1, (int) type->load());
        t.frequencyHz = freq->load();
        t.gainDb = gain->load();
        t.q = q->load();
        return t;
    }

    Handles::Handles (juce::AudioProcessorValueTreeState& state)
    {
        auto get = [&state] (const juce::String& id) {
            auto* p = state.getRawParameterValue (id);
            jassert (p != nullptr);
            return p;
        };

        outputGain = get (params::outputGain);
        amount = get (params::amount);

        for (int b = 0; b < dsp::numBands; ++b)
        {
            auto& h = bands[(size_t) b];
            h.on = get (bandId (b, on));
            h.type = get (bandId (b, type));
            h.freq = get (bandId (b, freq));
            h.gain = get (bandId (b, gain));
            h.q = get (bandId (b, q));
        }
    }
}
