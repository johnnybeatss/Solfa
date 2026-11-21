// Phase 6: dynamic EQ.

#include "dsp/EqEngine.h"
#include <PluginProcessor.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>
#include <vector>

using namespace solfa::dsp;

namespace
{
    constexpr double fs = 48000;
    constexpr int block = 480; // 10 ms

    BandTarget dynBell (double thresholdDb, double rangeDb, double attackMs = 10, double releaseMs = 150)
    {
        BandTarget t { true, FilterType::bell, 1000, 0.0, 1.0 };
        t.dynamic = true;
        t.dynThresholdDb = thresholdDb;
        t.dynRangeDb = rangeDb;
        t.dynAttackMs = attackMs;
        t.dynReleaseMs = releaseMs;
        return t;
    }

    // Amplitude of a sine whose RMS level is levelDb dBFS (a 0 dB-peak band-pass passes it fully).
    double amplitudeForLevel (double levelDb) { return std::sqrt (2.0) * std::pow (10.0, levelDb / 20.0); }

    // Runs a 1 kHz sine whose amplitude follows `amplitudeAt(blockIndex)` and returns the gain in
    // dB of each 10 ms block.
    template <typename Amp>
    std::vector<double> runTone (EqEngine& e, int blocks, Amp amplitudeAt, float rightSign = 1.0f)
    {
        std::vector<double> gains;
        juce::AudioBuffer<float> buffer (2, block);
        double phase = 0;
        const auto inc = 2.0 * std::numbers::pi * 1000.0 / fs;
        for (int k = 0; k < blocks; ++k)
        {
            const auto a = amplitudeAt (k);
            double inSq = 0, outSq = 0;
            for (int i = 0; i < block; ++i)
            {
                const auto x = (float) (a * std::sin (phase));
                phase += inc;
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, rightSign * x);
                inSq += (double) x * x;
            }
            e.process (buffer);
            for (int i = 0; i < block; ++i)
                outSq += (double) buffer.getSample (0, i) * buffer.getSample (0, i);
            gains.push_back (10.0 * std::log10 (outSq / inSq));
        }
        return gains;
    }

    double steadyGain (BandTarget t, double levelDb)
    {
        EqEngine e;
        e.prepare (fs, block, 2);
        e.setBand (0, t);
        const auto g = runTone (e, 100, [&] (int) { return amplitudeForLevel (levelDb); });
        return g.back();
    }
}

TEST_CASE ("Dynamic band reaches its full range when loud and stays flat when quiet", "[dynamic]")
{
    CHECK (steadyGain (dynBell (-30, -6), -10) == Catch::Approx (-6.0).margin (0.2)); // 20 dB over
    CHECK (steadyGain (dynBell (-30, -6), -50) == Catch::Approx (0.0).margin (0.05)); // below threshold
    CHECK (steadyGain (dynBell (-30, 4), -10) == Catch::Approx (4.0).margin (0.2));   // lift when loud
}

TEST_CASE ("Dynamic gain scales across the 12 dB knee", "[dynamic]")
{
    // 6 dB over threshold = half way: -3 dB of a -6 dB range.
    CHECK (steadyGain (dynBell (-30, -6), -24) == Catch::Approx (-3.0).margin (0.4));
}

TEST_CASE ("Dynamic band adds to the static gain", "[dynamic]")
{
    auto t = dynBell (-30, -6);
    t.gainDb = 4.0; // +4 static, -6 when loud: -2 dB overall
    CHECK (steadyGain (t, -10) == Catch::Approx (-2.0).margin (0.2));
    CHECK (steadyGain (t, -50) == Catch::Approx (4.0).margin (0.1));
}

TEST_CASE ("Attack and release shape the gain over time", "[dynamic]")
{
    EqEngine e;
    e.prepare (fs, block, 2);
    e.setBand (0, dynBell (-30, -6, 50, 300));
    const auto loud = amplitudeForLevel (-10), quiet = amplitudeForLevel (-40);
    // 1 s loud, then 3 s quiet.
    const auto g = runTone (e, 400, [&] (int k) { return k < 100 ? loud : quiet; });

    INFO ("first 10 ms " << g[0] << " dB, after 0.5 s " << g[50] << " dB, 30 ms after drop " << g[103]
                         << " dB, 3 s after drop " << g.back() << " dB");
    CHECK (g[0] > -3.0);                                         // attack takes time
    CHECK (g[50] == Catch::Approx (-6.0).margin (0.3));          // fully engaged
    CHECK (g[103] < -2.0);                                       // release takes time
    CHECK (g.back() == Catch::Approx (0.0).margin (0.1));        // fully released
}

TEST_CASE ("Dynamic with zero range matches a static band exactly", "[dynamic][null]")
{
    EqEngine a, b;
    a.prepare (fs, block, 2);
    b.prepare (fs, block, 2);
    auto dyn = dynBell (-40, 0);
    dyn.gainDb = 3.0;
    auto stat = dyn;
    stat.dynamic = false;
    a.setBand (0, dyn);
    b.setBand (0, stat);

    juce::AudioBuffer<float> x (2, block), y (2, block);
    for (int k = 0; k < 20; ++k)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < block; ++i)
                x.setSample (ch, i, 0.5f * std::sin (0.13f * (float) (k * block + i)));
        y.makeCopyOf (x);
        a.process (x);
        b.process (y);
        for (int i = 0; i < block; ++i)
            REQUIRE (juce::exactlyEqual (x.getSample (0, i), y.getSample (0, i)));
    }
}

TEST_CASE ("A Mid dynamic band ignores loud side-only content", "[dynamic][placement]")
{
    EqEngine e;
    e.prepare (fs, block, 2);
    auto t = dynBell (-30, -6);
    t.placement = StereoPlacement::mid;
    e.setBand (0, t);
    const auto g = runTone (e, 100, [&] (int) { return amplitudeForLevel (-10); }, -1.0f); // L = -R
    CHECK (g.back() == Catch::Approx (0.0).margin (0.05));
}

TEST_CASE ("Auto gain follows the static curve, not the dynamics", "[dynamic][autogain]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (fs, block);
    auto set = [&] (const juce::String& id, float v) {
        auto* p = plugin.getState().getParameter (id);
        p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    using namespace solfa::params;
    set (bandId (3, on), 1.0f);
    set (bandId (3, freq), 1000.0f);
    set (bandId (3, gain), 3.0f);
    set (bandId (3, dyn), 1.0f);
    set (bandId (3, thr), -30.0f);
    set (bandId (3, range), -9.0f);

    auto runAt = [&] (double levelDb) {
        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;
        double phase = 0;
        for (int k = 0; k < 60; ++k)
        {
            for (int i = 0; i < block; ++i)
            {
                const auto x = (float) (amplitudeForLevel (levelDb) * std::sin (phase));
                phase += 2.0 * std::numbers::pi * 1000.0 / fs;
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, x);
            }
            plugin.processBlock (buffer, midi);
        }
        return plugin.getAutoGainDb();
    };
    const auto quiet = runAt (-50);
    const auto loud = runAt (-10);
    CHECK (quiet < 0.0); // compensating the +3 dB static boost
    CHECK (loud == Catch::Approx (quiet).margin (1e-9));
}
