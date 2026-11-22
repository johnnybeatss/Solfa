#pragma once

#include "../Parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

class PluginProcessor;

namespace solfa::ui
{
    using APVTS = juce::AudioProcessorValueTreeState;

    // A labelled rotary knob attached to one parameter (re-attachable).
    struct Knob : juce::Component
    {
        explicit Knob (const juce::String& labelText);
        void attach (APVTS& state, const juce::String& paramId);
        void detach() { attachment.reset(); }
        void resized() override;

        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    // A labelled combo box attached to one choice parameter (re-attachable).
    struct Choice : juce::Component
    {
        Choice (const juce::String& labelText, const juce::StringArray& items);
        void attach (APVTS& state, const juce::String& paramId);
        void detach() { attachment.reset(); }
        void resized() override;

        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
    };

    // Height of a panel's title row; the carved strip sits just below it.
    inline constexpr float headerHeight = 28.0f;

    // Controls for the band selected in the display.
    class BandPanel : public juce::Component
    {
    public:
        explicit BandPanel (APVTS& state);
        void showBand (int band); // -1 = none
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        APVTS& state;
        int band = -1;
        juce::ToggleButton on { "On" }, dynamic { "Dynamic" };
        std::unique_ptr<APVTS::ButtonAttachment> onAttachment, dynamicAttachment;
        Choice type, slope, place;
        Knob freq { "Freq" }, gain { "Gain" }, q { "Q" };
        Knob threshold { "Threshold" }, range { "Range" }, attack { "Attack" }, release { "Release" };
    };

    // Focus, Key Snap, Literal, Resonance and Amount.
    class SpiritPanel : public juce::Component, private juce::Timer
    {
    public:
        SpiritPanel (PluginProcessor& processor);
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;

        PluginProcessor& processor;
        Knob amount { "Amount" }, focus { "Focus" }, literal { "Literal" }, resonance { "Resonance" }, decay { "Decay" };
        Choice reference, key, scale;
        juce::ToggleButton pure { "Pure" };
        std::unique_ptr<APVTS::ButtonAttachment> pureAttachment;
        juce::Label focusReadout;
    };

    // "About the science": what the evidence does and doesn't support.
    class SciencePanel : public juce::Component
    {
    public:
        SciencePanel();
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override { setVisible (false); }
    };
}
