#pragma once

#include "Parameters.h"
#include "dsp/EqEngine.h"
#include "dsp/Loudness.h"

#include <juce_audio_processors/juce_audio_processors.h>

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Hosts cope badly with zero programs; presets are handled separately (Phase 4).
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() noexcept { return state; }
    const solfa::dsp::EqEngine& getEngine() const noexcept { return engine; }

    // Current static auto-gain compensation in dB (0 when Auto Gain is off or the EQ is flat).
    double getAutoGainDb() const noexcept { return autoGainDb; }

private:
    void processChunk (juce::AudioBuffer<float>& buffer) noexcept;

    juce::AudioProcessorValueTreeState state;
    solfa::params::Handles handles;
    solfa::dsp::EqEngine engine;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGain { 1.0f };

    // Auto gain
    solfa::dsp::AutoGainEstimator autoGainEstimator;
    unsigned lastCurveVersion = 0;
    double curveLoudnessDb = 0.0;
    double autoGainDb = 0.0;

    // Level-matched compare
    solfa::dsp::LoudnessMeter inputMeter, outputMeter;
    juce::AudioBuffer<float> dry;
    juce::SmoothedValue<float> compareMix { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> matchGain { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
