// Phase 5: Key Snap, Resonance, Literal and factory presets.

#include "Presets.h"
#include "dsp/Resonator.h"
#include "dsp/Tuning.h"
#include <PluginProcessor.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>

using namespace solfa;

namespace
{
    void setParam (PluginProcessor& p, const juce::String& id, float value)
    {
        auto* param = p.getState().getParameter (id);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (value));
    }

    float getParam (PluginProcessor& p, const juce::String& id)
    {
        return p.getState().getRawParameterValue (id)->load();
    }

    // Steady-state gain in dB of the whole plugin for a sine at frequencyHz.
    double pluginGainDb (PluginProcessor& plugin, double fs, double frequencyHz)
    {
        constexpr int block = 480;
        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;
        double phase = 0, inSq = 0, outSq = 0;
        const auto inc = 2.0 * std::numbers::pi * frequencyHz / fs;
        for (int k = 0; k < 100; ++k)
        {
            for (int i = 0; i < block; ++i)
            {
                const auto x = (float) (0.1 * std::sin (phase));
                phase += inc;
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, x);
                if (k >= 80)
                    inSq += (double) x * x;
            }
            plugin.processBlock (buffer, midi);
            if (k >= 80)
                for (int i = 0; i < block; ++i)
                    outSq += (double) buffer.getSample (0, i) * buffer.getSample (0, i);
        }
        return 10.0 * std::log10 (outSq / inSq);
    }
}

TEST_CASE ("Key Snap lands on the right notes", "[tuning]")
{
    using dsp::Scale;
    CHECK (dsp::snapToScale (528.0, { 440.0, 0, Scale::chromatic }) == Catch::Approx (523.2511).margin (1e-3)); // C5
    CHECK (dsp::snapToScale (528.0, { 444.0, 0, Scale::chromatic }) == Catch::Approx (528.0102).margin (1e-3)); // C5 at A=444
    CHECK (dsp::snapToScale (528.0, { 432.0, 0, Scale::chromatic }) == Catch::Approx (513.7370).margin (1e-3)); // C5 at A=432
    // D major has C# but not C: 528 Hz goes to C#5 (84 cents away) rather than B4 (116 cents).
    CHECK (dsp::snapToScale (528.0, { 440.0, 2, Scale::major }) == Catch::Approx (554.3653).margin (1e-3));
    // A minor contains A: 432 Hz at A=440 snaps to A4.
    CHECK (dsp::snapToScale (432.0, { 440.0, 9, Scale::minor }) == Catch::Approx (440.0).margin (1e-9));
    CHECK (dsp::centsFromNearestNote (528.0, 440.0) == Catch::Approx (15.64).margin (0.01));
    CHECK (dsp::centsFromNearestNote (432.0, 440.0) == Catch::Approx (-31.77).margin (0.01));
}

TEST_CASE ("Resonance Q gives the requested ring time", "[resonance]")
{
    // A unit impulse into one voice-equivalent band-pass decays 60 dB in about T60 seconds.
    constexpr double fs = 48000, f = 528, t60 = 1.0;
    const auto q = dsp::Resonator::qForDecay (f, t60);
    const auto w0 = 2.0 * std::numbers::pi * f / fs;
    const auto alpha = std::sin (w0) / (2.0 * q);
    dsp::Biquad bp;
    bp.setCoefficients ({ alpha / (1 + alpha), 0, -alpha / (1 + alpha), -2 * std::cos (w0) / (1 + alpha), (1 - alpha) / (1 + alpha) });

    // Peak envelope in the first and last 10 ms windows around t = 0.1 s and t = 0.1 + t60.
    auto envelopeAt = [&] (double startSeconds, std::vector<double>& y) {
        double peak = 0;
        for (int i = (int) (startSeconds * fs); i < (int) ((startSeconds + 0.01) * fs); ++i)
            peak = std::max (peak, std::abs (y[(size_t) i]));
        return peak;
    };
    std::vector<double> y ((size_t) (1.5 * fs));
    for (size_t i = 0; i < y.size(); ++i)
        y[i] = bp.process (i == 0 ? 1.0 : 0.0);
    const auto dropDb = 20.0 * std::log10 (envelopeAt (0.1, y) / envelopeAt (0.1 + t60, y));
    CHECK (dropDb == Catch::Approx (60.0).margin (3.0));
}

TEST_CASE ("Resonance rings at the focus and stays quiet elsewhere", "[resonance]")
{
    constexpr double fs = 48000;
    auto ringLevelDb = [&] (double toneHz) {
        dsp::Resonator r;
        r.prepare (fs, 2);
        r.setFocus (528);
        r.setDecaySeconds (1.0);
        r.setAmount (1.0);
        juce::AudioBuffer<float> buffer (2, 480);
        double phase = 0, ringSq = 0;
        const auto inc = 2.0 * std::numbers::pi * toneHz / fs;
        for (int k = 0; k < 200; ++k) // 2 s of tone
        {
            for (int i = 0; i < 480; ++i)
            {
                const auto x = (float) (0.1 * std::sin (phase));
                phase += inc;
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, x);
            }
            const auto dry = buffer;
            r.process (buffer);
            if (k >= 150)
                for (int i = 0; i < 480; ++i)
                {
                    const auto added = buffer.getSample (0, i) - dry.getSample (0, i);
                    ringSq += (double) added * added;
                }
        }
        return 10.0 * std::log10 (ringSq / (50.0 * 480.0 * 0.005)); // relative to the tone's power
    };
    const auto atFocus = ringLevelDb (528);
    const auto away = ringLevelDb (700);
    INFO ("at focus " << atFocus << " dB, at 700 Hz " << away << " dB");
    CHECK (atFocus == Catch::Approx (0.0).margin (1.0)); // the 0 dB-peak voice at 528 Hz reinforces the tone
    CHECK (away < atFocus - 30.0);
}

TEST_CASE ("Resonance at zero leaves audio bit-for-bit untouched", "[resonance][null]")
{
    dsp::Resonator r;
    r.prepare (48000, 2);
    r.setAmount (0.0);
    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.3f * std::sin (0.05f * (float) i));
    const auto input = buffer;
    r.process (buffer);
    for (int i = 0; i < 256; ++i)
        REQUIRE (juce::exactlyEqual (buffer.getSample (0, i), input.getSample (0, i)));
    CHECK_FALSE (r.isActive());
}

TEST_CASE ("Literal adds its boost at the Key-Snapped focus", "[literal]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000, 480);
    setParam (plugin, params::autoGain, 0.0f);
    setParam (plugin, params::focus, 528.0f);
    setParam (plugin, params::literal, 3.0f);

    // Default tuning A=440, chromatic: 528 Hz snaps to C5 = 523.25 Hz.
    pluginGainDb (plugin, 48000, 1000); // settle
    CHECK (plugin.getEffectiveFocusHz() == Catch::Approx (523.2511).margin (1e-3));
    CHECK (pluginGainDb (plugin, 48000, 523.2511) == Catch::Approx (3.0).margin (0.1));

    setParam (plugin, params::pure, 1.0f);
    CHECK (pluginGainDb (plugin, 48000, 528.0) == Catch::Approx (3.0).margin (0.1));
    CHECK (plugin.getEffectiveFocusHz() == Catch::Approx (528.0).margin (1e-3));
}

TEST_CASE ("Factory presets follow the design rules", "[presets]")
{
    const auto& all = presets::factory();
    REQUIRE (all.size() == 11);
    CHECK (all[0].name == "Init");

    const juce::StringArray forbidden { "heal", "anxiety", "sleep", "dna", "brainwave", "cure", "therap",
        "stress", "cortisol", "chakra", "medical", "pain" };

    for (const auto& p : all)
    {
        INFO (p.name);
        for (const auto& word : forbidden)
        {
            CHECK_FALSE (p.name.containsIgnoreCase (word));
            CHECK_FALSE (p.character.containsIgnoreCase (word));
        }
        CHECK (p.focusHz >= 100.0);
        CHECK (p.focusHz <= 2000.0);
        CHECK (p.resonancePercent <= 25.0);

        for (const auto& b : p.bands)
        {
            // Broad and gentle: +/-4 dB at most, Q 0.3-1.2 (tilts use the fixed tilt Q).
            CHECK (std::abs (b.gainDb) <= 4.0);
            CHECK (b.q >= 0.3);
            CHECK (b.q <= 1.2);
            // Every band sits on an octave of the named number, or is a plain shelf/tilt helper.
            const auto octaves = std::log2 (b.frequencyHz / p.focusHz);
            const bool onOctave = std::abs (octaves - std::round (octaves)) < 1e-3;
            const bool helper = b.type == dsp::FilterType::highShelf || b.type == dsp::FilterType::lowShelf
                             || b.type == dsp::FilterType::tilt;
            CHECK ((onOctave || helper));
        }
    }
}

TEST_CASE ("Loading a preset sets the bands and switches the rest off", "[presets]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000, 480);

    // Dirty a spare band first: loading a preset must turn it off.
    setParam (plugin, params::bandId (20, params::on), 1.0f);
    setParam (plugin, params::bandId (20, params::gain), 6.0f);

    const auto& all = presets::factory();
    int heart = -1;
    for (int i = 0; i < (int) all.size(); ++i)
        if (all[(size_t) i].name == "Heart 528")
            heart = i;
    REQUIRE (heart > 0);

    plugin.setCurrentProgram (heart);
    CHECK (plugin.getCurrentProgram() == heart);
    CHECK (plugin.getProgramName (heart) == "Heart 528");

    const auto& p = all[(size_t) heart];
    for (int b = 0; b < (int) p.bands.size(); ++b)
    {
        CHECK (getParam (plugin, params::bandId (b, params::on)) > 0.5f);
        CHECK (getParam (plugin, params::bandId (b, params::freq)) == Catch::Approx (p.bands[(size_t) b].frequencyHz).margin (0.5));
        CHECK (getParam (plugin, params::bandId (b, params::gain)) == Catch::Approx (p.bands[(size_t) b].gainDb).margin (0.01));
    }
    CHECK (getParam (plugin, params::bandId (20, params::on)) < 0.5f);
    CHECK (getParam (plugin, params::focus) == Catch::Approx (528.0).margin (0.5));
    CHECK (getParam (plugin, params::resAmount) == Catch::Approx (p.resonancePercent).margin (0.1));

    // Presets jump rather than sweep: after one block the curve is already at the preset.
    setParam (plugin, params::autoGain, 0.0f);
    setParam (plugin, params::resAmount, 0.0f);
    juce::AudioBuffer<float> buffer (2, 480);
    buffer.clear();
    juce::MidiBuffer midi;
    plugin.processBlock (buffer, midi);
    const auto& band0 = plugin.getEngine().getBand (0).getCascade().sections[0];
    const auto expected = dsp::design ({ p.bands[0].type, p.bands[0].frequencyHz, p.bands[0].gainDb, p.bands[0].q }, 48000);
    CHECK (band0.b0 == Catch::Approx (expected.b0).epsilon (1e-3));
}

TEST_CASE ("Init returns to a bit-transparent state", "[presets][null]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000, 512);
    plugin.setCurrentProgram (5);
    plugin.setCurrentProgram (0);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    for (int k = 0; k < 200; ++k) // let any ringing and tails die away
    {
        buffer.clear();
        plugin.processBlock (buffer, midi);
    }
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, 0.2f * std::sin (0.03f * (float) i));
    const auto input = buffer;
    plugin.processBlock (buffer, midi);
    for (int i = 0; i < 512; ++i)
        REQUIRE (juce::exactlyEqual (buffer.getSample (0, i), input.getSample (0, i)));
}

TEST_CASE ("User presets save and load the full state", "[presets]")
{
    const auto file = juce::File::createTempFile (presets::userPresetExtension);
    PluginProcessor a;
    setParam (a, params::bandId (7, params::on), 1.0f);
    setParam (a, params::bandId (7, params::gain), -4.5f);
    setParam (a, params::focus, 852.0f);
    setParam (a, params::phaseMode, 1.0f);
    auto xml = a.saveStateXml();
    REQUIRE (xml != nullptr);
    REQUIRE (presets::writeUserPreset (file, *xml));

    PluginProcessor b;
    auto loaded = presets::readUserPreset (file.withFileExtension (presets::userPresetExtension));
    REQUIRE (loaded != nullptr);
    REQUIRE (b.loadStateXml (*loaded));
    CHECK (getParam (b, params::bandId (7, params::on)) > 0.5f);
    CHECK (getParam (b, params::bandId (7, params::gain)) == Catch::Approx (-4.5f).margin (0.01f));
    CHECK (getParam (b, params::focus) == Catch::Approx (852.0f).margin (0.5f));
    CHECK (getParam (b, params::phaseMode) == Catch::Approx (1.0f));

    // Something that isn't a Solfa state is rejected.
    juce::XmlElement other ("NotSolfa");
    CHECK_FALSE (b.loadStateXml (other));
    file.withFileExtension (presets::userPresetExtension).deleteFile();
    file.deleteFile();
}
