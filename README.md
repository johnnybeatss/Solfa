# Solfa

A parametric EQ whose presets are named after the solfeggio frequencies and 432 Hz tuning,
and designed from psychoacoustics research to shape the *feel* of a sound.

Solfa makes no health or therapeutic claims. The research behind its design, including
what the evidence does and doesn't support, is in [`docs/research/`](docs/research/).

## Status

**Phase 2 (EQ engine)**: 8 bands (bell, low/high shelf, low/high cut, tilt), an Amount
macro and output gain, with analog-matched filters (see [`docs/dsp-decisions.md`](docs/dsp-decisions.md)).

**Phase 3 (gain staging)**: Auto Gain keeps the loudness steady as you EQ, and Compare plays the
unprocessed sound at the same loudness so you can judge the tone, not the volume.
There is no custom UI yet: the plugin shows a plain list of its parameters until Phase 5.

| Format | Platforms | DAWs |
|---|---|---|
| VST3 | macOS, Windows, Linux | FL Studio, Ableton Live, Cubase, Studio One, Reaper, Bitwig |
| AU | macOS | Logic Pro, GarageBand, Ableton Live, Studio One |
| AAX | macOS, Windows | Pro Tools (see below) |
| CLAP | macOS, Windows, Linux | Bitwig, Reaper, FL Studio |
| Standalone | all | none (runs on its own) |

## Building on your Mac

One-time setup:

```bash
xcode-select --install          # Apple's compilers
brew install cmake ninja        # needs Homebrew: https://brew.sh
git clone --recurse-submodules <this repo's URL>
cd solfa
```

Build (Debug is faster to compile; use Release when making music with it):

```bash
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Builds --target Solfa_All Solfa_CLAP Tests
ctest --test-dir Builds --output-on-failure
```

The plugins are copied into `~/Library/Audio/Plug-Ins/` automatically after each build, so
rescan plugins in your DAW and Solfa appears under **Solfa Audio**.

Prefer Xcode? `cmake -B Xcode -G Xcode` then open `Xcode/Solfa.xcodeproj`.

### Pro Tools (AAX)

Retail Pro Tools only loads AAX plugins signed through Avid/PACE. To test without signing,
use the developer build of Pro Tools that Avid provides to registered AAX developers.
Apply for a free Avid developer account early, because approval can take weeks.

## CI

Every push builds all formats on macOS, Windows and Linux, runs the unit tests, runs
[pluginval](https://github.com/Tracktion/pluginval) at strictness 10, and runs Apple's `auval` on
the AU. Unsigned builds are attached to each run as downloadable zips.

Builds are **unsigned**. A plugin you download from CI to your Mac needs its quarantine
flag removed before it will load:

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Solfa.vst3
```

## Credits

Built with [JUCE](https://juce.com) 9 (Starter licence) and the
[Pamplejuce](https://github.com/sudara/pamplejuce) template (MIT, see `LICENSE-pamplejuce.txt`).
CLAP support via [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions).
