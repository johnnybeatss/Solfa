// Phase 7: interface logic (mapping, curve model, analyser, Spectrum Grab) and a screenshot.

#include "helpers/test_helpers.h"
#include "PluginEditor.h"
#include "Presets.h"
#include "ui/CurveModel.h"
#include "ui/Spectrum.h"
#include <PluginProcessor.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>

using namespace solfa;

TEST_CASE ("Display mapping round-trips frequency and gain", "[ui]")
{
    ui::Mapping m { 800.0f, 300.0f, 18.0 };
    for (double f : { 20.0, 100.0, 528.0, 1000.0, 9000.0, 20000.0 })
        CHECK (m.frequencyForX (m.xForFrequency (f)) == Catch::Approx (f).epsilon (1e-5));
    for (double d : { -18.0, -3.5, 0.0, 6.0, 18.0 })
        CHECK (m.dbForY (m.yForDb (d)) == Catch::Approx (d).margin (1e-4));
    CHECK (m.xForFrequency (20.0) == Catch::Approx (0.0).margin (1e-4));
    CHECK (m.xForFrequency (20000.0) == Catch::Approx (800.0).margin (1e-3));
    CHECK (m.yForDb (0.0) == Catch::Approx (150.0).margin (1e-4));
}

TEST_CASE ("The drawn curve is the engine's response", "[ui]")
{
    PluginProcessor plugin;
    auto set = [&] (const juce::String& id, float v) {
        auto* p = plugin.getState().getParameter (id);
        p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    using namespace params;
    set (bandId (2, on), 1.0f);
    set (bandId (2, freq), 1000.0f);
    set (bandId (2, gain), 6.0f);
    set (bandId (2, q), 1.0f);
    set (bandId (5, on), 1.0f);
    set (bandId (5, type), (float) dsp::FilterType::highCut);
    set (bandId (5, freq), 8000.0f);
    set (bandId (5, q), 0.7071f); // plain Butterworth: no resonant lift below the corner

    params::Handles handles (plugin.getState());
    ui::CurveModel model;
    model.update (handles, 48000.0, 528.0);

    const auto bell = dsp::design ({ dsp::FilterType::bell, 1000.0, 6.0, 1.0 }, 48000.0);
    const auto cut = dsp::designCascade ({ dsp::FilterType::highCut, 8000.0, 0.0, 0.7071 }, 12, 48000.0);
    for (double f : { 100.0, 1000.0, 4000.0, 12000.0 })
    {
        const auto expected = dsp::magnitudeDb (bell, f, 48000.0) + 10.0 * std::log10 (dsp::cascadePowerGain (cut, f, 48000.0));
        CHECK (model.totalDb (f) == Catch::Approx (expected).margin (1e-6));
    }
    CHECK (model.totalDb (1000.0) == Catch::Approx (6.0).margin (0.05));
}

TEST_CASE ("Analyser reads a sine at its level and frequency", "[ui][analyser]")
{
    ui::Spectrum spectrum;
    spectrum.setSampleRate (48000.0);
    std::vector<float> tone ((size_t) ui::Spectrum::fftSize);
    for (size_t i = 0; i < tone.size(); ++i)
        tone[i] = 0.5f * (float) std::sin (2.0 * std::numbers::pi * 1000.0 * (double) i / 48000.0);
    spectrum.push (tone.data(), (int) tone.size());
    spectrum.update (1.0 / 30.0);

    // 1 kHz is the tilt pivot, so the display level equals the sine's peak level: -6 dBFS.
    double peak = -200, peakFreq = 0;
    for (double f = 900; f <= 1100; f += 1.0)
        if (spectrum.levelDbAt (f) > peak)
        {
            peak = spectrum.levelDbAt (f);
            peakFreq = f;
        }
    CHECK (peak == Catch::Approx (-6.02).margin (1.0));
    CHECK (peakFreq == Catch::Approx (1000.0).margin (6.0));
}

TEST_CASE ("Spectrum Grab finds the nearest peak and ignores flat regions", "[ui][analyser]")
{
    ui::Spectrum spectrum;
    spectrum.setSampleRate (48000.0);
    std::vector<double> levels ((size_t) spectrum.getNumBins(), -60.0);
    auto binOf = [&] (double f) { return (size_t) std::lround (f * ui::Spectrum::fftSize / 48000.0); };
    levels[binOf (440.0)] = -10.0;
    levels[binOf (440.0) - 1] = -16.0;
    levels[binOf (440.0) + 1] = -16.0;
    spectrum.setLevelsForTesting (levels);

    CHECK (spectrum.findPeakNear (400.0) == Catch::Approx (440.0).margin (3.0)); // within 1/3 octave
    CHECK (spectrum.findPeakNear (2000.0) == Catch::Approx (2000.0));           // flat: unchanged
}

TEST_CASE ("Double-click logic adds a band in the first free slot", "[ui]")
{
    runWithinPluginEditor ([] (PluginProcessor& plugin) {
        auto* editor = dynamic_cast<PluginEditor*> (plugin.getActiveEditor());
        REQUIRE (editor != nullptr);
        const auto band = editor->getDisplay().addBand (750.0, 4.5);
        CHECK (band == 0);
        CHECK (plugin.getState().getRawParameterValue (params::bandId (0, params::on))->load() > 0.5f);
        CHECK (plugin.getState().getRawParameterValue (params::bandId (0, params::freq))->load() == Catch::Approx (750.0f).margin (0.5f));
        CHECK (plugin.getState().getRawParameterValue (params::bandId (0, params::gain))->load() == Catch::Approx (4.5f).margin (0.01f));
        CHECK (editor->getDisplay().getSelectedBand() == 0);
    });
}

// Hidden by default (run with "[screenshot]"): renders the editor to a PNG for visual review.
TEST_CASE ("Editor screenshot", "[.][screenshot]")
{
    runWithinPluginEditor ([] (PluginProcessor& plugin) {
        plugin.prepareToPlay (48000.0, 512);
        plugin.setCurrentProgram (5); // Heart 528
        auto set = [&] (const juce::String& id, float v) {
            auto* p = plugin.getState().getParameter (id);
            p->setValueNotifyingHost (p->convertTo0to1 (v));
        };
        set (params::bandId (3, params::on), 1.0f);
        set (params::bandId (3, params::freq), 2500.0f);
        set (params::bandId (3, params::gain), -4.0f);
        set (params::bandId (3, params::dyn), 1.0f);
        set (params::bandId (3, params::range), 5.0f);

        // Feed some noise-plus-tones so the analyser has something to show.
        juce::Random rng (7);
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        double phase = 0;
        for (int k = 0; k < 40; ++k)
        {
            for (int i = 0; i < 512; ++i)
            {
                const auto x = 0.05f * (rng.nextFloat() * 2.0f - 1.0f) + 0.2f * (float) std::sin (phase) + 0.1f * (float) std::sin (3.1 * phase);
                phase += 2.0 * std::numbers::pi * 220.0 / 48000.0;
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, x);
            }
            plugin.processBlock (buffer, midi);
        }

        auto* editor = dynamic_cast<PluginEditor*> (plugin.getActiveEditor());
        REQUIRE (editor != nullptr);
        editor->getDisplay().refreshNow();
        editor->getDisplay().setSelectedBand (0);
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        const auto path = juce::SystemStats::getEnvironmentVariable ("SOLFA_SNAPSHOT", "/tmp/solfa-editor.png");
        juce::File file (path);
        file.deleteFile();
        juce::FileOutputStream stream (file);
        REQUIRE (juce::PNGImageFormat().writeImageToStream (image, stream));
    });
}
