// Phase 8: linear-phase mode.

#include "dsp/FilterDesign.h"
#include "dsp/LinearPhase.h"
#include <PluginProcessor.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <numbers>
#include <thread>

using namespace solfa;
using LP = dsp::LinearPhaseEq;

namespace
{
    double firMagnitudeDb (const std::vector<float>& ir, double f, double fs)
    {
        std::complex<double> sum = 0.0;
        for (size_t n = 0; n < ir.size(); ++n)
            sum += (double) ir[n] * std::polar (1.0, -2.0 * std::numbers::pi * f * (double) n / fs);
        return 20.0 * std::log10 (std::abs (sum));
    }

    void setParam (PluginProcessor& p, const juce::String& id, float v)
    {
        auto* param = p.getState().getParameter (id);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (v));
    }

    // Process silence until the linear-phase FIR is live (the convolution loads it asynchronously).
    bool waitForFilter (PluginProcessor& plugin, int block)
    {
        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;
        // One block sets the mode, then design once. (Redesigning on every attempt would keep
        // replacing the FIR before the convolution finished loading it.)
        buffer.clear();
        plugin.processBlock (buffer, midi);
        plugin.getLinearPhase().designNow();
        for (int attempt = 0; attempt < 300; ++attempt)
        {
            buffer.clear();
            plugin.processBlock (buffer, midi);
            if (plugin.getLinearPhase().isFilterActive())
                return true;
            std::this_thread::sleep_for (std::chrono::milliseconds (10));
        }
        return false;
    }

    double sineGainDb (PluginProcessor& plugin, double f, int block)
    {
        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;
        double phase = 0, inSq = 0, outSq = 0;
        for (int k = 0; k < 120; ++k)
        {
            for (int i = 0; i < block; ++i)
            {
                const auto x = (float) (0.1 * std::sin (phase));
                phase += 2.0 * std::numbers::pi * f / 48000.0;
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

TEST_CASE ("A flat curve gives a single centred impulse", "[linearphase]")
{
    const int order = 12, n = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<double> power ((size_t) n / 2 + 1, 1.0);
    std::vector<float> ir;
    LP::designFir (power, order, fft, ir);
    REQUIRE ((int) ir.size() == n);
    CHECK (ir[(size_t) n / 2] == Catch::Approx (1.0).margin (1e-5)); // also confirms the inverse FFT scaling
    for (int i = 0; i < n; i += 37)
        if (i != n / 2)
            REQUIRE (std::abs (ir[(size_t) i]) < 1e-5f);
}

TEST_CASE ("The FIR follows the EQ magnitude and is symmetric", "[linearphase]")
{
    const double fs = 48000;
    const int order = 14, n = 1 << order;
    const dsp::BandSettings bell { dsp::FilterType::bell, 1000.0, 6.0, 1.0 };
    const auto c = dsp::design (bell, fs);
    std::vector<double> power ((size_t) n / 2 + 1);
    for (size_t k = 0; k < power.size(); ++k)
        power[k] = dsp::powerGain (c, std::max ((double) k * fs / n, 1.0), fs);

    juce::dsp::FFT fft (order);
    std::vector<float> ir;
    LP::designFir (power, order, fft, ir);

    for (double f : { 100.0, 500.0, 1000.0, 2000.0, 8000.0 })
        CHECK (firMagnitudeDb (ir, f, fs) == Catch::Approx (dsp::magnitudeDb (c, f, fs)).margin (0.1));
    for (int k = 1; k < n / 2; k += 13)
        REQUIRE (ir[(size_t) (n / 2 + k)] == Catch::Approx (ir[(size_t) (n / 2 - k)]).margin (1e-6));
}

TEST_CASE ("Latency is half the FIR and scales with sample rate", "[linearphase]")
{
    CHECK (LP::latencyFor (LP::off, 48000) == 0);
    CHECK (LP::latencyFor (LP::standard, 48000) == 2048);
    CHECK (LP::latencyFor (LP::hd, 48000) == 8192);
    CHECK (LP::latencyFor (LP::standard, 96000) == 4096);
    CHECK (LP::latencyFor (LP::standard, 44100) == 2048);

    PluginProcessor plugin;
    setParam (plugin, params::phaseMode, 1.0f);
    plugin.prepareToPlay (48000, 512);
    CHECK (plugin.getLatencySamples() == 2048);
}

TEST_CASE ("Flat linear phase is a pure delay of the reported latency", "[linearphase]")
{
    constexpr int block = 512;
    PluginProcessor plugin;
    setParam (plugin, params::phaseMode, 1.0f);
    plugin.prepareToPlay (48000, block);
    REQUIRE (waitForFilter (plugin, block));

    const auto latency = plugin.getLatencySamples();
    std::vector<float> in, out;
    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    juce::Random rng (3);
    for (int k = 0; k < 20; ++k)
    {
        for (int i = 0; i < block; ++i)
        {
            const auto x = rng.nextFloat() - 0.5f;
            buffer.setSample (0, i, x);
            buffer.setSample (1, i, x);
            in.push_back (x);
        }
        plugin.processBlock (buffer, midi);
        for (int i = 0; i < block; ++i)
            out.push_back (buffer.getSample (0, i));
    }
    double worst = 0;
    for (size_t i = (size_t) latency; i < out.size(); ++i)
        worst = std::max (worst, (double) std::abs (out[i] - in[i - (size_t) latency]));
    INFO ("worst error " << worst);
    CHECK (worst < 1e-4); // about -80 dB: FFT convolution rounding, not bit-exact
}

TEST_CASE ("Linear phase and zero latency apply the same gain", "[linearphase]")
{
    constexpr int block = 480;
    auto configure = [] (PluginProcessor& p, float mode) {
        setParam (p, params::phaseMode, mode);
        setParam (p, params::autoGain, 0.0f);
        setParam (p, params::bandId (0, params::on), 1.0f);
        setParam (p, params::bandId (0, params::type), (float) dsp::FilterType::bell); // band 1 defaults to a low shelf
        setParam (p, params::bandId (0, params::freq), 1000.0f);
        setParam (p, params::bandId (0, params::gain), 6.0f);
        setParam (p, params::bandId (0, params::q), 1.0f);
        p.prepareToPlay (48000, block);
    };
    PluginProcessor zero, linear;
    configure (zero, 0.0f);
    configure (linear, 1.0f);
    REQUIRE (waitForFilter (linear, block));

    for (double f : { 1000.0, 3000.0, 200.0 })
    {
        const auto a = sineGainDb (zero, f, block), b = sineGainDb (linear, f, block);
        INFO (f << " Hz: zero latency " << a << " dB, linear phase " << b << " dB");
        CHECK (b == Catch::Approx (a).margin (0.2));
        if (f == 1000.0)
            CHECK (b == Catch::Approx (6.0).margin (0.1));
    }
}
