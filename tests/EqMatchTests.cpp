// Phase 9: EQ Match analysis and fitting.

#include "dsp/EqEngine.h"
#include "dsp/EqMatch.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>

using namespace solfa::dsp;

namespace
{
    std::vector<double> normalised (const std::vector<double>& freqs, std::vector<double> curve)
    {
        double sum = 0;
        int count = 0;
        for (size_t i = 0; i < freqs.size(); ++i)
            if (freqs[i] >= 100.0 && freqs[i] <= 10000.0)
            {
                sum += curve[i];
                ++count;
            }
        for (auto& v : curve)
            v -= sum / count;
        return curve;
    }
}

TEST_CASE ("Long-term spectrum of white noise is flat, and level changes show up", "[match]")
{
    // Smoothing averages power per bin (a density), so white noise reads the same at every
    // frequency and an EQ'd copy differs by exactly the EQ.
    LongTermSpectrum spectrum (48000.0), quieter (48000.0);
    juce::Random rng (5);
    std::vector<float> x (48000 * 8), y (x.size());
    for (size_t i = 0; i < x.size(); ++i)
    {
        x[i] = rng.nextFloat() - 0.5f;
        y[i] = 0.5f * x[i]; // -6.02 dB
    }
    spectrum.push (x.data(), (int) x.size());
    quieter.push (y.data(), (int) y.size());
    CHECK (spectrum.getSecondsAnalysed() == Catch::Approx (8.0).margin (0.2));

    const std::vector<double> f { 100.0, 1000.0, 10000.0 };
    const auto a = spectrum.levelsDb (f, 1.0 / 3.0), b = quieter.levelsDb (f, 1.0 / 3.0);
    CHECK (a[0] == Catch::Approx (a[1]).margin (1.0));
    CHECK (a[2] == Catch::Approx (a[1]).margin (1.0));
    for (size_t i = 0; i < f.size(); ++i)
        CHECK (a[i] - b[i] == Catch::Approx (6.02).margin (0.05));
}

TEST_CASE ("The fit reproduces a known EQ curve", "[match]")
{
    const double fs = 48000;
    const auto f = match::logGrid();
    const std::vector<FittedBand> truth { { FilterType::bell, 1000.0, 4.0, 1.0 }, { FilterType::lowShelf, 120.0, -3.0, 0.707 },
        { FilterType::bell, 5000.0, -2.5, 2.0 } };
    const auto target = match::responseDb (truth, f, fs);

    const auto bands = match::fit (f, target, 8, fs);
    const auto fitted = match::responseDb (bands, f, fs);
    const auto err = match::rmsErrorDb (f, target, fitted);
    INFO (bands.size() << " bands, RMS error " << err << " dB");
    CHECK (err < 0.5);
    CHECK (bands.size() <= 8);
}

TEST_CASE ("Matching a reference EQ'd with a known curve recovers that curve", "[match]")
{
    // Current = white noise. Reference = the same kind of noise through a known EQ.
    constexpr double fs = 48000;
    const int seconds = 12;
    juce::Random rng (11);
    std::vector<float> current ((size_t) (fs * seconds)), reference ((size_t) (fs * seconds));
    for (auto& s : current)
        s = 0.25f * (rng.nextFloat() * 2.0f - 1.0f);

    EqEngine engine;
    engine.prepare (fs, 512, 1);
    engine.setBand (0, { true, FilterType::bell, 2000.0, 5.0, 1.2 });
    engine.setBand (1, { true, FilterType::highShelf, 8000.0, -4.0, 0.707 });
    juce::AudioBuffer<float> block (1, 512);
    for (size_t start = 0; start < reference.size(); start += 512)
    {
        for (int i = 0; i < 512; ++i)
            block.setSample (0, i, 0.25f * (rng.nextFloat() * 2.0f - 1.0f));
        engine.process (block);
        std::copy_n (block.getReadPointer (0), 512, reference.begin() + (long) start);
    }

    LongTermSpectrum refSpec (fs), curSpec (fs);
    refSpec.push (reference.data(), (int) reference.size());
    curSpec.push (current.data(), (int) current.size());

    const auto f = match::logGrid();
    const auto target = match::matchCurve (f, refSpec.levelsDb (f, 1.0 / 3.0), curSpec.levelsDb (f, 1.0 / 3.0), 1.0);
    const auto bands = match::fit (f, target, 8, fs);
    const auto fitted = normalised (f, match::responseDb (bands, f, fs));

    const std::vector<FittedBand> truth { { FilterType::bell, 2000.0, 5.0, 1.2 }, { FilterType::highShelf, 8000.0, -4.0, 0.707 } };
    const auto expected = normalised (f, match::responseDb (truth, f, fs));

    // Compare over 60 Hz - 15 kHz, where noise statistics are good.
    double s = 0;
    int count = 0;
    for (size_t i = 0; i < f.size(); ++i)
        if (f[i] >= 60.0 && f[i] <= 15000.0)
        {
            s += (fitted[i] - expected[i]) * (fitted[i] - expected[i]);
            ++count;
        }
    const auto rms = std::sqrt (s / count);
    INFO (bands.size() << " bands, RMS error vs the known EQ " << rms << " dB");
    CHECK (rms < 1.0);
}

TEST_CASE ("Match amount scales and clamps the target", "[match]")
{
    const auto f = match::logGrid (40);
    std::vector<double> ref (f.size(), 0.0), cur (f.size(), 0.0);
    for (size_t i = 0; i < f.size(); ++i)
        ref[i] = f[i] > 5000.0 ? 30.0 : 0.0; // huge treble difference
    const auto full = match::matchCurve (f, ref, cur, 1.0, 12.0);
    const auto half = match::matchCurve (f, ref, cur, 0.5, 12.0);
    for (size_t i = 0; i < f.size(); ++i)
    {
        CHECK (std::abs (full[i]) <= 12.0 + 1e-9);
        CHECK (half[i] == Catch::Approx (0.5 * full[i]).margin (1e-9));
    }
}

#include "PluginEditor.h"
#include "helpers/test_helpers.h"
#include "ui/CurveModel.h"
#include <PluginProcessor.h>

namespace
{
    // White noise for `current`, the same noise through a known EQ for `reference`.
    void feedKnownEq (solfa::ui::MatchPanel& panel)
    {
        constexpr double fs = 48000;
        juce::Random rng (21);
        EqEngine engine;
        engine.prepare (fs, 512, 1);
        engine.setBand (0, { true, FilterType::lowShelf, 150.0, 4.0, 0.707 });
        engine.setBand (1, { true, FilterType::bell, 3000.0, -5.0, 1.5 });
        juce::AudioBuffer<float> block (1, 512);
        std::vector<float> flat (512);
        for (int k = 0; k < (int) (fs * 10 / 512); ++k)
        {
            for (int i = 0; i < 512; ++i)
            {
                flat[(size_t) i] = 0.2f * (rng.nextFloat() * 2.0f - 1.0f);
                block.setSample (0, i, 0.2f * (rng.nextFloat() * 2.0f - 1.0f));
            }
            engine.process (block);
            panel.getCurrent().push (flat.data(), 512);
            panel.getReference().push (block.getReadPointer (0), 512);
        }
    }
}

TEST_CASE ("Apply match writes bands that reproduce the reference's tonal difference", "[match][ui]")
{
    runWithinPluginEditor ([] (PluginProcessor& plugin) {
        plugin.prepareToPlay (48000.0, 512);
        auto* editor = dynamic_cast<PluginEditor*> (plugin.getActiveEditor());
        REQUIRE (editor != nullptr);
        auto& panel = editor->getMatchPanel();
        feedKnownEq (panel);
        REQUIRE (panel.hasBothSpectra());

        const auto written = panel.applyMatch();
        CHECK (written > 0);
        CHECK (written <= 8);

        solfa::params::Handles handles (plugin.getState());
        solfa::ui::CurveModel model;
        model.update (handles, 48000.0, 528.0);
        const auto f = match::logGrid();
        std::vector<double> applied (f.size());
        for (size_t i = 0; i < f.size(); ++i)
            applied[i] = model.totalDb (f[i]);
        const std::vector<FittedBand> truth { { FilterType::lowShelf, 150.0, 4.0, 0.707 }, { FilterType::bell, 3000.0, -5.0, 1.5 } };
        const auto a = normalised (f, applied), e = normalised (f, match::responseDb (truth, f, 48000.0));
        double s = 0;
        int n = 0;
        for (size_t i = 0; i < f.size(); ++i)
            if (f[i] >= 60.0 && f[i] <= 15000.0)
            {
                s += (a[i] - e[i]) * (a[i] - e[i]);
                ++n;
            }
        INFO (written << " bands written, RMS error vs the known EQ " << std::sqrt (s / n) << " dB");
        CHECK (std::sqrt (s / n) < 1.5);

        // Applying again re-uses the same slots instead of piling up new bands.
        int enabledBefore = 0, enabledAfter = 0;
        for (int b = 0; b < solfa::dsp::numBands; ++b)
            enabledBefore += handles.bands[(size_t) b].on->load() > 0.5f ? 1 : 0;
        panel.applyMatch();
        for (int b = 0; b < solfa::dsp::numBands; ++b)
            enabledAfter += handles.bands[(size_t) b].on->load() > 0.5f ? 1 : 0;
        CHECK (enabledAfter == enabledBefore);
    });
}

TEST_CASE ("Match panel screenshot", "[.][screenshot]")
{
    runWithinPluginEditor ([] (PluginProcessor& plugin) {
        plugin.prepareToPlay (48000.0, 512);
        auto* editor = dynamic_cast<PluginEditor*> (plugin.getActiveEditor());
        REQUIRE (editor != nullptr);
        auto& panel = editor->getMatchPanel();
        feedKnownEq (panel);
        panel.setVisible (true);
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        juce::File file (juce::SystemStats::getEnvironmentVariable ("SOLFA_MATCH_SNAPSHOT", "/tmp/solfa-match.png"));
        file.deleteFile();
        juce::FileOutputStream stream (file);
        REQUIRE (juce::PNGImageFormat().writeImageToStream (image, stream));
    });
}
