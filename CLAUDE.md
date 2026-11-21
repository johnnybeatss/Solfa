# CLAUDE.md

Guidance for Claude Code when working in this repository.

## Project

Solfa is a JUCE 9 audio plugin (parametric EQ with "spiritual frequency" presets) built from
the Pamplejuce template. Formats: VST3, AU, AAX, CLAP, Standalone. Owner develops on macOS.
Not being sold yet: no licensing, signing or store work unless asked.

The design rationale and evidence are in `docs/research/`. Read the report before changing
DSP behaviour or presets.

## Build commands

```bash
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Builds --target Solfa_All Solfa_CLAP Tests Benchmarks
ctest --test-dir Builds --output-on-failure        # on headless Linux: xvfb-run -a ctest ...
./Builds/Tests "[dsp]"                              # run one tag
```

pluginval (must pass at strictness 10):
`pluginval --strictness-level 10 --validate Builds/Solfa_artefacts/Release/VST3/Solfa.vst3`

## Layout

- `source/`: plugin code (compiled into the `SharedCode` interface library, shared with tests)
- `tests/`: Catch2 tests; `benchmarks/`: Catch2 benchmarks
- `JUCE/` (pinned to tag 9.0.6), `cmake/`, `modules/clap-juce-extensions/`: git submodules

## Rules (from the research; don't break without a reason)

- **Never change** `PLUGIN_MANUFACTURER_CODE` (Slfa), `PLUGIN_CODE` (Sfeq), `BUNDLE_ID`, or any
  parameter ID once released: DAW sessions depend on them. Bump a parameter's version number
  in `juce::ParameterID` instead of renaming it.
- Audio thread: no allocation, locks or `Coefficients::make*` calls in `processBlock`. Read
  parameters through the APVTS atomics. Use `juce::ScopedNoDenormals`.
- Smooth every audible parameter (20-50 ms): frequency and Q in log space, gain in dB. Snap
  (don't ramp) when a preset loads.
- Filter maths in double precision. Correct the response near Nyquist (analog-matched design).
- The "Amount" macro scales band gains in dB. Never implement it as a dry/wet crossfade:
  mixing a minimum-phase EQ with its dry signal comb-filters.
- Preset curves stay broad and gentle (about ±0.5-4 dB, Q 0.3-1.2) unless the user explicitly
  picks Literal mode.
- Tests must cover: bit-exact null at flat settings, response accuracy at 44.1-192 kHz, the
  "+g then -g" identity, clicks under fast automation, state save/restore.
- Pro-Q-style *capabilities* are welcome, but never copy FabFilter's name, look, layout or
  artwork (see `docs/roadmap.md`).
- **No health claims** in UI text, preset names, docs or comments (no "heals", "reduces
  anxiety", "improves sleep", "repairs DNA", "brainwave"). Describe sound character only.
