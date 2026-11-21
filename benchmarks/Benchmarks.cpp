TEST_CASE ("Boot performance")
{
    BENCHMARK_ADVANCED ("Processor constructor")
    (Catch::Benchmark::Chronometer meter)
    {
        std::vector<Catch::Benchmark::storage_for<PluginProcessor>> storage (size_t (meter.runs()));
        meter.measure ([&] (int i) { storage[(size_t) i].construct(); });
    };

    BENCHMARK_ADVANCED ("Processor destructor")
    (Catch::Benchmark::Chronometer meter)
    {
        std::vector<Catch::Benchmark::destructable_object<PluginProcessor>> storage (size_t (meter.runs()));
        for (auto& s : storage)
            s.construct();
        meter.measure ([&] (int i) { storage[(size_t) i].destruct(); });
    };

    BENCHMARK_ADVANCED ("Editor open and close")
    (Catch::Benchmark::Chronometer meter)
    {
        PluginProcessor plugin;

        // due to complex construction logic of the editor, let's measure open/close together
        meter.measure ([&] (int /* i */) {
            auto editor = plugin.createEditorAndMakeActive();
            plugin.editorBeingDeleted (editor);
            delete editor;
            return plugin.getActiveEditor();
        });
    };
}

TEST_CASE ("EQ processing cost")
{
    PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 512);

    // All eight bands on with non-zero gain: the worst steady-state case.
    for (int b = 0; b < solfa::dsp::numBands; ++b)
    {
        auto set = [&] (const char* suffix, float value) {
            auto* p = plugin.getState().getParameter (solfa::params::bandId (b, suffix));
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        };
        set (solfa::params::on, 1.0f);
        set (solfa::params::gain, 3.0f);
    }

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample (ch, i, 0.1f * std::sin (0.05f * (float) i));
    plugin.processBlock (buffer, midi); // let smoothing settle into steady state
    for (int k = 0; k < 10; ++k)
        plugin.processBlock (buffer, midi);

    BENCHMARK ("8 bands, stereo, 512 samples")
    {
        plugin.processBlock (buffer, midi);
        return buffer.getSample (0, 0);
    };
}
