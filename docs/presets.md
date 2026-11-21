# Factory presets

Each preset is named after a solfeggio / 432 Hz number. The EQ moves are broad and gentle and sit
on octaves of that number where it audibly matters (the numbers themselves all fall in the
low-mid "mud/honk" range, so literal boosts there mostly sound worse; see the research report).
The **Focus** frequency is the number itself; Resonance and Literal work from it.

These are starting curves derived from the research, not validated results. Refine them by ear
with level-matched listening (Auto Gain on, Compare), at two playback levels, on several sources.

| Preset | Sound | EQ (band: type, Hz, dB, Q) | Focus | Resonance |
|---|---|---|---|---|
| Init | Flat | none | 528 | off |
| Grounding 174 | Weight in the lows, less mud, softer top | low shelf 87 (174/2) +2.0 Q0.7 · bell 348 (174x2) -1.5 Q1.0 · high shelf 8000 -1.0 Q0.7 | 174 | 15%, 1.5 s |
| Restore 285 | Warm, rounded | tilt 1140 (285x4) -2.0 · high shelf 9120 (285x32) -1.5 Q0.7 | 285 | 15%, 1.5 s |
| Release 396 | Clears low-mid cloud | bell 396 -2.0 Q1.0 · tilt 1584 (396x4) +1.0 | 396 | 10%, 1.2 s |
| Change 417 | Less boxy, more presence | bell 417 -1.5 Q1.2 · bell 3336 (417x8) +1.5 Q0.8 | 417 | 15%, 1.5 s |
| Heart 528 | Closer, more intimate | bell 4224 (528x8) +1.5 Q0.8 · bell 264 (528/2) -1.0 Q1.0 · high shelf 10000 -1.0 Q0.7 | 528 | 20%, 1.8 s |
| Connect 639 | Open, wide | bell 319.5 (639/2) -1.5 Q1.0 · high shelf 10224 (639x16) +1.5 Q0.7 | 639 | 15%, 1.5 s |
| Clarity 741 | Clear, present, not harsh | bell 370.5 (741/2) -1.5 Q1.0 · bell 2964 (741x4) +1.0 Q0.7 | 741 | 10%, 1.2 s |
| Intuition 852 | Airy, light | high shelf 13632 (852x16) +2.5 Q0.7 · low shelf 106.5 (852/8) -1.5 Q0.7 | 852 | 20%, 2.5 s |
| Crown 963 | Ethereal | high shelf 7704 (963x8) +3.0 Q0.7 · low shelf 120.4 (963/8) -2.5 Q0.7 | 963 | 20%, 3.0 s |
| Natural 432 | No EQ; selects A = 432 Hz tuning | none | 432 | 15%, 2.0 s |

## Key Snap, Pure and Literal

- **Key Snap** (default): the Focus is moved to the nearest note of the chosen key and scale, at
  the chosen reference (A = 440, 432 or 444 Hz). Example: 528 Hz becomes C5 = 523.25 Hz at
  A = 440, or 528.01 Hz at A = 444.
- **Pure**: use the Focus exactly, even between notes (e.g. 528 Hz is 15.6 cents sharp of C5 at
  A = 440; 285, 639, 852 and 963 Hz are each about a quarter-tone off).
- **Literal Boost** (0-3 dB): a narrow bell (Q 6) at the Focus, for hearing the number directly.
  It is the one deliberately narrow move, and lands in the low-mid range.

## Resonance

Four tuned resonators at Focus x 1/2, 1, 2 and 4 (weights 0.5, 1, 0.7, 0.4), excited by the EQ'd
signal and added back. Each peaks at 0 dB, so it only sustains energy already present near its
frequency and rings out over the Decay time (T60). Measured: a tone at the Focus is reinforced
(about +6 dB at 100%), a tone at 700 Hz picks up almost nothing (-43 dB). Resonance is not
included in Auto Gain's estimate; use Compare to judge it at matched loudness.

Presets load instantly (no sweep). In a DAW they appear in the host's preset/program list until
the Solfa interface (Phase 7) adds a browser.
