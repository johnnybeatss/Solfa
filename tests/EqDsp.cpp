// Tests for the EQ maths and engine. Expected values come from the analog prototypes
// (computed independently in each test), not from the implementation under test.

#include "dsp/EqEngine.h"
#include "dsp/FilterDesign.h"
#include <PluginProcessor.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>
#include <vector>

using namespace solfa::dsp;

namespace
{
    constexpr double sampleRates[] = { 44100.0, 48000.0, 96000.0, 192000.0 };

    // Worst |digital - analog| in dB over a dense log grid up to 20 kHz (or just under Nyquist).
    double worstError (const BandSettings& requested, double fs)
    {
        const auto s = sanitise (requested, fs);
        const auto c = design (s, fs);
        const auto top = std::min (20000.0, 0.499 * fs);
        double worst = 0.0;
        for (int i = 0; i < 400; ++i)
        {
            const auto f = 10.0 * std::pow (top / 10.0, i / 399.0);
            const auto ref = analogMagnitudeDb (s, f);
            if (ref < -40.0)
                continue;
            worst = std::max (worst, std::abs (magnitudeDb (c, f, fs) - ref));
        }
        return worst;
    }

    double worstErrorOver (FilterType type, std::initializer_list<double> freqs, std::initializer_list<double> gains,
        std::initializer_list<double> qs, double maxFreqRatio)
    {
        double worst = 0.0;
        for (auto fs : sampleRates)
            for (auto f : freqs)
                for (auto g : gains)
                    for (auto q : qs)
                        if (f <= maxFreqRatio * fs)
                            worst = std::max (worst, worstError ({ type, f, g, q }, fs));
        return worst;
    }

    // Steady-state gain in dB of the engine for a sine at frequencyHz.
    double measureGainDb (EqEngine& engine, double fs, double frequencyHz)
    {
        constexpr int block = 480;
        juce::AudioBuffer<float> buffer (2, block);
        double phase = 0.0;
        const auto inc = 2.0 * std::numbers::pi * frequencyHz / fs;
        double inSq = 0.0, outSq = 0.0;
        const int totalBlocks = (int) (0.6 * fs / block); // 0.6 s: smoothing + filter settling
        for (int k = 0; k < totalBlocks; ++k)
        {
            std::vector<float> in ((size_t) block);
            for (int i = 0; i < block; ++i)
            {
                in[(size_t) i] = (float) (0.25 * std::sin (phase));
                phase += inc;
                buffer.setSample (0, i, in[(size_t) i]);
                buffer.setSample (1, i, in[(size_t) i]);
            }
            engine.process (buffer);
            if (k >= totalBlocks - 20)
                for (int i = 0; i < block; ++i)
                {
                    inSq += in[(size_t) i] * in[(size_t) i];
                    outSq += buffer.getSample (0, i) * buffer.getSample (0, i);
                }
        }
        return 10.0 * std::log10 (outSq / inSq);
    }

    BandTarget bell (double f, double g, double q) { return { true, FilterType::bell, f, g, q }; }
}

TEST_CASE ("Analog prototypes hit their nominal gains", "[design]")
{
    CHECK (analogMagnitudeDb ({ FilterType::bell, 1000, 6, 1 }, 1000) == Catch::Approx (6.0).margin (1e-9));
    CHECK (analogMagnitudeDb ({ FilterType::bell, 1000, -9, 2 }, 1000) == Catch::Approx (-9.0).margin (1e-9));
    CHECK (analogMagnitudeDb ({ FilterType::lowShelf, 200, 6, 0.707 }, 1) == Catch::Approx (6.0).margin (1e-3));
    CHECK (analogMagnitudeDb ({ FilterType::lowShelf, 200, 6, 0.707 }, 200) == Catch::Approx (3.0).margin (1e-9));
    CHECK (analogMagnitudeDb ({ FilterType::highShelf, 5000, -4, 0.707 }, 1.0e6) == Catch::Approx (-4.0).margin (1e-3));
    CHECK (analogMagnitudeDb ({ FilterType::tilt, 1000, 6, 0.5 }, 1) == Catch::Approx (-3.0).margin (1e-3));
    CHECK (analogMagnitudeDb ({ FilterType::tilt, 1000, 6, 0.5 }, 1.0e6) == Catch::Approx (3.0).margin (1e-3));
    CHECK (analogMagnitudeDb ({ FilterType::lowCut, 100, 0, 0.7071067811865476 }, 100) == Catch::Approx (-3.0103).margin (1e-3));
}

TEST_CASE ("Matched design tracks the analog response at 44.1-192 kHz", "[design]")
{
    // Tolerances are the measured worst cases (Phase 2 prototype + this implementation),
    // rounded up. They guard against regressions; tighten them if the design improves.
    const auto bellErr = worstErrorOver (FilterType::bell, { 30, 100, 400, 1000, 3000, 8000, 12000, 16000 },
        { -18, -6, 3, 12, 18 }, { 0.3, 0.707, 2, 6 }, 0.37);
    const auto lowShelfErr = worstErrorOver (FilterType::lowShelf, { 30, 100, 400, 1000, 3000, 8000 },
        { -12, -3, 3, 12 }, { 0.5, 0.707, 1.4 }, 0.37);
    const auto highShelfErr = worstErrorOver (FilterType::highShelf, { 1000, 3000, 6000, 10000 },
        { -12, -3, 3, 12 }, { 0.5, 0.707, 1.0 }, 0.23);
    const auto tiltErr = worstErrorOver (FilterType::tilt, { 300, 1000, 3000 }, { -6, 6 }, { 0.5 }, 0.37);
    const auto highCutErr = worstErrorOver (FilterType::highCut, { 1000, 5000, 10000, 16000 },
        { 0 }, { 0.5, 0.707, 1.4 }, 0.37);
    const auto lowCutErr = worstErrorOver (FilterType::lowCut, { 20, 40, 100, 300, 1000 },
        { 0 }, { 0.5, 0.707, 1.4 }, 0.37);

    INFO ("bell " << bellErr << " lowShelf " << lowShelfErr << " highShelf " << highShelfErr
                  << " tilt " << tiltErr << " highCut " << highCutErr << " lowCut " << lowCutErr);
    CHECK (bellErr < 1.5); // worst: 16 kHz bell at 44.1 kHz (RBJ bilinear: 12.4 dB there)
    CHECK (lowShelfErr < 0.5);
    CHECK (highShelfErr < 1.0);
    CHECK (tiltErr < 0.5);
    CHECK (highCutErr < 1.5);
    CHECK (lowCutErr < 0.5);
}

TEST_CASE ("Matched design avoids bilinear cramping near Nyquist", "[design]")
{
    // The research measured RBJ coming out 4.2 dB short at 20 kHz for this band at 44.1 kHz.
    const BandSettings s { FilterType::bell, 16000, 6, 1 };
    const double fs = 44100;
    const auto ref = analogMagnitudeDb (s, 20000);

    const auto bilinearErr = std::abs (magnitudeDb (designBilinear (s, fs), 20000, fs) - ref);
    const auto matchedErr = std::abs (magnitudeDb (design (s, fs), 20000, fs) - ref);

    INFO ("bilinear " << bilinearErr << " dB, matched " << matchedErr << " dB");
    CHECK (bilinearErr > 3.0);
    CHECK (matchedErr < 0.75);
}

TEST_CASE ("A boost followed by the same cut is an identity", "[design]")
{
    for (auto type : { FilterType::bell, FilterType::lowShelf, FilterType::highShelf, FilterType::tilt })
        for (auto fs : sampleRates)
            for (double f : { 60.0, 1000.0, 9000.0 })
            {
                const auto up = design ({ type, f, 7.5, 1.2 }, fs);
                const auto down = design ({ type, f, -7.5, 1.2 }, fs);
                for (double probe : { 20.0, 200.0, 2000.0, 15000.0 })
                    REQUIRE (magnitudeDb (up, probe, fs) + magnitudeDb (down, probe, fs) == Catch::Approx (0.0).margin (1e-7)); // rounding only
            }
}

TEST_CASE ("Low frequencies stay precise at 192 kHz", "[design]")
{
    // Float32 coefficients were measured off by 2.27 dB here; double precision must not be.
    const BandSettings s { FilterType::bell, 20, 6, 4 };
    const auto c = design (s, 192000);
    for (double f : { 15.0, 19.0, 20.0, 21.0, 30.0 })
        CHECK (magnitudeDb (c, f, 192000) == Catch::Approx (analogMagnitudeDb (s, f)).margin (0.01));
}

TEST_CASE ("Designed filters are always stable", "[design]")
{
    juce::Random rng (1234);
    for (int i = 0; i < 20000; ++i)
    {
        const auto fs = sampleRates[rng.nextInt (4)];
        BandSettings s;
        s.type = (FilterType) rng.nextInt (numFilterTypes);
        s.frequencyHz = 20.0 * std::pow (1000.0, rng.nextDouble());
        s.gainDb = -18.0 + 36.0 * rng.nextDouble();
        s.q = 0.1 * std::pow (180.0, rng.nextDouble());
        const auto c = design (s, fs);
        INFO ("type " << (int) s.type << " f " << s.frequencyHz << " g " << s.gainDb << " q " << s.q << " fs " << fs);
        REQUIRE (std::isfinite (c.b0));
        // Jury criterion for a second-order denominator 1 + a1 z^-1 + a2 z^-2
        REQUIRE (std::abs (c.a2) < 1.0);
        REQUIRE (std::abs (c.a1) < 1.0 + c.a2);
    }
}

TEST_CASE ("A bell applies its gain at the centre frequency", "[engine]")
{
    for (auto fs : { 44100.0, 96000.0 })
    {
        EqEngine engine;
        engine.prepare (fs, 480, 2);
        engine.setBand (3, bell (1000, 6, 1));
        CHECK (measureGainDb (engine, fs, 1000) == Catch::Approx (6.0).margin (0.05));
    }
}

TEST_CASE ("Amount scales band gains in dB", "[engine]")
{
    EqEngine engine;
    engine.prepare (48000, 480, 2);
    engine.setBand (2, bell (500, 6, 1));
    engine.setAmount (0.5);
    CHECK (measureGainDb (engine, 48000, 500) == Catch::Approx (3.0).margin (0.05));
}

TEST_CASE ("Low cut removes content below its corner", "[engine]")
{
    EqEngine engine;
    engine.prepare (48000, 480, 2);
    engine.setBand (0, { true, FilterType::lowCut, 100, 0, 0.707 });
    // 12 dB/oct, ~1.74 octaves below the corner: expect roughly -21 dB.
    CHECK (measureGainDb (engine, 48000, 30) < -18.0);
    CHECK (measureGainDb (engine, 48000, 2000) == Catch::Approx (0.0).margin (0.05));
}

TEST_CASE ("Disabled bands leave audio bit-for-bit untouched", "[engine][null]")
{
    EqEngine engine;
    engine.prepare (48000, 512, 2);
    engine.setBand (1, bell (300, 0, 1)); // enabled but 0 dB: also an identity

    juce::AudioBuffer<float> buffer (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, 0.3f * std::sin (0.01f * (float) (i * (ch + 1))));
    const auto input = buffer;

    engine.process (buffer);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE (juce::exactlyEqual (buffer.getSample (ch, i), input.getSample (ch, i)));
}

TEST_CASE ("Switching a band off returns to bit-exact transparency", "[engine][null]")
{
    EqEngine engine;
    engine.prepare (48000, 480, 2);
    engine.setBand (4, bell (2000, 9, 2));
    measureGainDb (engine, 48000, 2000);

    engine.setBand (4, { false, FilterType::bell, 2000, 9, 2 });
    measureGainDb (engine, 48000, 2000); // 0.6 s: ramp down and let the filter tail decay

    juce::AudioBuffer<float> buffer (2, 480);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 480; ++i)
            buffer.setSample (ch, i, 0.2f * std::sin (0.07f * (float) i));
    const auto input = buffer;
    engine.process (buffer);

    CHECK (engine.isBypassed());
    for (int i = 0; i < 480; ++i)
        REQUIRE (juce::exactlyEqual (buffer.getSample (0, i), input.getSample (0, i)));
}

// MSVC 14.52 crashes with "fatal error C1001: Internal compiler error" when optimising this test
// (seen on the windows-latest CI image). Optimisation is irrelevant for a test, so turn it off here.
#if defined(_MSC_VER) && ! defined(__clang__)
    #pragma optimize ("", off)
#endif
TEST_CASE ("Fast automation does not click", "[engine][automation]")
{
    // A 1 kHz sine through a +12 dB bell whose frequency jumps between 300 Hz and 3 kHz every
    // 50 ms. With smoothing, the output's second difference must stay within what a smooth
    // signal of that amplitude can produce; a click shows up as a spike far above it.
    constexpr double fs = 48000;
    constexpr int block = 64;
    EqEngine engine;
    engine.prepare (fs, block, 2);

    const auto w = 2.0 * std::numbers::pi * 1000.0 / fs;
    double phase = 0.0, maxSecondDiff = 0.0, peak = 0.0;
    float y1 = 0.0f, y2 = 0.0f;
    juce::AudioBuffer<float> buffer (2, block);

    for (int k = 0; k < (int) (2.0 * fs / block); ++k)
    {
        const bool high = ((k * block) / (int) (0.05 * fs)) % 2 == 1;
        engine.setBand (0, bell (high ? 3000 : 300, 12, 2));

        for (int i = 0; i < block; ++i)
        {
            const auto x = (float) (0.25 * std::sin (phase));
            phase += w;
            buffer.setSample (0, i, x);
            buffer.setSample (1, i, x);
        }
        engine.process (buffer);

        for (int i = 0; i < block; ++i)
        {
            const auto y = buffer.getSample (0, i);
            if (k > 10)
            {
                maxSecondDiff = std::max (maxSecondDiff, (double) std::abs (y - 2.0f * y1 + y2));
                peak = std::max (peak, (double) std::abs (y));
            }
            y2 = y1;
            y1 = y;
        }
    }

    // For a sinusoid of amplitude A, |second difference| <= A * w^2 (about 4 * A * sin^2(w/2)).
    const auto smoothLimit = peak * w * w;
    INFO ("max second difference " << maxSecondDiff << ", smooth-signal limit " << smoothLimit);
    CHECK (maxSecondDiff < 2.0 * smoothLimit);
}

#if defined(_MSC_VER) && ! defined(__clang__)
    #pragma optimize ("", on)
#endif

TEST_CASE ("Gains within 0.001 dB of zero count as exactly zero", "[engine][null]")
{
    // Guards the Apple Silicon case where parameter snapping yields about -5e-7 dB for "0 dB".
    EqEngine engine;
    engine.prepare (48000, 512, 2);
    engine.setBand (2, bell (700, -5.364418e-7, 1));

    juce::AudioBuffer<float> buffer (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, 0.4f * std::sin (0.02f * (float) i));
    const auto input = buffer;
    engine.process (buffer);

    CHECK (engine.isBypassed());
    for (int i = 0; i < 512; ++i)
        REQUIRE (juce::exactlyEqual (buffer.getSample (0, i), input.getSample (0, i)));
}
