#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Solfa's own visual identity: deep indigo night, warm gold curve, soft violet markers.
namespace solfa::ui::theme
{
    inline const juce::Colour background { 0xff14111f };
    inline const juce::Colour panel { 0xff1c1830 };
    inline const juce::Colour panelEdge { 0xff2e2850 };
    inline const juce::Colour grid { 0xff2a2540 };
    inline const juce::Colour gridText { 0xff6f6894 };
    inline const juce::Colour text { 0xffe8e4f5 };
    inline const juce::Colour dimText { 0xff9a93bd };
    inline const juce::Colour curve { 0xfff2c14e };      // warm gold
    inline const juce::Colour spectrum { 0xff4fb3a9 };   // muted teal
    inline const juce::Colour marker { 0xff8f7bd6 };     // soft violet
    inline const juce::Colour focus { 0xffff9f68 };      // amber
    inline const juce::Colour accent { 0xffb79cff };

    // One colour per band, cycling.
    inline juce::Colour band (int index)
    {
        static const juce::Colour palette[] = { juce::Colour (0xffff8a80), juce::Colour (0xffffc46b), juce::Colour (0xffd4e157),
            juce::Colour (0xff69f0ae), juce::Colour (0xff4dd0e1), juce::Colour (0xff82b1ff), juce::Colour (0xffb388ff),
            juce::Colour (0xfff48fb1) };
        return palette[index % 8];
    }

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel()
        {
            setColour (juce::ResizableWindow::backgroundColourId, background);
            setColour (juce::Slider::textBoxTextColourId, text);
            setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::rotarySliderFillColourId, curve);
            setColour (juce::Slider::rotarySliderOutlineColourId, grid);
            setColour (juce::Label::textColourId, dimText);
            setColour (juce::ComboBox::backgroundColourId, panel);
            setColour (juce::ComboBox::outlineColourId, panelEdge);
            setColour (juce::ComboBox::textColourId, text);
            setColour (juce::ComboBox::arrowColourId, dimText);
            setColour (juce::PopupMenu::backgroundColourId, panel);
            setColour (juce::PopupMenu::textColourId, text);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, panelEdge);
            setColour (juce::TextButton::buttonColourId, panel);
            setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.6f));
            setColour (juce::TextButton::textColourOffId, dimText);
            setColour (juce::TextButton::textColourOnId, text);
            setColour (juce::ToggleButton::textColourId, dimText);
            setColour (juce::ToggleButton::tickColourId, curve);
        }

        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end,
            juce::Slider& slider) override
        {
            const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
            const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
            const auto centre = bounds.getCentre();
            const auto angle = start + pos * (end - start);
            const auto bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
            const auto zeroAngle = bipolar ? start + (float) slider.valueToProportionOfLength (0.0) * (end - start) : start;

            juce::Path track;
            track.addCentredArc (centre.x, centre.y, radius - 2, radius - 2, 0.0f, start, end, true);
            g.setColour (grid);
            g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path value;
            value.addCentredArc (centre.x, centre.y, radius - 2, radius - 2, 0.0f, juce::jmin (zeroAngle, angle),
                juce::jmax (zeroAngle, angle), true);
            g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
            g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            const auto tip = centre.getPointOnCircumference (radius - 7, angle);
            g.setColour (text);
            g.drawLine ({ centre.getPointOnCircumference (radius * 0.25f, angle), tip }, 2.0f);
        }
    };
}
