#include "Parameters.h"

namespace solfa::params
{
    namespace
    {
        // Defaults: bands 1-8 form a classic layout (shelf, six bells, shelf); bands 9-24 are
        // spare bells. All are off, so a fresh instance is transparent.
        struct BandDefault
        {
            dsp::FilterType type;
            float freq;
            float q;
        };

        constexpr std::array<BandDefault, 8> classicDefaults { {
            { dsp::FilterType::lowShelf, 80.0f, 0.707f },
            { dsp::FilterType::bell, 200.0f, 1.0f },
            { dsp::FilterType::bell, 500.0f, 1.0f },
            { dsp::FilterType::bell, 1000.0f, 1.0f },
            { dsp::FilterType::bell, 2000.0f, 1.0f },
            { dsp::FilterType::bell, 4000.0f, 1.0f },
            { dsp::FilterType::bell, 8000.0f, 1.0f },
            { dsp::FilterType::highShelf, 12000.0f, 0.707f },
        } };

        BandDefault bandDefault (int b)
        {
            return b < (int) classicDefaults.size() ? classicDefaults[(size_t) b]
                                                    : BandDefault { dsp::FilterType::bell, 1000.0f, 1.0f };
        }

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

        const juce::StringArray typeNames { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Tilt",
            "Notch", "Band Pass" };
        jassert (typeNames.size() == dsp::numFilterTypes);

        juce::StringArray slopeNames;
        for (auto dbPerOct : dsp::slopeOptionsDbPerOct)
            slopeNames.add (juce::String (dbPerOct) + " dB/oct");

        const juce::StringArray placeNames { "Stereo", "Left", "Right", "Mid", "Side" };
        jassert (placeNames.size() == dsp::numPlacements);

        for (int b = 0; b < dsp::numBands; ++b)
        {
            const auto d = bandDefault (b);
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

            group->addChild (std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { bandId (b, slope), 1 }, prefix + "Slope", slopeNames, dsp::defaultSlopeIndex));

            group->addChild (std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { bandId (b, place), 1 }, prefix + "Placement", placeNames, 0));

            group->addChild (std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID { bandId (b, dyn), 1 }, prefix + "Dynamic", false));

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { bandId (b, thr), 1 }, prefix + "Threshold",
                juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -24.0f,
                juce::AudioParameterFloatAttributes().withLabel ("dB")));

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { bandId (b, range), 1 }, prefix + "Range",
                juce::NormalisableRange<float> (-18.0f, 18.0f, 0.01f), -6.0f,
                juce::AudioParameterFloatAttributes().withLabel ("dB")));

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { bandId (b, atk), 1 }, prefix + "Attack", logRange (1.0f, 200.0f), 10.0f,
                juce::AudioParameterFloatAttributes().withLabel ("ms")));

            group->addChild (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { bandId (b, rel), 1 }, prefix + "Release", logRange (10.0f, 2000.0f), 150.0f,
                juce::AudioParameterFloatAttributes().withLabel ("ms")));

            layout.add (std::move (group));
        }

        // Scales every band's gain in dB (never a dry/wet mix: see CLAUDE.md).
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { amount, 1 }, "Amount",
            juce::NormalisableRange<float> (0.0f, 150.0f, 0.1f), 100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        // On by default: listeners judge louder as better, so keep the level steady while EQing.
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { autoGain, 1 }, "Auto Gain", true));

        // Plays the unprocessed signal at the processed signal's measured loudness.
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { compare, 1 }, "Compare (level-matched bypass)", false,
            juce::AudioParameterBoolAttributes().withAutomatable (false)));

        // Focus, Key Snap, Literal, Resonance
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { focus, 1 }, "Focus", logRange (100.0f, 2000.0f), 528.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (formatHz)));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { pure, 1 }, "Pure (exact focus, no Key Snap)", false));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { tuneRef, 1 }, "Tuning Reference",
            juce::StringArray { "A = 440 Hz", "A = 432 Hz", "A = 444 Hz" }, 0));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { key, 1 }, "Key",
            juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { scale, 1 }, "Scale", juce::StringArray { "Chromatic", "Major", "Minor" }, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { literal, 1 }, "Literal Boost",
            juce::NormalisableRange<float> (0.0f, 3.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { resAmount, 1 }, "Resonance",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { resDecay, 1 }, "Resonance Decay", logRange (0.2f, 5.0f), 1.5f,
            juce::AudioParameterFloatAttributes().withLabel ("s")));

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
        const auto slopeIndex = juce::jlimit (0, (int) dsp::slopeOptionsDbPerOct.size() - 1, (int) slope->load());
        t.slopeDbPerOct = dsp::slopeOptionsDbPerOct[(size_t) slopeIndex];
        t.placement = (dsp::StereoPlacement) juce::jlimit (0, dsp::numPlacements - 1, (int) place->load());
        t.dynamic = dyn->load() >= 0.5f;
        t.dynThresholdDb = thr->load();
        t.dynRangeDb = range->load();
        t.dynAttackMs = atk->load();
        t.dynReleaseMs = rel->load();
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
        autoGain = get (params::autoGain);
        compare = get (params::compare);
        focus = get (params::focus);
        pure = get (params::pure);
        tuneRef = get (params::tuneRef);
        key = get (params::key);
        scale = get (params::scale);
        literal = get (params::literal);
        resAmount = get (params::resAmount);
        resDecay = get (params::resDecay);

        for (int b = 0; b < dsp::numBands; ++b)
        {
            auto& h = bands[(size_t) b];
            h.on = get (bandId (b, on));
            h.type = get (bandId (b, type));
            h.freq = get (bandId (b, freq));
            h.gain = get (bandId (b, gain));
            h.q = get (bandId (b, q));
            h.slope = get (bandId (b, slope));
            h.place = get (bandId (b, place));
            h.dyn = get (bandId (b, dyn));
            h.thr = get (bandId (b, thr));
            h.range = get (bandId (b, range));
            h.atk = get (bandId (b, atk));
            h.rel = get (bandId (b, rel));
        }
    }
}
