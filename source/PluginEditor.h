#pragma once

#include "PluginProcessor.h"

// Placeholder editor for Phase 1. The real UI (EQ curve, analyser, presets) is Phase 5.
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider outputSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label outputLabel { {}, "Output" };
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
