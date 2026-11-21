# Digital Parametric EQ DSP Design: Best Practices for a Production C++/JUCE Plugin

**How these notes were made (read first).** Research date: 2026-11-21. In this session WebFetch could not reach any primary host: w3.org, vicanek.de, cytomic.com, fabfilter.com, docs.juce.com, escholarship.org and shepazu.github.io all failed DNS under the session's network allowlist. Evidence therefore comes from three places, and each claim says which:
1. **[S] Search-verified.** The claim showed up in web-search result summaries or snippets for the cited URL. The full page was not read.
2. **[C] Computed in this session.** I ran my own NumPy calculations (scripts in the session scratchpad) with the standard RBJ formulas and analog prototypes. These are reproducible engineering numbers, not quotes from a source.
3. **[K] Canonical-source knowledge, not re-fetched.** These are well-known contents of the cited primary document, recalled from prior knowledge. Check them against the PDF or page before shipping code. Confidence is given each time.
Forum or vendor opinion is labelled **(forum)** or **(vendor)**. To have all primary sources re-read in full, add w3.org, cytomic.com, vicanek.de, fabfilter.com and docs.juce.com to the session's allowed network hosts.

---

## 1. RBJ Audio EQ Cookbook biquads: formulas, Q/BW/S definitions, numerical issues

### Takeaway
Use the RBJ cookbook (now a W3C Working Group Note, 8 June 2021) as the reference for magnitude design. Peaking boost and cut with the same Q are exact inverses, and shelves reach half the dB gain at f0. Compute coefficients in double. For low frequencies (below about 40 Hz) at high sample rates, run double-precision state, or use an SVF/TPT structure, because float32 direct-form coefficients produce errors of several dB.

### Cited Findings
- **Status:** the W3C "Audio EQ Cookbook" is a Working Group Note dated 08 June 2021. It is adapted, with permission, from Robert Bristow-Johnson's Audio-EQ-Cookbook.txt, and an editor's draft is kept on GitHub. [S] High confidence. — [W3C Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook); [older W3C copy](https://w3.org/2011/audio/audio-eq-cookbook.html)
- **Intermediate variables** [K, high confidence, matches the code in my scripts]. — [W3C Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook)
  - `A = 10^(dBgain/40)` (peaking and shelving only). `w0 = 2π·f0/Fs`.
  - From Q: `alpha = sin(w0)/(2Q)`.
  - From bandwidth BW in octaves: `alpha = sin(w0)·sinh( ln2/2 · BW · w0/sin(w0) )`. The `w0/sin(w0)` factor corrects bandwidth for bilinear warping.
  - From shelf slope S: `alpha = sin(w0)/2 · sqrt( (A + 1/A)(1/S − 1) + 2 )`.
- **PeakingEQ** [K, high]: `b0 = 1+α·A, b1 = −2cos w0, b2 = 1−α·A; a0 = 1+α/A, a1 = −2cos w0, a2 = 1−α/A`. — [W3C](https://www.w3.org/TR/audio-eq-cookbook)
- **Shelves** [K, high]. — [W3C](https://www.w3.org/TR/audio-eq-cookbook)
  - lowShelf: `b0 = A[(A+1) − (A−1)cos w0 + 2√A·α]`, `b1 = 2A[(A−1) − (A+1)cos w0]`, `b2 = A[(A+1) − (A−1)cos w0 − 2√A·α]`, `a0 = (A+1) + (A−1)cos w0 + 2√A·α`, `a1 = −2[(A−1) + (A+1)cos w0]`, `a2 = (A+1) + (A−1)cos w0 − 2√A·α`.
  - highShelf: the same with the sign of the (A−1) terms flipped.
- **Definitions** [S/K]. — [W3C](https://www.w3.org/TR/audio-eq-cookbook); [Synthizer mirror](https://synthizer.github.io/appendices/audio_eq_cookbook.html); [vult mirror](https://vult-dsp.github.io/vult/pages/audio-eq-cookbook.html)
  - For peakingEQ, BW is measured between the **midpoint (dBgain/2)** frequencies, not the −3 dB points. For BPF and notch it is measured between the −3 dB points.
  - S is a shelf-only parameter. **S = 1 is the steepest slope that stays monotonic** [S, a mirror states this].
  - The shelf gain at f0 is dBgain/2. I confirmed this [C]: an RBJ 6 dB high shelf gives exactly 3.00 dB at f0 at 44.1, 48 and 96 kHz.
- **Boost/cut symmetry** [C, derivation]: replacing A with 1/A in the peaking formulas swaps numerator and denominator. A +g bell and a −g bell with the same f0 and Q therefore cancel exactly. This is the "reciprocal" peaking behaviour engineers expect, and it is useful for null tests. — derived from the formulas at [W3C](https://www.w3.org/TR/audio-eq-cookbook)
- **Structure choice:** use transposed direct form II for floating point ("superior numerical characteristics to direct form II (non-transposed)"). Use direct form I, which has a single accumulation point, on fixed-point processors. [S, EarLevel (Nigel Redmon), a respected practitioner source, medium-high confidence] — [EarLevel Biquad C++ source code](https://www.earlevel.com/main/2012/11/26/biquad-c-source-code/)
- **DF-I vs DF-II-T** differ in error transfer, internal state amplitude distribution, noise floor, limit cycles and overflow tolerance. Rounding double coefficients to float32 shifts response in the low-frequency, high-Q regime, where a1 is about −2 and a2 about 1. [S, secondary practitioner guide, medium] — [leftbank Biquad Implementation Guide](https://note.com/leftbank/n/ndfef3dfec372?hl=en)
- **Pole sensitivity:** direct-form poles are very sensitive to quantisation near angles 0 and π. State-variable forms are "quite good for low frequencies". [S, mostly about fixed point] — [EarLevel Biquad calculator v2 discussion](https://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/)
- **Float32 coefficient error, measured [C]:** RBJ bell, +6 dB, Q = 4. Coefficients computed in double, then rounded to float32. Magnitude evaluated in double, 5–200 Hz.

  | Fs | f0 | a1 (double) | max deviation | peak gain (float32) | peak frequency (float32) |
  |---|---|---|---|---|---|
  | 48 kHz | 20 Hz | −1.99953 | 0.23 dB | 5.99 dB | 19.9 Hz |
  | 96 kHz | 20 Hz | −1.99977 | 0.71 dB | 5.81 dB | 20.4 Hz |
  | 192 kHz | 20 Hz | −1.99988 | **2.27 dB** | **6.34 dB** | **19.3 Hz** |
  | 192 kHz | 30 Hz | | 0.84 dB | | |

  This counts coefficient quantisation only. Float32 state noise in DF-II-T at low frequency adds to it.
- **JUCE coefficient handling:** the `dsp::IIR::Coefficients` constructor normalises all coefficients by a0 (forum). [S] — [JUCE forum: IIR raw coefficients](https://forum.juce.com/t/iir-filter-raw-coefficients-weird-values/55989)
- **JUCE factory functions:** `dsp::IIR::Coefficients::makePeakFilter(sampleRate, centreFrequency, Q, gainFactor)` and related factories exist. [S] — [JUCE IIR header (GitHub)](https://github.com/juce-framework/JUCE/blob/master/modules/juce_dsp/processors/juce_IIRFilter.h); [JUCE IIRCoefficients docs](https://docs.juce.com/master/classjuce_1_1IIRCoefficients.html)

### Inferences
- For a commercial EQ, the safest default is **double for coefficients and state** in the filter core. The audio buffer can stay float. The cost is small on modern x86 and ARM, and it removes the low-frequency, high-sample-rate problem shown in the table. An alternative is to keep float but use an SVF/TPT structure: it uses `g = tan(πf/fs)`, which keeps relative precision as f approaches 0, in place of a1 values crowded near −2.
- Reference cookbook Q and BW in the manual. Plugins define Q differently (constant-Q vs proportional-Q, bandwidth at −3 dB vs at half gain), and users compare against Pro-Q. Choose and document one definition.

### Gaps
- I could not fetch the W3C text to quote its implementation advice word for word. I recall that it recommends Direct Form 1 for simplicity, but that is unverified [K, medium].
- I found no authoritative measurement of float32 DF-II-T **state** noise versus DF-I or SVF for audio EQ. Measure it in-house.

---

## 2. Bilinear-transform cramping near Nyquist: size of the error and fixes

### Takeaway
At 44.1/48 kHz, an RBJ (bilinear) bell above about 8 kHz differs from its analog prototype by 1–6 dB on the upper skirt. This is because the bilinear transform forces a bell back to 0 dB at Nyquist. At 96 kHz the error falls to about 0.5–1.3 dB. Fixes are Orfanidis (prescribed Nyquist gain), Vicanek's matched biquads (2016) and matched shelves (2024), Massberg's analog-matched design, or oversampling. Vicanek-style matching costs only a little extra math per coefficient update and has zero latency, which makes it the best value for a zero-latency mode.

### Cited Findings
- **Mechanism:** the bilinear transform maps analog frequency nonlinearly. Prewarping is exact at one frequency only, and warping compresses hardest near Nyquist. [S] — [Wikipedia: Bilinear transform](https://en.wikipedia.org/wiki/Bilinear_transform); [DEV Community 5-band EQ in MATLAB](https://dev.to/lluisestape/designing-a-5-band-parametric-eq-from-the-biquad-up-in-matlab-367n); [spatialaudio bilinear notebook](https://nbviewer.jupyter.org/github/spatialaudio/digital-signal-processing-lecture/blob/master/filter_design/bilinear_transform.ipynb)
- **One 44.1 kHz example (forum-style blog, single case):** "the upper skirt has been pulled in by 350 Hz, squashed against Nyquist." [S] — [DEV Community](https://dev.to/lluisestape/designing-a-5-band-parametric-eq-from-the-biquad-up-in-matlab-367n)
- **Correction to a search summary:** some summaries say that a bilinear filter's gain "is normally zero at Nyquist". That holds for **lowpass** designs (z = −1 is a zero). For a **bell or shelf**, the bilinear transform maps s = ∞ to z = −1. The gain at Nyquist is therefore the analog gain at infinite frequency: 0 dB (unity) for a bell, and the full shelf gain for a high shelf [C/derivation]. The paper summary describes a 44.1 kHz bilinear response that "approaches zero too rapidly near the Nyquist frequency". — [Särkkä et al. eq-article (Aalto)](https://users.aalto.fi/~ssarkka/pub/eq-article.pdf)
- **Measured cramping [C].** Digital RBJ response minus the analog prototype, in dB:

  | Bell | Fs | at 0.75·f0 | at 1.25·f0 | at 20 kHz |
  |---|---|---|---|---|
  | 5 kHz, Q1, +6 | 44.1 kHz | −0.16 | −0.20 | −0.40 |
  | 10 kHz, Q1, +6 | 44.1 kHz | −0.71 | −1.07 | −1.78 |
  | 10 kHz, Q0.7, +12 | 44.1 kHz | −1.02 | −1.49 | **−5.23** (analog 5.75, digital 0.51) |
  | 16 kHz, Q1, +6 | 44.1 kHz | −2.30 | **−4.23** | −4.23 |
  | 16 kHz, Q2, −12 | 44.1 kHz | +3.48 | **+6.06** | +6.06 |
  | 10 kHz, Q1, +6 | 48 kHz | −0.58 | −0.85 | −1.60 |
  | 16 kHz, Q1, +6 | 48 kHz | −1.85 | −3.35 | −3.35 |
  | 16 kHz, Q2, −12 | 48 kHz | +2.98 | +5.34 | +5.34 |
  | 16 kHz, Q1, +6 | 96 kHz | −0.35 | −0.49 | −0.49 |
  | 16 kHz, Q2, −12 | 96 kHz | +0.74 | +1.28 | +1.28 |

  The 16 kHz rows have 1.25·f0 = 20 kHz, so the last two columns coincide.
  - **High shelf, 12 kHz, +6 dB:** at 20 kHz the analog prototype gives 5.27 dB and the digital filter gives 5.99 dB at 44.1 kHz (5.50 dB at 96 kHz). The digital shelf finishes its transition too early, so its shape is compressed. At f0 it matches exactly (3.00 dB).
  - Errors below 5 kHz are a few tenths of a dB at most.
- **Orfanidis 1997:** S. J. Orfanidis, "Digital Parametric Equalizer Design with Prescribed Nyquist-Frequency Gain", JAES vol. 45 no. 6, pp. 444–455, June 1997; presented at the 101st AES Convention, Nov 1996. Five constraints fix the five biquad coefficients: DC gain, peak frequency, peak gain, bandwidth and **Nyquist gain**, which is set to the analog gain at fs/2. A noted limitation is that magnitude matches only at the prescribed points. [S, bibliographic data high; design details from summaries, medium] — [AES e-library 7854](https://secure.aes.org/forum/pubs/journal/?elib=7854); [AES 7418](https://aes.org/publications/elibrary-page/?id=7418); [Särkkä et al.](https://users.aalto.fi/~ssarkka/pub/eq-article.pdf)
- **Vicanek 2016:** Martin Vicanek, "Matched Second Order Digital Filters" (Feb 2016). It starts from the bilinear design and adjusts it so the digital gain at Nyquist matches the analog gain there, using a closed-form Nyquist-gain expression in fc and Q. It cites Orfanidis as precedent. [S] Per [K, medium] it covers LP, HP, BP and peaking, with all coefficients in closed form, so they can be recomputed per block or per sample at the cost of a few transcendentals. — [Vicanek BiquadFits.pdf](https://www.vicanek.de/articles/BiquadFits.pdf)
- **Vicanek 2024:** "Matched Two-Pole Digital Shelving Filters" applies the same idea to second-order Butterworth shelves. The intro states that common designs "often have an unwanted transfer function cramping towards high frequencies". It includes pseudocode for matched high and low shelves. [S] — [Vicanek 2poleShelvingFits.pdf](https://www.vicanek.de/articles/2poleShelvingFits.pdf)
- **Massberg 2011:** "Digital Low-Pass Filter Design with Analog-Matched Magnitude Response", AES 131st Convention. It uses pre-warped prototypes with a matched magnitude response. [S] — [AES e-library 16077](https://www.aes.org/e-lib/browse.cfm?elib=16077)
- **Oversampling:** with a large enough oversampling factor, Nyquist moves far above the filter features and bilinear warping is minimised. [S] — [Wikipedia: Bilinear transform](https://en.wikipedia.org/wiki/Bilinear_transform)
- **Muranov thesis (UC San Diego), "Analysis and Design of Digital Parametric Filters Without Frequency Warping":** a further academic treatment of peaking-filter cramping. [S, not read] — [eScholarship PDF](https://escholarship.org/content/qt04z7p1h7/qt04z7p1h7.pdf)
- **Conformal maps:** discretisation methods other than the bilinear transform are surveyed in DAFx 2020. [S, not read] — [DAFx20in21 paper 15](https://dafx2020.mdw.ac.at/proceedings/papers/DAFx20in21_paper_15.pdf)
- **FabFilter Pro-Q 4:** Natural Phase mode's magnitude curve "matches the theoretical analog response precisely" (vendor help page). [S] — [FabFilter Pro-Q 4 Help: Processing mode](https://www.fabfilter.com/help/pro-q/using/processingmode); [Pro-Q 4 manual PDF](https://www.fabfilter.com/downloads/pdf/help/ffproq4-manual.pdf)

### Inferences
- **Recommended architecture:**
  - Zero-latency mode: matched-Nyquist second-order sections for bells and shelves (Vicanek 2016/2024, or Orfanidis for bells). There is no latency, no oversampling filter phase, and the extra cost is only per coefficient update.
  - Optional 2x oversampling: alternative or complement for bands above about 10 kHz. It costs extra CPU, adds aliasing-filter latency or phase (juce::dsp::Oversampling offers IIR or FIR polyphase), and still cramps a little.
  - Linear/natural-phase modes: build the FIR from the **analog** prototype magnitude, which removes cramping entirely.
- At 96 kHz and above, plain RBJ errors are small (about 1 dB worst case in my tests). Matched designs matter most at 44.1/48 kHz, which is still most sessions.

### Gaps
- I could not confirm which commercial plugins use which matching method. FabFilter (Natural Phase, analog-matched magnitude) is vendor-documented. For Pro-Q's Zero Latency mode and for other EQs (DMG, Sonnox, TDR, Waves, Kirchhoff), I found no reliable public statement. Do not claim "X uses Vicanek" without a source.
- I did not fetch the full Vicanek equations. Implement from the PDF once the host is reachable.

---

## 3. TPT / zero-delay-feedback state-variable filters (Simper, Zavalishin)

### Takeaway
A trapezoidal (TPT) SVF has the same LTI magnitude and phase response as a prewarped RBJ biquad. Because its states are integrator states, it stays well behaved when coefficients change quickly, even at audio rate. Use it, or use matched-coefficient biquads with per-sample smoothing, for any band that is automated or dynamic. Per-sample cost is comparable to a biquad, plus a `tan()` on each coefficient update.

### Cited Findings
- Simper (Cytomic) reports that the trapezoidal SVF stays stable and smooth under audio-rate parameter modulation. It is "both more numerically robust and far more conducive to modulation, even up to audio rate" than traditional biquad formulas (secondary write-ups of Simper's papers). [S, author's claim widely repeated, not an independent benchmark, medium-high] — [Cytomic SvfLinearTrapOptimised.pdf](https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised.pdf); [notebook.community summary](https://notebook.community/kysko/music-synthesizer-for-android/lab/Second%20order%20sections%20in%20matrix%20form); [JOS Digital State-Variable Filters](https://ccrma.stanford.edu/~jos/svf/svf.pdf)
- Simper's point (music-dsp): the SVF looks like a second-order biquad but is not one. Its feedback paths run parallel to the signal path, which is why it behaves differently under modulation. [S, (forum/mailing list)] — [music-dsp: Trapezoidal integrated optimised SVF v2](https://www.mail-archive.com/music-dsp@music.columbia.edu/msg02454.html); [narkive mirror](https://music-dsp.music.columbia.narkive.com/IzbZXmgi/trapezoidal-integrated-optimised-svf-v2)
- **Measured opinion (forum):** the SVF's behaviour is "mostly equivalent to what you can do with biquads, but they do not blow up under audio-rate modulation." [S] — [Cycling '74 forum](https://cycling74.com/forums/audio-rate-modulation-of-filters-at-high-resonance-how-can-i-do-it-without-things-getting-out-of-control)
- The older Chamberlin SVF works well only at low frequencies and becomes unstable around fs/6, or lower at high resonance. The trapezoidal version fixes this. [S] — [EarLevel: The digital state variable filter](https://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/)
- **Simper's SvfLinearTrapOptimised2 algorithm** [K, high confidence on structure, verify constants against the PDF]. — [Cytomic SvfLinearTrapOptimised2.pdf](https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf)
  - Coefficients: `g = tan(π·fc/fs)`, `k = 1/Q`, `a1 = 1/(1 + g(g+k))`, `a2 = g·a1`, `a3 = g·a2`.
  - Per sample: `v3 = v0 − ic2eq; v1 = a1·ic1eq + a2·v3; v2 = ic2eq + a2·ic1eq + a3·v3; ic1eq = 2v1 − ic1eq; ic2eq = 2v2 − ic2eq`.
  - Output: `m0·v0 + m1·v1 + m2·v2`.
  - Bell: `A = 10^(dB/40)`, `k = 1/(Q·A)`, `m0 = 1, m1 = k(A²−1), m2 = 0`.
  - Low shelf: `g = tan(π fc/fs)/√A`, `m0 = 1, m1 = k(A−1), m2 = A²−1`.
  - High shelf: `g = tan(π fc/fs)·√A`, `m0 = A², m1 = k(1−A)A, m2 = 1−A²`.
  - The response equals the corresponding RBJ filter.
- **Zavalishin, *The Art of VA Filter Design*:** rev. 2.1.0 is dated 28 Oct 2018. A 2.1.2 revision is referenced on KVR, and a "math companion" link was added in Dec 2023. It covers the topology-preserving transform, a generalisation of bilinear/trapezoidal integration with zero-delay feedback, and claims "nice amplitude and phase responses, nice time-varying behavior and plenty of options for nonlinearities". It builds on Zavalishin's 2008 "KeepTopology" paper. [S] — [VAFilterDesign_2.1.0.pdf (Native Instruments)](https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf); [KeepTopology.pdf](https://www.native-instruments.com/fileadmin/redaktion_upload/pdf/KeepTopology.pdf); [KVR book thread](https://www.kvraudio.com/forum/viewtopic.php?p=4930548)
- **JUCE `dsp::StateVariableTPTFilter`:** an IIR filter offering low, band and high-pass at 12 dB/oct on a TPT structure, with `enum class StateVariableTPTFilterType { lowpass, bandpass, highpass }`. It has **no bell or shelf modes**. [S] — [JUCE dsp processors group](https://docs.juce.com/master/group__juce__dsp-processors.html); [JUCE dsp namespace](https://docs.juce.com/master/namespacejuce_1_1dsp.html)
- A JUCE forum poster says `StateVariableFilter` and `StateVariableTPTFilter` have automatic cutoff smoothing and `dsp::IIR` does not. [S, (forum), low confidence: I believe StateVariableTPTFilter recomputes g immediately on `setCutoffFrequency()` without internal ramping. Check the source] — [JUCE forum: Smoothing IIR filter response](https://forum.juce.com/t/smoothing-iir-filter-response/39347)

### Inferences
- **Why biquads misbehave under modulation:** DF-I and DF-II-T states are weighted mixes of past inputs and outputs that are only valid for the coefficients that produced them. A coefficient jump leaves the "energy" in the state inconsistent with the new filter, which causes transients. Direct-form structures can even go unstable under fast switching when every frozen coefficient set is stable. SVF integrator states keep physical meaning across changes. This is the standard explanation consistent with the sources above. Treat it as high-confidence engineering consensus, not something proven here.
- **Cost:** per sample, about 6–8 multiply-adds (SVF with three-output mix) against 5 for DF-II-T. Recomputing `tan()` each sample is the main extra cost; use a fast tan approximation or recompute every N samples. Either structure supports SIMD across channels or bands (juce::dsp::SIMDRegister).
- For a parametric EQ in JUCE, **write your own TPT SVF bell/shelf/LP/HP class** following Simper's mixing coefficients. Do not rely on `StateVariableTPTFilter`, which lacks bell and shelf modes. Matched-Nyquist correction (section 2) is designed for biquad coefficients. To combine matching with the SVF you need either an SVF-to-biquad mapping or to accept bilinear cramping in SVF mode. This tradeoff is real and has no clean published solution that I could verify.

### Gaps
- I found no independent quantitative benchmark (THD of modulated output, transient energy) comparing SVF and DF-II-T under identical modulation. Build one: an audio-rate frequency sweep with a high-pass-energy click metric.
- I found no verified published method for an SVF with Vicanek-style Nyquist matching. Vicanek's 2024 shelving note might contain it; it was not fetched.

---

## 4. Parameter smoothing, JUCE classes, thread safety, denormals

### Takeaway
Smooth in perceptual domains: frequency multiplicatively (log), gain in dB, Q in log. Use about 20–50 ms ramps. Recompute coefficients per sample for SVF, or per small sub-block (8–32 samples) for biquads. Never allocate on the audio thread: `Coefficients::make*` returns a heap-allocated Ptr, so use `ArrayCoefficients` or your own math. Read parameters via APVTS atomics. Use `ScopedNoDenormals` in `processBlock`.

### Cited Findings
- `dsp::IIR` has no built-in coefficient smoothing (forum). [S] — [JUCE forum: Smoothing IIR filter response](https://forum.juce.com/t/smoothing-iir-filter-response/39347)
- Clicks come from resetting filter state to 0 or from rapid parameter jumps (forum). [S] — [JUCE forum: IIRFilter pops when frequency changes quickly](https://forum.juce.com/t/solved-what-causes-the-iirfilter-class-to-pop-when-filter-frequency-is-changed-quickly-and-how-can-i-fix-it/23660); [JUCE forum: clicking when changing IIRFilter params](https://forum.juce.com/t/clicking-noise-when-changing-iirfilter-cutoff-gain-parameters/47334)
- **Smoothing techniques (forum):**
  - Run the cutoff through a first-order lowpass as its own signal.
  - Recompute coefficients every few samples; no need to interpolate coefficients per sample (KVR).
  [S] — [JUCE forum: SmoothedValue, IIRFilter and ProcessorChain](https://forum.juce.com/t/smoothedvalue-iirfilter-and-processorchain/42815); [KVR: Smooth filter changes](https://www.kvraudio.com/forum/viewtopic.php?p=8608941)
- Do not call `juce::dsp::IIR::Coefficients<float>::makePeakFilter` in `processBlock`, because it allocates. Use `ArrayCoefficients` instead. Changing coefficients very fast also hits redundant checks inside `dsp::IIR` (forum, about 2 years old). [S] — [JUCE forum: dsp::IIR::Filter noise/glitches with LFO](https://forum.juce.com/t/dsp-noise-glitches-when-automating-frequency-via-slow-lfo/63949); [page 2](https://forum.juce.com/t/dsp-noise-glitches-when-automating-frequency-via-slow-lfo/63949?page=2); [Where to recalculate coefficients?](https://forum.juce.com/t/where-to-recalculate-coefficients/60702)
- **SmoothedValue:** a common usage is `smoothed.reset(sampleRate, 0.02)` for a 20 ms ramp (third-party example). JUCE offers `ValueSmoothingTypes::Linear` and `::Multiplicative` [K, high]. — [JUCE SmoothedValue docs](https://docs.juce.com/master/classSmoothedValue.html); [JUCE in-depth tutorial (third party)](https://brendanjameslynskey.github.io/JUCE_Tutorial/)
- **ProcessorDuplicator** turns a mono processor class into a multichannel one by duplicating it (`OwnedArray<MonoProcessorType>`). The usual pattern is `ProcessorDuplicator<IIR::Filter<float>, IIR::Coefficients<float>>` with one shared coefficient state. [S] — [juce_ProcessorDuplicator.h (GitHub)](https://github.com/juce-framework/JUCE/blob/master/modules/juce_dsp/processors/juce_ProcessorDuplicator.h)
- **Denormals:** `ScopedNoDenormals` turns off denormal handling (FTZ/DAZ), which would otherwise cripple IIR tails. The juce_dsp module's denormal-avoidance option "will add a slight performance overhead to the DSP module's filters". [S] — [juce_dsp.h (GitHub)](https://github.com/juce-framework/JUCE/blob/master/modules/juce_dsp/juce_dsp.h); [JUCE tutorial (third party)](https://brendanjameslynskey.github.io/JUCE_Tutorial/)
- **Thread safety:** pluginval added a test that calls `setValue` on parameters from several threads at once, mimicking automation plus GUI. [S] — [pluginval README](https://github.com/Tracktion/pluginval/blob/master/README.md)
- **APVTS** `getRawParameterValue()` returns `std::atomic<float>*` for lock-free reads in `processBlock`. GUI controls connect through `SliderAttachment` / `ParameterAttachment`. [K, high] — [JUCE AudioProcessorValueTreeState docs](https://docs.juce.com/master/classAudioProcessorValueTreeState.html)
- **JUCE 8 status:** the `juce_dsp` processors (IIR, StateVariableTPTFilter, ProcessorDuplicator) are present on the JUCE GitHub master branch. [S] — [JUCE juce_dsp on GitHub](https://github.com/juce-framework/JUCE/blob/master/modules/juce_dsp/juce_dsp.h). I found no evidence that JUCE 8 changed IIR smoothing behaviour [S, absence of evidence].

### Inferences
- **Smoothing domains:**
  - Frequency: smooth log2(f) linearly, or use Multiplicative SmoothedValue on Hz, so sweeps are uniform in octaves.
  - Gain: smooth dB linearly, then convert.
  - Q: smooth log(Q).
- **Time constants:** about 20 ms for user knobs; 30–50 ms for big frequency jumps such as preset changes. Use no extra smoothing for dynamic-EQ gain, which already has attack/release.
- **Coefficient update cadence:**
  - SVF: per sample while any smoother is active; skip when settled (`isSmoothing()`).
  - Biquad: sub-blocks of 16–32 samples. Pair matched-Nyquist biquads with SVF-like behaviour only if switches stay small.
- **Thread model:** atomic parameter reads on the audio thread → smoothers → coefficient math on the audio thread, allocation-free. If coefficients are expensive (FIR kernels for linear phase), build them on a background thread and hand them over with a lock-free swap: an atomic pointer to a pre-allocated double buffer, with crossfading.
- Preset loads and state restore should snap the smoothers (`setCurrentAndTargetValue`) so recall does not audibly sweep.

### Gaps
- I could not confirm the current JUCE version as of late 2026 (JUCE 8.x or later) or any juce_dsp API changes in 2025–26. Check the [JUCE changelog](https://github.com/juce-framework/JUCE/blob/master/CHANGE_LIST.md) and licence terms directly.
- The exact SmoothedValue signatures were not fetched this session.

---

## 5. Minimum-phase vs linear-phase vs natural phase; M/S; parallel dry/wet

### Takeaway
Ship minimum-phase (zero latency) as the default. Offer linear phase (FIR/FFT, with latency and pre-ringing) for mastering and parallel or multi-mic coherence. A "natural/analog phase" mode is a differentiator but is proprietary at FabFilter. A plain dry/wet knob on a minimum-phase EQ produces non-intuitive curves and real cancellations. Scale band gains instead, or offer the mix knob only in linear-phase mode with a latency-aligned dry path.

### Cited Findings
- **Pro-Q 4 linear phase:** latency depends on the resolution setting. A third-party summary gives about 70 ms (Low) to about 1509 ms (Maximum) at 44.1 kHz. FabFilter recommends Medium, and warns that Maximum gives "very high low-frequency resolution at the expense of a very large latency and possible pre-echo problems". Spectral bands automatically use linear phase. [S, latency numbers third party, medium] — [FabFilter Pro-Q 4 Help: Processing mode](https://www.fabfilter.com/help/pro-q/using/processingmode); [Rise Up Mastering: EQ phase modes](https://riseupmastering.com/docs/eq-phase-modes/)
- **Natural Phase:** magnitude matches analog precisely (vendor). Third-party summaries say it closely follows analog phase without noticeable pre-ringing or long latency. A forum user says it does add latency (conflicting, weight lightly). [S] — [FabFilter Help](https://www.fabfilter.com/help/pro-q/using/processingmode); [vi-control forum](https://vi-control.net/community/threads/fabfilter-pro-q-4.158729/page-6)
- **Pre-ringing:** linear-phase EQ pre-rings before transients such as drum hits. This is the main audible cost. [S, educational blog] — [SoundGym: linear phase EQ pre-ringing](https://www.soundgym.co/blog/item?id=linear-phase-eq-pre-ringing); [Boris FX: linear phase EQ](https://borisfx.com/blog/linear-phase-eq-when-to-use-it-for-cleaner-mixes/)
- **Dry/wet:** plugin developers say a true dry/wet with minimum-phase EQ is impossible, so they alter the filter design to approximate it. MDynamicEQ's Wet/Dry control "isn't a plain dry/wet mix". [S, (forum)] — [KVR: How to use the all-pass filter?](https://www.kvraudio.com/forum/viewtopic.php?p=5728746)
- **Real-world bug:** in Mixxx, a 50% mix with a Bessel or biquad EQ/isolator effect produced comb filtering. [S] — [Mixxx bug 1740521](https://bugs.launchpad.net/mixxx/+bug/1740521)
- **Dry/wet behaviour, measured [C]** (analog prototypes, `mix = 0.5 + 0.5·H`):
  - A 50% mix of a +12 dB, Q1 bell peaks at **+7.9 dB**, not +6 dB, and differs from a true +6 dB bell by up to 1.9 dB.
  - A 50% mix of a −30 dB, Q4 cut bottoms out at only −5.75 dB.
  - A 50% mix of a 24 dB/oct high-pass at 100 Hz has a **−12.9 dB cancellation notch at 109 Hz**: the phase is about 180° near fc.
  - A 12 dB/oct high-pass gives a −7.6 dB dip at 55 Hz.
- **Any latency mismatch:** 1–2 ms of extra delay on the wet path produces comb filtering when summed with dry. [S, educational wiki] — [MusicProductionWiki: wet/dry](https://musicproductionwiki.com/bible/wet-dry)
- **JUCE building blocks for FIR/FFT EQ:** `dsp::FIR`, `dsp::Convolution`, `dsp::FFT` and `dsp::Oversampling` exist in juce_dsp. [K, high] — [JUCE dsp namespace](https://docs.juce.com/master/namespacejuce_1_1dsp.html)

### Inferences
- **Linear-phase implementation sketch:**
  1. Evaluate the summed analog-prototype magnitude, with no cramping, on an N-point FFT grid. N = 8192–65536 depending on the low-frequency resolution wanted.
  2. Apply zero phase, IFFT, and centre-window (Kaiser or Blackman) to length L.
  3. Run partitioned FFT convolution. Report latency (L−1)/2 via `setLatencySamples`.
  4. Rebuild the kernel on a worker thread when parameters change and crossfade old and new outputs.
- **Natural/analog-phase approximations:** a minimum-phase FIR via the cepstral method from the analog magnitude has low latency but needs FFT processing. Alternatively, use matched-IIR for magnitude plus a short FIR phase-correction at high frequencies. FabFilter's actual method is unpublished.
- **Mid/side:** M = (L+R)/2, S = (L−R)/2; process each; L = M+S, R = M−S. Per-band channel routing (Stereo/L/R/M/S) is expected in a modern EQ. For dynamic bands, decide whether detection is linked (max or sum of channels) or independent.
- **Mix knob:** make it a *gain-scale* (depth) control that multiplies every band's dB gain, as in Pro-Q's global scale. Do not crossfade. In linear-phase mode only, a true dry/wet crossfade is coherent if the dry path is delayed by exactly the FIR latency.

### Gaps
- FabFilter's Natural Phase algorithm and latency are not publicly documented beyond the help page. I did not read the full help page (host blocked).

---

## 6. Dynamic EQ, resonance suppression, spectrum analyser

### Takeaway
Dynamic EQ: per band, a band-pass sidechain feeds an envelope detector and a gain computer that modulates band gain in dB, ideally with TPT filters. Soothe-style suppression detects resonant peaks relative to a smoothed spectral envelope and applies many level-dependent notches (vendor-level description only). Analyser: FFT 4096–8192, Hann window, 50–75% overlap, FFT run off the audio thread fed by an SPSC FIFO, per-bin ballistics, and an optional 3 or 4.5 dB/oct display tilt.

### Cited Findings
- **soothe2 (Oeksound), vendor/press descriptions:**
  - Detects resonances across the full range and reduces only the harsh parts, "where and when needed".
  - One review: it uses "spectral processing" to detect resonances and then applies "numerous level-sensitive notch filters".
  - Vendor: it is "free from pre-ring, crossovers, and summing artefacts".
  - Soft vs hard modes, the soft mode being less level-dependent.
  - v2 extends down to 20 Hz.
  - Attack/release for the notches; external sidechain.
  [S, vendor/press, not independently verified] — [Sound On Sound: soothe2 review](https://www.soundonsound.com/reviews/oeksound-soothe-2?amp); [oeksound soothe2 manual](https://oeksound.com/manuals/soothe2/); [KVR product page](https://kvraudio.com/product/soothe2-by-oeksound)
- **Pro-Q 4** has a documented dynamic EQ feature. [S, page not read] — [FabFilter Pro-Q 4 Help: Dynamic EQ](https://prod.fabfilter.com/help/pro-q/using/dynamic-eq)
- **JUCE spectrum analyser tutorial:**
  - `dsp::FFT` with a Hann window; `fftData` buffer sized 2× the FFT size; timer-driven redraw when a block is ready.
  - It does **not** use AbstractFifo. Forum users point out data-race and threading issues in it.
  - `AbstractFifo` is safe for single producer and single consumer and uses the `ScopedWrite` API. Without double buffering the FFT can get discontinuous input.
  - Architecture options (forum): (a) ring buffer and FFT on the audio thread, spectra sent via a lock-free queue; or (b) raw samples via a lock-free queue, FFT on the GUI or worker thread.
  [S] — [JUCE tutorial: spectrum analyser](https://docs.juce.com/master/tutorial_spectrum_analyser.html); [JUCE forum: data race in spectrum analyser tutorial](https://forum.juce.com/t/data-race-in-spectrum-analyser-tutorial/65715); [JUCE forum: lock-free queues and visualisation](https://forum.juce.com/t/lock-free-queues-and-visualization-of-data/20659); [AbstractFifo docs](https://docs.juce.com/master/classAbstractFifo.html); [CCRMA 320C FFT assignment](https://ccrma.stanford.edu/courses/320c/assignments/1-FFT/index.html)
- **Low-end resolution** is a known problem with FFT analysers. [S, forum thread, not read] — [JUCE forum: FFT spectroscope low-end resolution](https://forum.juce.com/t/fft-spectroscope-low-end-resolution/41209)
- **Display tilt:**
  - FabFilter's help says a 4.5 dB/oct default "results in a natural looking spectrum, resembling best how loudness is perceived by the human ear".
  - A FabFilter developer confirmed it is a crude "pleasing" reference. Voxengo SPAN reportedly uses the same default.
  - Pink noise looks flat at 3 dB/oct.
  - Conflict: one user says Pro-Q applies 0 dB/oct and Pro-MB 4.5. Treat which FabFilter product defaults to what as unresolved.
  [S] — [FabFilter Pro-Q 4 Help: Analyzer](https://fabfilter.com/help/pro-q/using/analyzer); [FabFilter forum topic 1608](https://prod.fabfilter.com/forum/topic/1608/pro-q-feature-request-spectrum-analyzer); [REW RTA help](https://www.roomeqwizard.com/betahelp/help/html/spectrum.html)

### Inferences
- **Dynamic EQ band design:**
  - Sidechain: an SVF band-pass at the band's f0/Q (or the external sidechain).
  - Detector: peak or RMS envelope, 1–100 ms attack, 10–1000 ms release, computed in dB.
  - Gain computer: threshold, ratio and range (max dB).
  - Output: band gain = static gain + dynamic offset, smoothed per sample, applied with a TPT SVF bell so audio-rate gain modulation does not click.
  - Lookahead is optional and adds latency.
  - Upward and downward modes are both expected.
- **Soothe-like processing at a high level:**
  - Short-time spectrum (or a filterbank) → compare each band with a smoothed or median spectral envelope → excess → per-band attenuation with attack/release → apply via a bank of dynamic notches or a smooth time-varying filter.
  - The vendor's "no pre-ring, no crossovers" claim suggests time-domain dynamic filters driven by analysis, not FFT-domain overlap-add. This is inference only.
- **Analyser specifics:**
  - Bin spacing is 44100/4096 ≈ 10.8 Hz and 48000/8192 ≈ 5.9 Hz [C].
  - Map bins to log-frequency pixels, using the max or interpolation across bins per pixel.
  - Apply per-bin attack/release ballistics in dB and an optional peak hold.
  - Run at about 30–60 fps. Rescale the FFT size with sample rate to keep the same Hz resolution at 96/192 kHz.
  - Show pre-EQ and post-EQ traces, plus the EQ curve computed from the actual digital coefficients (`|H(e^jw)|`), not the analog formulas, so the curve matches what you hear.

### Gaps
- I found no technical publication describing soothe2's actual algorithm. Treat the descriptions above as marketing-level.

---

## 7. Auto gain compensation

### Takeaway
Static compensation is predictable and automation-safe: compute it from the EQ curve, for example by weighting |H| over a pink or loudness-weighted spectrum. It is what FabFilter describes ("an educated guess based on the current EQ settings"). Dynamic, loudness-matched compensation tracks program material but pumps. Offer static by default.

### Cited Findings
- **Pro-Q Auto Gain** "automatically compensates for increase or loss of gain after EQ'ing". The Pro-Q 3 help describes it as an educated guess based on current EQ settings, not a precise loudness measurement. [S, vendor] — [FabFilter Pro-Q help: Output options](https://fabfilter.com/help/pro-q/using/output); [Pro-Q 4 help: Output](https://prod.fabfilter.com/help/pro-q/using/output)
- **Other approaches (forum):** some developers base the adjustment on the EQ curve (Crave EQ developer); others use LUFS-based gain. Users complain that signal-adaptive AGC moves the volume up and down while music plays. [S] — [KVR: EQs with auto-gain](https://www.kvraudio.com/forum/viewtopic.php?p=7123860); [KVR: Is automatic gain compensation for EQ something we want?](https://kvraudio.com/forum/viewtopic.php?p=7547360); [FabFilter forum topic 6673](https://prod.fabfilter.com/forum/topic/6673/add-automatic-make-up-gain-for-pro-q3)
- **Debate:** Sound On Sound and Production Expert discuss whether to use automatic level compensation. [S, not read] — [SOS Q&A on auto level compensation](https://www.soundonsound.com/sound-advice/q-should-use-my-plug-ins-automatic-level-compensation); [Production Expert](https://www.production-expert.com/home-page/2017/8/17/should-the-automatic-gain-compensation-feature-be-standard-on-all-eq-dynamics-plug-ins?format=amp)

### Inferences
- **Static algorithm:**
  - Compute `G = −10·log10( Σ_k W(f_k)·|H(f_k)|² / Σ_k W(f_k) )` on a log-spaced grid of 20 Hz–20 kHz.
  - Weight W with a pink spectrum (equal energy per octave), optionally times a K- or A-weighting curve.
  - Smooth G like any gain parameter.
  - It is deterministic, has no latency, and nulls in tests.
- **Dynamic algorithm:** run short-term loudness (BS.1770 K-weighted, about 3 s window) on input and output, then a slow servo (several seconds) on the difference with a cap. Use it only as an opt-in "match" function, ideally a one-shot "learn" that freezes the result.

### Gaps
- FabFilter's exact formula is not public.

---

## 8. Testing and verification

### Takeaway
Automate four things:
1. Analytic magnitude/phase checks of each band against the analog prototype and the coefficient-derived response, at every supported sample rate.
2. Bit-exact or −140 dB null tests (flat EQ = bypass; +g band followed by −g band = identity).
3. Impulse and swept-sine response capture through the built plugin.
4. pluginval at strictness 5 in CI and 10 before release, plus automation and denormal stress tests.

### Cited Findings
- **pluginval:**
  - Strictness 1–10; level 5 is "generally recognised as the lowest level for host compatibility".
  - Headless `./pluginval --strictness-level 5 <path>` returns exit code 0 on pass, which suits CI.
  - Recent releases add a concurrent-`setValue` parameter thread-safety test and a timeout option.
  - Passing level 10 improves DAW compatibility but "doesn't guarantee your plugin will be bug free".
  [S, high] — [pluginval README](https://github.com/Tracktion/pluginval/blob/master/README.md); [Testing plugins with pluginval](https://github.com/Tracktion/pluginval/blob/master/docs/Testing%20plugins%20with%20pluginval.md); [JUCE forum announcement](https://forum.juce.com/t/anouncing-pluginval-an-open-source-cross-platform-plugin-validation-tool/27710); [JUCE cookbook: pluginval](https://tobanteaudio.gitbook.io/juce-cookbook/testing/pluginval)
- **Community norm:** level 5 as the minimum and level 10 for release checks. Steinberg's forum references level 5 as the baseline. [S, community norm] — [Steinberg forum: pluginval](https://forums.steinberg.net/t/pluginval-plugin-validation-helpful/1029205)
- **Boost/cut symmetry** (section 1, [C]) makes "+g then −g = identity" a valid exact null test for RBJ/SVF bells.

### Inferences
- **Unit tests** (Catch2 or GoogleTest against the DSP core, outside the plugin wrapper):
  - (a) For each band type and a grid of f0 (20 Hz–20 kHz), Q (0.1–40) and gain (±30 dB), at fs ∈ {44.1, 48, 88.2, 96, 176.4, 192} kHz: compare the measured response (impulse → FFT) with `|H(e^jw)|` from the coefficients, within 0.01 dB. Compare against the analog prototype within a tolerance mask, for example 0.1 dB up to 0.45·fs for matched designs.
  - (b) A flat EQ, or all bands at 0 dB, must be bit-exact with bypass. Make gain = 0 dB mathematically exact in your filter: with A = 1, RBJ and SVF reduce to identity.
  - (c) Linear-phase mode, flat: output equals input delayed by the reported latency (null below −120 dB).
  - (d) Sample-rate independence: the same settings at different fs give matching responses below about 0.4·fs_min.
  - (e) Stability and finiteness under random audio-rate parameter modulation (no NaN/Inf, bounded output). Add a click metric: energy above 18 kHz in the output relative to a smoothed reference.
  - (f) Denormals: feed a signal then silence for 10 s and assert CPU time does not spike. With `ScopedNoDenormals` the states should flush.
  - (g) State save and restore round-trip, plus preset recall with no sweep.
  - (h) Latency reporting changes (phase-mode switch) are reported correctly. Hosts must handle `setLatencySamples` changes.
- **Integration:** also run Apple `auval` for AU and Steinberg's VST3 validator. Do null tests in at least two DAWs with automation lanes. Use REW or a MATLAB/Python harness that sweeps a log-sine through the rendered plugin.

### Gaps
- I did not retrieve the full list of pluginval test categories per strictness level.
- I did not verify AU/VST3 validator specifics this session.
