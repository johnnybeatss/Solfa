# Audio Plugin Frameworks, Formats, Licensing and Distribution (state as of 21 Nov 2026)

Research method note: juce.com, forum.juce.com and learn.microsoft.com could not be fetched directly from this session (network allowlist), so facts from those sites come from search-engine excerpts of the pages. Facts marked "[primary, verified]" were read directly from the project's own files on raw.githubusercontent.com or from `git ls-remote` tag listings on 21 Nov 2026. Confidence levels: High = primary source read directly; Medium = primary source seen only as a search excerpt, or several secondary sources agree; Low = a single secondary source or sources that conflict.

## 1. JUCE: version, licensing, CMake, WebView, DSP, CLAP

### Takeaway
JUCE is now on **major version 9** (latest tag **9.0.6**; 9.0.0 was announced around 21 Jul 2026). It is dual-licensed: **AGPLv3** or the commercial JUCE 9 EULA. The commercial tiers are unchanged from JUCE 8: Starter is free up to $20k revenue, Indie is $40/mo or $800 perpetual up to $300k, and Pro is $175/mo or $3,500 perpetual with no limit. No splash screen is required since JUCE 8. CMake (3.22+) is first-class, WebView UIs have been supported since 8.0.0, and the DSP module has everything a parametric EQ needs. **JUCE still has no native CLAP export in 9.0.6.** CLAP comes from the community clap-juce-extensions project.

### Cited Findings
- **Current version [primary, verified, High]:** the JUCE CHANGE_LIST lists 9.0.0 through **9.0.6** as the newest entries. 9.0.6 "Updated the bundled AAX SDK to 2.10" and "Added support for the AAX dynamic scale factor API". Git tags 9.0.3–9.0.6 exist. — [JUCE CHANGE_LIST.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/CHANGE_LIST.md); [JUCE releases](https://github.com/juce-framework/JUCE/releases)
- **JUCE 9.0.0 headline features [primary, High]:** a new SVG parser, variable fonts, a new macOS CoreAudio implementation, faster software rendering, OpenGL ES on Linux, better multi-touch, and "Improved the CMake build system for headless environments". 9.0.1 added "a new TypeScript npm package for WebView integration". — [JUCE CHANGE_LIST.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/CHANGE_LIST.md)
- **JUCE 9 release date (Medium):** KVR's "JUCE 9 Now Available" article is dated 21 July 2026 and describes a focus on UI, embedded support and cross-platform compatibility. A JUCE 9 preview branch preceded it. — [KVR: JUCE 9 Now Available](https://www.kvraudio.com/news/juce-9-now-available-67802); [JUCE forum: JUCE 9 is Available Now](https://forum.juce.com/t/juce-9-is-available-now/69175); [JUCE 9 preview branch](https://forum.juce.com/t/the-juce-9-preview-branch-is-available-now/68864)
- **AudioProcessor v2 not shipped yet (Medium, search excerpt of the announcement):** the JUCE team "is currently working on AudioProcessor v2, the foundation for sample-accurate automation, unique CLAP features, and MIDI messages in UMP format." — [JUCE forum: JUCE 9 is Available Now](https://forum.juce.com/t/juce-9-is-available-now/69175)
- **Licence [primary, High]:** "The JUCE Framework modules are dual-licensed under the AGPLv3 and the commercial JUCE licence". Anyone not using AGPLv3 agrees to the JUCE 9 EULA. The JUCE examples are ISC-licensed. — [JUCE LICENSE.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/LICENSE.md); [JUCE 9 EULA](https://juce.com/legal/juce-9-licence/)
- **No pricing or EULA changes for JUCE 9 (Medium, search excerpt):** the official post says the JUCE 9 EULA is "remaining unchanged from the last iteration of JUCE 8", with the same tier divisions and prices. — [JUCE forum: JUCE 9: No pricing or EULA changes](https://forum.juce.com/t/juce-9-no-pricing-or-eula-changes/69000)
- **Tiers and prices (Medium, search excerpt of the official page):**
  - Starter: free, revenue up to **$20,000**/yr, perpetual, all features.
  - Indie: **$40/user/month or $800 perpetual**, revenue up to **$300,000**, 1-month minimum term.
  - Pro: **$175/user/month or $3,500 perpetual**, no revenue limit, 12-month minimum term.

  — [Get JUCE](https://juce.com/get-juce/)
- **How the revenue limit works (Medium):** it is measured on revenue "received or obtained by the Licensee over the previous 12 months". If you exceed it, you must buy the appropriate tier or "immediately cease developing and Distributing Products". For individuals, only revenue from JUCE work counts. For companies, all revenue counts. — [JUCE 8 EULA](https://juce.com/legal/juce-8-licence/); [JUCE forum: Amendments to the JUCE 8 EULA](https://forum.juce.com/t/amendments-to-the-juce-end-user-licence-agreement-for-juce-8/61265)
- **History of the Indie terms (Low/Medium):** draft JUCE 8 terms had Indie at $200k and $50/mo. These were revised to $300k and $40/mo before release. — [JUCE forum: [ARCHIVED] JUCE 8 EULA](https://forum.juce.com/t/archived-juce-8-eula/60947)
- **Splash screen (Medium):** the "Made with JUCE" splash was required on JUCE 7 free/personal tiers and is "no longer required on 8". The JUCE 9 EULA is unchanged from 8. — [JUCE forum: Licensing for a free plug-in](https://forum.juce.com/t/licensing-for-a-free-plug-in/66179); [JUCE forum: juce8 license and amateur programmers](https://forum.juce.com/t/juce8-license-and-amateur-programmers/61017)
- **CMake [primary, High]:** "JUCE projects can be managed with either CMake or the Projucer". **CMake 3.22 or higher is required.**
  - Builds need C++17, Xcode 12.4+ and VS2019+.
  - Deployment targets: macOS 10.11 (x86_64, Arm64); Windows 10 1607 (x64, x86, Arm64, Arm64EC); mainstream Linux; iOS 12.
  - The README lists supported formats as "VST, VST3, AU, AUv3, AAX and LV2 audio plug-ins and plug-in hosts". CLAP is not in that list.

  — [JUCE README](https://raw.githubusercontent.com/juce-framework/JUCE/master/README.md)
- **WebView UI [primary, High]:** JUCE 8.0.0 "Added support for WebView based UIs", along with a Direct2D renderer, an animation module and the bundled AAX SDK. The WebView interop JS is published as the npm package `@juce-framework/webview`. Classes such as `WebSliderRelay` and `WebToggleButtonRelay` exist. Later 8.x releases improved Linux WebView. — [JUCE CHANGE_LIST.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/CHANGE_LIST.md); [JUCE BREAKING_CHANGES.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/BREAKING_CHANGES.md)
- **SDKs bundled with recent JUCE [primary, High]:**
  - 8.0.11 "Updated the VST3 SDK to 3.8.0 (MIT license)", updated AAX SDK 2.9.0, "Included the ASIO SDK source files directly (GPLv3 license)", and added the `juce_audio_processors_headless` module.
  - 9.0.6 bundles AAX SDK 2.10.

  — [JUCE CHANGE_LIST.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/CHANGE_LIST.md)
- **DSP module classes for an EQ [primary, High]:**
  - `juce_dsp` includes `ProcessorChain`, `ProcessorDuplicator`, `IIRFilter`, `FIRFilter`, `StateVariableTPTFilter`, `FirstOrderTPTFilter`, `LinkwitzRileyFilter`, `Oversampling`, `DryWetMixer`, `BallisticsFilter`, `FFT`, `Windowing`, `Convolution` and `FilterDesign`.
  - `dsp::IIR::Coefficients` has `makePeakFilter`, `makeLowShelf`, `makeHighShelf`, `makeLowPass`/`makeHighPass` (with Q), `makeBandPass`, `makeNotch`, `makeAllPass` and first-order variants.

  — [juce_dsp.h](https://raw.githubusercontent.com/juce-framework/JUCE/master/modules/juce_dsp/juce_dsp.h); [juce_IIRFilter.h](https://raw.githubusercontent.com/juce-framework/JUCE/master/modules/juce_dsp/processors/juce_IIRFilter.h)
- **CLAP in JUCE, roadmap vs reality:**
  - JUCE roadmaps (Q1 and Q3 2025) said JUCE 9 would "add the ability to author CLAP plug-ins directly". — [JUCE Roadmap Q1 2025](https://juce.com/blog/juce-roadmap-update-q1-2025/); [JUCE Roadmap Q3 2025](https://juce.com/blog/juce-roadmap-update-q3-2025/)
  - The 9.0.x changelog and README contain **no CLAP entries [primary, High]**. — [JUCE CHANGE_LIST.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/CHANGE_LIST.md)
  - A third-party JUCE 9 project's changelog says JUCE 9 has no native CLAP wrapper and uses a separate submodule. — [audio-plugin-coder CHANGELOG](https://github.com/Noizefield/audio-plugin-coder/blob/main/CHANGELOG.md)
- **clap-juce-extensions [primary, High]:**
  - MIT-licensed, "JUCE6, 7 and 8 Unofficial CLAP Plugin Support". It is "not supported by the JUCE team" and does not support CLAP hosting. Used by Surge, Dexed, ChowDSP and others.
  - CMake usage: add it after JUCE, then call `clap_juce_extensions_plugin(TARGET ... CLAP_ID ... CLAP_FEATURES ...)`.
  - The README says native support "may never happen". The latest tag is 0.26.0.

  — [clap-juce-extensions README](https://raw.githubusercontent.com/free-audio/clap-juce-extensions/main/README.md)
- **clap-juce-extensions breakage (Medium):** JUCE 8.0.11's new `juce_audio_processors_headless` module broke clap-juce-extensions. A fix was tracked as PR #169. — [JUCE forum: JUCE 8.0.11 is out](https://forum.juce.com/t/juce-8-0-11-is-out/67725)
- **CLAP output on JUCE 9.0.4 in practice (Low):** a project's PRs show VST3 and CLAP builds on JUCE 9.0.4, which suggests the extensions work with JUCE 9. — [Tsunami PR #14](https://github.com/felixuhmann/Tsunami/pull/14)

### Inferences
- For a closed-source commercial EQ by an indie developer, JUCE Starter (free, ≤$20k/yr) is the practical starting point. Budget for Indie ($800 one-off) once trailing-12-month revenue nears $20k.
- AGPLv3 is only viable if the plugin's source will be published.
- Treat CLAP-from-JUCE as "works but unofficial". Pin a clap-juce-extensions commit that is known to build with your JUCE version.
- The JUCE `dsp::IIR` biquads (RBJ-cookbook style) plus `ProcessorDuplicator` for stereo are enough for a standard parametric EQ. Coefficient smoothing (or SVF/TPT filters for modulation-safe sweeps) and possibly `Oversampling` for high-frequency bell cramping are design choices layered on top.

### Gaps
- I could not read the JUCE 9 EULA text directly, so tier details rely on search excerpts and the "no changes" announcement.
- I could not confirm whether clap-juce-extensions PR #169 is merged or whether it officially supports JUCE 9. It works for at least one project.
- I could not confirm the exact JUCE 9.0.6 release date. The tag exists as of 21 Nov 2026.

## 2. Steinberg VST3 SDK licensing, VST2, trademark

### Takeaway
Since **VST 3.8.0 (released 20 Oct 2025, announced 29 Oct 2025)** the VST3 SDK is **MIT-licensed**. The old GPLv3 and proprietary-agreement options are withdrawn. The latest tag is **v3.8.1**. Use of the VST name and logo is now optional, but if used it must follow Steinberg's usage guidelines. VST2 cannot be licensed by new developers: there have been no new agreements since about Oct 2018, and the public SDK was withdrawn in 2013.

### Cited Findings
- **License text [primary, High]:** the vst3sdk LICENSE.txt is the MIT License, "Copyright (c) 2026, Steinberg Media Technologies GmbH". — [vst3sdk LICENSE.txt](https://raw.githubusercontent.com/steinbergmedia/vst3sdk/master/LICENSE.txt)
- **Versions [primary, High]:** the README is titled "Welcome to VST SDK 3.8.x". Tags `v3.8.0_build_66` and `v3.8.1_build_84` exist. — [vst3sdk README](https://raw.githubusercontent.com/steinbergmedia/vst3sdk/master/README.md); [vst3sdk repo](https://github.com/steinbergmedia/vst3sdk)
- **Supported platforms [primary, High]:** Windows 11 (x86/x64/arm64/arm64EC; MSVC 2022/2026), macOS 10.14–26 (Xcode 10–26.5, Apple Silicon) and iOS 13–26. The SDK includes AAX, AUv3 and AU wrappers. The full SDK download includes the "VST 3 plug-in Test Host Application/Validator". — [vst3sdk README](https://raw.githubusercontent.com/steinbergmedia/vst3sdk/master/README.md)
- **Licensing change (High):** "since version 3.8, VST 3 is licensed under MIT license". GPLv3 and the Steinberg proprietary license "are no longer available". MIT does not require disclosing source. Existing products may keep their current license. The 3.8.0 SDK is dated 2025/10/20. — [VST 3 Licensing (dev portal)](https://steinbergmedia.github.io/vst3_dev_portal/pages/VST+3+Licensing/VST3+License); [Licensing FAQ](https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Licensing.html); [Version 3.8.0 notes](https://steinbergmedia.github.io/vst3_dev_portal/pages/Versions/Version+3.8.0.html)
- **Press release (High):** dated 29 Oct 2025. ASIO was also opened up at the same time. — [Steinberg press release PDF](https://ocl-steinberg-live.steinberg.net/_storage/asset/819253/storage/master/Press%20Release%20-%202025-10-29%20-%20VST%203.8%20-%20EN.pdf); [Sound On Sound](https://www.soundonsound.com/news/steinberg-adopt-mit-license-vst3); [heise](https://heise.de/-10963451); [CDM](https://cdm.link/open-steinberg-vst3-and-asio/)
- **Trademark and logo [primary, High]:**
  - "Trademark usage (e.g. 'VST' name or logo) is optional under MIT license, but if used, must comply with Steinberg's official trademark rules".
  - Logos must be used unaltered and only in product contexts: splash/about boxes, websites and manuals.
  - **Prohibited:** "Using 'VST' (or derivatives like 'VSTi') in company or product names" and applying the logo to non-VST-compatible products.
  - The Steinberg VST Usage Guidelines are included in the SDK.

  — [vst3sdk README §Trademark](https://raw.githubusercontent.com/steinbergmedia/vst3sdk/master/README.md)
- **VST2 (Medium):**
  - The VST2 SDK was removed from Steinberg's website on 9 Dec 2013. — [KVR news](https://www.kvraudio.com/news/steinberg-software-development-kit-sdk-for-vst-2-no-longer-available-24335)
  - New developers who did not sign a VST2 agreement before Oct 2018 must use VST3. — [Steinberg forum: VST 2 SDK discontinued](https://forums.steinberg.net/t/vst-2-sdk-discontinued/201774); [gearnews](https://www.gearnews.com/steinberg-ends-support-vst-2-sdk-dont-panic-plug-ins-safe)
  - Steinberg has also been removing VST2 hosting from its own products. — [gearnews: Steinberg discontinues support for VST2 plug-ins](https://www.gearnews.com/steinberg-discontinues-support-for-vst2-plug-ins)
- **Unverified claim (Low):** a forum thread claims the VST3 License Agreement v2.1 (Mar 2021) terminated prior VST2 agreements upon signing. — [Steinberg forum: VST 2 coming to an end](https://forums.steinberg.net/t/vst-2-coming-to-an-end/201777)

### Inferences
- A new indie EQ should ship **VST3, not VST2**. The MIT license removes the old need to sign and return Steinberg's proprietary agreement.
- Don't put "VST" in the product name. The logo is optional.
- The MIT change also removes the GPLv3 trap for Rust VST3 bindings once permissive bindings exist (see §6).

### Gaps
- I did not find a 3.8.1 changelog summary or release date. Only the tag was confirmed.

## 3. CLAP format: status, licensing, host support

### Takeaway
CLAP is an MIT-licensed open standard. The current spec version is **1.2.10**, and recent work includes webview, param-hovered and background-operations drafts. It is hosted natively by **Bitwig, REAPER, FL Studio** and (with reportedly partial or uneven support) **Studio One**. It is **not** hosted by Ableton Live, Logic Pro, Pro Tools or Cubase as of late 2026. CLAP is a cheap bonus format, not a required one.

### Cited Findings
- **License [primary, High]:** MIT License, "Copyright (c) 2021 Alexandre BIQUE". — [CLAP LICENSE](https://raw.githubusercontent.com/free-audio/clap/main/LICENSE)
- **Version [primary, High]:**
  - The latest tag is **1.2.10**, which adds a `param-hovered` draft extension and clarifies `CLAP_PARAM_IS_BYPASS`.
  - 1.2.9 added the `param-origin`, `transport-control` iteration and background-activation/state drafts.
  - 1.2.8 added UIKit GUI support.
  - 1.2.7 added the `webview` draft.

  — [CLAP ChangeLog](https://raw.githubusercontent.com/free-audio/clap/main/ChangeLog.md); [CLAP repo](https://github.com/free-audio/clap)
- **Framework support [primary, High]:** the CLAP README lists iPlug2 and **nice-plug** ("a liberally licensed, community-driven Rust audio plugin framework") as frameworks. — [CLAP README](https://raw.githubusercontent.com/free-audio/clap/main/README.md)
- **Developer resources:** [CLAP: Developers Getting Started](https://cleveraudio.org/developers-getting-started/)
- **Host support (Medium/Low, secondary sources that partly conflict):**
  - Bitwig and REAPER have full support, REAPER since v7.
  - FL Studio supports CLAP, from 21.2 or FL 2024 depending on the source.
  - Studio One supports it from 6.5 ("partial"). One 2026 chart lists "No" for Studio One Pro 7.
  - Ableton Live, Logic Pro and Pro Tools have none.
  - Cubase/Nuendo are "not yet".

  — [Spectral Colors: CLAP vs VST3 (2026)](https://spectral-colors.com/news/clap-vs-vst3-2026/); [ProducerGrid 2026](https://producergrid.com/blog/clap-plugin-format-everything-you-need-to-know/); [Antares format guide 2026](https://www.antarestech.com/blog/vst3-au-aax-clap-plugin-formats-2026); [musicianstool 2026](https://musicianstool.com/blog/vst3-vs-au-vs-clap-audio-plugins-explained)
- **Ableton (Medium):** a review of Live 12.4 notes "no support for ARA, CLAP". — [gearnews: Ableton Live 12.4](https://www.gearnews.com/ableton-live-12-review/)
- **Cubase 15 (Medium):** released Nov 2025. Release coverage lists DAWproject support and new effects, but no CLAP. — [Sonicstate: Cubase 15](https://sonicstate.com/news/2025/11/06/steinberg-releases-cubase-15/); [MusicRadar](https://www.musicradar.com/music-tech/steinberg-unveils-cubase-15-a-major-leap-forward-for-its-flagship-daw-with-new-instruments-effects-modulators-and-ai-powered-stem-separation)
- **Early Studio One problems (Low):** a 2024 KVR thread reported crashes in Studio One's early CLAP hosting. — [KVR: Tracking CLAP hosts and plugins](https://www.kvraudio.com/forum/viewtopic.php?t=583501&start=465)
- **Context (Medium):** an overview of CLAP's adoption two years in. — [Libre Arts: CLAP two years later](https://librearts.org/2024/11/clap-api-two-years-later/)

### Inferences
- For an EQ, CLAP adds little user-facing value (per-voice modulation matters more for synths). It is near-free to add via clap-juce-extensions, iPlug2, DPF or nih-plug, and pleases Bitwig and REAPER users.

### Gaps
- I found no official vendor statements (Image-Line, Fender/PreSonus, Steinberg, Ableton) on CLAP status. Host versions come from secondary blogs and should be verified against vendor release notes before marketing claims.

## 4. Audio Units (AUv2/AUv3), Apple Silicon, auval

### Takeaway
Logic Pro and GarageBand load only Audio Units, so a macOS EQ needs at minimum an **AUv2** component built as a **universal binary (arm64 + x86_64)** and validated with **`auval`**. AUv3 is required for iOS/iPadOS (as an app extension distributed through the App Store) and is optional on macOS. JUCE, iPlug2 and the VST3 SDK all provide AU/AUv3 wrappers.

### Cited Findings
- **Wrappers [primary, High]:** JUCE supports AU and AUv3 targets, with macOS deployment for x86_64 and Arm64 and iOS for Arm64. — [JUCE README](https://raw.githubusercontent.com/juce-framework/JUCE/master/README.md)
- **Wrappers [primary, High]:** iPlug2 targets AUv2 and AUv3. — [iPlug2 README](https://raw.githubusercontent.com/iPlug2/iPlug2/master/README.md)
- **Wrappers [primary, High]:** the VST3 SDK includes AU and AUv3 wrappers. — [vst3sdk README](https://raw.githubusercontent.com/steinbergmedia/vst3sdk/master/README.md)
- **Arm64 slice failures (Medium):** if a universal AU's arm64 slice fails to load, Logic may silently load the x86_64 slice under Rosetta. — [Steinberg forum: AUwrapped AUv2 runs as Intel in Logic](https://forums.steinberg.net/t/auwrapped-auv2-runs-as-intel-in-logic/827682)
- **Rosetta (Medium):** Logic recognises Intel-only plug-ins only when Rosetta is installed. — [Apple Support: Remove or disable Audio Units plug-ins in Logic Pro](https://support.apple.com/en-ae/HT2852)
- **`auval -comp` on arm64 (Medium):** the Component Manager API it tests is not supported on ARM64. Use `auval -v` / `auval -a`. `arch -x86_64 auval ...` tests the Intel slice under Rosetta. — [JUCE forum: JUCE and macOS 11 / ARM](https://forum.juce.com/t/juce-and-macos-11-arm/40285?page=6); [JUCE forum: Default AU fails auval on Apple Silicon](https://forum.juce.com/t/default-au-from-projucer-fails-auval-on-apple-silicon-big-sur-dtk-solved/42845)
- **Universal build that still fails (Medium):** a developer reported a universal binary that loaded on Apple Silicon but did not appear on an Intel Mac's Logic. Verify both slices with `lipo -info` or `file`. — [Apple Developer Forums thread 805894](https://developer.apple.com/forums/thread/805894)
- **AUv3 in Logic (Medium):** threads cover AUv3 in-process loading and Swift 6 framework issues. — [Apple Developer Forums: Can Logic Pro load an AUv3 in-process?](https://developer.apple.com/forums/thread/766503); [Logic cannot load v3 AU with Swift 6 framework](https://developer.apple.com/forums/thread/792845)
- **pluginval (High):** pluginval tests AU. Its stress test calls auval. — [pluginval README](https://raw.githubusercontent.com/Tracktion/pluginval/develop/README.md)

### Inferences
- Build with `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"`. Run `auval -v aufx <subtype> <manu>` on both architectures in CI. Ship AUv2 to `/Library/Audio/Plug-Ins/Components`.
- AUv3 on macOS is optional for v1. It becomes required for an iOS version.

### Gaps
- I did not find an official Apple document from 2025–26 that states current AU validation requirements for Logic. The guidance above comes from developer forums.

## 5. AAX / Pro Tools: Avid developer program and PACE signing

### Takeaway
Pro Tools loads only AAX, and retail Pro Tools loads only **PACE-signed** AAX. The AAX SDK, now at 2.10 and bundled in JUCE, is dual-licensed: Avid commercial terms or GPLv3. Basic PACE signing (wraptool plus an iLok) is reportedly provided **free to registered Avid developers**, normally listed at about $500/yr. The developer still needs an iLok and their own Apple and Windows code-signing certificates. The process means emailing Avid (audiosdk@avid.com) and signing agreements with Avid and PACE. Allow weeks.

### Cited Findings
- **SDK in JUCE [primary, High]:** JUCE 8.0.0 bundled the AAX SDK. 8.0.11 updated it to 2.9.0. 9.0.6 updated it to **2.10**. — [JUCE CHANGE_LIST.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/CHANGE_LIST.md)
- **Dual license (Medium):** the AAX SDK 2.8+ license banner offers commercial or GPLv3 terms. A JUCE 9 project reports that the bundled SDK 2.9 LICENSE.txt offers Avid commercial terms or GPLv3. — [JUCE forum: AAX open source release](https://forum.juce.com/t/aax-open-source-release/44017); [manguebyte issue #21](https://github.com/alessonqueirozdev-hub/manguebyte/issues/21)
- **Commercial AAX (Medium):** a commercial closed-source AAX product typically needs Avid commercial terms plus PACE signing. — [Ctrlr discussion #489](https://github.com/RomanKubiak/ctrlr/discussions/489)
- **Free plugin case (Low/Medium):** a March 2026 developer memo says Avid told them that for a free plugin "a commercial license is not required, only the PACE signing tools are needed". It lists PACE signing registration as "effectively free (Avid covers the full cost)". — [note.com memo: Preparation for AAX Plugin Development (March 2026)](https://note.com/kawato3/n/ne11473420ad5?hl=en)
- **Free signing for Avid developers (Medium/Low, forum reports):** registered Avid developers get a free PACE signing (Eden/wraptool) enrollment, normally about $500/year. You need your own iLok, and on Windows PACE wraps signtool, so you still need normal Windows code signing. — [HISE forum: Getting AAX signed](https://forum.hise.audio/topic/6195/getting-aax-signed-avid-test-etc); [HISE forum: AAX SDK compiled](https://forum.hise.audio/topic/6040/aax-sdk-compiled/28)
- **Development without signing (Medium):** unsigned AAX can be tested in **Pro Tools Developer** with a developer license on an iLok. Retail Pro Tools refuses unsigned AAX. — [truce.audio AAX docs](https://truce.audio/docs/formats/aax/); [Sineless issue #51](https://github.com/orizonaudio/Sineless/issues/51)
- **PACE cloud signing (Medium):** PACE's cloud signing is enterprise-priced and may be hard to justify for a new indie. It is separate from basic wraptool signing. — [Moonbase: Code signing audio plugins in 2025](https://moonbase.sh/articles/code-signing-audio-plugins-in-2025-a-round-up/)
- **Getting started (Low):** write to audiosdk@avid.com explaining who you are and what you are shipping. — [Virtual-FM-1 docs/AAX.md](https://github.com/jbschooley/Virtual-FM-1/blob/main/docs/AAX.md)
- **Developer portal:** [developer.avid.com](https://developer.avid.com)

### Inferences
- AAX is worth planning as a v1.1 or v2 target. The paperwork and PACE onboarding lead time, plus wraptool needing an iLok in CI (or a manual signing step), make it the most bureaucratic format.
- Signing must happen in a set order: PACE-wrap first, then (on macOS) notarize.

### Gaps
- I could not read Avid's current official developer program terms. developer.avid.com was not fetched. I could not confirm whether PACE signing is still free for commercial indie products in late 2026. Contact Avid to confirm.

## 6. Alternatives: iPlug2, nih-plug (and forks), DPF, Faust, Cmajor

### Takeaway
- **iPlug2:** a liberal zlib-like license, CLAP/VST3/AUv2/AUv3/AAX/WAM output, actively maintained in 2026, though still tagged "pre-release". It is the strongest royalty-free C++ alternative to JUCE.
- **nih-plug (Rust):** now officially in **maintenance mode** and points users to a community fork. Its VST3 bindings are GPLv3, so VST3 builds must be GPL unless bindings are replaced. A permissive replacement is being discussed post-VST-MIT.
- **DPF:** ISC-licensed; LV2/VST2/VST3/CLAP plus Linux-first formats.
- **Faust:** a DSP language that compiles to many targets, including DPF and iPlug2.
- **Cmajor:** a DSP language with JIT and hot reload; status and maturity are less clear.

### Cited Findings
- **iPlug2 [primary, High]:**
  - It "targets the CLAP, VST2, VST3, AUv2, AUv3, AAX (Native) and the Web Audio Module (WAM v1)". Minimum targets: Windows 8, macOS 13, Linux (X11), visionOS 26, iOS 15. ARM64EC is supported.
  - "licensed with a liberal zlib-like license … free to use in closed source projects". The recommended project starting point is iPlug2OOS.
  - UI options: IGraphics (NanoVG/Skia), or HTML/CSS or SwiftUI examples.
  - The latest tag is `v1.0.0-beta`.

  — [iPlug2 README](https://raw.githubusercontent.com/iPlug2/iPlug2/master/README.md); [iPlug2 GitHub org](https://github.com/iplug2)
- **iPlug2 activity (Low/Medium):** the main repo was updated 15 Sep 2026. — [pistack.xyz comparison (Sep 2026)](https://www.pistack.xyz/posts/2026-09-29-juce-vs-iplug2-vs-dpf-cpp-audio-plugin-framework-comparison/)
- **iPlug3 (Low):** a new "iPlug3" effort started 1 Jan 2026 as a clean-slate successor. — [iPlug3 GitHub](https://github.com/iPlug3)
- **nih-plug [primary, High]:**
  - "NOTE: NIH-plug the plugin framework is currently in maintenance mode. If you are interested in the framework rather than the plugin, please check out this community fork (codeberg.org/BillyDM/nih-plug)".
  - It exports VST3 and CLAP.
  - The framework is ISC-licensed, "However, the VST3 bindings (vst3-sys) used by nih_export_vst3!() are licensed under the GPLv3 … any VST3 plugins built with NIH-plug need to be able to comply with the terms of the GPLv3".

  — [nih-plug README](https://raw.githubusercontent.com/robbert-vdh/nih-plug/master/README.md)
- **nice-plug (High):** the CLAP README lists "nice-plug (codeberg.org/RustAudio/nice-plug), a liberally licensed, community-driven Rust audio plugin framework". It appears to be the community successor. — [CLAP README](https://raw.githubusercontent.com/free-audio/clap/main/README.md)
- **nih-plug issue tracker (Medium):**
  - #241 (Dec 2025) notes VST3 is now MIT.
  - #274 (Jun 2026) proposes switching to permissively licensed VST3 bindings.
  - #265 (Mar 2026) announces a maintained fork.

  — [nih-plug issues](https://github.com/robbert-vdh/nih-plug/issues)
- **DPF [primary, High]:**
  - "DPF can build for LADSPA, DSSI, LV2, VST2, VST3 and CLAP formats", plus a JACK/standalone mode.
  - "DPF is released under ISC … Some plugin formats may have additional restrictions".
  - Some sources also mention AU output.

  — [DPF README](https://raw.githubusercontent.com/DISTRHO/DPF/main/README.md)
- **Faust (Medium):** Grame lists iPlug2 Faust/libfaust JIT support. The Libre Audio Suite compiles Faust `.dsp` files into DPF plugin classes; its first release is listed for 27 Nov 2026 (unconfirmed). — [Powered By Faust (Grame)](https://faust.grame.fr/community/powered-by-faust/)
- **Cmajor [primary, High]:**
  - "a C-family language designed specifically for writing DSP". Docs are at cmajor.dev. There is a VS Code extension.
  - Building needs LLVM. Windows/arm64 plugin builds are unsupported.
  - The README footer says "copyright 2023 Cmajor Software Ltd".

  — [Cmajor README](https://raw.githubusercontent.com/cmajor-lang/cmajor/main/README.md)
- **Cmajor and Faust overview (Low):** a secondary 2026 overview of Cmajor (JIT, hot reload) and Faust. — [TT Lab 2026 deep dive](https://ttlab.hopto.org/blog/culture/2026-05-16-audio-plugin-development-2026-juce-8-vst3-au-aax-clap-iplug2-faust-cmajor-elementary-audio-deep-dive?lang=en)

### Inferences
- **Lowest friction for an indie C++ developer:** JUCE (biggest ecosystem, CI templates, tutorials, pluginval integration), accepting JUCE's commercial tiers.
- **Best no-royalty, permissive alternative:** iPlug2 (native CLAP and AAX; less polished widgets and docs; beta-tagged).
- **Rust:** nih-plug is usable but upstream is frozen. Prefer the community fork or nice-plug, and check the VST3 bindings' license status before shipping closed-source VST3.
- **Linux-centric work:** DPF.
- Faust is a good way to prototype EQ DSP, then export to JUCE, iPlug2 or DPF.

### Gaps
- I could not verify Cmajor's 2026 release activity or commercial plugin export licensing.
- I could not confirm whether nih-plug, its fork or nice-plug has merged MIT VST3 bindings.
- iPlug2 version tags are sparse, so maturity is judged from activity rather than releases.

## 7. Validation, QA tools and CI templates

### Takeaway
The standard stack:
- **pluginval** (Tracktion, free) for VST3, AU and VST2. v2.0.0 is in beta, adding built-in Steinberg VST3 validator runs, JUnit output, GitHub Actions and rtcheck.
- **auval** for AU.
- Steinberg's **validator** (in the VST3 SDK, and now inside pluginval at strictness ≥5).
- **clap-validator** for CLAP.
- **Pamplejuce** (Sudara): the de facto JUCE CMake + GitHub Actions template. It covers macOS, Windows and Linux builds, Catch2, pluginval, macOS notarization and Azure Trusted/Artifact Signing.

### Cited Findings
- **pluginval [primary, High]:** "a cross-platform plugin validator and tester … Test VST/AU/VST3 plugins … macOS/Windows/Linux … GUI or headless mode … separate process".
  - It can be added as a CMake target via CPM or submodule. "only JUCE 8 and 9 are currently supported/tested for this method".
  - Pre-built binaries include VST2 testing.

  — [pluginval README](https://raw.githubusercontent.com/Tracktion/pluginval/develop/README.md)
- **pluginval 2.0.0 [primary, High, beta status]:** the change list includes:
  - subcommand CLI
  - "acceptance testing" against reference WAVs (`pluginval test`)
  - `--junit-xml` and `--json-results`
  - GitHub Actions `Tracktion/pluginval/validate` and `Tracktion/pluginval/acceptance`
  - macOS real-time safety checking via rtcheck
  - "The Steinberg VST3 validator is now built into pluginval and runs automatically for VST3 plugins at strictness level 5 and above"
  - a new test that catches `juce::AudioParameterBool` restore issues
  - built on JUCE 9.0.4

  The latest git tag is **v2.0.0-beta3**, so 2.0 final is not confirmed released. — [pluginval CHANGELIST.md](https://raw.githubusercontent.com/Tracktion/pluginval/develop/CHANGELIST.md); [pluginval releases](https://github.com/Tracktion/pluginval/releases)
- **clap-validator [primary, High]:** "A validator and automatic test suite for CLAP plugins". Run it with `clap-validator validate /path/plugin.clap`. It has an experimental fuzzer (`clap-validator fuzz`). Builds are nightly dev builds, and macOS builds are unsigned. — [clap-validator README](https://raw.githubusercontent.com/free-audio/clap-validator/master/README.md)
- **Pamplejuce [primary, High]:**
  - C++23, JUCE 9.x as a git submodule tracking develop, CPM, CMake 3.25+, Catch2 v3.8.1, Tests and Benchmarks targets, Melatonin Inspector.
  - GitHub Actions: builds and tests on Linux, macOS and Windows; runs pluginval 1.x; Intel IPP; "Code signing and notarization on macOS"; "Windows code signing via Azure Trusted Signing".
  - Includes a CLAUDE.md/AGENTS.md setup wizard. Tests fail until code signing is set up.

  — [Pamplejuce README](https://raw.githubusercontent.com/sudara/pamplejuce/main/README.md); [Pamplejuce docs](https://melatonin.dev/manuals/pamplejuce/how-does-this-all-work/)
- **Pamplejuce workflow details (Medium):** it downloads pluginval v1.0.3 per OS, sets DISPLAY for Linux, and builds a `.pkg` with pkgbuild including the CLAP folder. — [build_and_test.yml](https://github.com/sudara/pamplejuce/blob/main/.github/workflows/build_and_test.yml)
- **Guides [Medium]:** [Melatonin: code sign and notarize macOS audio plugins in CI](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/); [Melatonin: Code signing on Windows with Azure Artifact Signing](https://melatonin.dev/blog/code-signing-on-windows-with-azure-trusted-signing/)
- **Steinberg validator (High):** it ships with the full VST3 SDK ("VST 3 plug-in Test Host Application/Validator"). — [vst3sdk README](https://raw.githubusercontent.com/steinbergmedia/vst3sdk/master/README.md)

### Inferences
Recommended CI gates for each build:
- pluginval at strictness 10 on VST3 and AU, with the VST3 validator built in once 2.0 is final
- `auval -v` on both architectures
- clap-validator if shipping CLAP
- Catch2 DSP unit tests, e.g. magnitude response of each band vs analytic biquad response
- pluginval 2.0 acceptance tests: null/regression renders against reference WAVs

### Gaps
- I could not confirm whether Pamplejuce includes a clap-validator step.
- I could not confirm the pluginval 2.0 final release date (beta3 as of Nov 2026).

## 8. Code signing and distribution (macOS and Windows)

### Takeaway
- **macOS:** Apple Developer Program at **$99/yr**. Sign bundles with a **Developer ID Application** cert. Build `.pkg` installers with `pkgbuild`/`productbuild`, signed with **Developer ID Installer**. Notarize with **`notarytool`** (altool notarization stopped working 1 Nov 2023) and staple.
- **Windows certificates:** private keys must live on HSMs or tokens (since June 2023). Max cert validity is now **460 days** (CA/B ballot CSC-31, effective 1 Mar 2026).
- **SmartScreen:** EV no longer grants instant SmartScreen reputation (since Aug 2024).
- **Cheapest Windows route:** **Azure Artifact Signing** (formerly Trusted Signing), about **$9.99/mo** Basic. It is open to **individuals only in the USA and Canada**, and organizations in more regions.
- **Windows installers:** Inno Setup or WiX.

### Cited Findings
- **Apple fee (Medium):** 99 USD per membership year or local equivalent; fee waivers for eligible nonprofits and education. — [Apple Developer Program membership details](https://developer.apple.com/programs/whats-included/) (cited via [asogauge 2026](https://asogauge.com/blog/apple-developer-account/) and [Spreadcast 2026](https://spreadcast.app/resources/apple-developer-program))
- **altool → notarytool (High):** Apple deprecated `altool` for notarization; it stopped working for notarization on 1 Nov 2023. notarytool accepts an app-specific password or an App Store Connect API key. — [Apple TN3147: Migrating to the latest notarization tool](https://developer.apple.com/documentation/technotes/tn3147-migrating-to-the-latest-notarization-tool.md); [WWDC22 10109](https://developer.apple.com/videos/play/wwdc2022/10109/)
- **Notarization in CI (Medium):** a step-by-step CI guide; Pamplejuce produces a `.pkg` and needs at least six secrets. App-specific passwords become invalid when the Apple ID password changes. — [Melatonin guide](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/)
- **Cert validity (High):** CA/B Forum Ballot CSC-31 cut the maximum validity of publicly trusted code-signing certificates from 39 months to **460 days**, for certificates issued on or after **1 March 2026**. DigiCert issues 459 days; SSL.com 458. — [CA/B Forum Ballot CSC-31](https://cabforum.org/2025/11/17/ballot-csc-31-maximum-validity-reduction); [DigiCert](https://www.digicert.com/blog/understanding-the-new-code-signing-certificate-validity-change); [SSL.com](https://www.ssl.com/article/reminder-reduced-code-signing-certificate-lifecycles/)
- **HSM requirement (Medium):** private keys must be on hardware or cloud HSMs. The roundup is a guide to OV/EV certificates, tokens vs cloud HSM, and PACE options for plugins. — [Moonbase: Code signing audio plugins in 2025](https://moonbase.sh/articles/code-signing-audio-plugins-in-2025-a-round-up/)
- **SmartScreen (High/Medium):**
  - From August 2024 the Microsoft Trusted Root Program removed EV code-signing OIDs, and "all Code Signing certificates will be treated equally".
  - Microsoft's SmartScreen page says paying for EV solely to avoid SmartScreen warnings is no longer justified.
  - Reputation is per file hash and builds over time.

  — [Microsoft Learn: SmartScreen reputation](https://learn.microsoft.com/windows/apps/package-and-deploy/smartscreen-reputation); [Microsoft Q&A: Trusted Root Program EV change](https://learn.microsoft.com/en-us/answers/questions/1846647/program-requirements-microsoft-trusted-root-progra); [ToDesktop PSA](https://www.todesktop.com/blog/posts/windows-apps-psa-ev-certs-do-not-grant-immediate-reputation-anymore)
- **Artifact Signing availability (Medium):**
  - Individual developers are currently limited to the USA and Canada; organizations can also apply from the EU and UK.
  - Microsoft points other individuals to OV certificates.
  - An April 2026 blog says self-employed individuals no longer need the 3-year business history required during preview.

  — [Microsoft Learn: Code signing options for Windows app developers](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/code-signing-options); [itwriting: Getting started with Azure Artifact Signing](https://www.itwriting.com/blog/12462-getting-started-with-azure-artifact-signing.html); [DevClass, Jan 2026](https://www.devclass.com/security/2026/01/14/code-signing-windows-apps-may-be-easier-and-more-secure-with-new-azure-artifact-service/4079554)
- **Artifact Signing price (Medium):**
  - Basic is **$9.99/mo** with 5,000 signatures. Premium is **$99.99/mo** with 100,000.
  - Billing starts on account creation and is not pro-rated.
  - The official page confirms the quotas, but the dollar figure was rendered as a placeholder in search excerpts. $9.99 comes from secondary sources.

  — [Azure Artifact Signing pricing](https://azure.microsoft.com/pricing/details/artifact-signing/); [G.D.G. Software FAQ](https://www.gdgsoft.com/faq/azure-trusted-signing-cost-effective-exe-code-signing); [4D forum](https://discuss.4d.com/t/windows-application-signing-with-ms-azure-artifact-signing-9-99-month/38154)
- **Pamplejuce Windows signing (High):** it wires up Azure Trusted/Artifact Signing in GitHub Actions. — [Pamplejuce README](https://raw.githubusercontent.com/sudara/pamplejuce/main/README.md)

### Inferences
- **macOS:** sign each bundle (VST3, component, CLAP) with Developer ID Application plus hardened runtime. Build per-format component pkgs with `pkgbuild` and combine with `productbuild`. Sign with Developer ID Installer, then `xcrun notarytool submit --wait`, then `xcrun stapler staple`.
- **Windows:** sign the `.vst3` DLL(s) and the installer (Inno Setup is the common indie choice; WiX for MSI). If the developer is outside the US or Canada and not incorporated, budget for an OV cert on a cloud HSM or token (renewed at most every ~15 months now), or form a company to use Artifact Signing.
- Expect early SmartScreen warnings regardless of certificate type.

### Gaps
- learn.microsoft.com could not be fetched, so the country list for individuals relies on search excerpts. Microsoft has been expanding availability; re-check before purchase.
- I found no primary citation for current Inno Setup or WiX versions. I did not research these; they are standard tools.

## 9. Licensing / copy protection and payment (merchant of record)

### Takeaway
Indie options run from simple serials to managed license platforms:
- **Moonbase:** merchant of record **plus** licensing; 5% + €1 per transaction; licensing free up to 10k validations.
- **Keyzy:** licensing only, not a merchant of record; free up to 20 licenses, then $24–$99/mo.
- **PACE iLok:** enterprise-grade, priced per contract.

General merchant-of-record platforms (Paddle, FastSpring, Gumroad, Lemon Squeezy) handle VAT and sales tax.

### Cited Findings
- **Moonbase (Medium, vendor page):**
  - Usage pricing of 5% + €1 per transaction with no monthly fee. Licensing is free with 10,000 validations included, then €5 per 10k.
  - It acts as merchant of record, collecting, filing and remitting VAT and sales tax.
  - The founder discussed the audio-plugin focus on The Audio Programmer podcast.

  — [Moonbase pricing](https://moonbase.sh/pricing/); [Moonbase vs Paddle](https://moonbase.sh/compare/paddle/); [Moonbase vs Gumroad](https://moonbase.sh/compare/gumroad/); [The Audio Programmer podcast ep. 35](https://www.theaudioprogrammer.com/podcast/how-to-sell-your-audio-plugins-w-tobias-madsen-moonbase-ep-35)
- **Keyzy (Medium, vendor page):** free with 20 licenses; paid tiers from $24/mo (Single) to $99/mo (Business, 10,000 licenses). It works with or without a merchant of record. Keyzy's blog argues that merchant-of-record total fees on a $99 sale land around 5.5–8%. — [Keyzy pricing](https://www.keyzy.io/pricing/); [Keyzy blog](https://www.keyzy.io/blog/)
- **PACE (Medium):** PACE products (iLok licensing, cloud signing) are enterprise-priced. — [Moonbase: Code signing audio plugins in 2025](https://moonbase.sh/articles/code-signing-audio-plugins-in-2025-a-round-up/)
- **JUCE marketplace (Medium):** Pamplejuce is listed there. JUCE historically shipped online-unlock classes (`juce_tracktion_marketplace` / `OnlineUnlockStatus`), as mentioned in the changelog ("expiring licenses to juce_tracktion_marketplace"). — [JUCE CHANGE_LIST.md](https://raw.githubusercontent.com/juce-framework/JUCE/master/CHANGE_LIST.md); [JUCE marketplace: Pamplejuce](https://marketplace.juce.com/products/pamplejuce)

### Inferences
- For a first EQ, use Moonbase (one vendor for storefront, tax and licensing, with JUCE integration available), or a merchant of record (Paddle, FastSpring, Lemon Squeezy) plus simple offline serial/key-file checks.
- iLok is overkill unless AAX buyers expect it.
- Keep protection light: EQs are commonly cracked, and heavy DRM hurts legitimate users.

### Gaps
- I did not verify current Paddle, FastSpring or Lemon Squeezy fee schedules (not searched; budget constraint).
- I did not verify PACE iLok licensing prices (they are not public).

## 10. DAW format support matrix (late 2026)

### Takeaway
To cover the major DAWs, ship **VST3 + AU (+ AAX for Pro Tools)**. CLAP is a bonus for Bitwig, REAPER, FL Studio and (partly) Studio One users.

| DAW | VST3 | AU | AAX | CLAP | VST2 |
|---|---|---|---|---|---|
| Logic Pro (mac) | No | **Yes (only format)** | No | No | No |
| Pro Tools | No | No | **Yes (only format)** | No | No |
| Ableton Live 12 | Yes | Yes (mac) | No | **No** | Yes |
| FL Studio | Yes | Yes (mac) | No | **Yes** (21.2 / 2024+) | Yes |
| Cubase/Nuendo 15 | **Yes** | No | No | No | Being phased out |
| Studio One 6.5/7 | Yes | Yes (mac) | No | Partial (conflicting reports) | Yes |
| REAPER 7 | Yes | Yes (mac) | No | **Yes** | Yes |
| Bitwig | Yes | No (VST/CLAP-centric; not verified) | No | **Yes** | Yes |

### Cited Findings
- **Ableton Live 12:** supports VST2, VST3 and AU; no CLAP. — [Dubspot: Live 12 vs FL Studio (2026)](https://blog.dubspot.com/plugins/compare/ableton-live-12-vs-fl-studio); [gearnews: Live 12.4](https://www.gearnews.com/ableton-live-12-review/)
- **Format overview by DAW** (Pro Tools AAX-only; Logic AU-only): [Antares: VST3 vs AU vs AAX vs CLAP (2026)](https://www.antarestech.com/blog/vst3-au-aax-clap-plugin-formats-2026); [Audio Tech Hub](https://audiotechhub.com/blogs/music-production/au-vs-vst3-vs-clap-plugin-formats)
- **CLAP host list:** see §3. — [Spectral Colors 2026](https://spectral-colors.com/news/clap-vs-vst3-2026/); [ProducerGrid 2026](https://producergrid.com/blog/clap-plugin-format-everything-you-need-to-know/)
- **Steinberg hosts and VST2:** Steinberg is discontinuing VST2 support in its products. — [gearnews](https://www.gearnews.com/steinberg-discontinues-support-for-vst2-plug-ins)
- **Pro Tools retail:** loads only PACE-signed AAX. — [truce.audio AAX docs](https://truce.audio/docs/formats/aax/)

### Inferences
**What Linux and iOS would add:**
- **Linux:** JUCE (VST3 and LV2), DPF (LV2, VST3, CLAP) and nih-plug/iPlug2 all build Linux plugins. Linux hosts are mainly REAPER and Bitwig (based on general knowledge, not re-verified this session), plus open-source hosts (Ardour, Carla). No code-signing or notarization step exists on Linux; distribute a tarball or deb.
- **iOS/iPadOS:** requires AUv3 as an app extension inside a containing app, distributed through the App Store under the same $99/yr Apple program. It adds App Review, sandboxing, touch UI and small-screen layout work. JUCE (iOS 12+) and iPlug2 (iOS 15+) support it.

### Gaps
- I found no first-party vendor format pages for each DAW (Image-Line, Fender/PreSonus, Bitwig, Cockos); the matrix relies on 2026 secondary guides. The Bitwig AU row and Studio One CLAP row are the least certain.
- Pro Tools "AAX only" and Logic "AU only" are long-standing and consistent across sources, but I did not see a 2026 first-party statement.
- Network restriction: juce.com, forum.juce.com and learn.microsoft.com were unreachable for direct fetch, so their facts are from search excerpts. Allowing those hosts would let these items be verified directly.
