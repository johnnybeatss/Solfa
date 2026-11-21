#pragma once

#include "../dsp/EqMatch.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class PluginProcessor;

namespace solfa::ui
{
    // EQ Match: capture a reference (audio file or live input) and the current material, then
    // fit ordinary Solfa bands to the difference. Shown as an overlay from the top bar.
    class MatchPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit MatchPanel (PluginProcessor& processor);
        ~MatchPanel() override;

        void paint (juce::Graphics&) override;
        void resized() override;
        void visibilityChanged() override;

        // Writes the fitted bands into the plugin; returns how many bands were written.
        int applyMatch();

        // For tests: feed captured audio directly.
        dsp::LongTermSpectrum& getReference() noexcept { return *reference; }
        dsp::LongTermSpectrum& getCurrent() noexcept { return *current; }
        bool hasBothSpectra() const noexcept { return reference->getNumFrames() > 2 && current->getNumFrames() > 2; }

    private:
        void timerCallback() override;
        void loadReferenceFile();
        double smoothingOctaves() const;
        std::vector<double> targetCurve() const;
        void setParam (const juce::String& id, float value);

        PluginProcessor& processor;
        std::shared_ptr<dsp::LongTermSpectrum> reference, current;

        // Cached fit for drawing (recomputed only when its inputs change).
        struct Fit
        {
            juce::String key;
            std::vector<double> target, response;
            int numBands = 0;
        } fitCache;
        const Fit& currentFit();
        std::vector<float> pullBuffer;
        juce::String referenceName { "none" };
        std::vector<int> matchBands; // slots written by the last Apply (re-used next time)
        std::unique_ptr<juce::FileChooser> chooser;
        bool loading = false;

        juce::TextButton loadButton { "Load reference file..." }, learnReference { "Learn reference" },
            learnCurrent { "Learn current" }, clearButton { "Clear" }, applyButton { "Apply match" }, doneButton { "Done" };
        juce::Slider amount { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
            maxBands { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
        juce::ComboBox smoothing;
        juce::Label amountLabel { {}, "Amount" }, bandsLabel { {}, "Max bands" }, smoothingLabel { {}, "Smoothing" };

        JUCE_DECLARE_WEAK_REFERENCEABLE (MatchPanel)
    };
}
