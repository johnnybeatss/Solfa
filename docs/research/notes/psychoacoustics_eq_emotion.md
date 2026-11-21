# Psychoacoustics of EQ, Timbre and Perceived Emotion (for designing emotion-targeted EQ presets)

Research notes compiled 2026-11-21. Scope: frequency-region descriptors, spectral centroid/tilt and emotion, equal-loudness, loudness bias and level matching, critical bands/ERB and resonance audibility, harmonic/octave/tuning interactions, and practical gain/Q conventions.

**How to read these notes**
- Each finding is tagged **[RESEARCH]** (peer-reviewed or standards), **[ENGINEERING]** (manufacturer docs, trade press, educational/engineering references, i.e. established practice or lore), or **[DERIVED]** (arithmetic from a cited formula).
- Confidence: **High** = replicated or standardised; **Medium** = a single good study or consistent practice; **Low** = small or pilot studies, secondary summaries, or lore.
- **Method caveat:** the session's network blocked full-text fetches from bcu.ac.uk, jyx.jyu.fi, arxiv.org, pmc.ncbi.nlm.nih.gov and interactiveaudiolab.github.io. Many findings therefore come from search-indexed abstracts and snippets of the primary papers rather than the full texts. Where a number could not be checked against the primary text, it is flagged. AES papers (Bücklein, Toole & Olive) are paywalled, so their exact threshold tables were not retrieved.

---

## 1. Which frequency regions map to perceptual descriptors (sub, bass, warmth, mud/boxiness, honk, presence, harshness/sibilance, brilliance, air)?

### Takeaway
Research shows that timbre semantics reduce to roughly three dimensions: luminance (bright vs dull), texture (smooth vs rough) and mass (thick vs thin). "Warm" and "bright" are the two most-used EQ words in crowdsourced data. Agreement on what EQ curve a word means is only moderate, so presets can match descriptors on average, but not for every listener or source. The fixed frequency-band charts (mud 200–500 Hz, presence 2–5 kHz, air 10 kHz and up) are engineering consensus, not lab-derived. Their boundaries shift by instrument and by source.

### Cited Findings
**Timbre semantic dimensions [RESEARCH]**
- Greek and English listeners rated tones on 30 adjectives. Data reduction gave three shared semantic dimensions, **luminance, texture and mass**, which the authors read as support for cross-language universality of timbre semantics. Texture was linked to the energy distribution of harmonic partials. Brilliance was linked to inharmonicity and spectral-centroid variation. (Zacharakis, Pastiadis & Reiss, *Music Perception*, 2014.) Confidence: High for the existence of the dimensions. — [Zacharakis, Pastiadis & Reiss 2014 (PDF)](https://joshreiss.github.io/documents/2014/Zacharakis%20Pastiadis%20Reiss%20-%20Music%20Perception.pdf)
- A follow-up compared semantics with pairwise dissimilarity ratings. "Texture" and "luminance" had the strongest associations with perceptual dimensions in both languages. **"Mass" failed to show strong correlations** with perceptual space. (*Music Perception*, 2015.) Confidence: Medium. — [Zacharakis, Pastiadis & Reiss 2015 (PDF)](https://joshreiss.github.io/documents/2015/Zacharakis%20Pastiadis%20Reiss%20-%20Music%20Perception%202015.pdf)
- A confirmatory paper revisited the model: "Revisiting the Luminance-Texture-Mass Model for Musical Timbre Semantics" (Zacharakis & Pastiadis, AES 2016). — [AES e-library 18372](https://aes2.org/publications/elibrary-page/?id=18372)
- An independent replication on FM-synthesis sounds found two factors. One loaded on **mass and darkness together**, the other on texture. These are congruent with the model, but the factor order suggests mass explains more variance. (Hayes & Saitis 2021.) Confidence: Medium. — [Hayes & Saitis 2021 (PDF)](https://comma.eecs.qmul.ac.uk/assets/pdf/hayes_perceptual_2021.pdf)

**Crowdsourced EQ descriptor data [RESEARCH]**
- In the SAFE project data (Stables, De Man et al.), **"warm" and "bright" are the top two descriptors overall**, with 193 and 153 instances. For the EQ effect specifically, the counts were 440 "warm" and 424 "bright". Confidence: High that these are the dominant words. — [Stables/De Man et al., semantic description of timbral transformations (QMUL accepted MS)](https://qmro.qmul.ac.uk/xmlui/bitstream/123456789/22150/1/De%20Man%20Semantic%20description%20of%20timbral%202016%20Accepted.pdf)
- Stasis, Stables & Hockman built descriptor sub-representations of "warm" and "bright" from about 900 SAFE instances (NIME 2017). Their AES 2016 workshop paper notes **"high within-term variance... suggesting disagreement amongst participants."** Confidence: High that variance is substantial. — [Stasis et al. NIME 2017 (proc. page)](https://nime.org/proc/sstasis2017); [Stasis et al. AES WIMP 2016 (core.ac.uk)](https://core.ac.uk/download/141206793.pdf); [Stasis PhD thesis, *Audio Equalisation Using Natural Language*](https://www.open-access.bcu.ac.uk/8847/1/PhD%20Thesis%20-%20Spyridon%20STASIS%20-%20Audio%20Equalisation%20Using%20Natural%20Language.pdf)
- SAFEEqualiser maps a typed word to **averaged** crowd parameter settings. Each word sets three parametric bands plus a low shelf and a high shelf. (Plugin description, not a peer-reviewed result.) [ENGINEERING] — [SAFEEqualiser (KVR)](https://www.kvraudio.com/product/safeequaliser-by-semantic-audio/details)
- In SocialEQ (Cartwright & Pardo), 887 Mechanical Turk participants completed 2,322 sessions, each learning an EQ curve for a word they chose. In the agreement ranking, **"warm" had the most sessions (64) but an agreement score of only 0.139**, while "tinny" ranked first for agreement (0.294, 8 sessions). Common words are therefore not necessarily the most consistent. (Collective Intelligence 2014; original paper ISMIR 2013.) Confidence: Medium. These figures come from a search summary of the paper; the full text was not fetched. — [Cartwright & Pardo 2014 (PDF)](https://interactiveaudiolab.github.io/assets/papers/cartwright_pardo_collintel_2014.pdf)
- Later work reuses these vocabularies. Audealize (Seetharaman & Pardo, JAES) seeds an EQ and reverb interface with crowdsourced descriptors. A 2022 paper uses word embeddings for automatic EQ. A 2025 arXiv paper, "Words of Warmth", focuses on the "warm" descriptor (contents not retrieved). — [Seetharaman & Pardo, Audealize (JAES PDF)](https://pseeth.github.io/public/papers/seetharaman_pardo_audealize_jaes.pdf); [Word Embeddings for Automatic Equalization (arXiv 2202.08898)](https://arxiv.org/pdf/2202.08898); ["Words of Warmth" (arXiv 2506.03993)](https://arxiv.org/html/2506.03993v1)

**Engineering frequency-band conventions [ENGINEERING] (Confidence: Medium as consensus, Low as precise boundaries)**
- iZotope: **200–500 Hz** is the "boxiness" zone for kick and bass, and **300–500 Hz** is "mud" for bass. — [iZotope EQ cheat sheet](https://www.izotope.com/en/learn/eq-cheat-sheet)
- Other cheat sheets list fullness/mud at **250–450 Hz**, honk at **450 Hz–1 kHz** (a wide cut there removes harsh, boxy character), presence at **2.5–4.5 kHz** for vocals, sibilance at **4–7 kHz** (some say 6–10 kHz), and air at **10–15 kHz** and above. — [Scribd EQ cheat sheet](https://www.scribd.com/document/714720470/eq-cheat-sheet); [Guitar Center EQ cheat sheet](https://www.guitarcenter.com/riffs/gear-tips/recording/eq-cheat-sheet-for-mixing); [Dummies: home recording EQ guidelines](https://www.dummies.com/article/technology/software/music-recording-software/general-music-recording-software/home-recording-eq-frequency-guidelines-179779/)
- The mastering.com cheat sheet itself warns against relying on such charts when mixing. — [mastering.com EQ cheat sheet (PDF)](https://mastering.com/wp-content/uploads/2017/07/EQ-CHEAT-SHEET.pdf)
- General EQ technique references from Sound on Sound. — [SOS: Using EQ](https://www.soundonsound.com/techniques/using-eq); [SOS: EQ, what do all those knobs do](https://www.soundonsound.com/sound-advice/eq-what-do-all-those-knobs-do)

### Inferences
- **Map preset goals onto the research axes** (luminance, texture, mass), not onto ad hoc words:
  - "Warm", "grounded" and "intimate" mostly mean low luminance with some added mass.
  - "Clear", "airy" and "uplifting" mean higher luminance with less mass.
  - "Calm" also involves smoothness (low roughness and low flux, see Q2). EQ cannot fully deliver that, because roughness comes from the source content.
- **Candidate preset curves, assembled from the consensus bands** (engineering lore informed by the research dimensions, Confidence: Medium):
  - *Warm*: gentle low-mid lift about 100–300 Hz plus a gentle high-shelf reduction above about 6–8 kHz, which lowers the centroid.
  - *Grounded*: low shelf or broad bell about 60–150 Hz, with a small cut around 250–400 Hz so the lift does not turn to mud.
  - *Clear*: broad cut 200–500 Hz, slight lift 2–5 kHz.
  - *Airy* or *ethereal*: high shelf at 10 kHz or above, plus a low-cut or low-shelf reduction to remove mass.
  - *Intimate*: modest presence (2–4 kHz), soft top end, and controlled 200–400 Hz proximity build-up.
  - *Uplifting*: raise the centroid via a tilt toward the highs; arousal rises with brightness (Q2).
- Because within-term variance is high, presets should be **starting points with a single "amount" macro**, not fixed curves. Measured descriptor curves (SAFE, SocialEQ) are averages across many sources, so a preset's effect will vary with the input signal.

### Gaps
- The exact SAFE and SocialEQ "warm" and "bright" curves (band gains and frequencies, and centroid shifts) could not be retrieved because the full texts were blocked. The Stasis thesis and the Cartwright & Pardo PDF hold these numbers.
- Bob Katz (*Mastering Audio*), Owsinski (*The Mixing Engineer's Handbook*) and Izhaki (*Mixing Audio*) frequency tables were not accessed online. No page-level citations are given for them.
- I found no peer-reviewed lab study that validates the cheat-sheet band boundaries such as "mud = 250–450 Hz". These boundaries remain practice-derived.

---

## 2. Spectral centroid, tilt and perceived brightness, arousal, valence and tension

### Takeaway
Perceived brightness tracks spectral centroid reasonably well (r of about 0.5), but not completely. Brighter, higher-centroid timbres raise **energy arousal** and **tension**. Valence is better predicted by spectral **slope** and attack and envelope features than by centroid alone. "Calm" relates more to low spectral flux (temporal stability) than to darkness. Darkening an EQ curve therefore lowers arousal reliably, while its effect on valence is weaker and less consistent.

### Cited Findings
- **Brightness vs centroid [RESEARCH]**: perceived timbral brightness correlated with raw spectral centroid at **r = 0.513 (p ≤ 0.01)**, but only weakly with centroid divided by the fundamental (r = 0.030). (Schubert & Wolfe, *Acta Acustica united with Acustica* 92, 2006, pp. 820–825.) Confidence: Medium-High. — [Schubert & Wolfe 2006 (PDF)](https://www.phys.unsw.edu.au/jw/reprints/SchubertWolfe06.pdf)
- Later JASA work found **consistent brightness shifts between tones with nearly identical centroids** (Siedenburg 2018), so centroid is not a complete brightness predictor. Saitis & Siedenburg (2020, JASA) found attack time and spectral centroid to be the two most robust correlates of timbre dissimilarity. These summaries come via a search synthesis, and the papers were not fetched directly. Confidence: Medium. — [Wikipedia: Spectral centroid (cites the literature)](https://en.wikipedia.org/wiki/Spectral_centroid); [PMC7536169](https://pmc.ncbi.nlm.nih.gov/articles/PMC7536169)
- **Eerola, Ferrer & Alluri (2012), "Timbre and affect dimensions", *Music Perception* 30(1):49** [RESEARCH]:
  - Design: 110 isolated instrument sounds, equalised for duration, pitch and dynamics, rated on valence, energy arousal, tension arousal, preference and intensity.
  - Valence and energy arousal were predicted at **50–57%** by a few features: the **ratio of high- to low-frequency energy**, **attack slope**, and **spectral flux**.
  - Confidence: High for the general result. The coefficient signs were not verified from the full text. — [Eerola et al. 2012 (JYX PDF)](https://jyx.jyu.fi/bitstream/123456789/48672/1/eerolatetal49.full.pdf); [JYX record](https://jyx.jyu.fi/jyx/Record/jyx_123456789_48672)
- A 2017 *Frontiers in Psychology* paper using the same three-dimensional affect model reports that [RESEARCH, via abstract and snippet]:
  - **Greater energy arousal goes with brighter sounds**: higher spectral centroid, slower decrease of spectral slope, and greater spectral emergence.
  - **Valence is more positive with lower spectral slopes**, greater emergence of strong partials, and sharper attack with earlier decay.
  - **Tension arousal goes with brighter sounds**, more spectral variation, and gentler attacks.
  - Confidence: Medium. — [Frontiers in Psychology 2017, 8:153](https://www.frontiersin.org/journals/psychology/articles/10.3389/fpsyg.2017.00153/full); [PMC5296353](https://pmc.ncbi.nlm.nih.gov/articles/PMC5296353)
- **Contrary evidence**: Dean & Bailes (2011) modelled continuous valence and arousal in one piece. Spectral centroid and flatness had **no direct overall influence** on perceived valence or arousal. This is cited in a 2026 McGill *Music Perception* paper. Confidence: Medium, and it shows that context matters. — [Heng et al. 2026 (McGill MPCL PDF)](https://www.mcgill.ca/mpcl/files/mpcl/heng_2026_muspercept.pdf)
- Wu, Horner & Lee (ISMIR 2013): emotion judgments correlate significantly with average spectral centroid for the more distinctive emotions (Happy, Joyful, Sad, Depressed, Shy). Horn and flute evoked sad and depressed. The direction per emotion was not visible in the snippet. Confidence: Medium. — [Wu et al. ISMIR 2013](https://archives.ismir.net/ismir2013/paper/000007.pdf); [Wu et al. ISMIR 2014](https://archives.ismir.net/ismir2014/paper/000109.pdf)
- A 2025 paper reports that **low spectral flux aligns with low-arousal (calm) states**. Confidence: Low-Medium; the source was only seen as a snippet. — [CAIT 2025 (UEF PDF)](https://cs.uef.fi/sipu/pub/CAIT-2025.pdf)
- **Juslin & Laukka** [RESEARCH, review]:
  - The 2003 *Psychological Bulletin* review (129(5):770–814) showed that emotion cues are shared between vocal expression and music performance.
  - Secondary sources attribute to Juslin & Laukka (2004) the cue mapping: **bright timbre goes with happiness, dull with sadness, sharp with anger, and soft with tenderness and fear**.
  - Confidence: Medium. The attribution comes from secondary citations. — [Juslin & Laukka 2003 (Scribd copy)](https://www.scribd.com/document/696920957/Juslin-emotion2003); [PMC3726864 (cites the mapping)](https://pmc.ncbi.nlm.nih.gov/articles/PMC3726864)
- Many earlier emotion studies did not isolate timbre from pitch, duration and intensity (noted in a 2018 *Frontiers* paper). [RESEARCH] — [Frontiers in Psychology 2018, 9:737](https://www.frontiersin.org/journals/psychology/articles/10.3389/fpsyg.2018.00737/epub)

### Inferences
- **Arousal axis (most reliable lever)**:
  - "Calm", "grounded" and "warm" presets: lower the centroid by tilting down the highs or reducing above about 5 kHz. Avoid presence peaks, which also raise tension.
  - "Uplifting", "energetic" and "clear" presets: raise the centroid with a gentle high-shelf or tilt. Confidence: Medium-High.
- **Valence axis (weaker lever)**: positive valence is linked to strong partials standing out ("emergence") and to the spectral-slope shape, not simply to brightness. A preset that boosts harshness (about 2–5 kHz, the ear's most sensitive region) can raise arousal while *lowering* valence through added tension. Confidence: Medium.
- **"Ethereal" and "airy"** most plausibly means high luminance with low mass: a high-shelf at 10 kHz or above plus reduced lows. This raises brightness without the tension of a 2–5 kHz boost. That combination is an inference from the frequency-sensitivity data in Q3 and has not been tested directly.
- EQ changes only the long-term spectrum. The attack and flux features that drive valence and calm come mostly from the arrangement and dynamics, so the emotional effect of EQ alone is partial.

### Gaps
- No direct experiment was found that applied EQ tilts (for example ±3 dB/oct) to full mixes and measured valence, arousal and calm. Most data use isolated instrument tones.
- The exact coefficients and signs for the high/low energy ratio in Eerola et al. (2012) could not be verified, because the full text was blocked.
- I found no study specifically linking spectral slope to "calmness" in finished music; this is inferred from the arousal and flux findings above.

---

## 3. Equal-loudness contours (ISO 226:2023), Fletcher-Munson, and maximum ear sensitivity

### Takeaway
ISO 226:2023 differs from ISO 226:2003 by **0.6 dB at most**, so it is practically the same. The key design fact is unchanged: the ear is least sensitive at low frequencies and at low playback levels, and most sensitive around **2.5–5 kHz** (ear-canal and concha resonances). A preset tuned at a loud level will sound thinner when played quieter. Boosts in the 2–5 kHz region are perceived very strongly.

### Cited Findings
- **ISO 226:2023** [STANDARD] is the third edition (2023-03), replacing ISO 226:2003 after a technical revision. It still covers pure tones only. Confidence: High. — [ISO 226:2023 (iso.org)](https://www.iso.org/standard/83117.html); [ANSI preview PDF](https://webstore.ansi.org/preview-pages/ISO/preview_ISO+226-2023.pdf)
- **What changed** [RESEARCH, review by Suzuki, Takeshima & Kurakata]: Confidence: High. — [Acoust. Sci. & Tech. 45(1):1–8, 2024, doi:10.1250/ast.e23.66](https://www.jstage.jst.go.jp/article/ast/45/1/45_e23.66/_article)
  - It aligns with ISO 389-7:2019, which lowered the 20 Hz hearing threshold by 0.4 dB.
  - It implements a 2004 formulation of the loudness power exponent, with improved numerical expressions.
  - Differences from the 2003 edition are **at most 0.6 dB**, and the review regards the two editions as the same "in terms of practical use".
- A Forum Acusticum 2023 paper frames the change as a clarification of the standard. — [Forum Acusticum 2023 paper (EAA)](https://dael.euracoustics.org/confs/fa2023/data/articles/000585.pdf)
- **Maximum sensitivity and ear-canal resonance** [RESEARCH / educational physics]:
  - Peak hearing sensitivity is about **3.5–4 kHz**, modelled as the closed-tube resonance of a roughly 2.4 cm ear canal (about 3.7 kHz). — [HyperPhysics: frequencies for maximum sensitivity](https://hyperphysics.gsu.edu/hbase/Sound/maxsens.html)
  - Measured adult ear-canal gain is typically **15–25 dB, peaking near 2.7 kHz** (quarter-wave resonance), with a **secondary peak near 5 kHz** from concha resonance. — [Goode, ORL Zürich abstract (PDF)](https://www.orl.uzh.ch/memro/program/3.9_Goode.pdf)
  - The quarter-wavelength model and measurements show a resonance near 4 kHz. — [UCL AUDL4007 notes (PDF)](https://www.phon.ucl.ac.uk/courses/spsci/AUDL4007/12.pdf)
  - Outer and middle ear together boost the 1–4 kHz region by about 10–20 dB. — [SFU Handbook for Acoustic Ecology tutorial](https://www.sfu.ca/sonic-studio-webdav/cmns/Handbook%20Tutorial/Audiology.html)
  - Confidence: High for the range. The exact peak varies between about 2.7 and 4 kHz, depending on the individual and the measurement method.
- Fletcher-Munson and equal-loudness lab material notes the sensitivity peak in part reflects the first ear-canal resonance. — [Stanford CCRMA loudness lab](https://ccrma.stanford.edu/CCRMA/Courses/SummerWorkshops/96/Psychoacoustics/labs/loudness)

### Inferences
- **Level-dependence trap**:
  - The equal-loudness contours flatten as level rises, so bass and extreme treble seem relatively stronger when loud.
  - A "warm" or "grounded" preset judged at loud monitor levels will feel thinner at background levels.
  - A plugin could offer a "listening level" compensation (loudness-contour-based low and high shelves) or recommend auditioning at a reference level.
  - Confidence: Medium-High. This is based on the well-established contour shapes; no listening study on EQ presets was found.
- **2–5 kHz sensitivity trap**: small boosts here have outsized perceptual weight and push toward "harsh" and tension. For "calm", "warm" or "intimate", keep 2–5 kHz neutral or slightly reduced. For "clear" or "presence", use small gains (about 1–3 dB, broad) rather than large ones.
- Because ISO 226:2023 is practically the same as the 2003 edition, any loudness-compensation curve already built on ISO 226:2003 does not need redesigning.

### Gaps
- The specific ISO 226:2023 phon-contour tables were not retrieved (paywalled). Use the standard itself for numeric curves.
- No study was found that measures how emotional descriptors of the same EQ curve shift with playback level.

---

## 4. Loudness bias in A/B comparisons, and auto-gain / level matching (LUFS, ITU-R BS.1770-5)

### Takeaway
Listening-test practice universally loudness-matches stimuli, because level differences confound preference: louder usually sounds "better". For a plugin, the defensible basis for auto-gain is **ITU-R BS.1770 (K-weighted, gated) loudness**, which is the current revision BS.1770-5 (11/2023). Matching by ear invites bias.

### Cited Findings
- **BS.1770-5** [STANDARD]: "Algorithms to measure audio programme loudness and true-peak audio level", revision 5, dated 11/2023, in force. Confidence: High. — [ITU-R BS.1770-5 (ITU)](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I); [BS.1770-5 PDF](https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.1770-5-202311-I!!PDF-E.pdf)
- **K-weighting** = a first-stage shelving filter plus a second-stage high-pass, followed by per-channel mean-square, channel weighting and gating. Confidence: High. — [ITU-R BS.1770-1 PDF (algorithm description)](https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.1770-1-200709-S!!PDF-E.pdf); [Wikipedia: LUFS](https://en.wikipedia.org/wiki/LUFS)
- **Gating**: the gating function was introduced in BS.1770-2, and EBU Tech 3341 attributes it to the BS.1770 recommendation.
  - Blocks are **400 ms with 75% overlap**.
  - There is an **absolute gate of −70 LUFS** and a relative gate (below).
  - LUFS = LKFS.
  - EBU R128's target level is **−23 LUFS**.
  - Confidence: High for BS.1770-4. I did not verify whether -5 changed these parameters. — [EBU Tech 3341 (PDF)](https://tech.ebu.ch/docs/tech/tech3341.pdf); [EBU R128 v5.0 (PDF)](https://tech-qual.ebu.ch/files/live/sites/tech/files/shared/r/r128v5_0.pdf)
- **Loudness matching in listening tests** [RESEARCH]:
  - Paired-comparison tests usually present loudness-matched stimuli to remove loudness effects on ratings.
  - Matching is often done by ear by the experimenter, which is identified as a major difference between lab tests and real-life comparisons.
  - An AES study let listeners set their own level before choosing a preferred loudspeaker.
  - Confidence: Medium-High. — [Brest/HAL loudspeaker preference study](https://hal.univ-brest.fr/hal-00638418/document); [AES e-lib 14978](https://www.aes.org/e-lib/browse.cfm?elib=14978)
- Equal-loudness matching procedures can introduce **experimenter bias**, and the choice of spectral-matching filter is itself a methodological variable. (Forum Acusticum 2025.) [RESEARCH] Confidence: Medium. — [Forum Acusticum 2025 paper (EAA)](https://dael.euracoustics.org/confs/fa2025/data/articles/000756.pdf)
- Minimum detectable level changes are reported as roughly **0.5–3 dB** depending on conditions. Toole & Olive (1988) found a **5 kHz, Q = 1 resonance just detectable at 0.25 dB in pink noise**. These figures come via the secondary summary. [RESEARCH, via ENGINEERING summary] Confidence: Medium. — [Audioholics: human hearing amplitude sensitivity](https://www.audioholics.com/room-acoustics/human-hearing-amplitude-sensitivity-part-1)
- A double-blind headphone EQ preference test (RTINGS, 35 participants, 5 curves) found no clearly preferred curve, except that the low-bass anchor was least preferred. It is a useful reference for randomised, level-controlled design. [ENGINEERING / informal research] Confidence: Low-Medium. — [RTINGS: One Curve Doesn't Fit All](https://www.rtings.com/headphones/learn/research/frequency-response-1-8)

### Inferences
- **Auto-gain design** (inferred from the standard plus the matching literature, Confidence: Medium-High):
  - Measure short-term or integrated BS.1770 loudness of input and output, using the same K-weighting and gating.
  - Apply make-up gain so that enabling a preset produces about 0 LU change.
  - Smooth the gain, with time constants on the order of seconds, to avoid pumping.
  - Offer a "match on bypass" A/B toggle.
- A static alternative is to compute, at preset-design time, the expected K-weighted loudness change of the EQ curve against a pink- or music-shaped reference spectrum, and pre-compensate. This avoids dynamic gain riding, but it is only approximate for program material that differs from the reference spectrum.
- Since detectable level differences can be well under 1 dB, matching should aim for **≤0.5 LU residual** if a preset's "feel" is to be judged on timbre rather than loudness.
- K-weighting places extra weight on frequencies above about 1.5–2 kHz (the shelving stage), roughly following ear sensitivity. Presence and air boosts will therefore receive proportionally more gain reduction under auto-gain than low-mid "warm" boosts. This is the desired behaviour, because it removes the "brighter = louder = better" confound.

### Gaps
- I found no primary source for a specific threshold such as "0.5 dB louder is reliably preferred" in EQ comparisons. The general louder-is-better bias is standard in listening-test methodology, but its precise magnitude was not sourced here.
- I did not verify whether BS.1770-5 changed any K-weighting or gating parameters relative to -4.

---

## 5. Critical bands / ERB, masking, narrow vs broad boosts, resonance audibility, and EQ JNDs (Bücklein 1981)

### Takeaway
The auditory filter bandwidth is ERB = 24.7(4.37 f_kHz + 1) Hz, which is about 1/3 to 1/6 octave above 500 Hz and proportionally wider at low frequencies. Bücklein (1981) showed **peaks are far more audible than equal-size dips**. Broad, low-Q changes are audible at very small gains because they shift energy across many auditory filters. Narrow boosts are audible mainly when they line up with musical content, and then they "ring" or sound resonant. For "tone colouring", use broad, gentle curves. For "surgical" work, cut narrow rather than boost narrow.

### Cited Findings
- **ERB formula** [RESEARCH]: **ERB = 24.7(4.37 f + 1)**, with f in kHz and ERB in Hz. It is valid at moderate levels for about 0.1–10 kHz, and was derived from notched-noise data. (Glasberg & Moore, *Hearing Research* 47:103–138, 1990.) The ERB-number scale is 21.4 log10(0.00437 f + 1), with f in Hz. Below about 500 Hz the filter bandwidth keeps shrinking, so the formula is less accurate there. Confidence: High. — [Wikipedia: Equivalent rectangular bandwidth](https://en.wikipedia.org/wiki/Equivalent_rectangular_bandwidth); [J.O. Smith, CCRMA: ERB](https://ccrma.stanford.edu/~jos/sasp/Equivalent_Rectangular_Bandwidth.html); [Wikipedia: Critical band](https://en.wikipedia.org/wiki/Critical_band)
- **Bücklein 1981** [RESEARCH]: "The Audibility of Frequency Response Irregularities", *JAES* 29(3):126–131.
  - Panel tests used music, speech and white noise.
  - **Peaks are much more audible than dips of equal size.** Peaks are detected even when same-size valleys are not.
  - In a telephone intelligibility test, peaks reduced intelligibility more than valleys.
  - A secondhand summary adds that with white noise, wider bandwidth means more total energy change and higher audibility.
  - Confidence: High for peaks vs dips. Exact thresholds were not retrieved (paywalled). — [AES e-library 10291](https://www.aes.org/e-lib/browse.cfm?elib=10291); [Stereophile: audibility of narrow EQ boosts and cuts](https://stereophile.com/content/audibility-narrow-eq-boosts-and-cuts)
- **Toole & Olive** [RESEARCH]:
  - The 1986 and 1988 AES papers measured resonance audibility thresholds as a function of frequency, Q, amplitude, program, listener, directivity and room.
  - Olive et al. (low-frequency follow-up, 63–500 Hz, 1990s) found that with **pink noise, detection thresholds decrease with increasing Q**. With **impulsive/pulse signals, thresholds drop considerably at high Q (Q = 30)**, which is the "ringing" effect. They used 70.7% thresholds by the UDTR adaptive procedure for single added peaks and notches.
  - The paper summary is from Fazenda et al. and the AES abstracts. Confidence: Medium-High. — [AES e-library 5024 (Toole & Olive)](https://aes2.org/publications/elibrary-page/?id=5024); [AES e-library 7868 (Olive et al.)](https://aes2.org/publications/elibrary-page/?id=7868); [Fazenda, Stephenson & Goldberg, perceptual thresholds of modal decay (PDF)](https://eprints.hud.ac.uk/id/eprint/24317/1/FazendaStephensonGoldberg-PerceptualThresholdsModalDecay.pdf)
- A Q = 1 resonance at 5 kHz was just detectable at **0.25 dB** in pink noise (Toole & Olive 1988, via secondary summary). Confidence: Medium. — [Audioholics](https://www.audioholics.com/room-acoustics/human-hearing-amplitude-sensitivity-part-1)
- **Content dependence** [ENGINEERING demonstrations]: a very narrow ±10 dB (Q = 24) change at 165 Hz is clearly audible when it coincides with bass notes, and otherwise much less so. Confidence: Medium. — [RealTraps: audibility of narrow-band EQ](https://realtraps.com/art_narrow.htm); [Stereophile](https://stereophile.com/content/audibility-narrow-eq-boosts-and-cuts)

### Inferences
- **[DERIVED] ERB in Q terms**, using Q ≈ f/ERB for a bell whose bandwidth equals one ERB:

  | Centre f | ERB (Hz) | Equivalent Q |
  |---|---|---|
  | 50 Hz | 30 | 1.7 |
  | 100 Hz | 36 | 2.8 |
  | 200 Hz | 46 | 4.3 |
  | 500 Hz | 79 | 6.4 |
  | 1 kHz | 133 | 7.5 |
  | 4 kHz | 457 | 8.8 |
  | 10 kHz | 1,104 | 9.1 |

  So a Q of about 9 or higher above 1 kHz (about 5 or higher at 200 Hz) affects less than one auditory filter. That is "surgical" territory, perceived as a pitched resonance or "ring" rather than a tonal tilt. Q ≤ 1 spans many ERBs and is perceived as colour or tilt.
- **Colouring presets should be cut-friendly and peak-averse**: given Bücklein's asymmetry, a "warm" effect made by *cutting* highs is less conspicuous, and less likely to sound like an artefact, than one made by boosting lows with a resonant bell. Narrow boosts are the main source of "honky", "ringy" and "cheap" artefacts.
- **Masking**: boosting a band raises its masking of adjacent bands (upward spread of masking is stronger toward higher frequencies, a standard psychoacoustic result). A broad low-mid "warm" boost can therefore mask presence detail. Pairing a warm boost with a slight 2–4 kHz lift, or a 250–400 Hz cut, is a reasoned compensation. The upward-spread claim was not separately sourced in this session.
- **JND guidance for preset macros**: broad changes below about 0.5 dB are near threshold. A useful "amount" range for colour presets is about 0.5–4 dB on broad shelves or bells, not the 10–15 dB ranges used for corrective work.

### Gaps
- Bücklein's and Toole & Olive's exact threshold tables (dB vs Q vs frequency, and music vs noise) are behind the AES paywall and were not retrieved.
- I found no controlled study of music-signal JNDs for parametric EQ gain at several Q values together with the frequency JND (how far a bell's centre must move to be noticed).
- I did not source a primary citation for the asymmetry of upward spread of masking in this session (it appears in Moore's *Introduction to the Psychology of Hearing*, which was not accessed).

---

## 6. Harmonic series and octave relationships, pitch chroma, and fixed-frequency boosts vs musical key and tuning ("spiritual frequencies")

### Takeaway
I found no peer-reviewed evidence that boosting a specific "special" frequency (432 Hz, 528 Hz, and so on) or its octaves produces specific emotions. 432 Hz tuning studies are small, mixed and mostly pilot-scale. Psychoacoustically, what matters is that a narrow fixed-frequency boost interacts with **whatever notes and harmonics happen to fall in it**. It is key-dependent and often heard as a resonance. A broad boost is key-independent and heard as timbre.

### Cited Findings
- **432 Hz evidence** [RESEARCH, low quality]:
  - Calamassi & Pomponi (2019) compared soundtracks tuned to 440 Hz and 432 Hz on different days. They reported slightly lower heart rate and blood pressure after 432 Hz, but the sample was very small and not randomised, so expectancy or relaxation could explain it. — [ScienceAlert summary](https://www.sciencealert.com/432hz-brain-tuning-is-an-ancient-idea-does-it-actually-work)
  - A 2023 Padova thesis reviewed 13 studies. The evidence was mixed, and "a clear understanding of the effects of 432 Hz could not be reached". — [Marmolaro 2023 thesis (PDF)](https://thesis.unipd.it/bitstream/20.500.12608/59910/1/Marmolaro%20Mattia.pdf)
  - A 2025 randomised cross-over trial in cancer patients found that both 432 Hz and 443 Hz tuning reduced heart rate. The median drop was −3 vs −1 bpm, p = 0.04, and there was no 440 Hz arm. — [PMC11755923](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC11755923/)
  - A 2022 randomised pilot in emergency nurses concluded that 432 Hz "may help" manage anxiety and that further studies are needed. — [PMC9534204](https://pmc.ncbi.nlm.nih.gov/articles/PMC9534204); [PubMed 35545982](https://pubmed.ncbi.nlm.nih.gov/35545982/)
  - A popular explainer quotes music psychologists who are sceptical. — [PsyPost](https://www.psypost.org/can-tuning-music-to-432hz-really-heal-you-scientists-explain-the-viral-trend/)
  - Confidence: Low for any specific effect.
  - Note: these studies concern **retuning the whole piece**, not EQ-boosting a frequency. No source tested EQ emphasis at 432 or 528 Hz.
- **Brightness depends on raw centroid, not on its relation to the fundamental** (Schubert & Wolfe 2006). Timbral colour from EQ is therefore driven by absolute frequency region more than by harmonic alignment with the note. Confidence: Medium. — [Schubert & Wolfe 2006](https://www.phys.unsw.edu.au/jw/reprints/SchubertWolfe06.pdf)
- **Narrow boosts are content-dependent**: they are audible when aligned with notes played, as in the 165 Hz Q = 24 demonstration. Confidence: Medium. — [RealTraps](https://realtraps.com/art_narrow.htm)

### Inferences
- **[DERIVED] Octave stacks**: emphasising f together with 2f, 4f, 8f... boosts one pitch class (chroma) across registers. This acts like a fixed "drone" emphasis.
  - 528 Hz sits **+15.6 cents above C5** (523.25 Hz in A440 equal temperament).
  - 432 Hz is **−31.8 cents** from A4 = 440.
  - Narrow boosts at these frequencies will therefore line up exactly with notes only in music tuned to match. Otherwise they sit between semitones and emphasise partials irregularly.
- **Design rule**:
  - If a "spiritual frequency" preset is desired for marketing reasons, implement it as **broad (Q ≤ about 1.5), low-gain (≤ about 2–3 dB)** emphasis. Its perceptual effect then derives from region (warmth or presence) and stays key-independent and inoffensive.
  - Alternatively, offer a **key- or tuning-aware mode** that snaps narrow bands to the song's tonic or fifth (and octaves) in the selected reference tuning.
  - Narrow fixed-Hz boosts (Q above about 5) will sound like resonances on some notes and do nothing on others (Q5 ERB table and Bücklein peak asymmetry).
- **Honest labelling**: claims that a frequency itself induces calm or healing are not supported. Copy should describe the timbral effect (warm, airy, and so on), and frequency-specific claims should be marked as tradition or lore.

### Gaps
- I found no study of perceptual or emotional effects of emphasising a frequency together with its octaves, or of "chroma-targeted" EQ.
- I found no peer-reviewed evidence on the 528 Hz ("Solfeggio") claims. None turned up in the searches.

---

## 7. Practical: gain/Q ranges for colouring vs surgical EQ, Pultec simultaneous boost/cut, tilt EQ

### Takeaway
Colouring EQ means broad (low-Q) curves at small gains. Surgical EQ means narrow (high-Q) curves, mostly cuts. The Pultec "trick" works because its passive boost and cut interact non-reciprocally: the result is a net low shelf boost with a dip just above it, which reads as "big but tight" lows. Tilt EQ is the most direct tool for moving the centroid, and the centroid is the variable most linked to brightness and arousal.

### Cited Findings
- **Pultec EQP-1A low-end trick** [ENGINEERING / manufacturer]:
  - Boost and cut at the same low frequency do not cancel. Because the EQP-1A is passive with no buffering between stages, the filters interact, the **cut becomes slightly resonant**, and the result is **more boost than cut plus a notch** that adds tightness. — [Universal Audio Pultec manual](https://help.uaudio.com/hc/en-us/articles/7183176266260-Pultec-Passive-EQ-Collection-Manual); [MusicRadar: Pultec low-end trick](https://www.musicradar.com/how-to/pultec-low-end-trick)
  - The boost and cut corner frequencies are slightly offset, producing a resonant shape. — [Bonedo (DE)](https://www.bonedo.de/artikel/einzelansicht/how-topultec-eq-bedienen-samt-pultec-trick.html)
  - The audible result is a tighter, less boomy low end. Typical settings are 60 Hz (or 30 Hz), boost about 6, then cut to taste. The original manual advises against using boost and cut together; the practice was unintended by the designers. — [Sweetwater: boost and attenuate on Pultec](https://www.sweetwater.com/insync/boost-and-attenuate-on-pultec-eqs/); [MusicRadar producer's guide](https://www.musicradar.com/news/producers-guide-pultec-eqp-1a)
  - Whether both sections are shelves is disputed in forums. — [Gearspace thread](https://gearspace.com/threads/pultec-question.77586/)
  - Confidence: Medium (consistent practice, and manufacturer-documented).
- **Colouring vs surgical ranges** [ENGINEERING]:
  - Cheat sheets recommend finding resonances with a narrow boost-and-sweep, then cutting. A wide cut around 450 Hz–1 kHz removes boxy or honky character. — [Scribd cheat sheet](https://www.scribd.com/document/714720470/eq-cheat-sheet)
  - Waves advises cutting rather than boosting for corrective work. — [Waves: boost or cut](https://waves.com/eq-tips-boost-or-cut-the-frequencies)
  - Confidence: Medium as practice. I found no source giving a numeric Q or gain standard.
- **Tilt and centroid** [RESEARCH linkage]: arousal and brightness rise with centroid, and with a slower decrease of spectral slope, i.e. a flatter tilt. — [Frontiers 2017, 8:153](https://www.frontiersin.org/journals/psychology/articles/10.3389/fpsyg.2017.00153/full); [Schubert & Wolfe 2006](https://www.phys.unsw.edu.au/jw/reprints/SchubertWolfe06.pdf)

### Inferences
- **Recommended parameter envelopes for emotion presets** (synthesis of Q5 thresholds and engineering practice, Confidence: Medium):

  | Preset role | Filter type | Gain | Q / slope |
  |---|---|---|---|
  | Colouring / feeling | Shelves or bells | ±0.5–4 dB | Q 0.3–1.2; tilt ±0.5–2 dB/oct about a 700 Hz–1 kHz pivot |
  | Character / problem zones (mud, honk) | Bells | −1 to −4 dB | Q 1–3 |
  | Surgical (resonance removal) | Notch-like cuts | −3 to −12 dB | Q 5–20 |
  | Narrow boosts (avoid in emotion presets) | — | ≤ +2 dB | Only above about 5 kHz, or tracking pitch |

  Narrow boosts produce resonance and ringing (Olive et al.: thresholds fall at high Q for transient content).
- **"Grounded" preset**: a Pultec-style low shelf boost at 40–80 Hz combined with a cut slightly above, about 150–300 Hz. This gives weight without mud and is a reasoned implementation of the trick, with interaction modelled rather than two independent filters.
- **"Calm" and "warm" via tilt**: a −0.5 to −1.5 dB/oct tilt lowers the centroid (arousal down) and avoids peaks, which matches Bücklein's asymmetry. **"Uplifting" and "clear"** use +0.5 to +1.5 dB/oct, kept out of a 2–5 kHz peak to avoid tension. Pair either with BS.1770 auto-gain so the tilt is not judged by loudness.
- **"Airy" and "ethereal"**: high shelf at 10–12 kHz, +1 to +4 dB, Q about 0.7, plus a gentle low shelf cut (−1 to −3 dB below about 150 Hz) to reduce "mass" (Zacharakis model).
- **"Intimate"**: mild presence (+1 to +2 dB, about 2.5–4 kHz, Q about 0.8), a mild high-shelf reduction above about 10 kHz, and a small cut around 300 Hz. These values are inferred, not validated.
- **Perceptual-trap checklist for preset QA**:
  - Level-match every preset with BS.1770.
  - Audition at two playback levels (equal-loudness).
  - Avoid narrow boosts (Bücklein, Toole & Olive).
  - Keep 2–5 kHz changes small (maximum ear sensitivity).
  - Test on several source types (descriptor variance in SAFE and SocialEQ).

### Gaps
- I found no FabFilter or iZotope manual or white paper with numeric "colouring vs surgical" Q and gain guidance in this session.
- No peer-reviewed study validates tilt-EQ slopes against emotion ratings on full mixes.
- Analog-modelled Pultec curves (exact interaction magnitude per knob setting) were not retrieved. They would need circuit simulation or measurement.
