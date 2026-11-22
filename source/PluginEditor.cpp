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
    background.draw (g, getLocalBounds(), [] (juce::Graphics& bg, juce::Rectangle<float> r) {
        namespace theme = ui::theme;
        // Jungle floor behind the panels
        bg.setGradientFill (juce::ColourGradient (juce::Colour (0xff110c24), 0, r.getY(), juce::Colour (0xff05040e), 0, r.getBottom(), false));
        bg.fillRect (r);

        // Top bar: a carved wooden beam
        const auto beam = r.removeFromTop ((float) topBarHeight);
        bg.setGradientFill (juce::ColourGradient (theme::panel.brighter (0.2f), 0, beam.getY(), theme::panel.darker (0.45f), 0,
            beam.getBottom(), false));
        bg.fillRect (beam);
        juce::Random rng (5);
        for (float y = beam.getY() + 4.0f; y < beam.getBottom() - 6.0f; y += 5.0f + 4.0f * rng.nextFloat())
        {
            juce::Path grain;
            grain.startNewSubPath (beam.getX(), y);
            for (float x = 60.0f; x < beam.getRight() + 60.0f; x += 60.0f)
                grain.quadraticTo (x - 30.0f, y + (rng.nextFloat() - 0.5f) * 3.0f, x, y);
            bg.setColour (juce::Colour (0xffe6d8ff).withAlpha (0.025f));
            bg.strokePath (grain, juce::PathStrokeType (1.0f));
        }
        bg.setColour (theme::panelEdge);
        bg.drawHorizontalLine ((int) beam.getBottom() - 1, beam.getX(), beam.getRight());
        ui::scenery::drawCarvedBand (bg, { beam.getX() + 8.0f, beam.getBottom() - 6.0f, beam.getWidth() - 16.0f, 3.5f },
            theme::curve.withAlpha (0.25f));

        // Logo: seed of life with a warm glow, and the name in gold
        const juce::Point<float> logo { 27.0f, 23.0f };
        theme::glow (bg, logo, 26.0f, theme::curve, 0.3f);
        ui::scenery::drawSeedOfLife (bg, logo, 13.0f, theme::curve, 1.2f);
        bg.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff0c8), 0, 8.0f, juce::Colour (0xffd99a45), 0, 30.0f, false));
        bg.setFont (solfa::ui::theme::displayFont (23.0f).withKerningFactor (0.12f));
        bg.drawText ("SOLFA", juce::Rectangle<float> (47, 5, 100, 26), juce::Justification::centredLeft);
        bg.setColour (theme::dimText);
        bg.setFont (juce::FontOptions (10.5f).withKerningFactor (0.1f));
        bg.drawText (juce::String ("v") + VERSION, juce::Rectangle<float> (48, 29, 90, 11), juce::Justification::centredLeft);
    });
}

void PluginEditor::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (topBarHeight).reduced (8, 9);
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
