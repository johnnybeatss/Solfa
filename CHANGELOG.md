# Changelog

## 0.9.1 (beta)

- **New look:** a jungle village at night. Moonlit sky with stars, palm and canopy silhouettes,
  stilt huts with lit doorways, an animated fire and drifting fireflies behind the analyser;
  carved-wood panels and top bar; stone knobs with glowing arcs; lantern-style band nodes; a
  glowing torch-gold EQ curve; violet light beams for the solfeggio markers; seed-of-life logo.
  All artwork is drawn procedurally (no image files) and the static scene is cached, so it costs
  nothing per frame except the fire and fireflies. Sound and parameters are unchanged.

## 0.9.0 (beta)

First complete build. Automated tests pass on macOS, Windows and Linux; not yet tested by hand
in DAWs (see `docs/testing-checklist.md`).

- **EQ engine:** 24 bands; bell, low/high shelf, tilt, notch, band-pass, low/high cut 6-96 dB/oct;
  analog-matched filters (accurate near the top of the audio range), double precision,
  smoothed automation, bit-exact transparency at flat settings.
- **Stereo:** per-band Stereo / Left / Right / Mid / Side.
- **Dynamic EQ** on bell, shelf and tilt bands (threshold, range, attack, release).
- **Auto Gain** (on by default) and loudness-matched **Compare**.
- **Presets:** 10 solfeggio / 432 Hz factory presets + Init; user presets saved to
  `Documents/Solfa/Presets`.
- **Focus & Resonance:** Focus frequency with Key Snap (A = 440 / 432 / 444, key, scale) or Pure;
  Literal boost; Resonance (tuned resonators at the Focus and its octaves).
- **Linear phase** (standard and HD) with host latency compensation.
- **EQ Match** from a reference file or live input; fits ordinary, editable bands.
- **Interface:** analyser, draggable nodes, Spectrum Grab, solfeggio markers, band and Focus
  panels, "About the science", resizable window.
- **Formats:** AU, VST3, AAX, CLAP, Standalone.
