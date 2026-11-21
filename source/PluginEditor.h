#pragma once

#include "PluginProcessor.h"
#include "ui/EqDisplay.h"
#include "ui/Panels.h"
#include "ui/Theme.h"

class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    solfa::ui::EqDisplay& getDisplay() noexcept { return display; }

private:
    void refreshPresetBox();
    void timerCallback() override { refreshPresetBox(); }

    PluginProcessor& solfaProcessor;
    solfa::ui::theme::LookAndFeel lookAndFeel;

    // Top bar
    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" };
    juce::ToggleButton autoGain { "Auto Gain" };
    juce::ComboBox phaseBox;
    std::unique_ptr<solfa::ui::APVTS::ComboBoxAttachment> phaseAttachment;
    juce::TextButton compare { "Compare" }, science { "?" };
    juce::Slider output { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label outputLabel { {}, "Output" };
    std::unique_ptr<solfa::ui::APVTS::SliderAttachment> outputAttachment;
    std::unique_ptr<solfa::ui::APVTS::ButtonAttachment> autoGainAttachment, compareAttachment;

    solfa::ui::EqDisplay display;
    solfa::ui::BandPanel bandPanel;
    solfa::ui::SpiritPanel spiritPanel;
    solfa::ui::SciencePanel sciencePanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
