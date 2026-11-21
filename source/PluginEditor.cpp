#include "PluginEditor.h"
#include "Presets.h"

using namespace solfa;

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), solfaProcessor (p), display (p), bandPanel (p.getState()), spiritPanel (p), matchPanel (p)
{
    setLookAndFeel (&lookAndFeel);

    rebuildPresetBox();
    presetBox.onChange = [this] { presetChosen (presetBox.getSelectedId()); };
    prevPreset.onClick = [this] {
        const auto n = solfaProcessor.getNumPrograms();
        presetChosen ((solfaProcessor.getCurrentProgram() + n - 1) % n + 1);
    };
    nextPreset.onClick = [this] { presetChosen ((solfaProcessor.getCurrentProgram() + 1) % solfaProcessor.getNumPrograms() + 1); };
    refreshPresetBox();

    phaseBox.addItemList ({ "Zero Latency", "Linear Phase", "Linear Phase HD" }, 1);
    phaseBox.setTooltip ("Linear phase: no phase shift, adds latency (the DAW compensates) and pre-ringing on sharp hits");
    phaseAttachment = std::make_unique<ui::APVTS::ComboBoxAttachment> (p.getState(), params::phaseMode, phaseBox);

    compare.setClickingTogglesState (true);
    compare.setTooltip ("Plays the unprocessed sound at the same loudness, so you judge tone, not volume");
    autoGainAttachment = std::make_unique<ui::APVTS::ButtonAttachment> (p.getState(), params::autoGain, autoGain);
    compareAttachment = std::make_unique<ui::APVTS::ButtonAttachment> (p.getState(), params::compare, compare);
    outputAttachment = std::make_unique<ui::APVTS::SliderAttachment> (p.getState(), params::outputGain, output);
    output.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 20);
    output.setTextValueSuffix (" dB");
    output.setColour (juce::Slider::thumbColourId, ui::theme::curve);
    output.setColour (juce::Slider::trackColourId, ui::theme::panelEdge);
    output.setColour (juce::Slider::backgroundColourId, ui::theme::grid);
    output.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    outputLabel.setJustificationType (juce::Justification::centredRight);
    science.onClick = [this] { sciencePanel.setVisible (true); sciencePanel.toFront (false); };
    matchButton.setTooltip ("EQ Match: copy the tonal balance of a reference track");
    matchButton.onClick = [this] { matchPanel.setVisible (true); matchPanel.toFront (false); };

    display.onSelectionChanged = [this] (int band) { bandPanel.showBand (band); };

    for (juce::Component* c : std::initializer_list<juce::Component*> { &presetBox, &prevPreset, &nextPreset, &phaseBox, &autoGain, &compare,
             &science, &matchButton, &output, &outputLabel, &display, &bandPanel, &spiritPanel })
        addAndMakeVisible (c);
    addChildComponent (sciencePanel);
    addChildComponent (matchPanel);

    setResizable (true, true);
    setResizeLimits (1000, 600, 1800, 1150); // narrower than 1000 px the top bar no longer fits
    setSize (1040, 680);
    startTimerHz (5); // keeps the preset box in sync with host-side preset changes
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PluginEditor::rebuildPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("Factory");
    const auto& factory = presets::factory();
    for (int i = 0; i < (int) factory.size(); ++i)
        presetBox.addItem (factory[(size_t) i].name, factoryIdBase + i);

    userPresets = presets::userPresetFiles();
    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("My presets");
        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i].getFileNameWithoutExtension(), userIdBase + i);
    }
    presetBox.addSeparator();
    presetBox.addItem ("Save preset...", saveId);
    presetBox.addItem ("Show presets folder", showFolderId);
    presetBox.setSelectedId (shownPresetId, juce::dontSendNotification);
}

void PluginEditor::presetChosen (int id)
{
    if (id >= factoryIdBase && id < userIdBase)
    {
        solfaProcessor.setCurrentProgram (id - factoryIdBase);
        lastHostProgram = solfaProcessor.getCurrentProgram();
        shownPresetId = id;
    }
    else if (id >= userIdBase && id < saveId && userPresets[id - userIdBase].existsAsFile())
    {
        if (auto xml = presets::readUserPreset (userPresets[id - userIdBase]); xml != nullptr && solfaProcessor.loadStateXml (*xml))
            shownPresetId = id;
    }
    else if (id == saveId)
    {
        presets::userPresetFolder().createDirectory();
        saveChooser = std::make_unique<juce::FileChooser> ("Save Solfa preset",
            presets::userPresetFolder().getChildFile ("My preset").withFileExtension (presets::userPresetExtension),
            juce::String ("*") + presets::userPresetExtension);
        juce::Component::SafePointer<PluginEditor> safe (this);
        saveChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
            [safe] (const juce::FileChooser& fc) {
                if (safe == nullptr || fc.getResult() == juce::File())
                    return;
                if (auto xml = safe->solfaProcessor.saveStateXml())
                    presets::writeUserPreset (fc.getResult(), *xml);
                safe->rebuildPresetBox();
                const auto saved = fc.getResult().withFileExtension (presets::userPresetExtension);
                for (int i = 0; i < safe->userPresets.size(); ++i)
                    if (safe->userPresets[i] == saved)
                        safe->shownPresetId = userIdBase + i;
                safe->presetBox.setSelectedId (safe->shownPresetId, juce::dontSendNotification);
            });
    }
    else if (id == showFolderId)
    {
        presets::userPresetFolder().createDirectory();
        presets::userPresetFolder().revealToUser();
    }
    presetBox.setSelectedId (shownPresetId, juce::dontSendNotification);
}

void PluginEditor::refreshPresetBox()
{
    // Follow preset changes made by the host (its own preset/program menu).
    if (solfaProcessor.getCurrentProgram() != lastHostProgram)
    {
        lastHostProgram = solfaProcessor.getCurrentProgram();
        shownPresetId = factoryIdBase + lastHostProgram;
    }
    if (presetBox.getSelectedId() != shownPresetId)
        presetBox.setSelectedId (shownPresetId, juce::dontSendNotification);
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
    presetBox.setBounds (top.removeFromLeft (180).reduced (4, 0));
    nextPreset.setBounds (top.removeFromLeft (28));
    top.removeFromLeft (12);
    phaseBox.setBounds (top.removeFromLeft (130));
    science.setBounds (top.removeFromRight (32));
    top.removeFromRight (8);
    matchButton.setBounds (top.removeFromRight (70));
    top.removeFromRight (8);
    output.setBounds (top.removeFromRight (124));
    outputLabel.setBounds (top.removeFromRight (46));
    top.removeFromRight (8);
    compare.setBounds (top.removeFromRight (90));
    top.removeFromRight (8);
    autoGain.setBounds (top.removeFromRight (100));

    auto bottom = r.removeFromBottom (juce::jlimit (170, 230, getHeight() / 3)).reduced (8, 6);
    bandPanel.setBounds (bottom.removeFromLeft (bottom.getWidth() * 11 / 20).reduced (0, 0).withTrimmedRight (4));
    spiritPanel.setBounds (bottom.withTrimmedLeft (4));

    display.setBounds (r.reduced (8, 0));
    matchPanel.setBounds (display.getBounds());
    sciencePanel.setBounds (getLocalBounds());
}
