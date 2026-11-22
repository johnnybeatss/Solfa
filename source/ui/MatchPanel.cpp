#include "MatchPanel.h"
#include "../PluginProcessor.h"
#include "CurveModel.h"
#include "Scenery.h"
#include "Theme.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace solfa::ui
{
    namespace
    {
        double sampleRateOf (const PluginProcessor& p) { return p.getSampleRate() > 0.0 ? p.getSampleRate() : 48000.0; }

        std::vector<double> centred (const std::vector<double>& f, std::vector<double> c)
        {
            double sum = 0;
            int n = 0;
            for (size_t i = 0; i < f.size(); ++i)
                if (f[i] >= 100.0 && f[i] <= 10000.0)
                {
                    sum += c[i];
                    ++n;
                }
            for (auto& v : c)
                v -= n > 0 ? sum / n : 0.0;
            return c;
        }
    }

    MatchPanel::MatchPanel (PluginProcessor& p)
        : processor (p),
          reference (std::make_shared<dsp::LongTermSpectrum> (sampleRateOf (p))),
          current (std::make_shared<dsp::LongTermSpectrum> (sampleRateOf (p))),
          pullBuffer ((size_t) dsp::AnalyzerFifo::capacity)
    {
        learnReference.setClickingTogglesState (true);
        learnCurrent.setClickingTogglesState (true);
        learnReference.setTooltip ("Play the reference track through Solfa while this is on");
        learnCurrent.setTooltip ("Play your own track through Solfa while this is on");

        loadButton.onClick = [this] { loadReferenceFile(); };
        learnReference.onClick = [this] {
            if (learnReference.getToggleState())
            {
                reference = std::make_shared<dsp::LongTermSpectrum> (sampleRateOf (processor));
                referenceName = "live input";
                learnCurrent.setToggleState (false, juce::dontSendNotification);
            }
        };
        learnCurrent.onClick = [this] {
            if (learnCurrent.getToggleState())
            {
                current = std::make_shared<dsp::LongTermSpectrum> (sampleRateOf (processor));
                learnReference.setToggleState (false, juce::dontSendNotification);
            }
        };
        clearButton.onClick = [this] {
            reference = std::make_shared<dsp::LongTermSpectrum> (sampleRateOf (processor));
            current = std::make_shared<dsp::LongTermSpectrum> (sampleRateOf (processor));
            referenceName = "none";
            repaint();
        };
        applyButton.onClick = [this] { applyMatch(); };
        doneButton.onClick = [this] { setVisible (false); };

        amount.setRange (0.0, 100.0, 1.0);
        amount.setValue (100.0, juce::dontSendNotification);
        amount.setTextValueSuffix (" %");
        maxBands.setRange (2.0, 12.0, 1.0);
        maxBands.setValue (8.0, juce::dontSendNotification);
        smoothing.addItemList ({ "1/3 octave", "1/6 octave", "1/12 octave" }, 1);
        smoothing.setSelectedItemIndex (0, juce::dontSendNotification);
        for (auto* s : { &amount, &maxBands })
        {
            s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 20);
            s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            s->setColour (juce::Slider::thumbColourId, theme::curve);
            s->onValueChange = [this] { repaint(); };
        }
        smoothing.onChange = [this] { repaint(); };

        for (juce::Component* c : std::initializer_list<juce::Component*> { &loadButton, &learnReference, &learnCurrent, &clearButton,
                 &applyButton, &doneButton, &amount, &maxBands, &smoothing, &amountLabel, &bandsLabel, &smoothingLabel })
            addAndMakeVisible (c);
    }

    MatchPanel::~MatchPanel() { stopTimer(); }

    void MatchPanel::visibilityChanged()
    {
        if (isVisible())
        {
            // Drop whatever queued up while the panel was closed, then start listening.
            processor.getInputFifo().pull (pullBuffer.data(), (int) pullBuffer.size());
            startTimerHz (15);
        }
        else
        {
            stopTimer();
            learnReference.setToggleState (false, juce::dontSendNotification);
            learnCurrent.setToggleState (false, juce::dontSendNotification);
        }
    }

    void MatchPanel::timerCallback()
    {
        const auto n = processor.getInputFifo().pull (pullBuffer.data(), (int) pullBuffer.size());
        if (learnReference.getToggleState())
            reference->push (pullBuffer.data(), n);
        if (learnCurrent.getToggleState())
            current->push (pullBuffer.data(), n);
        repaint();
    }

    double MatchPanel::smoothingOctaves() const
    {
        switch (smoothing.getSelectedItemIndex())
        {
            case 1: return 1.0 / 6.0;
            case 2: return 1.0 / 12.0;
            default: return 1.0 / 3.0;
        }
    }

    std::vector<double> MatchPanel::targetCurve() const
    {
        const auto f = dsp::match::logGrid();
        return dsp::match::matchCurve (f, reference->levelsDb (f, smoothingOctaves()), current->levelsDb (f, smoothingOctaves()),
            amount.getValue() / 100.0);
    }

    const MatchPanel::Fit& MatchPanel::currentFit()
    {
        const auto key = juce::String (reference->getNumFrames()) + "/" + juce::String (current->getNumFrames()) + "/"
                       + juce::String (amount.getValue()) + "/" + juce::String (smoothing.getSelectedItemIndex()) + "/"
                       + juce::String (maxBands.getValue()) + "/" + juce::String ((juce::pointer_sized_int) reference.get());
        if (key != fitCache.key)
        {
            const auto f = dsp::match::logGrid();
            fitCache.key = key;
            fitCache.target = targetCurve();
            const auto bands = dsp::match::fit (f, fitCache.target, (int) maxBands.getValue(), sampleRateOf (processor));
            fitCache.response = dsp::match::responseDb (bands, f, sampleRateOf (processor));
            fitCache.numBands = (int) bands.size();
        }
        return fitCache;
    }

    void MatchPanel::setParam (const juce::String& id, float value)
    {
        if (auto* p = processor.getState().getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
    }

    int MatchPanel::applyMatch()
    {
        if (! hasBothSpectra())
            return 0;

        // Slots: the bands this panel wrote last time, then any free bands.
        auto& state = processor.getState();
        std::vector<int> slots = matchBands;
        for (int b = 0; b < dsp::numBands; ++b)
            if (std::find (slots.begin(), slots.end(), b) == slots.end()
                && state.getRawParameterValue (params::bandId (b, params::on))->load() < 0.5f)
                slots.push_back (b);

        const auto limit = std::min ((int) maxBands.getValue(), (int) slots.size());
        const auto f = dsp::match::logGrid();
        const auto bands = dsp::match::fit (f, targetCurve(), limit, sampleRateOf (processor));

        using namespace params;
        std::vector<int> written;
        for (size_t i = 0; i < bands.size(); ++i)
        {
            const auto b = slots[i];
            const auto& fb = bands[i];
            setParam (bandId (b, type), (float) fb.type);
            setParam (bandId (b, freq), (float) fb.frequencyHz);
            setParam (bandId (b, gain), (float) fb.gainDb);
            setParam (bandId (b, q), (float) fb.q);
            setParam (bandId (b, slope), (float) dsp::defaultSlopeIndex);
            setParam (bandId (b, place), 0.0f);
            setParam (bandId (b, dyn), 0.0f);
            setParam (bandId (b, on), 1.0f);
            written.push_back (b);
        }
        // Old match bands not needed any more are switched off.
        for (auto b : matchBands)
            if (std::find (written.begin(), written.end(), b) == written.end())
                setParam (bandId (b, on), 0.0f);

        matchBands = written;
        repaint();
        return (int) written.size();
    }

    void MatchPanel::loadReferenceFile()
    {
        if (loading)
            return;
        chooser = std::make_unique<juce::FileChooser> ("Choose a reference track", juce::File(),
            "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        juce::WeakReference<MatchPanel> weak (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [weak] (const juce::FileChooser& fc) {
                if (weak == nullptr)
                    return;
                const auto file = fc.getResult();
                if (! file.existsAsFile())
                    return;
                weak->loading = true;
                weak->referenceName = file.getFileName() + " (analysing...)";
                weak->repaint();

                // Analyse on a background thread; hand the result back on the message thread.
                juce::Thread::launch ([weak, file] {
                    juce::AudioFormatManager formats;
                    formats.registerBasicFormats();
                    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
                    std::shared_ptr<dsp::LongTermSpectrum> result;
                    if (reader != nullptr)
                    {
                        result = std::make_shared<dsp::LongTermSpectrum> (reader->sampleRate);
                        const auto total = std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 600));
                        const int chunk = 65536;
                        juce::AudioBuffer<float> buffer ((int) reader->numChannels, chunk);
                        std::vector<float> mono ((size_t) chunk);
                        for (juce::int64 pos = 0; pos < total; pos += chunk)
                        {
                            const auto n = (int) std::min<juce::int64> (chunk, total - pos);
                            reader->read (&buffer, 0, n, pos, true, true);
                            for (int i = 0; i < n; ++i)
                            {
                                float sum = 0.0f;
                                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                                    sum += buffer.getSample (ch, i);
                                mono[(size_t) i] = sum / (float) juce::jmax (1, buffer.getNumChannels());
                            }
                            result->push (mono.data(), n);
                        }
                    }
                    juce::MessageManager::callAsync ([weak, result, name = file.getFileName()] {
                        if (weak == nullptr)
                            return;
                        weak->loading = false;
                        if (result != nullptr && result->getNumFrames() > 2)
                        {
                            weak->reference = result;
                            weak->referenceName = name;
                        }
                        else
                        {
                            weak->referenceName = name + " (could not read)";
                        }
                        weak->repaint();
                    });
                });
            });
    }

    void MatchPanel::paint (juce::Graphics& g)
    {
        g.fillAll (theme::background.withAlpha (0.96f));
        auto r = getLocalBounds().reduced (20, 14);
        auto heading = r.removeFromTop (24);
        const auto glyph = heading.removeFromLeft (22).toFloat().getCentre();
        theme::glow (g, glyph, 18.0f, theme::curve, 0.3f);
        scenery::drawSeedOfLife (g, glyph, 9.0f, theme::curve, 1.0f);
        heading.removeFromLeft (6);
        g.setColour (theme::curve);
        g.setFont (juce::FontOptions (17.0f, juce::Font::bold).withKerningFactor (0.08f));
        g.drawText ("EQ Match", heading, juce::Justification::centredLeft);

        g.setFont (juce::FontOptions (12.0f));
        g.setColour (theme::dimText);
        g.drawText ("Reference: " + referenceName + "  (" + juce::String (reference->getSecondsAnalysed(), 1) + " s)   |   Current: "
                        + juce::String (current->getSecondsAnalysed(), 1) + " s analysed",
            r.removeFromTop (18), juce::Justification::centredLeft);

        // Plot area (between the status line and the controls)
        auto plot = getLocalBounds().reduced (20, 14).withTrimmedTop (48).withTrimmedBottom (78).toFloat();
        scenery::drawCarvedPanel (g, plot.expanded (4.0f), theme::curve, 0.0f);
        Mapping m { plot.getWidth(), plot.getHeight(), 15.0 };
        const auto f = dsp::match::logGrid();
        auto path = [&] (const std::vector<double>& c) {
            juce::Path p;
            for (size_t i = 0; i < f.size(); ++i)
            {
                const auto x = plot.getX() + m.xForFrequency (f[i]);
                const auto y = plot.getY() + m.yForDb (juce::jlimit (-15.0, 15.0, c[i]));
                i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
            }
            return p;
        };
        g.setColour (theme::grid);
        g.drawHorizontalLine (juce::roundToInt (plot.getCentreY()), plot.getX(), plot.getRight());

        const auto so = smoothingOctaves();
        if (reference->getNumFrames() > 2)
        {
            g.setColour (theme::accent.withAlpha (0.8f));
            g.strokePath (path (centred (f, reference->levelsDb (f, so))), juce::PathStrokeType (1.2f));
        }
        if (current->getNumFrames() > 2)
        {
            g.setColour (theme::spectrum.withAlpha (0.9f));
            g.strokePath (path (centred (f, current->levelsDb (f, so))), juce::PathStrokeType (1.2f));
        }
        if (hasBothSpectra())
        {
            const auto& fit = currentFit();
            g.setColour (theme::curve.withAlpha (0.5f));
            const float dashes[] = { 4.0f, 3.0f };
            juce::Path dashed;
            juce::PathStrokeType (1.5f).createDashedStroke (dashed, path (fit.target), dashes, 2);
            g.fillPath (dashed);
            g.setColour (theme::curve);
            g.strokePath (path (fit.response), juce::PathStrokeType (2.2f));
            g.setFont (juce::FontOptions (11.0f));
            g.drawText (juce::String (fit.numBands) + " bands", plot.reduced (6).removeFromTop (14), juce::Justification::topRight);
        }
        else
        {
            g.setColour (theme::dimText);
            g.setFont (juce::FontOptions (13.0f));
            g.drawFittedText ("1. Load a reference file, or press Learn reference and play the reference through Solfa.\n"
                              "2. Press Learn current and play your own track for 10-30 seconds.\n"
                              "3. Adjust Amount and Smoothing, then Apply match: Solfa creates normal bands you can tweak.",
                plot.reduced (16).toNearestInt(), juce::Justification::centred, 4);
        }
        g.setFont (juce::FontOptions (10.0f));
        g.setColour (theme::accent);
        g.drawText ("reference", plot.reduced (6).removeFromBottom (12), juce::Justification::bottomLeft);
        g.setColour (theme::spectrum);
        g.drawText ("current", plot.reduced (6).removeFromBottom (12).withTrimmedLeft (70), juce::Justification::bottomLeft);
        g.setColour (theme::curve);
        g.drawText ("match (dashed: target)", plot.reduced (6).removeFromBottom (12).withTrimmedLeft (130), juce::Justification::bottomLeft);
    }

    void MatchPanel::resized()
    {
        auto r = getLocalBounds().reduced (20, 14);
        auto bottom = r.removeFromBottom (70);
        auto row1 = bottom.removeFromTop (30);
        loadButton.setBounds (row1.removeFromLeft (170));
        row1.removeFromLeft (8);
        learnReference.setBounds (row1.removeFromLeft (130));
        row1.removeFromLeft (8);
        learnCurrent.setBounds (row1.removeFromLeft (120));
        row1.removeFromLeft (8);
        clearButton.setBounds (row1.removeFromLeft (70));
        doneButton.setBounds (row1.removeFromRight (80));
        row1.removeFromRight (8);
        applyButton.setBounds (row1.removeFromRight (120));

        bottom.removeFromTop (10);
        auto row2 = bottom.removeFromTop (28);
        amountLabel.setBounds (row2.removeFromLeft (56));
        amount.setBounds (row2.removeFromLeft (220));
        row2.removeFromLeft (16);
        smoothingLabel.setBounds (row2.removeFromLeft (72));
        smoothing.setBounds (row2.removeFromLeft (120));
        row2.removeFromLeft (16);
        bandsLabel.setBounds (row2.removeFromLeft (70));
        maxBands.setBounds (row2.removeFromLeft (200));
    }
}
