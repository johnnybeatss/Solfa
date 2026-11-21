#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

namespace
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    juce::AudioBuffer<float> makeSine (int numChannels, int numSamples)
    {
        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample (ch, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / (float) sampleRate));
        return buffer;
    }

    void setOutputGainDb (PluginProcessor& plugin, float db)
    {
        auto* param = plugin.getState().getParameter (solfa::params::outputGain);
        REQUIRE (param != nullptr);
        param->setValueNotifyingHost (param->convertTo0to1 (db));
    }
}

TEST_CASE ("Plugin identity", "[instance]")
{
    PluginProcessor plugin;

    CHECK_THAT (plugin.getName().toStdString(), Catch::Matchers::Equals ("Solfa"));
    CHECK (plugin.getProgramName (0).isNotEmpty()); // Steinberg's validator requires this
    CHECK_FALSE (plugin.acceptsMidi());
    CHECK (plugin.getTailLengthSeconds() == 0.0);
}

TEST_CASE ("At default settings the plugin is bit-transparent", "[dsp][null]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (sampleRate, blockSize);

    auto input = makeSine (2, blockSize);
    auto buffer = input;
    juce::MidiBuffer midi;
    plugin.processBlock (buffer, midi);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < blockSize; ++i)
            REQUIRE (juce::exactlyEqual (buffer.getSample (ch, i), input.getSample (ch, i)));
}

TEST_CASE ("Output gain reaches its target after smoothing", "[dsp][gain]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (sampleRate, blockSize);
    setOutputGainDb (plugin, 6.0f);

    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buffer;

    // 30 ms smoothing at 48 kHz = 1440 samples; run well past it.
    for (int block = 0; block < 8; ++block)
    {
        buffer = makeSine (2, blockSize);
        plugin.processBlock (buffer, midi);
    }

    const auto input = makeSine (2, blockSize);
    const auto expected = juce::Decibels::decibelsToGain (6.0f);
    for (int i = 0; i < blockSize; i += 37)
        CHECK (buffer.getSample (0, i) == Catch::Approx (input.getSample (0, i) * expected).margin (1e-6));
}

TEST_CASE ("Gain changes are smoothed, not stepped", "[dsp][gain]")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (sampleRate, blockSize);
    setOutputGainDb (plugin, 12.0f);

    juce::AudioBuffer<float> buffer (1, blockSize);
    buffer.clear();
    for (int i = 0; i < blockSize; ++i)
        buffer.setSample (0, i, 1.0f); // DC so the output is the gain curve itself

    juce::AudioBuffer<float> stereo (2, blockSize);
    stereo.copyFrom (0, 0, buffer, 0, 0, blockSize);
    stereo.copyFrom (1, 0, buffer, 0, 0, blockSize);

    juce::MidiBuffer midi;
    plugin.processBlock (stereo, midi);

    const auto finalGain = juce::Decibels::decibelsToGain (12.0f);
    CHECK (stereo.getSample (0, 0) < 1.1f); // starts near unity
    CHECK (stereo.getSample (0, blockSize - 1) < finalGain); // still ramping after one block
    for (int i = 1; i < blockSize; ++i)
        REQUIRE (stereo.getSample (0, i) >= stereo.getSample (0, i - 1)); // monotonic ramp
}

TEST_CASE ("State round-trips through save and load", "[state]")
{
    PluginProcessor source;
    setOutputGainDb (source, -7.5f);

    juce::MemoryBlock blob;
    source.getStateInformation (blob);

    PluginProcessor restored;
    restored.setStateInformation (blob.getData(), (int) blob.getSize());

    const auto* value = restored.getState().getRawParameterValue (solfa::params::outputGain);
    REQUIRE (value != nullptr);
    CHECK (value->load() == Catch::Approx (-7.5f).margin (0.01f));
}

TEST_CASE ("Editor opens and closes", "[editor]")
{
    runWithinPluginEditor ([] (PluginProcessor& plugin) {
        REQUIRE (plugin.getActiveEditor() != nullptr);
    });
}

TEST_CASE ("State restore fixes bools a host left between 0 and 1", "[state]")
{
    // Reproduces pluginval's state-restoration failure: a bool at a raw 0.43 (= off) must read
    // exactly 0 after restoring a state where it was off.
    PluginProcessor plugin;
    juce::MemoryBlock blob;
    plugin.getStateInformation (blob); // band 2 is off by default

    auto* on = plugin.getState().getParameter (solfa::params::bandId (1, solfa::params::on));
    REQUIRE (on != nullptr);
    on->setValueNotifyingHost (0.43f);
    plugin.setStateInformation (blob.getData(), (int) blob.getSize());
    CHECK (on->getValue() == 0.0f);
}
