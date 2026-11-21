// Phase 4: 24 bands, notch / band-pass, 6-96 dB/oct cuts, stereo placement.
// Expected values come from analog Butterworth / prototype formulas, not the implementation.

#include "dsp/EqEngine.h"
#include "dsp/FilterDesign.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>

using namespace solfa::dsp;

namespace
{
    double cascadeDb (const CascadeCoefficients& c, double f, double fs)
    {
        return 10.0 * std::log10 (cascadePowerGain (c, f, fs));
    }

    // Ideal analog Butterworth magnitude in dB.
    double butterworthDb (bool lowPass, int order, double f, double fc)
    {
        const auto r = std::pow (f / fc, 2.0 * order);
        return lowPass ? -10.0 * std::log10 (1.0 + r) : 10.0 * std::log10 (r / (1.0 + r));
    }

    juce::AudioBuffer<float> makeStereo (int n, float leftScale, float rightScale)
    {
        juce::AudioBuffer<float> b (2, n);
        for (int i = 0; i < n; ++i)
        {
            const auto x = 0.3f * std::sin (0.031f * (float) i) + 0.1f * std::sin (0.37f * (float) i);
            b.setSample (0, i, leftScale * x);
            b.setSample (1, i, rightScale * x);
        }
        return b;
    }

    BandTarget band (FilterType type, double f, double g, double q, StereoPlacement place = StereoPlacement::stereo)
    {
        BandTarget t { true, type, f, g, q };
        t.placement = place;
        return t;
    }

    void run (EqEngine& e, juce::AudioBuffer<float>& b, int blocks = 40)
    {
        for (int k = 0; k < blocks; ++k)
            e.process (b);
    }
}

TEST_CASE ("Butterworth section Qs match the textbook values", "[cascade]")
{
    CHECK (butterworthSectionQ (2, 0) == Catch::Approx (0.70711).margin (1e-4));
    CHECK (butterworthSectionQ (3, 0) == Catch::Approx (1.0).margin (1e-4));
    CHECK (butterworthSectionQ (4, 0) == Catch::Approx (0.54120).margin (1e-4));
    CHECK (butterworthSectionQ (4, 1) == Catch::Approx (1.30656).margin (1e-4));
    CHECK (butterworthSectionQ (5, 0) == Catch::Approx (0.61803).margin (1e-4));
    CHECK (butterworthSectionQ (5, 1) == Catch::Approx (1.61803).margin (1e-4));
}

TEST_CASE ("Cut filters follow the analog Butterworth response at every slope", "[cascade]")
{
    for (double fs : { 48000.0, 96000.0 })
        for (auto slope : slopeOptionsDbPerOct)
            for (bool lowPass : { true, false })
            {
                const auto order = slope / 6;
                const BandSettings s { lowPass ? FilterType::highCut : FilterType::lowCut, 1000.0, 0.0,
                    1.0 / std::numbers::sqrt2 };
                const auto c = designCascade (s, slope, fs);

                // Tight where the filter passes sound, looser deep in the stop band (near Nyquist a
                // digital filter cannot follow the analog curve exactly).
                double worstPass = 0.0, worstStop = 0.0;
                for (int i = 0; i < 200; ++i)
                {
                    const auto f = 20.0 * std::pow (1000.0, i / 199.0); // 20 Hz - 20 kHz
                    const auto ref = butterworthDb (lowPass, order, f, 1000.0);
                    if (ref < -60.0)
                        continue;
                    const auto err = std::abs (cascadeDb (c, f, fs) - ref);
                    (ref > -12.0 ? worstPass : worstStop) = std::max (ref > -12.0 ? worstPass : worstStop, err);
                }
                INFO ("fs " << fs << " slope " << slope << (lowPass ? " high-cut" : " low-cut") << " pass-band "
                            << worstPass << " dB, stop-band " << worstStop << " dB");
                CHECK (worstPass < 0.5);
                CHECK (worstStop < 1.5);
                CHECK (c.numSections == (order + 1) / 2);
            }
}

TEST_CASE ("Cut Q adds a corner resonance only when raised", "[cascade]")
{
    const double fs = 48000;
    auto peakDb = [&] (double q) {
        const auto c = designCascade ({ FilterType::highCut, 2000, 0, q }, 24, fs);
        double peak = -100;
        for (int i = 0; i < 400; ++i)
            peak = std::max (peak, cascadeDb (c, 100.0 * std::pow (100.0, i / 399.0), fs));
        return peak;
    };
    CHECK (peakDb (1.0 / std::numbers::sqrt2) < 0.05);
    CHECK (peakDb (2.0) > 3.0);
}

TEST_CASE ("Notch is deep at its frequency and transparent away from it", "[shapes]")
{
    const double fs = 48000;
    const auto c = designCascade ({ FilterType::notch, 1000, 0, 4 }, 12, fs);
    CHECK (cascadeDb (c, 1000, fs) < -60.0);
    CHECK (cascadeDb (c, 4000, fs) == Catch::Approx (0.0).margin (0.05));
    CHECK (cascadeDb (c, 250, fs) == Catch::Approx (0.0).margin (0.05));
}

TEST_CASE ("Band-pass peaks at 0 dB and rejects far frequencies", "[shapes]")
{
    for (double fs : { 44100.0, 96000.0 })
    {
        const BandSettings s { FilterType::bandPass, 2000, 0, 2 };
        const auto c = designCascade (s, 12, fs);
        CHECK (cascadeDb (c, 2000, fs) == Catch::Approx (0.0).margin (0.1));
        for (double f : { 1000.0, 1500.0, 2700.0, 4000.0 })
            CHECK (cascadeDb (c, f, fs) == Catch::Approx (analogMagnitudeDb (s, f)).margin (0.3));
        for (double f : { 200.0, 500.0, 8000.0 }) // already 10-20 dB down
            CHECK (cascadeDb (c, f, fs) == Catch::Approx (analogMagnitudeDb (s, f)).margin (0.6));
    }
}

TEST_CASE ("Every designed cascade is stable", "[cascade]")
{
    juce::Random rng (99);
    for (int i = 0; i < 20000; ++i)
    {
        const double fs = rng.nextBool() ? 44100.0 : 96000.0;
        BandSettings s;
        s.type = (FilterType) rng.nextInt (numFilterTypes);
        s.frequencyHz = 20.0 * std::pow (1000.0, rng.nextDouble());
        s.gainDb = -18.0 + 36.0 * rng.nextDouble();
        s.q = 0.1 * std::pow (180.0, rng.nextDouble());
        const auto slope = slopeOptionsDbPerOct[(size_t) rng.nextInt ((int) slopeOptionsDbPerOct.size())];
        const auto c = designCascade (s, slope, fs);
        for (int k = 0; k < c.numSections; ++k)
        {
            const auto& sec = c.sections[(size_t) k];
            INFO ("type " << (int) s.type << " f " << s.frequencyHz << " q " << s.q << " slope " << slope);
            REQUIRE (std::isfinite (sec.b0));
            REQUIRE (std::abs (sec.a2) < 1.0);
            REQUIRE (std::abs (sec.a1) < 1.0 + sec.a2);
        }
    }
}

TEST_CASE ("A Left band leaves the right channel bit-for-bit untouched", "[placement]")
{
    EqEngine e;
    e.prepare (48000, 512, 2);
    e.setBand (0, band (FilterType::bell, 1000, 9, 1, StereoPlacement::left));

    auto buffer = makeStereo (512, 1.0f, 1.0f);
    const auto input = buffer;
    e.process (buffer);

    bool leftChanged = false;
    for (int i = 0; i < 512; ++i)
    {
        REQUIRE (juce::exactlyEqual (buffer.getSample (1, i), input.getSample (1, i)));
        leftChanged = leftChanged || ! juce::exactlyEqual (buffer.getSample (0, i), input.getSample (0, i));
    }
    CHECK (leftChanged);
}

TEST_CASE ("Mid bands ignore pure side content and Side bands ignore mono content", "[placement]")
{
    SECTION ("Mid band, side-only signal (L = -R)")
    {
        EqEngine e;
        e.prepare (48000, 512, 2);
        e.setBand (3, band (FilterType::bell, 800, 12, 1, StereoPlacement::mid));
        auto buffer = makeStereo (512, 1.0f, -1.0f);
        const auto input = buffer;
        run (e, buffer, 1);
        for (int i = 0; i < 512; ++i)
            REQUIRE (buffer.getSample (0, i) == Catch::Approx (input.getSample (0, i)).margin (1e-7));
    }

    SECTION ("Side band, mono signal (L = R)")
    {
        EqEngine e;
        e.prepare (48000, 512, 2);
        e.setBand (3, band (FilterType::bell, 800, 12, 1, StereoPlacement::side));
        auto buffer = makeStereo (512, 1.0f, 1.0f);
        const auto input = buffer;
        run (e, buffer, 1);
        for (int i = 0; i < 512; ++i)
            REQUIRE (buffer.getSample (1, i) == Catch::Approx (input.getSample (1, i)).margin (1e-7));
    }
}

TEST_CASE ("All 24 bands work in series", "[engine]")
{
    // 24 bells of +0.5 dB at 1 kHz should add up to +12 dB at 1 kHz.
    constexpr double fs = 48000;
    EqEngine e;
    e.prepare (fs, 480, 2);
    for (int b = 0; b < numBands; ++b)
        e.setBand (b, band (FilterType::bell, 1000, 0.5, 1));

    juce::AudioBuffer<float> buffer (2, 480);
    double phase = 0, inSq = 0, outSq = 0;
    const auto inc = 2.0 * std::numbers::pi * 1000.0 / fs;
    for (int k = 0; k < 60; ++k)
    {
        for (int i = 0; i < 480; ++i)
        {
            const auto x = (float) (0.05 * std::sin (phase));
            phase += inc;
            buffer.setSample (0, i, x);
            buffer.setSample (1, i, x);
            if (k >= 40)
                inSq += (double) x * x;
        }
        e.process (buffer);
        if (k >= 40)
            for (int i = 0; i < 480; ++i)
                outSq += (double) buffer.getSample (0, i) * buffer.getSample (0, i);
    }
    CHECK (10.0 * std::log10 (outSq / inSq) == Catch::Approx (12.0).margin (0.1));
}
