#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/CurveModel.h"

#include <cstring>
#include "Presets.h"
#include "dsp/Tuning.h"

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
    // Linear phase: the design thread reads the curve straight from the parameter atomics,
    // using its own CurveModel (touched only on that thread).
    linearPhase.curveHash = [this] { return curveHash(); };
    linearPhase.curvePower = [this, model = std::make_shared<solfa::ui::CurveModel>()] (const std::vector<double>& freqs,
                                 std::vector<double>& power, double fs) {
        model->update (handles, fs, effectiveFocusHz.load());
        power.resize (freqs.size());
        for (size_t i = 0; i < freqs.size(); ++i)
            power[i] = std::pow (10.0, model->totalDb (std::max (freqs[i], 1.0)) / 10.0);
    };
}

PluginProcessor::~PluginProcessor()
{
    cancelPendingUpdate();
    linearPhase.release();
}

std::uint64_t PluginProcessor::curveHash() const noexcept
{
    std::uint64_t h = 1469598103934665603ull; // FNV-1a over the values that shape the static curve
    auto mix = [&h] (float v) {
        std::uint32_t bits;
        std::memcpy (&bits, &v, sizeof bits);
        h = (h ^ bits) * 1099511628211ull;
    };
    for (const auto& b : handles.bands)
        for (auto* p : { b.on, b.type, b.freq, b.gain, b.q, b.slope })
            mix (p->load());
    mix (handles.amount->load());
    mix (handles.literal->load());
    mix ((float) effectiveFocusHz.load());
    return h;
}

void PluginProcessor::handleAsyncUpdate()
{
    setLatencySamples (solfa::dsp::LinearPhaseEq::latencyFor (reportedPhaseMode, getSampleRate()));
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto numChannels = getTotalNumOutputChannels();
    engine.prepare (sampleRate, samplesPerBlock, numChannels);

    autoGainEstimator.prepare (sampleRate);
    lastCurveVersion = engine.getVersion() + 1; // force a first estimate
    curveLoudnessDb = autoGainDb = 0.0;

    resonator.prepare (sampleRate, numChannels);
    inputMeter.prepare (sampleRate, numChannels);
    outputMeter.prepare (sampleRate, numChannels);
    dry.setSize (juce::jmax (1, numChannels), juce::jmax (samplesPerBlock, 512));
    linearPhase.prepare (sampleRate, dry.getNumSamples(), numChannels);
    reportedPhaseMode = (int) handles.phaseMode->load();
    setLatencySamples (solfa::dsp::LinearPhaseEq::latencyFor (reportedPhaseMode, sampleRate));

    outputGain.reset (sampleRate, gainSmoothingSeconds);
    outputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain ((float) solfa::dsp::snapToUnity (handles.outputGain->load())));
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

    inputFifo.push (buffer);

    for (int b = 0; b < solfa::dsp::numBands; ++b)
        engine.setBand (b, handles.bands[(size_t) b].read());
    engine.setAmount (handles.amount->load() / 100.0);

    // Focus frequency: Key Snap onto the chosen key/scale unless Pure is on.
    solfa::dsp::Tuning tuning;
    const auto refIndex = juce::jlimit (0, (int) solfa::dsp::referenceOptionsHz.size() - 1, (int) handles.tuneRef->load());
    tuning.referenceA4 = solfa::dsp::referenceOptionsHz[(size_t) refIndex];
    tuning.keyRoot = juce::jlimit (0, 11, (int) handles.key->load());
    tuning.scale = (solfa::dsp::Scale) juce::jlimit (0, solfa::dsp::numScales - 1, (int) handles.scale->load());
    const double focusHz = handles.focus->load();
    const auto focus = handles.pure->load() >= 0.5f ? focusHz : solfa::dsp::snapToScale (focusHz, tuning);
    effectiveFocusHz.store (focus);

    solfa::dsp::BandTarget literal;
    literal.enabled = true; // 0 dB means identity, so the band switches itself off
    literal.type = solfa::dsp::FilterType::bell;
    literal.frequencyHz = focus;
    literal.gainDb = handles.literal->load();
    literal.q = literalQ;
    engine.setLiteral (literal);

    resonator.setFocus (focus);
    resonator.setDecaySeconds (handles.resDecay->load());
    resonator.setAmount (handles.resAmount->load() / 100.0);

    // Phase mode: tell the host about latency changes from the message thread.
    const auto phase = juce::jlimit (0, 2, (int) handles.phaseMode->load());
    linearPhase.setMode (phase);
    if (phase != reportedPhaseMode)
    {
        reportedPhaseMode = phase;
        triggerAsyncUpdate();
    }

    // Work in chunks no larger than the preallocated dry buffer (no allocation here).
    const auto numChannels = juce::jmin (getTotalNumInputChannels(), dry.getNumChannels());
    for (int start = 0; start < buffer.getNumSamples(); start += dry.getNumSamples())
    {
        const auto len = juce::jmin (dry.getNumSamples(), buffer.getNumSamples() - start);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), numChannels, start, len);
        processChunk (chunk);
    }

    analyzerFifo.push (buffer);
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

    // 2. EQ, then Resonance (excited by the EQ'd signal)
    const bool linear = linearPhase.getMode() != solfa::dsp::LinearPhaseEq::off;
    if (linear)
    {
        engine.updateOnly (numSamples); // keeps the static curve current for auto gain
        linearPhase.process (buffer);
    }
    else
    {
        engine.process (buffer);
    }
    resonator.process (buffer);

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

    const auto totalDb = solfa::dsp::snapToUnity (handles.outputGain->load() + autoGainDb);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain ((float) totalDb));
    if (snapOutputGain.exchange (false))
        outputGain.setCurrentAndTargetValue (outputGain.getTargetValue());

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
            const auto drySample = linear ? linearPhase.getDelayedInput().getSample (ch, i) : dry.getSample (ch, i);
            y += mix * (g * drySample - y);
        }
    }
}

int PluginProcessor::getNumPrograms()
{
    return (int) solfa::presets::factory().size();
}

void PluginProcessor::setCurrentProgram (int index)
{
    const auto& all = solfa::presets::factory();
    if (index < 0 || index >= (int) all.size())
        return;

    currentProgram = index;
    solfa::presets::apply (all[(size_t) index], state);
    engine.requestSnap(); // presets jump, they don't sweep
    snapOutputGain.store (true);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    const auto& all = solfa::presets::factory();
    return index >= 0 && index < (int) all.size() ? all[(size_t) index].name : juce::String();
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = saveStateXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        loadStateXml (*xml);
}

std::unique_ptr<juce::XmlElement> PluginProcessor::saveStateXml()
{
    return state.copyState().createXml();
}

bool PluginProcessor::loadStateXml (const juce::XmlElement& xml)
{
    {
        if (! xml.hasTagName (state.state.getType()))
            return false;

        state.replaceState (juce::ValueTree::fromXml (xml));

        // APVTS skips a parameter whose value already "means" the restored value. A host can leave
        // a bool at, say, 0.43 (= off); restoring "off" would then leave the raw 0.43 in place.
        // Push every parameter that is not exactly at its restored value.
        for (auto* p : getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
                if (auto* raw = state.getRawParameterValue (ranged->paramID))
                {
                    const auto target = ranged->convertTo0to1 (raw->load());
                    if (! juce::approximatelyEqual (ranged->getValue(), target))
                        ranged->setValueNotifyingHost (target);
                }
    }
    engine.requestSnap(); // a loaded state jumps, it doesn't sweep
    snapOutputGain.store (true);
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
