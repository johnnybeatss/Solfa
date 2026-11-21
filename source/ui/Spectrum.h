#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace solfa::ui
{
    // UI-side spectrum analyser: keeps the latest fftSize samples, computes a Hann-windowed FFT,
    // applies peak-hold-and-fall ballistics and a display tilt (dB/oct around 1 kHz).
    class Spectrum
    {
    public:
        static constexpr int fftOrder = 13; // 8192 points: ~5.9 Hz bins at 48 kHz
        static constexpr int fftSize = 1 << fftOrder;
        static constexpr double tiltDbPerOctave = 4.5;
        static constexpr double floorDb = -100.0;

        Spectrum();

        void setSampleRate (double newSampleRate) noexcept { sampleRate = newSampleRate; }
        double getSampleRate() const noexcept { return sampleRate; }

        // Feed new samples (UI thread), then call update() once per frame.
        void push (const float* samples, int numSamples) noexcept;
        void update (double frameSeconds) noexcept;

        // Display level in dB (tilted, smoothed) at a frequency, interpolated between bins.
        double levelDbAt (double frequencyHz) const noexcept;

        // Spectrum Grab: the frequency of the strongest peak within +/- octaveRadius of
        // frequencyHz (parabolic interpolation between bins). Returns frequencyHz if flat.
        double findPeakNear (double frequencyHz, double octaveRadius = 1.0 / 3.0) const noexcept;

        // Direct access for tests: set the display levels (one per bin) instead of measuring.
        void setLevelsForTesting (const std::vector<double>& levelsDb) { levels = levelsDb; }
        int getNumBins() const noexcept { return fftSize / 2 + 1; }

    private:
        double binFrequency (double bin) const noexcept { return bin * sampleRate / fftSize; }

        double sampleRate = 48000.0;
        juce::dsp::FFT fft { fftOrder };
        juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false };
        std::vector<float> history = std::vector<float> ((size_t) fftSize, 0.0f);
        std::vector<float> work = std::vector<float> ((size_t) fftSize * 2, 0.0f);
        std::vector<double> levels;
        int writePos = 0;
    };
}
