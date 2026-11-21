# DSP decisions (Phase 2)

## Filter design: analog-matched biquads

**Decision:** bell, shelves, tilt and high-cut use an analog-matched design; low-cut uses the RBJ
cookbook (bilinear) design. All maths in double precision, Transposed Direct Form II.

**How the matched design works** (`source/dsp/FilterDesign.cpp`):

1. Map the analog prototype's poles exactly with `z = exp(sT)`.
2. Fit the numerator so the digital magnitude equals the analog magnitude at DC, at Nyquist,
   and at one more frequency. Try two candidates (the band frequency and fs/4) and keep the
   one with the smaller error on a 16-point check grid.
3. Design cuts as the exact inverse of the matching boost. This keeps the fit well conditioned
   and makes "+g then -g" an identity.

**Measured worst-case error vs the analog prototype** (Python prototype, 44.1-192 kHz,
f0 20 Hz-20 kHz, gain ±18 dB, Q 0.1-18):

| Type | Matched: median / worst | RBJ bilinear: median / worst |
|---|---|---|
| Bell | 0.01 / 2.0 dB | 0.07 / 12.4 dB |
| Low shelf | 0.02 / 1.6 dB | 0.05 / 12.0 dB |
| High-cut | 0.05 / 1.3 dB | 0.57 / 26.9 dB |
| Low-cut | worse than RBJ below 10 kHz | **chosen** |

The unit tests pin these with tolerances (`tests/EqDsp.cpp`), including the research's
reference case: a 16 kHz, Q 1, +6 dB bell at 44.1 kHz is 4.23 dB short at 20 kHz with RBJ and
0.54 dB with the matched design.

**Known limits:**

- High shelves above about 0.25 × sample rate with Q above 1 still deviate by several dB near
  Nyquist (the analog shape mostly lies above Nyquist). Shelf Q is capped at 2.
- Low-cut near Nyquist cramps like any bilinear design; irrelevant for real low-cut use.

## Why not a state-variable filter (SVF)?

The plan called for a test between matched biquads and Andrew Simper's SVF. The SVF's static
response is the same as the RBJ bilinear design, so it inherits the cramping in the table
above, and no published method combines it with analog matching. The matched biquad passes
the fast-automation click test (`"Fast automation does not click"`: max second difference
0.0173 vs a smooth-signal limit of 0.0167, threshold 2x), so the SVF was not needed.

## Smoothing and updates

- Frequency and Q are smoothed in log2, gain in dB, over 30 ms.
- Coefficients are recomputed every 16 samples while a parameter is moving, otherwise never.
- Bell/shelf/tilt bands switch on and off by ramping their gain to 0 dB. Cut filters, which
  have no gain, crossfade over 30 ms when switched (the only place a dry/wet mix is used, and
  only during the switch).
- A band at 0 dB whose filter state has decayed is skipped entirely, so a flat EQ is
  bit-for-bit transparent.

## Cost

8 bands active, stereo, 512 samples at 48 kHz: about 25 µs per block (Release, Linux CI box),
about 0.2% of the 10.7 ms real-time budget.

## Gain staging (Phase 3)

**Auto Gain (on by default).** A static estimate computed from the EQ curve, not by riding the
signal (signal-following gain is reported to "pump"). The curve's power gain is averaged over
64 log-spaced points from 20 Hz to 20 kHz (equal weights = pink noise) with each point
K-weighted per ITU-R BS.1770, and the output is turned down by the result (limit ±18 dB). A
flat EQ gives exactly 0 dB, so the default setting stays bit-transparent.

Measured with pink noise through the plugin (`tests/GainStaging.cpp`), loudness change with
Auto Gain on:

| Curve | Compensation | Residual loudness change |
|---|---|---|
| High shelf +6 dB @ 3 kHz | -3.74 dB | +0.15 dB |
| Low shelf +6 dB @ 150 Hz | -0.96 dB | -0.03 dB |
| Bell -8 dB @ 1 kHz, Q 0.7 | +1.36 dB | +0.07 dB |
| Tilt +4 dB @ 1 kHz | -0.97 dB | +0.07 dB |

Real music is not pink noise, so expect larger residuals on very bass- or treble-heavy material;
that is what Compare is for.

**Compare (level-matched bypass).** Plays the unprocessed signal turned to the processed
signal's measured loudness. Both are measured continuously with K-weighted meters (3 s
integration, BS.1770 "short-term"), so the match reflects the actual audio. Switching
crossfades over 30 ms; the matching gain follows over 200 ms. In the test, the processed
signal was +2.95 dB louder and Compare played the dry signal at +2.98 dB.

The K-weighting filters use libebur128's parameterisation, which reproduces the 48 kHz
coefficients published in BS.1770 to about 1e-15 and works at any sample rate.

## Bands & shapes (Phase 4)

- **24 bands**, all in series, each skipped entirely while inactive. All 24 active cost about
  79 µs per 512-sample stereo block at 48 kHz (about 0.7% of the real-time budget).
- **Cut slopes 6-96 dB/oct** are Butterworth cascades: order n = slope / 6, one first-order
  section for odd n plus n/2 biquads with the textbook Butterworth Qs. The band's Q scales the
  sharpest section, so Q 0.707 is a plain Butterworth and higher Q adds a corner resonance.
  High-cut sections are analog-matched (first-order: exact pole, magnitude fitted at DC and
  Nyquist); low-cut sections are bilinear, as before. Measured against the ideal analog
  Butterworth at 48/96 kHz: within 0.5 dB wherever the filter passes sound, within 1.5 dB in
  the stop band down to -60 dB.
- **Notch** uses the bilinear design so its zero sits exactly on the unit circle (measured
  deeper than -60 dB at the centre). **Band-pass** is analog-matched with a 0 dB peak.
- **Stereo placement** per band: Stereo, Left, Right, Mid or Side. Mid/Side bands convert to
  M = (L+R)/2, S = (L-R)/2, filter one part and convert back; tests confirm a Left band leaves
  the right channel bit-for-bit untouched, a Mid band ignores pure side content and a Side band
  ignores mono content. Auto Gain counts one-channel and Mid/Side bands as affecting half the
  signal power (an approximation).

## Dynamic EQ (Phase 6)

Available on bell, shelf and tilt bands: **Dynamic** on/off, **Threshold**, **Range**, **Attack**,
**Release**.

- **Detector:** listens to the band's own region of the band's *input*: band-pass at the band
  frequency and Q for bells and tilt, low-pass for low shelves, high-pass for high shelves,
  following the band's stereo placement (Stereo averages both channels; Mid/Side detect M or S).
  The level is an RMS average over about two periods of the band frequency (1-20 ms), so it
  reads the average level rather than riding each waveform peak.
- **Gain law:** as the level rises above Threshold the band's gain moves from its static gain
  towards static + Range, reaching it 12 dB above Threshold (a soft, linear-in-dB knee).
  Negative Range tames a region when it gets loud; positive Range lifts it. The Amount macro
  scales Range as well as the static gain.
- **Attack/Release** act on the gain change in dB, like a compressor's gain smoothing: attack
  while the band moves towards its range, release while it returns.
- **Cost control:** choosing the analog-matched fit is the expensive part of designing a band,
  so a dynamic band chooses once per shape (type/frequency/Q/static gain) and reuses that
  choice while only the dynamic gain moves. 24 dynamic bands with constantly moving gains cost
  about 199 µs per 512-sample stereo block at 48 kHz (about 1.9% of the real-time budget).
- **Auto Gain** follows the static curve only, so it does not pump with the dynamics.

Measured (tests/DynamicEq.cpp): 20 dB over threshold gives exactly the Range (-6.00 dB, +4.00
dB); 6 dB over gives half (-3.05 dB); 50 ms attack moves only -0.48 dB in the first 10 ms;
300 ms release still holds -5.47 dB 30 ms after the signal drops.

## Linear phase (Phase 8)

**Phase** setting: Zero Latency (default, the IIR engine), Linear Phase (4096-tap FIR at 48 kHz,
latency 2048 samples ≈ 43 ms) or Linear Phase HD (16384 taps, 8192 samples ≈ 171 ms, finer
low-frequency detail). FIR length scales with sample rate to keep the same resolution.

- **Design** (background thread, never the audio thread): the static EQ curve (same design
  functions as the display and engine, including Amount and Literal) is sampled on the FFT grid
  as a real, even (zero-phase) spectrum, inverse-FFT'd, centred and Hann-windowed. A flat
  curve gives a single unit impulse at the centre (tested; this also pins JUCE's 1/N inverse
  FFT scaling). The FIR's magnitude matches the IIR design within 0.1 dB from 100 Hz to 8 kHz
  and the impulse is symmetric (tested).
- **Hand-off:** a lock-free slot passes each new FIR to the audio thread, which calls
  `juce::dsp::Convolution::loadImpulseResponse` (wait-free, and JUCE requires it on the audio
  thread). The convolution only swaps in a new IR inside `process()`, so it runs on every block
  in this mode; until the mode's first FIR is live (plus 100 ms for the engine's crossfade from
  its default undelayed IR) Solfa outputs the input delayed by the same latency, so timing never
  jumps. Curve edits crossfade inside JUCE's engine.
- **Latency** is reported to the host (updated from the message thread when the mode changes).
  Compare's dry signal is delayed to match.
- **Measured:** flat linear phase equals the input delayed by exactly the reported latency
  (worst difference 1.8e-7); a +6 dB bell gives the same gain in both modes (6.00 vs 5.998 dB).
  The default Zero Latency mode stays bit-exact at flat settings; linear phase is not bit-exact
  (FFT convolution rounding, about -135 dB here).

**Limits:** in linear-phase mode Dynamic bands act as static bands and Left/Right/Mid/Side
bands act as stereo bands (one FIR for both channels); the display says so when it applies.
Switching mode changes latency, which some hosts handle with a brief glitch. Linear phase
pre-rings before sharp transients, which is why it is not the default.

## EQ Match (Phase 9)

- **Capture:** long-term average power spectra (Hann-windowed 8192-point FFT, 50% overlap) of a
  reference (an audio file analysed on a background thread, up to 10 minutes, or audio played
  through Solfa) and of the current material (Solfa's input, before any processing).
- **Target:** reference minus current on a 160-point log grid (20 Hz-20 kHz), each smoothed over
  1/3, 1/6 or 1/12 octave, normalised so the 100 Hz-10 kHz average is 0 dB (tonal balance, not
  level), clamped to ±12 dB and scaled by Amount.
- **Fit:** greedy placement (largest weighted deviation -> bell at that frequency with Q from its
  half-height width, or a shelf when the deviation runs off either end), then three passes of
  weighted least squares on all band gains together. Bands are ordinary Solfa bands (Q 0.3-4,
  gain ±12 dB) written into free slots; applying again replaces the bands Match created last
  time.
- **Measured:** a known 3-band curve is fitted with 3 bands to 0.05 dB RMS; white noise EQ'd with a
  known curve and matched against plain noise recovers that curve to 0.20 dB RMS (7 bands), and
  through the plugin's own Apply to 0.24 dB RMS (5 bands).
