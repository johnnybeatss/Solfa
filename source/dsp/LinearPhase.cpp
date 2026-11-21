#include "LinearPhase.h"

#include <cmath>
#include <numbers>

namespace solfa::dsp
{
    LinearPhaseEq::LinearPhaseEq() : juce::Thread ("Solfa linear phase") {}

    LinearPhaseEq::~LinearPhaseEq()
    {
        release();
        delete pending.exchange (nullptr);
        delete spent.exchange (nullptr);
    }

    int LinearPhaseEq::firOrder (int m, double fs) noexcept
    {
        const int base = m == hd ? 14 : 12;
        const int scale = (int) std::lround (std::log2 (std::max (fs, 8000.0) / 48000.0));
        return juce::jlimit (10, 17, base + scale);
    }

    int LinearPhaseEq::latencyFor (int m, double fs) noexcept
    {
        return m == off ? 0 : (1 << firOrder (m, fs)) / 2;
    }

    void LinearPhaseEq::prepare (double fs, int maxBlockSize, int numChannels)
    {
        release();
        sampleRate.store (fs);
        channels = juce::jlimit (1, 2, numChannels);

        convolution.prepare ({ fs, (juce::uint32) maxBlockSize, (juce::uint32) channels });
        const auto maxLatency = latencyFor (hd, fs);
        delayLine.setSize (channels, maxLatency + maxBlockSize + 1);
        delayLine.clear();
        delayedInput.setSize (channels, maxBlockSize);
        delayedInput.clear();
        delayWrite = 0;
        activeMode = -1; // force a reset on the first setMode()
        lastHash = 0;

        startThread (juce::Thread::Priority::low);
    }

    void LinearPhaseEq::release()
    {
        stopThread (2000);
    }

    void LinearPhaseEq::setMode (int newMode) noexcept
    {
        mode.store (newMode);
        if (newMode == activeMode)
            return;

        activeMode = newMode;
        const auto fs = sampleRate.load();
        latency = latencyFor (newMode, fs);
        expectedSize = newMode == off ? 0 : 1 << firOrder (newMode, fs);
        delayLine.clear();
        delayWrite = 0;
        convolution.reset();
        loadedSinceSwitch = false;
        filterActive = false;
        settleSamples = (int) (0.1 * fs);
    }

    void LinearPhaseEq::process (juce::AudioBuffer<float>& buffer) noexcept
    {
        const auto numChannels = juce::jmin (buffer.getNumChannels(), channels);
        const auto numSamples = juce::jmin (buffer.getNumSamples(), delayedInput.getNumSamples());
        const auto size = delayLine.getNumSamples();

        // 1. Delayed copy of the input (pass-through until the FIR is live, and Compare's dry).
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* line = delayLine.getWritePointer (ch);
            const auto* in = buffer.getReadPointer (ch);
            auto* out = delayedInput.getWritePointer (ch);
            auto w = delayWrite;
            for (int i = 0; i < numSamples; ++i)
            {
                line[w] = in[i];
                out[i] = line[(w - latency + size) % size];
                w = (w + 1) % size;
            }
        }
        delayWrite = (delayWrite + numSamples) % size;

        // 2. Pick up a newly designed FIR (only once the previous one has been handed back).
        if (pending.load() != nullptr && spent.load() == nullptr)
            if (auto* ir = pending.exchange (nullptr))
            {
                if (ir->getNumSamples() == expectedSize)
                {
                    convolution.loadImpulseResponse (std::move (*ir), sampleRate.load(), juce::dsp::Convolution::Stereo::no,
                        juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
                    loadedSinceSwitch = true;
                }
                spent.store (ir); // the design thread deletes it
            }

        // 3. Always run the convolution: JUCE only swaps in a newly loaded IR inside process().
        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numChannels, (size_t) numSamples);
        convolution.process (juce::dsp::ProcessContextReplacing<float> (block));

        // Use its output once this mode's FIR is live and the engine's crossfade from its default
        // (undelayed) IR has finished; until then output the delayed input instead.
        if (! filterActive && loadedSinceSwitch && convolution.getCurrentIRSize() == expectedSize)
        {
            settleSamples -= numSamples;
            filterActive = settleSamples <= 0;
        }
        if (! filterActive)
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.copyFrom (ch, 0, delayedInput, ch, 0, numSamples);
    }

    void LinearPhaseEq::designFir (const std::vector<double>& power, int order, juce::dsp::FFT& fft, std::vector<float>& ir)
    {
        const int n = 1 << order;
        std::vector<juce::dsp::Complex<float>> spectrum ((size_t) n), time ((size_t) n);
        for (int k = 0; k < n; ++k)
        {
            const auto bin = std::min (k, n - k); // real, even spectrum -> zero-phase impulse
            spectrum[(size_t) k] = { (float) std::sqrt (std::max (power[(size_t) bin], 0.0)), 0.0f };
        }
        fft.perform (spectrum.data(), time.data(), true); // JUCE scales the inverse by 1/N

        ir.resize ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            const auto window = 0.5 * (1.0 - std::cos (2.0 * std::numbers::pi * i / n)); // Hann, peak at the centre
            ir[(size_t) i] = (float) (time[(size_t) ((i + n / 2) % n)].real() * window);
        }
    }

    void LinearPhaseEq::designIfNeeded (bool force)
    {
        const juce::ScopedLock lock (designLock);
        const auto m = mode.load();
        if (m == off || ! curveHash || ! curvePower)
            return;

        const auto fs = sampleRate.load();
        const auto order = firOrder (m, fs);
        const auto hash = curveHash() * 1000003u + (std::uint64_t) order * 7919u + (std::uint64_t) fs;
        if (! force && hash == lastHash)
            return;
        lastHash = hash;

        if (fft == nullptr || fftOrder != order)
        {
            fft = std::make_unique<juce::dsp::FFT> (order);
            fftOrder = order;
        }

        const int n = 1 << order;
        std::vector<double> freqs ((size_t) n / 2 + 1), power;
        for (size_t k = 0; k < freqs.size(); ++k)
            freqs[k] = (double) k * fs / n;
        curvePower (freqs, power, fs);

        std::vector<float> ir;
        designFir (power, order, *fft, ir);

        auto* buffer = new juce::AudioBuffer<float> (1, n);
        buffer->copyFrom (0, 0, ir.data(), n);
        delete pending.exchange (buffer); // an unconsumed older design is simply replaced
        delete spent.exchange (nullptr);
    }

    void LinearPhaseEq::run()
    {
        while (! threadShouldExit())
        {
            designIfNeeded (false);
            delete spent.exchange (nullptr);
            wait (40);
        }
    }
}
