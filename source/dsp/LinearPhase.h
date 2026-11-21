#pragma once

#include <atomic>
#include <functional>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

namespace solfa::dsp
{
    // Linear-phase EQ: a background thread turns the EQ's magnitude curve into a symmetric FIR
    // (zero-phase spectrum -> inverse FFT -> centred, Hann-windowed) and hands it to the audio
    // thread through a lock-free slot. The audio thread loads it into juce::dsp::Convolution
    // (loadImpulseResponse is wait-free and must be called from the audio thread) and convolves.
    //
    // Latency = FIR length / 2. Until the first FIR for a mode is active, the input is passed
    // through delayed by the same latency, so timing never jumps.
    class LinearPhaseEq : private juce::Thread
    {
    public:
        enum Mode
        {
            off = 0,
            standard = 1, // 4096 taps at 48 kHz (~43 ms)
            hd = 2        // 16384 taps at 48 kHz (~171 ms)
        };

        // Called on the design thread only.
        std::function<std::uint64_t()> curveHash;                                           // changes when the curve changes
        std::function<void (const std::vector<double>& freqs, std::vector<double>& power, double sampleRate)> curvePower;

        LinearPhaseEq();
        ~LinearPhaseEq() override;

        void prepare (double sampleRate, int maxBlockSize, int numChannels);
        void release();

        // FIR order (log2 of length) for a mode at a sample rate, keeping the same frequency
        // resolution at every rate.
        static int firOrder (int mode, double sampleRate) noexcept;
        static int latencyFor (int mode, double sampleRate) noexcept;

        // Audio thread.
        void setMode (int mode) noexcept;
        int getMode() const noexcept { return mode.load(); }
        void process (juce::AudioBuffer<float>& buffer) noexcept;
        // The input delayed by the current latency (valid after process()), for Compare.
        const juce::AudioBuffer<float>& getDelayedInput() const noexcept { return delayedInput; }
        bool isFilterActive() const noexcept { return filterActive; }

        // Design synchronously on the calling thread (tests).
        void designNow() { designIfNeeded (true); }

        // Pure function (exposed for tests): fill `ir` (length 2^order) with the linear-phase
        // FIR for magnitudes sqrt(power) sampled at bins k * fs / N.
        static void designFir (const std::vector<double>& power, int order, juce::dsp::FFT& fft, std::vector<float>& ir);

    private:
        void run() override;
        void designIfNeeded (bool force);

        std::atomic<int> mode { off };
        std::atomic<double> sampleRate { 48000.0 };
        int channels = 2;

        // Design thread
        juce::CriticalSection designLock; // design thread vs designNow() (never the audio thread)
        std::uint64_t lastHash = 0;
        std::unique_ptr<juce::dsp::FFT> fft;
        int fftOrder = 0;

        // Hand-off: design thread -> audio thread, and consumed buffers back for deletion.
        std::atomic<juce::AudioBuffer<float>*> pending { nullptr };
        std::atomic<juce::AudioBuffer<float>*> spent { nullptr };

        // Audio thread
        juce::dsp::Convolution convolution;
        int activeMode = off;
        int expectedSize = 0;
        int latency = 0;
        bool loadedSinceSwitch = false;
        int settleSamples = 0; // after the FIR goes live, wait out the engine's internal crossfade
        bool filterActive = false;
        juce::AudioBuffer<float> delayLine, delayedInput;
        int delayWrite = 0;
    };
}
