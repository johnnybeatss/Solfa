#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace solfa::dsp
{
    // Wait-free hand-off of the output signal (mono mix) from the audio thread to the analyser.
    // The audio thread only writes; the UI thread only reads. If the UI falls behind, new
    // samples that do not fit are dropped (the analyser just skips ahead).
    class AnalyzerFifo
    {
    public:
        static constexpr int capacity = 1 << 15;

        void push (const juce::AudioBuffer<float>& buffer) noexcept
        {
            const auto numChannels = juce::jmax (1, buffer.getNumChannels());
            const auto n = juce::jmin (buffer.getNumSamples(), fifo.getFreeSpace());
            if (n <= 0)
                return;

            const auto scale = 1.0f / (float) numChannels;
            const auto scope = fifo.write (n);
            auto copy = [&] (int start, int size, int offset) {
                for (int i = 0; i < size; ++i)
                {
                    float sum = 0.0f;
                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                        sum += buffer.getSample (ch, offset + i);
                    data[(size_t) (start + i)] = sum * scale;
                }
            };
            copy (scope.startIndex1, scope.blockSize1, 0);
            copy (scope.startIndex2, scope.blockSize2, scope.blockSize1);
        }

        // UI thread: copies up to maxSamples into dest, returns how many were copied.
        int pull (float* dest, int maxSamples) noexcept
        {
            const auto n = juce::jmin (maxSamples, fifo.getNumReady());
            const auto scope = fifo.read (n);
            std::copy_n (data.data() + scope.startIndex1, scope.blockSize1, dest);
            std::copy_n (data.data() + scope.startIndex2, scope.blockSize2, dest + scope.blockSize1);
            return n;
        }

    private:
        juce::AbstractFifo fifo { capacity };
        std::vector<float> data = std::vector<float> ((size_t) capacity, 0.0f);
    };
}
