#pragma once

#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>
#include "SolfaFonts.h"

// Solfa's own visual identity: a mystical jungle village at night. Indigo sky with an aurora,
// amethyst-stained wood, sacred-gold curve, aqua spirit-mist analyser, amethyst solfeggio markers,
// rose-flame Focus, spirit lights.
namespace solfa::ui::theme
{
    // Bundled typefaces (SIL Open Font License 1.1; licences in assets/fonts): Uncial Antiqua for the
    // logo and headings, Macondo for all other text. One shared copy for every editor instance.
    struct Fonts
    {
        static juce::Typeface::Ptr load (const char* data, int size)
        {
            return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        }
        juce::Typeface::Ptr display = load (SolfaFonts::UncialAntiquaRegular_ttf, SolfaFonts::UncialAntiquaRegular_ttfSize);
        juce::Typeface::Ptr text = load (SolfaFonts::MacondoRegular_ttf, SolfaFonts::MacondoRegular_ttfSize);
    };

    // Heading font (logo, panel titles). Body text uses Macondo automatically via the LookAndFeel.
    inline juce::FontOptions displayFont (float height)
    {
        return juce::FontOptions (juce::SharedResourcePointer<Fonts>()->display).withHeight (height);
    }

    inline const juce::Colour background { 0xff0a0816 }; // indigo night
    inline const juce::Colour panel { 0xff1b1329 };      // amethyst-stained wood
    inline const juce::Colour panelEdge { 0xff5c4683 };  // violet rim
    inline const juce::Colour grid { 0x26c8b8ff };       // faint lilac lines (translucent)
    inline const juce::Colour gridText { 0xffa497d2 };
    inline const juce::Colour text { 0xfff1eaff };       // moon-white
    inline const juce::Colour dimText { 0xffb3a6d4 };
    inline const juce::Colour curve { 0xffffcf6e };      // sacred gold
    inline const juce::Colour spectrum { 0xff5ee8e0 };   // spirit aqua
    inline const juce::Colour marker { 0xffb48cff };     // amethyst
    inline const juce::Colour focus { 0xffff5c9a };      // rose flame
    inline const juce::Colour accent { 0xffa8e6ff };     // moonlight
    inline const juce::Colour firefly { 0xff8ff7ff };

    // One colour per band, cycling: rose, gold, mint, aqua, sapphire, amethyst, orchid, coral.
    inline juce::Colour band (int index)
    {
        static const juce::Colour palette[] = { juce::Colour (0xffff8fd0), juce::Colour (0xffffc76b), juce::Colour (0xff9dffb0),
            juce::Colour (0xff5ee8e0), juce::Colour (0xff7fb2ff), juce::Colour (0xffa98bff), juce::Colour (0xffe07bff),
            juce::Colour (0xffff7f8a) };
        return palette[index % 8];
    }

    // A soft round glow (radial gradient) centred on a point.
    inline void glow (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour colour, float alpha)
    {
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (alpha), c.x, c.y, colour.withAlpha (0.0f), c.x + radius, c.y, true));
        g.fillEllipse (c.x - radius, c.y - radius, radius * 2, radius * 2);
    }

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel()
        {
            // JUCE resolves default (unnamed) fonts through the *default* LookAndFeel, so set Macondo
            // there as well as here; it then applies to labels, menus, combo boxes and tooltips.
            setDefaultSansSerifTypeface (fonts->text);
            juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface (fonts->text);

            setColour (juce::ResizableWindow::backgroundColourId, background);
            setColour (juce::Slider::textBoxTextColourId, text);
            setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour (juce::Slider::rotarySliderFillColourId, curve);
            setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black.withAlpha (0.5f));
            setColour (juce::Slider::thumbColourId, curve);
            setColour (juce::Label::textColourId, dimText);
            setColour (juce::ComboBox::backgroundColourId, panel.darker (0.3f));
            setColour (juce::ComboBox::outlineColourId, panelEdge);
            setColour (juce::ComboBox::textColourId, text);
            setColour (juce::ComboBox::arrowColourId, curve);
            setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff130e20));
            setColour (juce::PopupMenu::textColourId, text);
            setColour (juce::PopupMenu::headerTextColourId, curve);
            setColour (juce::PopupMenu::highlightedBackgroundColourId, panelEdge);
            setColour (juce::PopupMenu::highlightedTextColourId, text);
            setColour (juce::TextButton::buttonColourId, panel);
            setColour (juce::TextButton::buttonOnColourId, curve);
            setColour (juce::TextButton::textColourOffId, text.withAlpha (0.85f));
            setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1a0f2a));
            setColour (juce::ToggleButton::textColourId, dimText);
            setColour (juce::ToggleButton::tickColourId, curve);
            setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff130e20));
            setColour (juce::TooltipWindow::textColourId, text);
            setColour (juce::TooltipWindow::outlineColourId, panelEdge);
        }

        // Carved stone knob: rune dots around the rim, a glowing arc, a lit pointer bead.
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end,
            juce::Slider& slider) override
        {
            const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
            const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
            if (radius < 6.0f)
                return;
            const auto centre = bounds.getCentre();
            const auto angle = start + pos * (end - start);
            const auto bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
            const auto zeroAngle = bipolar ? start + (float) slider.valueToProportionOfLength (0.0) * (end - start) : start;
            const auto fill = slider.findColour (juce::Slider::rotarySliderFillColourId);
            const auto arcR = radius - 3.0f;
            const auto bodyR = radius * 0.66f;

            // Rune dots
            g.setColour (dimText.withAlpha (0.35f));
            for (int i = 0; i <= 10; ++i)
            {
                const auto p = centre.getPointOnCircumference (radius - 0.5f, start + (end - start) * (float) i / 10.0f);
                g.fillEllipse (p.x - 0.9f, p.y - 0.9f, 1.8f, 1.8f);
            }

            // Arc track and the value arc with a glow
            juce::Path track;
            track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, start, end, true);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            if (std::abs (angle - zeroAngle) > 1.0e-3f)
            {
                juce::Path value;
                value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, juce::jmin (zeroAngle, angle), juce::jmax (zeroAngle, angle), true);
                g.setColour (fill.withAlpha (0.22f));
                g.strokePath (value, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (fill);
                g.strokePath (value, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // Stone body
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillEllipse (centre.x - bodyR, centre.y - bodyR + 2.0f, bodyR * 2, bodyR * 2);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3f3659), centre.x - bodyR * 0.4f, centre.y - bodyR * 0.5f,
                juce::Colour (0xff110d1d), centre.x + bodyR, centre.y + bodyR, true));
            g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2, bodyR * 2);
            g.setColour (panelEdge.withAlpha (0.9f));
            g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2, bodyR * 2, 1.0f);
            g.setColour (fill.withAlpha (0.18f));
            g.drawEllipse (centre.x - bodyR * 0.62f, centre.y - bodyR * 0.62f, bodyR * 1.24f, bodyR * 1.24f, 0.8f);

            // Pointer bead
            const auto tip = centre.getPointOnCircumference (bodyR * 0.68f, angle);
            glow (g, tip, 6.0f, fill, 0.5f);
            g.setColour (fill.brighter (0.4f));
            g.fillEllipse (tip.x - 2.2f, tip.y - 2.2f, 4.4f, 4.4f);
        }

        void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
        {
            const auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
            g.setGradientFill (juce::ColourGradient (panel.brighter (0.05f), 0, 0, panel.darker (0.5f), 0, (float) height, false));
            g.fillRoundedRectangle (r, 4.0f);
            const auto hot = box.hasKeyboardFocus (true) || box.isMouseOver (true);
            g.setColour (hot ? curve.withAlpha (0.8f) : panelEdge);
            g.drawRoundedRectangle (r, 4.0f, 1.0f);

            // Small diamond instead of an arrow
            const auto cx = (float) width - 11.0f, cy = (float) height * 0.5f, d = 3.5f;
            juce::Path p;
            p.startNewSubPath (cx, cy - d);
            p.lineTo (cx + d, cy);
            p.lineTo (cx, cy + d);
            p.lineTo (cx - d, cy);
            p.closeSubPath();
            g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 0.9f : 0.3f));
            g.fillPath (p);
        }

        // Wooden plaques; "on" plaques glow gold.
        void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down) override
        {
            const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
            const auto on = b.getToggleState();
            if (on)
            {
                glow (g, r.getCentre(), r.getWidth() * 0.7f, curve, 0.25f);
                g.setGradientFill (juce::ColourGradient (curve.brighter (0.2f), 0, r.getY(), juce::Colour (0xffd2953c), 0, r.getBottom(), false));
            }
            else
                g.setGradientFill (juce::ColourGradient (panel.brighter (highlighted ? 0.25f : 0.1f), 0, r.getY(), panel.darker (0.5f), 0,
                    r.getBottom(), false));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (on ? curve.brighter (0.3f) : (highlighted ? curve.withAlpha (0.7f) : panelEdge));
            g.drawRoundedRectangle (r, 4.0f, down ? 1.6f : 1.0f);
        }

        // Toggle: a carved diamond that lights up.
        void drawTickBox (juce::Graphics& g, juce::Component& c, float x, float y, float w, float h, bool ticked, bool enabled, bool, bool) override
        {
            const auto cx = x + w * 0.5f, cy = y + h * 0.5f, d = juce::jmin (w, h) * 0.45f;
            juce::Path p;
            p.startNewSubPath (cx, cy - d);
            p.lineTo (cx + d, cy);
            p.lineTo (cx, cy + d);
            p.lineTo (cx - d, cy);
            p.closeSubPath();
            const auto colour = c.findColour (juce::ToggleButton::tickColourId).withMultipliedAlpha (enabled ? 1.0f : 0.4f);
            if (ticked)
            {
                glow (g, { cx, cy }, d * 2.6f, colour, 0.45f);
                g.setColour (colour);
                g.fillPath (p);
            }
            else
            {
                g.setColour (juce::Colours::black.withAlpha (0.4f));
                g.fillPath (p);
            }
            g.setColour (ticked ? colour.brighter (0.4f) : panelEdge.brighter (0.3f));
            g.strokePath (p, juce::PathStrokeType (1.0f));
        }

        // Horizontal sliders: a vine with a glowing bead.
        void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
            juce::Slider::SliderStyle style, juce::Slider& s) override
        {
            if (style != juce::Slider::LinearHorizontal)
            {
                LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);
                return;
            }
            const auto cy = (float) y + (float) h * 0.5f;
            const auto left = (float) x + 4.0f, right = (float) (x + w) - 4.0f;
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawLine (left, cy, right, cy, 3.5f);
            const auto bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
            const auto from = bipolar ? left + (right - left) * (float) s.valueToProportionOfLength (0.0) : left;
            const auto thumb = s.findColour (juce::Slider::thumbColourId);
            g.setColour (thumb.withAlpha (0.85f));
            g.drawLine (juce::jmin (from, pos), cy, juce::jmax (from, pos), cy, 2.2f);
            glow (g, { pos, cy }, 9.0f, thumb, 0.45f);
            g.setColour (thumb.brighter (0.3f));
            g.fillEllipse (pos - 4.0f, cy - 4.0f, 8.0f, 8.0f);
        }

    private:
        juce::SharedResourcePointer<Fonts> fonts;
    };
}
