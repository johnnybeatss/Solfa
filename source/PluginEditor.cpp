#include "PluginEditor.h"
#include "Presets.h"

using namespace solfa;

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), solfaProcessor (p), display (p), bandPanel (p.getState()), spiritPanel (p)
{
    setLookAndFeel (&lookAndFeel);

    for (const auto& preset : presets::factory())
        presetBox.addItem (preset.name, presetBox.getNumItems() + 1);
    presetBox.onChange = [this] {
        const auto index = presetBox.getSelectedItemIndex();
        if (index >= 0 && index != solfaProcessor.getCurrentProgram())
            solfaProcessor.setCurrentProgram (index);
    };
    prevPreset.onClick = [this] {
        const auto n = solfaProcessor.getNumPrograms();
        solfaProcessor.setCurrentProgram ((solfaProcessor.getCurrentProgram() + n - 1) % n);
        refreshPresetBox();
    };
    nextPreset.onClick = [this] {
        solfaProcessor.setCurrentProgram ((solfaProcessor.getCurrentProgram() + 1) % solfaProcessor.getNumPrograms());
        refreshPresetBox();
    };
    refreshPresetBox();

    phaseBox.addItemList ({ "Zero Latency", "Linear Phase", "Linear Phase HD" }, 1);
    phaseBox.setTooltip ("Linear phase: no phase shift, adds latency (the DAW compensates) and pre-ringing on sharp hits");
    phaseAttachment = std::make_unique<ui::APVTS::ComboBoxAttachment> (p.getState(), params::phaseMode, phaseBox);

    compare.setClickingTogglesState (true);
    compare.setTooltip ("Plays the unprocessed sound at the same loudness, so you judge tone, not volume");
    autoGainAttachment = std::make_unique<ui::APVTS::ButtonAttachment> (p.getState(), params::autoGain, autoGain);
    compareAttachment = std::make_unique<ui::APVTS::ButtonAttachment> (p.getState(), params::compare, compare);
    outputAttachment = std::make_unique<ui::APVTS::SliderAttachment> (p.getState(), params::outputGain, output);
    output.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    output.setTextValueSuffix (" dB");
    output.setColour (juce::Slider::thumbColourId, ui::theme::curve);
    output.setColour (juce::Slider::trackColourId, ui::theme::panelEdge);
    output.setColour (juce::Slider::backgroundColourId, ui::theme::grid);
    output.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    outputLabel.setJustificationType (juce::Justification::centredRight);
    science.onClick = [this] { sciencePanel.setVisible (true); sciencePanel.toFront (false); };

    display.onSelectionChanged = [this] (int band) { bandPanel.showBand (band); };

    for (juce::Component* c : std::initializer_list<juce::Component*> { &presetBox, &prevPreset, &nextPreset, &phaseBox, &autoGain, &compare,
             &science, &output, &outputLabel, &display, &bandPanel, &spiritPanel })
        addAndMakeVisible (c);
    addChildComponent (sciencePanel);

    setResizable (true, true);
    setResizeLimits (860, 560, 1800, 1150);
    setSize (1040, 680);
    startTimerHz (5); // keeps the preset box in sync with host-side preset changes
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PluginEditor::refreshPresetBox()
{
    presetBox.setSelectedItemIndex (solfaProcessor.getCurrentProgram(), juce::dontSendNotification);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::theme::background);
    g.setColour (ui::theme::text);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("Solfa", juce::Rectangle<int> (16, 8, 90, 32), juce::Justification::centredLeft);
    g.setColour (ui::theme::dimText);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (juce::String ("v") + VERSION, juce::Rectangle<int> (86, 20, 60, 14), juce::Justification::centredLeft);
}

void PluginEditor::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (48).reduced (8, 8);
    top.removeFromLeft (140);
    prevPreset.setBounds (top.removeFromLeft (28));
    presetBox.setBounds (top.removeFromLeft (220).reduced (4, 0));
    nextPreset.setBounds (top.removeFromLeft (28));
    top.removeFromLeft (12);
    phaseBox.setBounds (top.removeFromLeft (150));
    science.setBounds (top.removeFromRight (32));
    top.removeFromRight (8);
    output.setBounds (top.removeFromRight (170));
    outputLabel.setBounds (top.removeFromRight (50));
    top.removeFromRight (8);
    compare.setBounds (top.removeFromRight (90));
    top.removeFromRight (8);
    autoGain.setBounds (top.removeFromRight (100));

    auto bottom = r.removeFromBottom (juce::jlimit (170, 230, getHeight() / 3)).reduced (8, 6);
    bandPanel.setBounds (bottom.removeFromLeft (bottom.getWidth() * 11 / 20).reduced (0, 0).withTrimmedRight (4));
    spiritPanel.setBounds (bottom.withTrimmedLeft (4));

    display.setBounds (r.reduced (8, 0));
    sciencePanel.setBounds (getLocalBounds());
}
