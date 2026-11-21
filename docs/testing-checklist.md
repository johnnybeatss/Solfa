# Manual DAW testing checklist

The automated tests check the maths, the audio engine and plugin-format validity (pluginval at
strictness 10, Apple's auval). They can't check how Solfa behaves in your DAWs or how it sounds.
Work through this once per DAW and note anything odd (what you did, what happened, DAW and
version).

## 1. Loading (every DAW)

- [ ] Solfa appears after a plugin rescan, under **Solfa Audio**
- [ ] It loads on a stereo track and on a mono track without errors
- [ ] The window opens, resizes (drag the corner), and reopens at the same size
- [ ] Two instances on different tracks work independently

## 2. Sound and transparency

- [ ] With Init loaded, bypassing Solfa makes no difference at all (null test: duplicate the track,
      invert one copy's polarity, the sum should be silent)
- [ ] A big boost (+12 dB bell) sounds clean: no crackles, no distortion
- [ ] Moving a node while audio plays is smooth (no zipper noise or clicks)
- [ ] Automating band frequency and gain plays back smoothly
- [ ] CPU stays reasonable with all 24 bands on (check the DAW's CPU meter)

## 3. Features

- [ ] Each factory preset loads instantly from Solfa's menu, and from the DAW's preset/program
      menu where the DAW offers one
- [ ] **Save preset...** writes a file to Documents/Solfa/Presets and it appears under My presets
- [ ] Auto Gain: toggling a boost on/off doesn't jump in loudness much
- [ ] Compare: switching sounds like a tone change, not a volume change
- [ ] Dynamic band: set Range -6 dB on a loud region; the dashed curve shows the swing and loud
      moments get tamed
- [ ] Mid/Side: a Side band affects only the stereo width
- [ ] Resonance at 20-50%: the Focus frequency rings with the music and stops with it
- [ ] Key Snap: changing the key moves the Focus readout to an in-key note; Pure restores the exact
      number
- [ ] Linear Phase: the DAW compensates (the track stays in time with the others); the display
      shows the latency
- [ ] EQ Match: load a reference file, learn your track for ~20 s, Apply; bands appear and sound
      closer to the reference
- [ ] Spectrum Grab: Alt-click near a peak creates a band on it

## 4. Session recall

- [ ] Save the session with non-default settings (bands, presets, Focus, Phase); close and
      reopen: everything comes back exactly
- [ ] Bounce/export offline: the result matches real-time playback

## 5. DAW-specific

- **Logic Pro / GarageBand (AU):** Logic validates new AUs on first scan; Solfa should pass. Check
  Logic's preset menu (factory presets).
- **Ableton Live:** test VST3 and AU; check the latency display in linear-phase modes.
- **FL Studio:** VST3 and CLAP; check that the window resizes inside FL's wrapper.
- **Pro Tools:** needs the AAX build signed (or Pro Tools Developer); see the README.
- **Reaper / Bitwig:** also try the CLAP version.
