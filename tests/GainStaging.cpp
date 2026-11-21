// Phase 3: loudness metering, static auto gain and level-matched compare.

#include "dsp/Loudness.h"
#include <PluginProcessor.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace solfa;

namespace
{
    constexpr double fs = 48000.0;
    constexpr int block = 512;

    void setParam (PluginProcessor& p, const juce::String& id, float value)
    {
        auto* param = p.getState().getParameter (id);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (value));
    }

    void setBand (PluginProcessor& p, int band, dsp::FilterType type, float freq, float gain, float q)
    {
        setParam (p, params::bandId (band, params::on), 1.0f);
        setParam (p, params::bandId (band, params::type), (float) type);
        setParam (p, params::bandId (band, params::freq), freq);
        setParam (p, params::bandId (band, params::gain), gain);
        setParam (p, params::bandId (band, params::q), q);
    }

    // Paul Kellett's pink noise filter (musicdsp.org), fed by a seeded white source.
    struct PinkNoise
    {
        juce::Random rng { 42 };
        double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;

        float next()
        {
            const auto white = rng.nextDouble() * 2.0 - 1.0;
            b0 = 0.99886 * b0 + white * 0.0555179;
            b1 = 0.99332 * b1 + white * 0.0750759;
            b2 = 0.96900 * b2 + white * 0.1538520;
            b3 = 0.86650 * b3 + white * 0.3104856;
            b4 = 0.55000 * b4 + white * 0.5329522;
            b5 = -0.7616 * b5 - white * 0.0168980;
            const auto pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362;
            b6 = white * 0.115926;
            return (float) (0.05 * pink);
        }
    };

    struct LoudnessResult
    {
        double inputDb, outputDb;
    };

    // Runs pink noise through the plugin for `seconds` and returns K-weighted loudness of the
    // input and output over the last part of the run (BS.1770 mean square, 10 log10).
    LoudnessResult runPink (PluginProcessor& plugin, double seconds, juce::AudioBuffer<float>* lastIn = nullptr,
        juce::AudioBuffer<float>* lastOut = nullptr)
    {
        dsp::LoudnessMeter in, out;
        in.prepare (fs, 2, 1.0);
        out.prepare (fs, 2, 1.0);
        PinkNoise noise;
        juce::AudioBuffer<float> buffer (2, block), input (2, block);
        juce::MidiBuffer midi;

        const int blocks = (int) (seconds * fs / block);
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < block; ++i)
            {
                const auto x = noise.next();
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, x);
            }
            input.makeCopyOf (buffer);
            plugin.processBlock (buffer, midi);
            in.process (input.getArrayOfReadPointers(), 2, block);
            out.process (buffer.getArrayOfReadPointers(), 2, block);
        }
        if (lastIn != nullptr)
            lastIn->makeCopyOf (input);
        if (lastOut != nullptr)
            lastOut->makeCopyOf (buffer);
        return { 10.0 * std::log10 (in.getMeanSquare()), 10.0 * std::log10 (out.getMeanSquare()) };
    }
}

TEST_CASE ("K-weighting reproduces the BS.1770 coefficients at 48 kHz", "[loudness]")
{
    const auto k = dsp::KWeighting::design (48000.0);
    CHECK (k.shelf.b0 == Catch::Approx (1.53512485958697).margin (1e-12));
    CHECK (k.shelf.b1 == Catch::Approx (-2.69169618940638).margin (1e-12));
    CHECK (k.shelf.b2 == Catch::Approx (1.19839281085285).margin (1e-12));
    CHECK (k.shelf.a1 == Catch::Approx (-1.69065929318241).margin (1e-12));
    CHECK (k.shelf.a2 == Catch::Approx (0.73248077421585).margin (1e-12));
    CHECK (k.highPass.a1 == Catch::Approx (-1.99004745483398).margin (1e-12));
    CHECK (k.highPass.a2 == Catch::Approx (0.99007225036621).margin (1e-12));
}

TEST_CASE ("Auto gain is exactly zero for a flat EQ", "[autogain]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (fs, block);
    runPink (plugin, 0.1);
    CHECK (plugin.getAutoGainDb() == 0.0);
}

TEST_CASE ("Auto gain keeps pink-noise loudness steady", "[autogain]")
{
    struct Case
    {
        dsp::FilterType type;
        float freq, gain, q;
    };

    for (const auto& c : { Case { dsp::FilterType::highShelf, 3000, 6, 0.707f },
                           Case { dsp::FilterType::lowShelf, 150, 6, 0.707f },
                           Case { dsp::FilterType::bell, 1000, -8, 0.7f },
                           Case { dsp::FilterType::tilt, 1000, 4, 0.5f } })
    {
        PluginProcessor plugin;
        plugin.prepareToPlay (fs, block);
        setBand (plugin, 2, c.type, c.freq, c.gain, c.q);

        const auto r = runPink (plugin, 4.0);
        INFO ("type " << (int) c.type << " auto gain " << plugin.getAutoGainDb() << " dB, loudness change "
                      << (r.outputDb - r.inputDb) << " dB");
        CHECK (std::abs (plugin.getAutoGainDb()) > 0.5); // it actually compensated
        CHECK (r.outputDb - r.inputDb == Catch::Approx (0.0).margin (0.5));
    }
}

TEST_CASE ("Without auto gain a boost makes it louder", "[autogain]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (fs, block);
    setParam (plugin, params::autoGain, 0.0f);
    setBand (plugin, 0, dsp::FilterType::highShelf, 3000, 6, 0.707f);

    const auto r = runPink (plugin, 3.0);
    CHECK (plugin.getAutoGainDb() == 0.0);
    CHECK (r.outputDb - r.inputDb > 2.0);
}

TEST_CASE ("Compare plays the dry signal at the processed loudness", "[compare]")
{
    // Reference: processed loudness with auto gain off (so there is a level difference to match).
    PluginProcessor processed;
    processed.prepareToPlay (fs, block);
    setParam (processed, params::autoGain, 0.0f);
    setBand (processed, 3, dsp::FilterType::bell, 1000, 9, 0.7f);
    const auto wet = runPink (processed, 5.0);

    PluginProcessor comparing;
    comparing.prepareToPlay (fs, block);
    setParam (comparing, params::autoGain, 0.0f);
    setBand (comparing, 3, dsp::FilterType::bell, 1000, 9, 0.7f);
    setParam (comparing, params::compare, 1.0f);
    juce::AudioBuffer<float> lastIn, lastOut;
    const auto cmp = runPink (comparing, 12.0, &lastIn, &lastOut);

    INFO ("processed " << (wet.outputDb - wet.inputDb) << " dB, compare " << (cmp.outputDb - cmp.inputDb) << " dB");
    CHECK (wet.outputDb - wet.inputDb > 2.0);
    CHECK (cmp.outputDb - cmp.inputDb == Catch::Approx (wet.outputDb - wet.inputDb).margin (0.5));

    // And it really is the dry signal: output / input is (nearly) constant across the block.
    double minRatio = 1e9, maxRatio = 0;
    for (int i = 0; i < block; ++i)
    {
        const auto x = lastIn.getSample (0, i);
        if (std::abs (x) < 1e-3f)
            continue;
        const auto r = lastOut.getSample (0, i) / x;
        minRatio = std::min (minRatio, (double) r);
        maxRatio = std::max (maxRatio, (double) r);
    }
    CHECK (maxRatio / minRatio < 1.01);
}
