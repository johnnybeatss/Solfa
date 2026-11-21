#include "Panels.h"
#include "../PluginProcessor.h"
#include "../dsp/Tuning.h"
#include "Theme.h"

#include <cmath>

namespace solfa::ui
{
    //==============================================================================
    Knob::Knob (const juce::String& labelText)
    {
        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (11.0f));
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 14);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxTextColourId, theme::text);
        addAndMakeVisible (slider);
        addAndMakeVisible (label);
    }

    void Knob::attach (APVTS& state, const juce::String& paramId)
    {
        attachment.reset();
        attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId, slider);
    }

    void Knob::resized()
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (14));
        slider.setBounds (r);
    }

    Choice::Choice (const juce::String& labelText, const juce::StringArray& items)
    {
        label.setText (labelText, juce::dontSendNotification);
        label.setFont (juce::FontOptions (11.0f));
        box.addItemList (items, 1);
        addAndMakeVisible (box);
        addAndMakeVisible (label);
    }

    void Choice::attach (APVTS& state, const juce::String& paramId)
    {
        attachment.reset();
        attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId, box);
    }

    void Choice::resized()
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (14));
        box.setBounds (r.removeFromTop (22));
    }

    //==============================================================================
    BandPanel::BandPanel (APVTS& s)
        : state (s),
          type ("Shape", { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Tilt", "Notch", "Band Pass" }),
          slope ("Slope", { "6 dB", "12 dB", "18 dB", "24 dB", "36 dB", "48 dB", "72 dB", "96 dB" }),
          place ("Placement", { "Stereo", "Left", "Right", "Mid", "Side" })
    {
        for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &dynamic, &type, &slope, &place, &freq, &gain, &q,
                 &threshold, &range, &attack, &release })
            addChildComponent (c);
        showBand (-1);
    }

    void BandPanel::showBand (int newBand)
    {
        band = newBand;
        onAttachment.reset();
        dynamicAttachment.reset();
        for (auto* k : { &freq, &gain, &q, &threshold, &range, &attack, &release })
            k->detach();
        for (auto* c : { &type, &slope, &place })
            c->detach();

        const bool visible = band >= 0;
        for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &dynamic, &type, &slope, &place, &freq, &gain, &q,
                 &threshold, &range, &attack, &release })
            c->setVisible (visible);

        if (visible)
        {
            using namespace params;
            onAttachment = std::make_unique<APVTS::ButtonAttachment> (state, bandId (band, params::on), on);
            dynamicAttachment = std::make_unique<APVTS::ButtonAttachment> (state, bandId (band, dyn), dynamic);
            type.attach (state, bandId (band, params::type));
            slope.attach (state, bandId (band, params::slope));
            place.attach (state, bandId (band, params::place));
            freq.attach (state, bandId (band, params::freq));
            gain.attach (state, bandId (band, params::gain));
            q.attach (state, bandId (band, params::q));
            threshold.attach (state, bandId (band, thr));
            range.attach (state, bandId (band, params::range));
            attack.attach (state, bandId (band, atk));
            release.attach (state, bandId (band, rel));
            for (auto* k : { &freq, &gain, &q, &threshold, &range, &attack, &release })
                k->slider.setColour (juce::Slider::rotarySliderFillColourId, theme::band (band));
        }
        repaint();
    }

    void BandPanel::paint (juce::Graphics& g)
    {
        g.setColour (theme::panel);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (band >= 0 ? theme::band (band) : theme::dimText);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (band >= 0 ? "Band " + juce::String (band + 1) : "Band", getLocalBounds().reduced (10, 6).removeFromTop (18),
            juce::Justification::centredLeft);
        if (band < 0)
        {
            g.setColour (theme::dimText);
            g.setFont (juce::FontOptions (12.0f));
            g.drawFittedText ("Double-click the display to add a band. Drag a node to move it, use the wheel for Q, "
                              "right-click for more, Alt-click to grab a spectrum peak.",
                getLocalBounds().reduced (14, 30), juce::Justification::topLeft, 4);
        }
    }

    void BandPanel::resized()
    {
        auto r = getLocalBounds().reduced (10, 6);
        auto header = r.removeFromTop (20);
        header.removeFromLeft (70);
        on.setBounds (header.removeFromLeft (50));
        dynamic.setBounds (header.removeFromLeft (80));

        auto left = r.removeFromLeft (r.getWidth() * 3 / 7);
        auto combos = left.removeFromTop (38);
        const auto cw = combos.getWidth() / 3;
        type.setBounds (combos.removeFromLeft (cw).reduced (2, 0));
        slope.setBounds (combos.removeFromLeft (cw).reduced (2, 0));
        place.setBounds (combos.reduced (2, 0));
        const auto kw = left.getWidth() / 3;
        for (auto* k : { &freq, &gain, &q })
            k->setBounds (left.removeFromLeft (kw));

        r.removeFromLeft (8);
        const auto dw = r.getWidth() / 4;
        for (auto* k : { &threshold, &range, &attack, &release })
            k->setBounds (r.removeFromLeft (dw).withTrimmedTop (12));
    }

    //==============================================================================
    SpiritPanel::SpiritPanel (PluginProcessor& p)
        : processor (p),
          reference ("Tuning", { "A = 440 Hz", "A = 432 Hz", "A = 444 Hz" }),
          key ("Key", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }),
          scale ("Scale", { "Chromatic", "Major", "Minor" })
    {
        auto& s = processor.getState();
        amount.attach (s, params::amount);
        focus.attach (s, params::focus);
        literal.attach (s, params::literal);
        resonance.attach (s, params::resAmount);
        decay.attach (s, params::resDecay);
        reference.attach (s, params::tuneRef);
        key.attach (s, params::key);
        scale.attach (s, params::scale);
        pureAttachment = std::make_unique<APVTS::ButtonAttachment> (s, params::pure, pure);
        focus.slider.setColour (juce::Slider::rotarySliderFillColourId, theme::focus);
        resonance.slider.setColour (juce::Slider::rotarySliderFillColourId, theme::accent);
        decay.slider.setColour (juce::Slider::rotarySliderFillColourId, theme::accent);

        focusReadout.setJustificationType (juce::Justification::centredLeft);
        focusReadout.setColour (juce::Label::textColourId, theme::focus);
        for (juce::Component* c : std::initializer_list<juce::Component*> { &amount, &focus, &literal, &resonance, &decay, &reference,
                 &key, &scale, &pure, &focusReadout })
            addAndMakeVisible (c);
        startTimerHz (10);
    }

    void SpiritPanel::timerCallback()
    {
        const auto f = processor.getEffectiveFocusHz();
        const auto ref = dsp::referenceOptionsHz[(size_t) juce::jlimit (0, 2, (int) processor.getState().getRawParameterValue (params::tuneRef)->load())];
        const auto cents = dsp::centsFromNearestNote (f, ref);
        focusReadout.setText ("Rings at " + juce::String (f, 2) + " Hz (" + (std::abs (cents) < 0.5 ? juce::String ("in tune")
                                  : juce::String (juce::roundToInt (cents)) + " cents)"), juce::dontSendNotification);
    }

    void SpiritPanel::paint (juce::Graphics& g)
    {
        g.setColour (theme::panel);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (theme::focus);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("Focus & Resonance", getLocalBounds().reduced (10, 6).removeFromTop (18), juce::Justification::centredLeft);
    }

    void SpiritPanel::resized()
    {
        auto r = getLocalBounds().reduced (10, 6);
        auto header = r.removeFromTop (20);
        header.removeFromLeft (150);
        focusReadout.setBounds (header);

        auto combos = r.removeFromTop (38);
        const auto cw = (combos.getWidth() - 60) / 3;
        reference.setBounds (combos.removeFromLeft (cw).reduced (2, 0));
        key.setBounds (combos.removeFromLeft (cw).reduced (2, 0));
        scale.setBounds (combos.removeFromLeft (cw).reduced (2, 0));
        pure.setBounds (combos.withTrimmedTop (14));

        const auto kw = r.getWidth() / 5;
        for (auto* k : { &focus, &literal, &resonance, &decay, &amount })
            k->setBounds (r.removeFromLeft (kw));
    }

    //==============================================================================
    SciencePanel::SciencePanel() { setInterceptsMouseClicks (true, false); }

    void SciencePanel::paint (juce::Graphics& g)
    {
        g.fillAll (theme::background.withAlpha (0.94f));
        auto r = getLocalBounds().reduced (40, 30);
        g.setColour (theme::curve);
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText ("About the science", r.removeFromTop (28), juce::Justification::centredLeft);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (13.0f));
        const juce::String text =
            "Solfa's presets are named after the popular solfeggio and 432 Hz numbers. They shape timbre: warmth, clarity, "
            "air and weight. Solfa makes no health or therapeutic claims.\n\n"
            "What the evidence supports: broad EQ moves change how bright or dark a sound feels, and darker sounds tend to feel "
            "calmer while brighter sounds feel more energetic. Louder also tends to sound \"better\", which is why Auto Gain and "
            "Compare keep the level steady while you judge the tone.\n\n"
            "What it does not support: the solfeggio set (174-963 Hz) comes from 1990s numerology, not medieval chant. Studies of "
            "432 Hz and 528 Hz music are small pilot studies, and an EQ boost is not the same as retuning a song.\n\n"
            "How Solfa uses the numbers: the presets work on each number's octaves, where the ear notices them, Key Snap keeps "
            "the Focus in tune with your song, and Resonance only sustains what is already in your music.\n\n"
            "Full sources: docs/research/report.md in the Solfa repository.\n\nClick anywhere to close.";
        g.drawFittedText (text, r, juce::Justification::topLeft, 40, 1.0f);
    }
}
