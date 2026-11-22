#pragma once

#include "PluginProcessor.h"
#include "ui/EqDisplay.h"
#include "ui/MatchPanel.h"
#include "ui/Panels.h"
#include "ui/Scenery.h"
#include "ui/Theme.h"

class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    solfa::ui::EqDisplay& getDisplay() noexcept { return display; }
    solfa::ui::MatchPanel& getMatchPanel() noexcept { return matchPanel; }
    juce::ComboBox& getPresetBox() noexcept { return presetBox; }

private:
    void refreshPresetBox();
    void rebuildPresetBox();
    void presetChosen (int itemId);

    static constexpr int factoryIdBase = 1, userIdBase = 1000, saveId = 9000, showFolderId = 9001;
    juce::Array<juce::File> userPresets;
    int shownPresetId = factoryIdBase, lastHostProgram = -1;
    std::unique_ptr<juce::FileChooser> saveChooser;
    void timerCallback() override { refreshPresetBox(); }

    PluginProcessor& solfaProcessor;
    solfa::ui::theme::LookAndFeel lookAndFeel;
    solfa::ui::scenery::CachedLayer background; // beam, logo and jungle floor
    static constexpr int topBarHeight = 50;

    // Top bar
    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" };
    juce::ToggleButton autoGain { "Auto Gain" };
    juce::ComboBox phaseBox;
    std::unique_ptr<solfa::ui::APVTS::ComboBoxAttachment> phaseAttachment;
    juce::TextButton compare { "Compare" }, science { "?" }, matchButton { "Match" };
    juce::Slider output { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label outputLabel { {}, "Output" };
    std::unique_ptr<solfa::ui::APVTS::SliderAttachment> outputAttachment;
    std::unique_ptr<solfa::ui::APVTS::ButtonAttachment> autoGainAttachment, compareAttachment;

    solfa::ui::EqDisplay display;
    solfa::ui::BandPanel bandPanel;
    solfa::ui::SpiritPanel spiritPanel;
    solfa::ui::SciencePanel sciencePanel;
    solfa::ui::MatchPanel matchPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
