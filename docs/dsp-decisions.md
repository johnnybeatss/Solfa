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
