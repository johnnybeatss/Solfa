#pragma once

#include "CurveModel.h"
#include "Spectrum.h"

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class PluginProcessor;

namespace solfa::ui
{
    // The main display: analyser, EQ curves, solfeggio markers and draggable band nodes.
    //
    //   drag node ........ frequency (x) and gain (y)
    //   wheel on node .... Q
    //   double-click ..... add a band (empty space) / remove a band (on a node)
    //   right-click node . type, slope, placement, dynamic, remove
    //   Alt-click ........ Spectrum Grab: add a band on the nearest spectral peak
    class EqDisplay : public juce::Component, private juce::Timer
    {
    public:
        explicit EqDisplay (PluginProcessor& processor);
        ~EqDisplay() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

        int getSelectedBand() const noexcept { return selected; }
        void setSelectedBand (int band);
        std::function<void (int)> onSelectionChanged;

        // Creates a band at a frequency/gain using the first free slot; returns it (or -1).
        int addBand (double frequencyHz, double gainDb, dsp::FilterType type = dsp::FilterType::bell);

        // Spectrum Grab: the frequency a click at x would grab.
        double grabFrequencyAt (float x) const;

        Spectrum& getSpectrum() noexcept { return spectrum; }
        void refreshNow(); // pull analyser data and rebuild the curve (also used by tests)

    private:
        void timerCallback() override;
        juce::Rectangle<float> plotArea() const;
        Mapping mapping() const;
        juce::Point<float> nodePosition (int band) const;
        int nodeAt (juce::Point<float> p) const;
        void setParam (const juce::String& id, float value);
        void showBandMenu (int band);

        PluginProcessor& processor;
        params::Handles handles;
        CurveModel model;
        Spectrum spectrum;
        std::vector<float> pullBuffer;

        int selected = -1;
        int dragging = -1;
        bool showGrab = false;
        juce::Point<float> mousePos;
    };
}
