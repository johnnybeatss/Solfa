#include "EqDisplay.h"
#include "../PluginProcessor.h"
#include "../dsp/Tuning.h"
#include "Theme.h"

#include <cmath>

namespace solfa::ui
{
    namespace
    {
        constexpr std::array<double, 10> solfeggio { 174, 285, 396, 417, 432, 528, 639, 741, 852, 963 };
        constexpr double spectrumTopDb = 6.0, spectrumBottomDb = -84.0;

        juce::String formatHz (double f)
        {
            return f < 1000.0 ? juce::String (juce::roundToInt (f)) + " Hz" : juce::String (f / 1000.0, 2) + " kHz";
        }

        juce::String noteName (double f, double reference)
        {
            static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            const auto note = (int) std::lround (69.0 + 12.0 * std::log2 (f / reference));
            const auto cents = dsp::centsFromNearestNote (f, reference);
            auto s = juce::String (names[((note % 12) + 12) % 12]) + juce::String (note / 12 - 1);
            if (std::abs (cents) >= 0.5)
                s << (cents > 0 ? " +" : " ") << juce::roundToInt (cents) << "c";
            return s;
        }
    }

    EqDisplay::EqDisplay (PluginProcessor& p)
        : processor (p), handles (p.getState()), pullBuffer ((size_t) dsp::AnalyzerFifo::capacity)
    {
        setOpaque (true);
        startTimerHz (30);
        refreshNow();
    }

    EqDisplay::~EqDisplay() { stopTimer(); }

    void EqDisplay::refreshNow()
    {
        const auto fs = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
        const auto n = processor.getAnalyzerFifo().pull (pullBuffer.data(), (int) pullBuffer.size());
        spectrum.setSampleRate (fs);
        spectrum.push (pullBuffer.data(), n);
        spectrum.update (1.0 / 30.0);
        model.update (handles, fs, processor.getEffectiveFocusHz());
    }

    void EqDisplay::timerCallback()
    {
        refreshNow();
        repaint();
    }

    void EqDisplay::resized() {}

    juce::Rectangle<float> EqDisplay::plotArea() const
    {
        return getLocalBounds().toFloat().reduced (34.0f, 22.0f).withTrimmedTop (12.0f);
    }

    Mapping EqDisplay::mapping() const
    {
        const auto area = plotArea();
        return { area.getWidth(), area.getHeight(), 18.0 };
    }

    juce::Point<float> EqDisplay::nodePosition (int band) const
    {
        const auto area = plotArea();
        const auto m = mapping();
        const auto& t = model.getTarget (band);
        const auto f = juce::jlimit (Mapping::minHz, Mapping::maxHz, t.frequencyHz);
        // Gain bands sit at their gain; other shapes sit on their own curve at the band frequency.
        const auto d = dsp::isGainTypeFor (t.type) ? t.gainDb : juce::jlimit (-18.0, 18.0, model.bandDb (band, f));
        return { area.getX() + m.xForFrequency (f), area.getY() + m.yForDb (d) };
    }

    int EqDisplay::nodeAt (juce::Point<float> p) const
    {
        int best = -1;
        float bestDistance = 11.0f;
        for (int b = 0; b < dsp::numBands; ++b)
        {
            if (! model.isEnabled (b))
                continue;
            const auto d = nodePosition (b).getDistanceFrom (p);
            if (d < bestDistance)
            {
                best = b;
                bestDistance = d;
            }
        }
        return best;
    }

    double EqDisplay::grabFrequencyAt (float x) const
    {
        const auto f = mapping().frequencyForX (x - plotArea().getX());
        return spectrum.findPeakNear (juce::jlimit (Mapping::minHz, Mapping::maxHz, f));
    }

    void EqDisplay::setSelectedBand (int band)
    {
        if (band == selected)
            return;
        selected = band;
        if (onSelectionChanged)
            onSelectionChanged (band);
        repaint();
    }

    void EqDisplay::setParam (const juce::String& id, float value)
    {
        if (auto* p = processor.getState().getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
    }

    int EqDisplay::addBand (double frequencyHz, double gainDb, dsp::FilterType type)
    {
        for (int b = 0; b < dsp::numBands; ++b)
        {
            if (handles.bands[(size_t) b].on->load() >= 0.5f)
                continue;
            using namespace params;
            setParam (bandId (b, params::type), (float) type);
            setParam (bandId (b, freq), (float) juce::jlimit (20.0, 20000.0, frequencyHz));
            setParam (bandId (b, gain), (float) juce::jlimit (-18.0, 18.0, gainDb));
            setParam (bandId (b, q), 1.0f);
            setParam (bandId (b, slope), (float) dsp::defaultSlopeIndex);
            setParam (bandId (b, place), 0.0f);
            setParam (bandId (b, dyn), 0.0f);
            setParam (bandId (b, on), 1.0f);
            refreshNow();
            setSelectedBand (b);
            return b;
        }
        return -1;
    }

    //==============================================================================
    void EqDisplay::paint (juce::Graphics& g)
    {
        g.fillAll (theme::background);
        const auto area = plotArea();
        const auto m = mapping();
        auto xOf = [&] (double f) { return area.getX() + m.xForFrequency (f); };
        auto yOf = [&] (double d) { return area.getY() + m.yForDb (d); };

        // Grid
        g.setFont (juce::FontOptions (10.0f));
        for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
        {
            g.setColour (theme::grid);
            g.drawVerticalLine (juce::roundToInt (xOf (f)), area.getY(), area.getBottom());
            g.setColour (theme::gridText);
            const auto label = f < 1000.0 ? juce::String ((int) f) : juce::String ((int) (f / 1000.0)) + "k";
            g.drawText (label, juce::Rectangle<float> (xOf (f) - 20, area.getBottom() + 3, 40, 12), juce::Justification::centred);
        }
        for (int d = -18; d <= 18; d += 6)
        {
            g.setColour (d == 0 ? theme::grid.brighter (0.3f) : theme::grid);
            g.drawHorizontalLine (juce::roundToInt (yOf (d)), area.getX(), area.getRight());
            g.setColour (theme::gridText);
            g.drawText ((d > 0 ? "+" : "") + juce::String (d), juce::Rectangle<float> (area.getX() - 32, yOf (d) - 6, 28, 12),
                juce::Justification::centredRight);
        }

        // Spectrum (filled)
        {
            juce::Path spec;
            spec.startNewSubPath (area.getX(), area.getBottom());
            for (float x = 0; x <= area.getWidth(); x += 2.0f)
            {
                const auto level = spectrum.levelDbAt (m.frequencyForX (x));
                const auto t = juce::jlimit (0.0, 1.0, (level - spectrumBottomDb) / (spectrumTopDb - spectrumBottomDb));
                spec.lineTo (area.getX() + x, area.getBottom() - (float) t * area.getHeight());
            }
            spec.lineTo (area.getRight(), area.getBottom());
            spec.closeSubPath();
            g.setGradientFill (juce::ColourGradient (theme::spectrum.withAlpha (0.35f), 0, area.getY(),
                theme::spectrum.withAlpha (0.05f), 0, area.getBottom(), false));
            g.fillPath (spec);
        }

        // Solfeggio markers and the Focus
        const auto refIndex = juce::jlimit (0, 2, (int) handles.tuneRef->load());
        const auto reference = dsp::referenceOptionsHz[(size_t) refIndex];
        for (auto f : solfeggio)
        {
            const auto x = xOf (f);
            g.setColour (theme::marker.withAlpha (0.35f));
            const float dashes[] = { 3.0f, 4.0f };
            g.drawDashedLine ({ x, area.getY(), x, area.getBottom() }, dashes, 2, 1.0f);
        }
        g.setColour (theme::marker.withAlpha (0.8f));
        g.setFont (juce::FontOptions (9.0f));
        {
            // Place each label on the first of three rows where it doesn't overlap its neighbour.
            std::array<float, 3> rowEnd { -1.0e9f, -1.0e9f, -1.0e9f };
            for (auto f : solfeggio)
            {
                const auto x = xOf (f);
                size_t row = 0;
                while (row + 1 < rowEnd.size() && x - 12 < rowEnd[row] + 2)
                    ++row;
                rowEnd[row] = x + 12;
                g.drawText (juce::String ((int) f), juce::Rectangle<float> (x - 12, 1 + (float) row * 9.0f, 24, 10),
                    juce::Justification::centred);
            }
        }
        {
            const auto focus = processor.getEffectiveFocusHz();
            const auto x = xOf (focus);
            g.setColour (theme::focus.withAlpha (0.8f));
            g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());
            g.setFont (juce::FontOptions (10.0f));
            g.drawText ("Focus " + formatHz (focus) + "  " + noteName (focus, reference),
                juce::Rectangle<float> (x + 4, area.getBottom() - 16, 160, 12), juce::Justification::centredLeft);
        }

        // Per-band fills and dynamic swing
        auto curvePath = [&] (auto&& dbAt) {
            juce::Path p;
            bool first = true;
            for (float x = 0; x <= area.getWidth(); x += 2.0f)
            {
                const auto y = yOf (juce::jlimit (-24.0, 24.0, dbAt (m.frequencyForX (x))));
                if (first)
                    p.startNewSubPath (area.getX(), y);
                else
                    p.lineTo (area.getX() + x, y);
                first = false;
            }
            return p;
        };
        g.saveState();
        g.reduceClipRegion (area.toNearestInt());
        for (int b = 0; b < dsp::numBands; ++b)
        {
            if (! model.isEnabled (b))
                continue;
            auto p = curvePath ([&] (double f) { return model.bandDb (b, f); });
            auto fill = p;
            fill.lineTo (area.getRight(), yOf (0));
            fill.lineTo (area.getX(), yOf (0));
            fill.closeSubPath();
            g.setColour (theme::band (b).withAlpha (b == selected ? 0.22f : 0.10f));
            g.fillPath (fill);
            g.setColour (theme::band (b).withAlpha (0.55f));
            g.strokePath (p, juce::PathStrokeType (1.0f));

            const auto& t = model.getTarget (b);
            if (t.dynamic && dsp::isGainTypeFor (t.type))
            {
                auto swing = curvePath ([&] (double f) { return model.bandDynamicDb (b, f); });
                juce::Path dashed;
                const float dashes[] = { 4.0f, 3.0f };
                juce::PathStrokeType (1.2f).createDashedStroke (dashed, swing, dashes, 2);
                g.setColour (theme::band (b).withAlpha (0.8f));
                g.fillPath (dashed);
            }
        }

        // Total curve
        g.setColour (theme::curve);
        g.strokePath (curvePath ([&] (double f) { return model.totalDb (f); }),
            juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.restoreState();

        // Nodes
        for (int b = 0; b < dsp::numBands; ++b)
        {
            if (! model.isEnabled (b))
                continue;
            const auto pos = nodePosition (b);
            const auto r = b == selected ? 8.0f : 6.5f;
            g.setColour (theme::band (b));
            g.fillEllipse (pos.x - r, pos.y - r, 2 * r, 2 * r);
            g.setColour (theme::background);
            g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText (juce::String (b + 1), juce::Rectangle<float> (pos.x - r, pos.y - r, 2 * r, 2 * r), juce::Justification::centred);
            if (b == selected)
            {
                g.setColour (theme::text);
                g.drawEllipse (pos.x - r - 3, pos.y - r - 3, 2 * r + 6, 2 * r + 6, 1.2f);
                const auto& t = model.getTarget (b);
                juce::String info = formatHz (t.frequencyHz);
                if (dsp::isGainTypeFor (t.type))
                    info << "  " << (t.gainDb > 0 ? "+" : "") << juce::String (t.gainDb, 1) << " dB";
                info << "  Q " << juce::String (t.q, 2);
                g.setFont (juce::FontOptions (11.0f));
                g.drawText (info, juce::Rectangle<float> (pos.x + 12, pos.y - 22, 200, 14), juce::Justification::centredLeft);
            }
        }

        // Spectrum Grab preview
        if (showGrab && area.contains (mousePos))
        {
            const auto f = grabFrequencyAt (mousePos.x);
            const auto x = xOf (f);
            g.setColour (theme::spectrum);
            g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());
            g.setFont (juce::FontOptions (11.0f));
            g.drawText ("Grab " + formatHz (f), juce::Rectangle<float> (x + 4, area.getY() + 4, 120, 14), juce::Justification::centredLeft);
        }

        g.setColour (theme::panelEdge);
        g.drawRect (area, 1.0f);
    }

    //==============================================================================
    void EqDisplay::mouseDown (const juce::MouseEvent& e)
    {
        mousePos = e.position;
        const auto band = nodeAt (e.position);

        if (e.mods.isPopupMenu())
        {
            if (band >= 0)
            {
                setSelectedBand (band);
                showBandMenu (band);
            }
            return;
        }

        if (band >= 0)
        {
            setSelectedBand (band);
            dragging = band;
            for (auto* suffix : { params::freq, params::gain })
                if (auto* p = processor.getState().getParameter (params::bandId (band, suffix)))
                    p->beginChangeGesture();
            return;
        }

        if (e.mods.isAltDown() && plotArea().contains (e.position))
        {
            addBand (grabFrequencyAt (e.position.x), -3.0);
            return;
        }

        setSelectedBand (-1);
    }

    void EqDisplay::mouseDrag (const juce::MouseEvent& e)
    {
        if (dragging < 0)
            return;
        const auto area = plotArea();
        const auto m = mapping();
        const auto f = juce::jlimit (20.0, 20000.0, m.frequencyForX (e.position.x - area.getX()));
        const auto d = juce::jlimit (-18.0, 18.0, m.dbForY (e.position.y - area.getY()));

        if (auto* p = processor.getState().getParameter (params::bandId (dragging, params::freq)))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) f));
        if (dsp::isGainTypeFor (model.getTarget (dragging).type))
            if (auto* p = processor.getState().getParameter (params::bandId (dragging, params::gain)))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) d));
        refreshNow();
        repaint();
    }

    void EqDisplay::mouseUp (const juce::MouseEvent&)
    {
        if (dragging >= 0)
            for (auto* suffix : { params::freq, params::gain })
                if (auto* p = processor.getState().getParameter (params::bandId (dragging, suffix)))
                    p->endChangeGesture();
        dragging = -1;
    }

    void EqDisplay::mouseMove (const juce::MouseEvent& e)
    {
        mousePos = e.position;
        showGrab = e.mods.isAltDown();
        if (showGrab)
            repaint();
    }

    void EqDisplay::mouseExit (const juce::MouseEvent&)
    {
        showGrab = false;
        repaint();
    }

    void EqDisplay::mouseDoubleClick (const juce::MouseEvent& e)
    {
        const auto band = nodeAt (e.position);
        if (band >= 0)
        {
            setParam (params::bandId (band, params::on), 0.0f);
            setSelectedBand (-1);
            return;
        }
        const auto area = plotArea();
        if (! area.contains (e.position))
            return;
        const auto m = mapping();
        addBand (m.frequencyForX (e.position.x - area.getX()), std::round (m.dbForY (e.position.y - area.getY()) * 2.0) / 2.0);
    }

    void EqDisplay::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        auto band = nodeAt (e.position);
        if (band < 0)
            band = selected;
        if (band < 0)
            return;
        const auto q = handles.bands[(size_t) band].q->load();
        setParam (params::bandId (band, params::q), (float) juce::jlimit (0.1, 18.0, q * std::exp2 (wheel.deltaY * 2.0)));
        refreshNow();
        repaint();
    }

    void EqDisplay::showBandMenu (int band)
    {
        using namespace params;
        const auto& t = model.getTarget (band);
        juce::PopupMenu menu, types, slopes, places;
        const char* typeNames[] = { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Tilt", "Notch", "Band Pass" };
        for (int i = 0; i < dsp::numFilterTypes; ++i)
            types.addItem (100 + i, typeNames[i], true, (int) t.type == i);
        for (int i = 0; i < (int) dsp::slopeOptionsDbPerOct.size(); ++i)
            slopes.addItem (200 + i, juce::String (dsp::slopeOptionsDbPerOct[(size_t) i]) + " dB/oct", true,
                t.slopeDbPerOct == dsp::slopeOptionsDbPerOct[(size_t) i]);
        const char* placeNames[] = { "Stereo", "Left", "Right", "Mid", "Side" };
        for (int i = 0; i < dsp::numPlacements; ++i)
            places.addItem (300 + i, placeNames[i], true, (int) t.placement == i);

        menu.addSectionHeader ("Band " + juce::String (band + 1));
        menu.addSubMenu ("Shape", types);
        menu.addSubMenu ("Slope", slopes, t.type == dsp::FilterType::lowCut || t.type == dsp::FilterType::highCut);
        menu.addSubMenu ("Placement", places);
        menu.addItem (400, "Dynamic", dsp::isGainTypeFor (t.type), t.dynamic);
        menu.addSeparator();
        menu.addItem (500, "Remove band");

        juce::Component::SafePointer<EqDisplay> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe, band, t] (int result) {
            if (safe == nullptr || result == 0)
                return;
            auto& self = *safe;
            if (result >= 100 && result < 200)
                self.setParam (bandId (band, type), (float) (result - 100));
            else if (result >= 200 && result < 300)
                self.setParam (bandId (band, slope), (float) (result - 200));
            else if (result >= 300 && result < 400)
                self.setParam (bandId (band, place), (float) (result - 300));
            else if (result == 400)
                self.setParam (bandId (band, dyn), t.dynamic ? 0.0f : 1.0f);
            else if (result == 500)
            {
                self.setParam (bandId (band, on), 0.0f);
                self.setSelectedBand (-1);
            }
            self.refreshNow();
            self.repaint();
        });
    }
}
