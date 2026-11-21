#pragma once

#include "Parameters.h"
#include "dsp/EqEngine.h"
#include "dsp/AnalyzerFifo.h"
#include "dsp/LinearPhase.h"
#include "dsp/Loudness.h"
#include "dsp/Resonator.h"

#include <juce_audio_processors/juce_audio_processors.h>

class PluginProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

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

    // Factory presets are exposed as host programs (a preset browser arrives with the UI).
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() noexcept { return state; }
    const solfa::dsp::EqEngine& getEngine() const noexcept { return engine; }

    // Current static auto-gain compensation in dB (0 when Auto Gain is off or the EQ is flat).
    double getAutoGainDb() const noexcept { return autoGainDb; }

    // Focus frequency after Key Snap (or exact when Pure is on), as used by Literal and Resonance.
    double getEffectiveFocusHz() const noexcept { return effectiveFocusHz.load(); }

    // Output (mono mix) for the analyser in the editor.
    solfa::dsp::AnalyzerFifo& getAnalyzerFifo() noexcept { return analyzerFifo; }
    // Input (before any processing), for EQ Match's "learn" capture.
    solfa::dsp::AnalyzerFifo& getInputFifo() noexcept { return inputFifo; }

    // Linear-phase mode (Phase parameter). Exposed for tests and the UI.
    solfa::dsp::LinearPhaseEq& getLinearPhase() noexcept { return linearPhase; }

    // Literal mode band width (deliberately narrow: the one place presets may go narrow).
    static constexpr double literalQ = 6.0;

private:
    void processChunk (juce::AudioBuffer<float>& buffer) noexcept;
    void handleAsyncUpdate() override; // reports latency changes to the host (message thread)
    std::uint64_t curveHash() const noexcept;

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

    // Phase 5
    solfa::dsp::Resonator resonator;
    std::atomic<double> effectiveFocusHz { 528.0 };
    std::atomic<bool> snapOutputGain { false };
    int currentProgram = 0;

    solfa::dsp::AnalyzerFifo analyzerFifo, inputFifo;

    solfa::dsp::LinearPhaseEq linearPhase;
    int reportedPhaseMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
