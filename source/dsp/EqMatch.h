#pragma once

#include "FilterDesign.h"

#include <juce_dsp/juce_dsp.h>
#include <vector>

// EQ Match: compare the long-term spectrum of a reference with the current material and fit
// ordinary Solfa bands (bells and shelves) to the difference. Runs on UI/background threads only.
namespace solfa::dsp
{
    // Long-term average power spectrum (Hann-windowed 8192-point FFT, 50% overlap).
    class LongTermSpectrum
    {
    public:
        static constexpr int fftOrder = 13;
        static constexpr int fftSize = 1 << fftOrder;

        explicit LongTermSpectrum (double fs = 48000.0) : sampleRate (fs) { reset(); }

        void reset();
        void setSampleRate (double fs) { sampleRate = fs; reset(); }
        void push (const float* samples, int numSamples);

        int getNumFrames() const noexcept { return frames; }
        double getSecondsAnalysed() const noexcept { return frames * (fftSize / 2) / sampleRate; }

        // Average level in dB at each frequency, smoothed over a band `smoothingOctaves` wide.
        std::vector<double> levelsDb (const std::vector<double>& freqs, double smoothingOctaves) const;

    private:
        void analyseFrame();

        double sampleRate;
        juce::dsp::FFT fft { fftOrder };
        juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false };
        std::vector<float> frame = std::vector<float> ((size_t) fftSize, 0.0f);
        std::vector<float> work = std::vector<float> ((size_t) fftSize * 2, 0.0f);
        std::vector<double> powerSum;
        int filled = 0, frames = 0;
    };

    struct FittedBand
    {
        FilterType type = FilterType::bell;
        double frequencyHz = 1000.0, gainDb = 0.0, q = 1.0;
    };

    namespace match
    {
        // Log-spaced analysis grid, 20 Hz - 20 kHz.
        std::vector<double> logGrid (int numPoints = 160);

        // Reference minus current, clamped to +/-maxDb, scaled by amount (0-1) and normalised so its
        // average over 100 Hz - 10 kHz is 0 dB (match the tonal balance, not the level).
        std::vector<double> matchCurve (const std::vector<double>& freqs, const std::vector<double>& referenceDb,
            const std::vector<double>& currentDb, double amount, double maxDb = 12.0);

        // Fit up to maxBands bells/shelves to targetDb (greedy placement + least-squares gains).
        std::vector<FittedBand> fit (const std::vector<double>& freqs, const std::vector<double>& targetDb, int maxBands,
            double sampleRate);

        // Combined response of fitted bands, in dB, at each frequency.
        std::vector<double> responseDb (const std::vector<FittedBand>& bands, const std::vector<double>& freqs, double sampleRate);

        // Weighted RMS difference (40 Hz - 16 kHz) between two curves, in dB.
        double rmsErrorDb (const std::vector<double>& freqs, const std::vector<double>& a, const std::vector<double>& b);
    }
}
