#include "PluginProcessor.h"

namespace
{
    constexpr double gainSmoothingSeconds = 0.03; // within the 20-50 ms range from the research
}

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "SolfaState", solfa::params::createLayout()),
      handles (state)
{
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    outputGain.reset (sampleRate, gainSmoothingSeconds);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (handles.outputGain->load()));
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    for (int b = 0; b < solfa::dsp::numBands; ++b)
        engine.setBand (b, handles.bands[(size_t) b].read());
    engine.setAmount (handles.amount->load() / 100.0);
    engine.process (buffer);

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (handles.outputGain->load()));

    // Unity and not ramping: leave the audio untouched (keeps the flat setting bit-exact).
    if (! outputGain.isSmoothing() && juce::exactlyEqual (outputGain.getTargetValue(), 1.0f))
        return;

    const auto numChannels = getTotalNumInputChannels();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto g = outputGain.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    // Temporary until the real UI (Phase 5): JUCE's generic editor lists every parameter,
    // which is enough for listening tests in a DAW.
    return new juce::GenericAudioProcessorEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
