# Roadmap

| Phase | Content | Status |
|---|---|---|
| 1 | Skeleton: JUCE 9, all formats, CI, pluginval | Done |
| 2 | EQ engine: analog-matched filters, smoothing, Amount | Done |
| 3 | Auto Gain, level-matched Compare | Done |
| 4 | Bands & shapes: 24 bands, notch, band-pass, 6-96 dB/oct cuts, per-band Stereo/Left/Right/Mid/Side | Done |
| 5 | Presets (solfeggio-named), Key Snap, Literal mode, **Resonance** | Done (see `docs/presets.md`) |
| 6 | Dynamic EQ (per-band threshold / range / attack / release) | Done |
| 7 | Interface: draggable curve, analyser, Spectrum Grab, solfeggio markers, "About the science" | Done |
| 8 | Linear-phase mode | Done |
| 9 | EQ Match (copy a reference track's tonal balance) | Done |
| 10 | DAW testing and beta | Next |

## Resonance (Phase 5)

Adds the preset's frequencies to the sound, which an EQ alone cannot do. A small bank of tuned,
high-Q resonators ("sympathetic strings") at the preset frequency and chosen octaves is excited
by the incoming audio and mixed in quietly, so it rings with the music and falls silent with it.
Tuning follows Key Snap (A = 440/432/444 + key) by default; a "Pure" switch uses the exact
solfeggio number even when it falls between notes.

## Relationship to FabFilter Pro-Q

Solfa offers capabilities that are standard in modern pro EQs (many bands, dynamic EQ, linear
phase, EQ match, spectrum grab, M/S). It is **not** a replica: it must not copy Pro-Q's name,
look, layout or artwork, and Pro-Q's "Natural Phase" method is unpublished and not reproduced.
