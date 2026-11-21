#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      outputAttachment (p.getState(), solfa::ids::outputGain, outputSlider)
{
    outputLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputSlider);
    addAndMakeVisible (outputLabel);
    setSize (420, 300);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15131c));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (22.0f));
    g.drawText (PRODUCT_NAME_WITHOUT_VERSION, getLocalBounds().removeFromTop (60), juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (juce::String ("v") + VERSION + " (" + CMAKE_BUILD_TYPE + ")",
        getLocalBounds().removeFromBottom (24), juce::Justification::centred);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (20).withTrimmedTop (50).withTrimmedBottom (20);
    outputLabel.setBounds (area.removeFromTop (20));
    outputSlider.setBounds (area.withSizeKeepingCentre (140, 160));
}
