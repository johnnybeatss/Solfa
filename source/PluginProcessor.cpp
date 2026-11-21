#include "PluginProcessor.h"

namespace
{
    constexpr double gainSmoothingSeconds = 0.03; // within the 20-50 ms range from the research
    constexpr double matchSmoothingSeconds = 0.2; // level match follows the meters gently
    constexpr double maxCompensationDb = 18.0;
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
    const auto numChannels = getTotalNumOutputChannels();
    engine.prepare (sampleRate, samplesPerBlock, numChannels);

    autoGainEstimator.prepare (sampleRate);
    lastCurveVersion = engine.getVersion() + 1; // force a first estimate
    curveLoudnessDb = autoGainDb = 0.0;

    inputMeter.prepare (sampleRate, numChannels);
    outputMeter.prepare (sampleRate, numChannels);
    dry.setSize (juce::jmax (1, numChannels), juce::jmax (samplesPerBlock, 512));

    outputGain.reset (sampleRate, gainSmoothingSeconds);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (handles.outputGain->load()));
    compareMix.reset (sampleRate, gainSmoothingSeconds);
    compareMix.setCurrentAndTargetValue (handles.compare->load() >= 0.5f ? 1.0f : 0.0f);
    matchGain.reset (sampleRate, matchSmoothingSeconds);
    matchGain.setCurrentAndTargetValue (1.0f);
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

    // Work in chunks no larger than the preallocated dry buffer (no allocation here).
    const auto numChannels = juce::jmin (getTotalNumInputChannels(), dry.getNumChannels());
    for (int start = 0; start < buffer.getNumSamples(); start += dry.getNumSamples())
    {
        const auto len = juce::jmin (dry.getNumSamples(), buffer.getNumSamples() - start);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), numChannels, start, len);
        processChunk (chunk);
    }
}

void PluginProcessor::processChunk (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    // 1. Meter the input; keep a dry copy only while Compare is in use.
    inputMeter.process (buffer.getArrayOfReadPointers(), numChannels, numSamples);
    compareMix.setTargetValue (handles.compare->load() >= 0.5f ? 1.0f : 0.0f);
    const bool compareInUse = compareMix.isSmoothing() || compareMix.getTargetValue() > 0.0f;
    if (compareInUse)
        for (int ch = 0; ch < numChannels; ++ch)
            dry.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // 2. EQ
    engine.process (buffer);

    // 3. Output gain + static auto gain (re-estimated only when the curve changes)
    if (engine.getVersion() != lastCurveVersion)
    {
        lastCurveVersion = engine.getVersion();
        curveLoudnessDb = engine.isBypassed()
                            ? 0.0
                            : autoGainEstimator.estimateDb ([this] (double f) { return engine.curvePowerAt (f); });
    }
    autoGainDb = handles.autoGain->load() >= 0.5f
                   ? juce::jlimit (-maxCompensationDb, maxCompensationDb, -curveLoudnessDb)
                   : 0.0;

    outputGain.setTargetValue (juce::Decibels::decibelsToGain ((float) (handles.outputGain->load() + autoGainDb)));

    // Unity and not ramping: leave the audio untouched (keeps the flat setting bit-exact).
    if (outputGain.isSmoothing() || ! juce::exactlyEqual (outputGain.getTargetValue(), 1.0f))
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto g = outputGain.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer (ch)[i] *= g;
        }
    }

    // 4. Meter the output, then blend towards the level-matched dry signal if comparing.
    outputMeter.process (buffer.getArrayOfReadPointers(), numChannels, numSamples);
    if (! compareInUse)
        return;

    const auto inMs = inputMeter.getMeanSquare();
    const auto outMs = outputMeter.getMeanSquare();
    const auto match = inMs > 1.0e-10 ? std::sqrt (outMs / inMs) : 1.0;
    const auto maxMatch = juce::Decibels::decibelsToGain (maxCompensationDb);
    matchGain.setTargetValue ((float) juce::jlimit (1.0 / maxMatch, maxMatch, match));

    for (int i = 0; i < numSamples; ++i)
    {
        const auto mix = compareMix.getNextValue();
        const auto g = matchGain.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& y = buffer.getWritePointer (ch)[i];
            y += mix * (g * dry.getSample (ch, i) - y);
        }
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
