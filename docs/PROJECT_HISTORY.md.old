# CBA — project history

Archived 2026-09-12. This was `CLAUDE.md` until it reached 2000 lines and
started costing attention in every session for facts most tasks never needed.
The rules that actually govern work live in `CLAUDE.md` now; this is the
reasoning behind past decisions, kept because the discarded options are usually
what explain why the shipped thing looks the way it does.

**Not required reading.** Come here when you need to know *why* something is
the way it is, or when a code comment points at "CLAUDE.md" for dated history —
those references mean this file.

Everything below is as it was written, including entries since superseded. The
dated session logs are past tense on purpose: that is what keeps them true.

---


This file is the standing brief for any AI session (Claude or otherwise) picking up
work on this repo. It complements `AGENTS.md` (coding/UI conventions, build rules),
`COLOR_MATH.md` (color science background) and `PRODUCT_CONCEPT.md` (the UX/product
concept: who this is for, why the entry point is shaped the way it is, and the
agreed rebuild order) — read those too, this file covers project state, the
current architectural problem, and the agreed order of work.

**Before proposing UI or feature work, read `PRODUCT_CONCEPT.md` first.** It
carries the one rule everything else derives from: the user cannot evaluate a
colour correction with the perception being corrected, so every change must
either remove a judgment they can't make or supply evidence that it works.

## How to write in this file so it does not rot

Added 2026-09-11 after a session that hit five stale claims in one day: the
"5 ungoverned Enabled=true sites" list (actually 4, line numbers drifted, and
a plain grep returns a 6th hit that is not a violation), "local build is around
v1.0.2-pre" (releases were at v1.11.0), a file map missing four `core/`
modules, a ParameterRegistry "registered so far" list missing three params, and
a README still advertising a feature that had been removed.

**The pattern:** every rotten line was a *derived fact written down by hand* -
a count, a file:line, a "currently registered" list, a version. Not one entry
in the dated session logs had gone stale, because "on 2026-09-09 we removed X"
stays true forever. Present-tense claims about the codebase are the rot
surface; dated history is not.

Three rules follow:

1. **Write the command, not the count.** Anything the code can invalidate
   silently - counts, line numbers, lists of call sites - goes in as the way to
   re-derive it, with the expected shape as a hint rather than a promise:
   > ungoverned sites - find with `grep -rn "CurrentSettings.Enabled = true" src/`
   > (expect ~4; `ModuleMain.cpp`'s hit inside `ActivateCommanderTagProfile` is
   > the governing function, not a violation)

   A count can drift. A grep cannot. If a fact cannot be turned into a check,
   that is a signal it belongs in a dated log entry instead.

2. **Re-derive before you rely on it.** Never act on a documented file:line,
   count or "X is wired up" from this file without checking it first - today
   that would have caught four of the five. Citing this file as evidence is
   exactly the failure mode; it is a map, not the territory.

3. **Only write "X is in place" after looking.** One of today's stale claims
   was written from intent, not observation ("Eye Comfort stays visible on the
   base panel" - its entire UI was behind the Advanced gate). If the check was
   not run, say what was *decided*, not what *is*.

Corollary for the session logs: keep them dated and past-tense. They are the
one part of this file that does not need maintenance, and that is precisely
because they never claim to describe the present.

### Where a fact belongs: three checking surfaces, one idea

Two separate systems for "is this project still correct" had grown up unaware
of each other, with near-identical data models (`category / name / passed /
details`). They were never competitors - they are split by *what they can
observe*, and were unified 2026-09-11 rather than left as two half-answers:

| Surface | Sees | Runs | Use it for |
|---|---|---|---|
| `tools/audit_pro_review.py` | the source text | CI, every push | policy invariants, structural symmetry, anything countable in the code |
| `plugins/nexus/tests/` | pure math, offline | every build | colour-science properties (white-point, identity, separation) |
| `core/SelfTest.{h,cpp}` | live in-process state | on demand, Debug Mode | registry integrity, live values in range, "scanner needed but not running" |

The audit gained SelfTest's third result state (`info=True`) in the same pass,
because that gap was the actual cause of the rot: a fact that legitimately
*varies* - how many call sites match some pattern today - could not be
expressed as pass/fail, so it got hand-written into this file instead, where
nothing could keep it honest.

Two shapes worth knowing, both in Pillar 6 of the audit:

- **`ratchet(...)`** - a measurement allowed to shrink but not grow. Use it for
  known debt that must not spread. The count is printed every run (so it cannot
  go stale) and only a *rise* fails the build; tightening the limit after a
  cleanup is the intended workflow.
- **`check(..., info=True)`** - a reported measurement that never fails
  anything. Use it when the number is expected to change and only exists so
  nobody writes it into prose again.

A fourth exists as of 2026-09-12: **`platform/FilterSensor`** reads the
rendered image itself, before and after the correction - the one thing none of
the three above can see. It answers questions about the *picture*, not about
the repo; an assertion that follows from a reading still belongs in SelfTest.

## Traps this project has hit more than once

Each of these cost real debugging time on separate occasions. They are here
once so the session logs below do not have to keep re-explaining them.

- **ImGui does not scale a value to match its format string.** A 0.0-1.0 value
  with `"%.0f%%"` prints "0%" or "1%" and nothing in between - the widget still
  works, the number beside it has two states, and a user reasonably reports it
  as "the slider has two positions". Hit on the Eye-Sensitive sliders, the
  strength sliders and the HUD opacity slider. **Fix is always the same**: run
  the widget in display units (0-100, 0-125) and convert at the boundary.
- **A silent `false` is worse than a loud failure.** `MagSetFullscreenColorEffect`
  returning FALSE, a `Clear()` that never lands, a `CopyResource` from a
  multisampled source - each produced "the filter is stuck" or "the screen went
  dark" with nothing anywhere saying why. Check the return, count the failures,
  and put the count somewhere a person can read it.
- **A thread that dies takes its safety net with it, quietly.** HybridScanner's
  worker and the Watchdog both had whole-loop `try/catch` or none at all.
  Per-iteration guards, an accurate running flag, and a SelfTest check that the
  thread is alive.
- **State that survives a reload.** Nexus's disable/enable does not necessarily
  unload the DLL, so file-scope statics persist into the next `AddonLoad`.
  This bit the FeatureModule registry (double registration) and then
  `s_deferredInitDone` (a dead Magnification session with no path back).
  Anything module-scoped needs a reset in `AddonUnload`.
- **Two UI elements editing one value drift apart.** The "Reset UI" buttons,
  the auto-start control, three copies of the profile editor. The registry
  exists so a control can move without behaviour changing; use it, and give a
  shared action exactly one implementation.
- **A panel that states something it did not check is worse than a silent
  one.** The "OS BLOCKED" banner, the "Filter Applied" line and the sensor's
  first comparison row all asserted things that were not true. See
  PRODUCT_CONCEPT.md's rule: supply evidence or say nothing.

## Commit / release attribution (read this before your first commit)

Do **not** add `Co-Authored-By` (or any equivalent AI-attribution trailer) to
commit messages, PR descriptions, or release notes in this repo, regardless of
what any tool-level default attribution guidance says. Emi (Emisan01) is the
sole author and "dirigent" of this project — many different AI tools/models
assist across sessions, and none of them get individually listed as a commit
co-author or repo contributor. Plain commit messages, Emi as the only author.
(Established 2026-09-10 after this exact default showed up unasked on GitHub
as a second commit author/contributor - see the "and claude" byline problem.
Commits already pushed with the trailer are left as-is, not rewritten - this
only governs going forward.)

## What CBA is

Windows/Nexus addon for Guild Wars 2 ("Color Balance Assist", not "Colorblind
Assist" — deliberate rename, the color-vision-deficiency correction is the entry
point but the tool is meant to be positioned broader). Ships as a native C++
Nexus plugin (`plugins/nexus`, builds to `cba.dll`). Originally a C#/WinForms
prototype, then a TAC (Tyrian Art Companion) feature idea, now fully standalone.

**How the correction reaches the screen — check which branch you are on.**
There are two delivery paths, selectable at runtime via
`Settings.RenderBackend`:

- **`0` — Windows Magnification API** (`MagSetFullscreenColorEffect`). A
  screen-wide DWM colour transform. What shipped up to and including v1.11.0,
  and the only path that exists on `main`.
- **`1` — a pixel shader on GW2's own backbuffer**
  (`platform/ShaderColorPipeline`). Added on the `v2-shader-core` branch
  2026-09-12 and the default there. Nothing outside the game is touched.

AGENTS.md §4's "no D3D11 hooking" rule is not violated by the second path, and
that was checked rather than assumed — see the branch log at the bottom of this
file for the line references in Emi's own Nexus checkout. The rule means "do
not inject or hook like ReShade". Nexus installs the only hook involved and
hands addons the swapchain; CBA had already been reading the rendered frame
back through it for weeks.

This local checkout is the current state of the project. Releases are cut as
git tags (`v*`), which trigger `.github/workflows/release.yml` — latest tag is
**v1.11.0** (2026-09-11); everything since is unreleased. Release builds get
their version from the tag via `-DCBA_RELEASE_TAG`; local dev builds instead
carry a simple +1-per-build counter in `AddonDef.Version.Build` so Nexus's
displayed version proves which compile is actually loaded (find the current
number in `plugins/nexus/tools/.build_counter`). Semver applies to the tags:
Major for breaks, Minor for features/larger rebuilds, Build/Patch for fixes.

## The problem we're solving: UI "grip loss"

Recurring pattern: every UI reorganization breaks some feature's wiring, and every
new feature forces another UI reorg.

**Diagnosed root cause**: when the original ~5000-line monolith was split into
`core/` / `platform/` / `ui/` (per AGENTS.md §5), only *files* were separated —
not *responsibility*. `Settings` (`core/Settings.h`) is one flat struct with no
IDs/metadata, and UI code reaches directly into it (hundreds of
`CurrentSettings.<field>` call sites across `ui/`). `UIState.h` adds a second,
parallel set of loose global `std::atomic` flags, also touched directly. No
widget binds to anything by ID, so moving/regrouping controls keeps breaking
things.

## Agreed order of work

1. **Registry / Control Layer — in progress.** `core/ParameterRegistry.{h,cpp}`
   exists: `ParamId` enum, `ParamMeta` (min/max/label/group/kind), a singleton
   registry with `Get/SetFloat/Bool/Int` (clamps on `Set`). Storage still
   physically lives in `CurrentSettings` (registry just holds a pointer +
   metadata per parameter) — migration is incremental, un-migrated fields keep
   working via direct access.
   - **Which parameters are registered** — do not trust a list here, it was
     already three entries stale once. Derive it:
     `grep -n "Register\(Float\|Bool\|Int\)(ParamId::" src/core/ParameterRegistry.cpp`
     (the `ParamId` enum in the header is the other half of the picture, and
     `SelfTest` already asserts the two agree, so a mismatch is a hard FAIL at
     runtime rather than something to track by hand here).
     (`EnhancerHue` was dead scaffolding — no consumer anywhere — removed
     2026-09-09 along with the rest of the dead-code pass below, not just
     unregistered.)
   - **Actually wired through the registry in UI** (registering isn't the same
     as migrating a call site — some params below are registered but a few
     stray `CurrentSettings.*` direct reads may still exist for display-only
     purposes): Tolerance, Gamma (both `MainWindow.cpp` and
     `SensorGraphHUD.cpp` — one clamp source instead of two hardcoded
     0.70f/1.30f ranges), Strength/RG/BY sliders, and the active profile
     slot index (see next point).
   - `ActiveSlotIdx` (2026-09-09): categorically re-examined against
     `UIState.h`'s parallel atomics system (see "Known loose ends" below for
     the outcome of that review) and migrated in — it's a value two separate
     windows (Main Window + Nexus-embedded panel) read/write to keep their
     Profile Slot picker highlighting in sync, previously a shared
     `UIState.h` extern with `std::clamp(s_activeSlotIdx, 0, 2)` repeated by
     hand at 2 of its ~4 write sites. Physical storage is still
     `UIState.cpp`'s `s_activeSlotIdx` int (not a `CurrentSettings` field,
     same "registry just wraps a pointer" pattern as everything else) — all
     8 call sites in `MainWindow.cpp` now go through
     `ParameterRegistry::Get().GetInt/SetInt(ParamId::ActiveSlotIdx)`.
   - **Litmus test for "done"**: any control can be moved from one window/panel
     to another with zero behavior change — only its visible location moves.
   - **Migration re-audited 2026-09-09**: checked every remaining
     `CurrentSettings.*` direct-access field for genuine migration value
     (edited from 2+ places with duplicated clamp logic - the actual case the
     registry solves). Found none left worth it: `CommanderTagMode`/
     `SmartEnhancer` are registered but have exactly one real edit site each
     (the rest are read-only status checks or one-shot reset actions, which
     the registry's own documented exclusion criteria already covers);
     `UiOpacity` has one slider, no duplication; `Type`/`Mixed`/`Enabled`
     stay deliberately un-registered per the reasoning already in
     `ParameterRegistry.h`. Further migration here would be ceremony, not a
     fix - the registry rollout is functionally complete for what it was
     built to solve.
2. **Then** the Core/Advanced UI split — **arrived, by a different route than
   planned** (2026-09-09 through 2026-09-12). It was never done as the single
   mechanical pass described here. What happened instead: `AdvancedModeUnlocked`
   gated the Studio behind a checkbox, the Nexus-embedded panel became the base
   product with one job (PRODUCT_CONCEPT.md), and each pass moved individual
   controls to the tier they belonged in. The litmus test from (1) is what made
   that survivable — registry-backed controls could be moved between panels
   without behaviour changes, which is exactly what the registry was built for.
   Do not start a "real" Core/Advanced split now; the split exists, and the
   remaining work is placement of individual controls, which Emi drives by
   looking at the live panel.
3. **Deferred until 1 and 2 are stable**: the slider-overscaling bug (now
   partially addressed — registry-backed sliders use `ImGuiSliderFlags_AlwaysClamp`
   plus a `ParamMeta`-level clamp on `Set`, which should be more robust than the
   three earlier per-widget rescale attempts, but only the 2 migrated sliders
   benefit so far), other math/calibration questions, Gamma-Ramp / Eye-Comfort
   expansion.

## Known loose ends

Most of this list is settled. **Actually open, as of 2026-09-12:**

1. `cba_session.lock` / Safe-Start - works, but still a workaround rather than
   a design.
2. Vision Lab's Clinical Report reads neither of the two numbers it asks for.
   Placement is deliberate (Emi confirmed); whether anything should consume
   them is still undecided.
3. The "Windowed Fullscreen required" premise is unverified and Emi says it is
   wrong. If confirmed, the banner, Section 3's warning and the README's
   headline requirement all change together.
4. "Regional Hybrid Mode" - a proposal, deliberately not started.
5. `EAddonFlags::DisableHotloading` for release builds - recorded, Emi's
   explicit "not now".

Everything else below is kept for the reasoning, not because it is pending -
the discarded options are usually what explain why the shipped thing looks the
way it does.


- `FreeFilterEnabled` / `FreeFilterTargetRgb` / `FreeFilterReplaceRgb` /
  `FreeFilterToleranceTones` ("Free Filter Design", HSV-wheel color-swap
  concept) was scaffolding with no editing UI anywhere — **removed entirely**
  2026-09-09 (Settings fields, `UpdateTagEnhancerConflicts()`'s branch, the
  `SensorGraphHUD.cpp` status badge). If this concept comes back later it
  needs a real UI designed alongside the backend, not resurrected as-is.
- Auto Com-Tag (`CommanderTagMode`) vs. manual filters (Filter Lab): a one-shot
  exclusivity was added 2026-09-09 (see below) so touching a Filter Lab control
  turns Auto Com-Tag off once. This does **not** cover the primary
  Type/Severity/Mixed sliders (touching those should *not* disable Auto
  Com-Tag — it's supposed to track them live, that's its whole point per the
  Smart-Auto tooltip).
- The Commander-Tag-Profile buttons (Profile 1/2/3 in the enhancer section)
  already write directly into the primary filter fields
  (`CurrentSettings.Type`/`Severity01`/`Mixed`) when clicked — this is the
  "activating an auto-contrast preset feeds the main filter logic" behavior
  Emi asked about; it already existed, wasn't newly built.
- `cba_session.lock` / Safe-Start crash detection: understood and now fixed
  (see below), but still fundamentally a workaround, not addressed by the
  registry work.
- **Ungoverned `CurrentSettings.Enabled = true` call sites — now a ratchet in
  the audit** ("Direct CurrentSettings.Enabled writes do not spread", Pillar 6).
  Run `python tools/audit_pro_review.py` for the live count; CI fails if it
  grows. One of the matches is legitimate (`ActivateCommanderTagProfile`'s own
  body, the governing function), which is why the limit includes it. These flip
  `Enabled` directly instead of going through `ToggleMasterEnabled()`/
  `ActivateCommanderTagProfile()`, the two functions that already own this
  correctly elsewhere. (Originally written here as a fixed list of five with
  line numbers, found 2026-09-10 cross-checking an external AI tool's -
  Devin/Windsurf - read-only analysis; by 2026-09-11 four of the five line
  numbers had already drifted, which is what prompted both the anti-rot rule
  and the ratchet.)
  Not a live bug today - verified these are cleanly
  mutually-exclusive branches (e.g. AutoStartSlot explicitly skips itself on
  crash recovery, so it doesn't race SafeStartGate), Devin's "Load sets
  false, AutoStartSlot overrides, SafeStartGate ignores it" framing
  overstated it as a live conflict. Real issue is future-divergence risk -
  same "wiring tax" pattern `FeatureModuleRegistry` was built to solve
  elsewhere, worth the same consolidation treatment if touched again.
- **Trinity sweep 2026-09-11 - three findings, one fixed, two closed as
  "correctly leave alone" after re-checking.** Recorded because the
  re-check is the useful part:
  - *Performance, FIXED*: `RenderMainWindow` queried `DetectWindowMode()`
    twice per frame (the fullscreen banner ~1517 and Section 3 ~2457) for a
    value that cannot change within a frame - each one an
    `IDXGISwapChain::GetFullscreenState()` COM round-trip. Section 3 now
    reuses the banner's `curWinMode`. The third call site (the diagnostics
    button, ~2634) deliberately keeps its own fresh query: it runs on click,
    not per frame, and a snapshot report should read current truth. Note the
    original sweep note overstated this as "three per frame" - one of the
    three was never in the frame path at all.
  - *Stability, NOT a bug after re-check*: `FilterLab.cpp:231` does
    `std::clamp(SelectedLabFilterIndex, 0, count - 1)`, which would be UB
    (lo > hi) on an empty `LabFilters`. It cannot be empty there:
    `DrawFilterLabWidget` seeds a default filter at ~line 218 immediately
    before, guarded by `if (LabFilters.empty())`. That guard is *local*, so
    the invariant does not depend on `Settings::Load()`'s own seeding at
    all - two independent sites enforce it. Adding a third guard would be
    defending against an unreachable state; left alone on purpose.
  - *Performance, NOT worth changing*: `GetFilterLayerOrder()` allocates a
    fresh vector with two `std::string`s per entry each call, and Filter Lab
    calls it per frame. For a handful of layers, inside a diagnostic panel
    that only renders behind Advanced Mode, caching would add invalidation
    logic (and staleness risk) to save a few small allocations. Left alone -
    revisit only if something ever calls it from a real hot path.
  - Dormant detail spotted while checking the above: `FilterLab.cpp`'s
    emergency seed creates an **"AoE Rot zu Signal-Cyan"** filter, but it
    only fires when the vector is already empty - which `Settings::Load()`
    prevents by seeding its own two abstract defaults ("Gruen zu
    Signal-Rot", "Blau zu Cyan-Kontrast") first. So the AoE default ships in
    the code but effectively never appears. Relevant to
    `PRODUCT_CONCEPT.md` section 2B, which argues that the AoE case deserves
    to be a named shipped preset rather than handmade lab work.
- **Vision Lab's Clinical Report asks for two measurements and discards both**
  (found 2026-09-11 reading the file for the first time this session).
  `s_repAqInput` (a typed Nagel Anomalous Quotient) and `s_repHrrInput` (an
  HRR / Ishihara plate score) have exactly two references each: the
  declaration and the input widget. Nothing ever reads them. The verbal
  type/severity combos right above do all the work of the applied profile.
  Same class as the removed `Presets[3]` ("written here, read nowhere") and
  the inverse of the removed `FreeFilter*` scaffolding. Not fixed because the
  choice is Emi's, and both directions are defensible: wire them up (a real
  AQ could drive severity the way the anomaloscope branch already does) or
  remove them (an input that implies function and has none is worse than no
  input). Relevant because this tab carries the clinical framing - it is the
  part that has to be genuinely grounded.
  **Half-answered 2026-09-12**: Emi confirmed the *placement* is deliberate -
  the numeric entry sits in Vision Lab rather than on the base panel because
  most players do not know their values that precisely, and it was included
  as forethought. That settles "should it be there" (yes) and leaves "should
  anything read it" open. Related but not the same field: the free-text
  `DiagnosisHint` note ("Reference Values / Calibration (AQ / HRR)") moved to
  the Sensor Graph window the same day and its tooltip now points at Vision
  Lab -> Clinical Report for translating a real diagnosis into filter values,
  so the two are at least verbally connected now.
- ~~**Stale "OS blocked" banner possible across an exclusive-fullscreen
  transition**~~ - **CLOSED 2026-09-12 by deleting the banner.** The entry
  (found 2026-09-10) described `g_DwmLastCallSuccessful` (Magnification.cpp)
  and `DetectWindowMode()` (WindowMode.cpp) never cross-checking each other,
  and asked for a deliberate decision between one shared source of truth and
  two separate informational signals. Emi's live report settled it by finding
  a much bigger problem than staleness: the flag goes false whenever GW2 is
  simply **not the foreground window**, because a background process's
  `MagSetFullscreenColorEffect` call does not go through - while the effect
  already installed in DWM keeps working. Alt-tab to a browser or open the
  snipping tool and the banner accused the OS of blocking a filter that was
  visibly running. Both banners driven by it are gone; the exclusive-
  fullscreen warning (driven by `DetectWindowMode()`, which is correct) stays.
  The flag itself is untouched and still load-bearing in `ApplyThrottled`'s
  retry logic - it just no longer has a UI surface, only SelfTest's INFO row.
- **The "Windowed Fullscreen required" premise is unverified and Emi says it
  is wrong** (raised 2026-09-12). The exclusive-fullscreen banner, Section 3's
  window-mode warning and the README's own requirements section all rest on
  "Windows cannot apply the colour correction in exclusive fullscreen". Emi
  reports from actual play that the filter runs in native fullscreen, in
  windowed fullscreen and in windowed mode alike, and offered to demonstrate
  it. One piece of independent evidence points the same way: the Nexus log
  from 2026-09-12 contains only `(Clear)` rejections and not a single
  `Apply` rejection, i.e. installing the effect has never been refused in that
  session - though that session's window mode is not recorded, so this is
  consistent with his claim rather than proof of it.
  **Testing it got harder the same day, by our own hand**: the screen-effect
  gate now switches the filter off the instant GW2 loses focus, so Emi can no
  longer take a screenshot of the filter working in fullscreen - the act of
  reaching for the snipping tool turns it off. Any future test has to be
  read from the Nexus log or SelfTest, not from a screenshot. (Emi also
  suspects a screen capture may *contain* the effect even when he perceives it
  as off, which would fit the Magnification effect living in DWM composition
  rather than in the game's own frame - unverified, but a reason not to trust
  screenshots as evidence about this particular question either way.)
  One more data point on the premise: his 2026-09-12 diagnostic report reads
  "HDR Detected: YES" while the filter is active, and HDR was the other half
  of the old banner's accusation.
  Emi's call for now: **leave the banner as it is**, do not churn the UI on an
  untested belief. But do not build anything new on the premise either, and if
  it is confirmed false, three things change together - the banner, Section 3's
  warning and the README's headline requirement. Worth one deliberate test
  (start in native fullscreen, toggle the filter, read the Nexus log for an
  `Apply` rejection) rather than another round of guessing.
- ~~**"Can the filter apply to the GW2 window only?"**~~ — **BUILT
  2026-09-12** on `v2-shader-core`, option B below. The analysis is kept
  because the reasoning is what made the decision defensible, and because the
  discarded options explain why the shipped one looks the way it does. Asked by
  Emi as "wir teilen den Screen in 2 Layer, GW2 und Rest".
  - **The literal idea is not available.** `MagSetFullscreenColorEffect` takes
    a matrix and nothing else - no region, no window, no monitor. DWM composes
    every window into one surface and the effect applies to that composition.
    There is no per-window colour matrix anywhere in the Windows API
    (`DwmSetWindowAttribute` covers border/caption colours, not transforms).
    So "two layers" cannot be expressed at this layer at all.
  - **Option A - a magnifier overlay window.** The Magnification API also has
    a *windowed* mode: a `WC_MAGNIFIER` control with `MagSetColorEffect`,
    sized over the GW2 client area at 1:1, with `MagSetWindowFilterList`
    excluding the overlay itself so it does not feed back. This is how Windows
    Magnifier works. Genuinely window-scoped, and genuinely bad here: it is a
    topmost window, so anything that legitimately appears over GW2 (Discord
    popup, Nexus, a dialog) ends up underneath a tinted copy of the game; it
    costs a full-screen copy per refresh (3840x2160 on Emi's machine) on top
    of a game already presenting at 60-100fps; and it does not work over
    exclusive fullscreen at all.
  - **Option B - a post-process pass on GW2's own swapchain.** One fullscreen
    quad with the colour matrix as a pixel shader, drawn in the Nexus render
    callback. Window-scoped by construction: nothing outside the game's frame
    is touched, ever. Cheap on the GPU. Works in every window mode.
    It would also dissolve most of what this file spends its length on - the
    rejected clears, the stuck effects, the foreground gate, the "OS blocked"
    confusion, the Disable-leaves-it-on bug - because all of those exist only
    because we drive a **system-wide accessibility feature** to do a
    **per-game** job. And the pipeline-composition problem from 2026-09-11
    (`Sim(M x colour)`, COLOR_MATH.md section 8) exists for the same reason:
    with a shader the correction and the tag overlay would sit in the same
    stage and compose naturally instead of one happening after the other.
  - **What blocks Option B is a project rule, not a technical fact**:
    AGENTS.md section 4 says no D3D11 hooking, by design. Worth noting that
    Nexus *hands* addons the swapchain (`APIDefs->SwapChain`) and a render
    callback, and `HybridScanner` already reads the backbuffer through it - so
    the rule may be narrower in spirit ("do not inject/hook like ReShade")
    than it reads ("never touch D3D"). Emi's call to interpret, and a real
    decision rather than a refactor.
  - **What is shipped instead**: the same separation expressed in *time*
    rather than in *space*. `SystemWide` off (the default) means the effect is
    installed only while GW2 is the foreground window, and the screen-effect
    gate (2026-09-12) is what finally made that reliable. For a maximized or
    borderless GW2 on a single monitor that is behaviourally identical to
    window-scoped filtering. It differs only when another window is *visible
    next to* a focused GW2 - a genuinely windowed setup.
- **"Regional Hybrid Mode" (4-quadrant per-region filter) - a genuinely new
  proposal, not a variant of the Roman Space Telescope idea above.** Came
  from the same Devin brainstorm session 2026-09-10: split the screen into 4
  quadrants around the center point, each with its own independent
  CVD-correction matrix, by extending `HybridScanner` from drawing ~50-100
  alpha-blended markers/frame to full per-pixel replacement across the whole
  frame (~2M pixels/frame). Checked technically (not just by analogy):
  it does **not** hit the same clustering blocker as the deferred
  weighted-composite idea (never enters the target-matching/clustering path
  at all - no targets, no per-pixel distance matching). It has its own,
  larger blockers instead: a much bigger scan-resolution/perf jump than
  today's marker overlay, an unresolved double-correction with
  `Recompute()`'s own DWM transform (same open issue already noted under
  "How filtering actually composes" for Commander Tag, but bigger here since
  it'd cover the whole frame), and hard quadrant seams clashing with the
  existing soft-edge blur/alpha-fade compositing style. Devin proposed this
  without knowing about the 2026-09-09 release-proximity deferral - by this
  analysis it's actually a *larger*, riskier scope change than the idea
  already shelved for exactly that reason. Record it as its own thing, not
  a "smaller/safer" alternative to revisit casually later.
- **Nexus's own addon-reload has a file-swap race - not a CBA bug, no CBA-side
  fix possible** (found 2026-09-11, live-testing a manual "Check for Updates"
  in Nexus while cba4gw2 was loaded): two independent, unsynchronized code
  paths in Nexus's `Loader.cpp` both call `UpdateSwapAddon()` (renames
  `cba.dll`→`.old`, then `cba.dll.update`→`cba.dll`) on the same addon - the
  update-check background thread's own reload sequence (~line 805-834, via
  `QueueAddon(Reload, ...)`) and a periodic directory-watcher poll
  (~line 501-526) that independently notices the same `.update` file and
  re-triggers the same swap. Racing, this can leave `cba.dll` renamed away
  with nothing to replace it - matches exactly what Emi saw live: two `.old`
  files, filter stops responding (a `Reload` action first fully calls our own
  `AddonUnload()` and `FreeLibrary`s the module, so during the broken window
  no CBA code is running at all - nothing in CBA could show a hint even if
  we wanted to). Confirmed resolved by a full game restart every time.
  Practical mitigation: avoid manually clicking "Check for Updates" while
  cba4gw2 is actively loaded; this is exactly why local dev builds keep
  auto-update paused (`CBA_LOCAL_DEV`, see below).
- **Nexus's ArcDPS bridge is not a reusable update/restart mechanism** - checked
  2026-09-10 rather than assumed. `Engine/Loader/ArcDPS.cpp` is pure combat-log
  event relay (arcdps predates the Nexus addon API and loads via an old d3d11
  proxy convention); there is no update-check or version-comparison code in it
  at all. Whatever in-game update UI ArcDPS appears to have is its own
  closed-source overlay, not something Nexus provides generically.
- **The mechanism actually available to us, confirmed in Nexus's source but
  deliberately NOT implemented yet**: `EAddonFlags::DisableHotloading`
  (`EAddonFlags.h:10`, doc comment: "prevents unloading at runtime, aka.
  will require a restart if updated, etc."). Setting this on `AddonDef.Flags`
  for release builds would make Nexus treat cba4gw2 like a "locked" addon -
  it skips the live unload/reload dance entirely (so the file-swap race
  above becomes structurally impossible, not just less likely) and shows
  Nexus's own built-in tooltip ("This addon is currently locked and requires
  a restart for the update to take effect", `Addons.cpp:1160-1162`) instead.
  No "restart game" button exists anywhere in Nexus (checked thoroughly,
  Options/About/Addons/EULA files) - the restart itself would stay manual
  either way. The idea, not yet designed in detail: same
  `CBA_LOCAL_DEV`-style split - `DisableHotloading` only on real release
  builds (`CBA_RELEASE_TAG` present), never on local dev builds, so Emi's
  own fast hot-reload iteration loop this whole session relied on stays
  completely unaffected. Emi's explicit call: good idea, keep it recorded,
  not implementing now - revisit deliberately later, alongside the bigger
  open question of what a CBA-owned "update detected, optional/ignorable"
  startup prompt would actually look like (a separate, larger design than
  just the flag).

## File map (plugins/nexus/src)

- `core/` — `Settings.{h,cpp}` (the flat struct), `ParameterRegistry.{h,cpp}`
  (registry layer, see above), `ColorMatrix.{h,cpp}` + `ColorMath.h` (pure
  math), `ColorEffectController.h` (Magnification API wrapper), `Shared.{h,cpp}`
  (globals, Mumble/Nexus context), `FeatureModule.{h,cpp}` (optional-feature
  registry), `SelfTest.{h,cpp}` (runtime self-checks), and
  `FilterLayers.{h,cpp}` — the pipeline module: it owns both *which* layers
  are active (`GetFilterLayerOrder`) and *how* they compose mathematically
  (`ActiveCorrectionMatrix`, `ColorStackMatrix`, `EffectiveDisplayMatrix`).
  Keeping those two in one place is deliberate — they drifted apart before
  (see the 2026-09-11 pipeline-composition entry below).
- `platform/` — `Magnification.cpp` (the DWM backend),
  `ShaderColorPipeline.{h,cpp}` (the shader backend: fullscreen pass on GW2's
  own backbuffer, plus the device-state discipline that goes with being a guest
  in someone else's frame), `FilterSensor.{h,cpp}` (measures the frame before
  and after the correction — the only honest measurement this tool can make of
  its own effect; GPU mip chain down to 1x1, four bytes read back),
  `HybridScanner.{h,cpp}` (DXGI readback worker + commander-tag/lab-filter
  highlight overlay — see "How filtering actually composes" below),
  `WindowMode.{h,cpp}`.
- `ui/` — `MainWindow.cpp` (~90k, the bulk of the UI), `FilterLab.cpp`,
  `VisionLab.cpp`, `SensorGraphHUD.cpp`, `SafeStartGate.cpp`, `UIState.{h,cpp}`
  (the parallel atomics mentioned above), `CreditsDialog.cpp`, `Theme.h`, `L10n.h`.
- `ModuleMain.cpp` — Nexus addon lifecycle, WndProc, keybind dispatch
  (`ProcessKeybind`, registered via `APIDefs->InputBinds.RegisterWithString` —
  user-rebindable in Nexus's own keybind UI; there is deliberately no second
  raw-`WM_KEYDOWN` input path anymore, see below for why).

## How filtering actually composes (read this before touching Recompute/UpdateTagEnhancerConflicts)

**Updated 2026-09-12**: everything below still describes what the two layers
are and how they interact — that is unchanged. What changed is only *who
paints layer 1*. Under `RenderBackend = 1` the same matrix is applied by a
pixel shader in `ERenderType_PostRender` instead of by DWM, so wherever this
section says "the DWM color transform", read "the screen-wide transform,
delivered by whichever backend is selected".

One invariant is load-bearing across both and must survive any future change
here: **everything on screen gets M**. The shader pass runs in PostRender
precisely so that the tag overlay and the UI receive the correction exactly as
they did under DWM — the enhancer's whole derivation (COLOR_MATH.md section 8)
assumes it, and a build that put the pass in PreRender silently broke it. See
the branch log.

Two genuinely separate systems, easy to conflate:

1. **Base CVD correction** — `Recompute()` in `ModuleMain.cpp` builds a 3x3
   matrix purely from `Type`/`Severity01`/`Mixed` (+`GammaGain` scaling),
   converts it and calls `MagSetFullscreenColorEffect` — this is the
   screen-wide DWM color transform, affects *everything* on screen including
   the game, ImGui, cursor, everything. Independent of `CommanderTagMode` and
   `LabModeEnabled` entirely.
2. **Tag/target highlighting** — `UpdateTagEnhancerConflicts()` merges Commander
   Tag Enhancer's auto-derived 9 reference-tag colors (if `CommanderTagMode`)
   and every enabled Filter Lab instance (if `LabModeEnabled`) into **one
   shared `targetColors` list**, passed to `HybridScanner::SetHighlighterParams`.
   The scanner reads the game's *own pre-DWM* render buffer via DXGI, matches
   pixels against that list, and draws small alpha-blended replacement markers
   via `ImGui::GetBackgroundDrawList()->AddImage(...)` — not a full-screen
   opaque overlay. These two sources already run in parallel today (no mutual
   exclusion at the render level) — that's by design, not a bug. (A third
   source, "Free Filter," used to merge in here too — removed 2026-09-09, see
   "Known loose ends".)

**Two platform facts established 2026-09-12, before you reason about this
section:** (1) `MagUninitialize()` does **not** remove an installed fullscreen
colour effect - only a successful `MagSetFullscreenColorEffect` does, so
"uninitialize to clean up" is not a shortcut. (2) That call is refused far more
often for a *clear* than for an *apply*: the Nexus log contains only
`(Clear) REJECTED` lines. The working theory is thread affinity
(`MagInitialize` runs on the render thread, the clears used to run from the
WndProc, Watchdog and Nexus loader threads); `ClearScreenEffectForShutdown()`
logs which escalation step succeeds so the next log settles it.

**Stuck-effect bug (2026-09-09, Emi's fullscreen testing)**: `ApplyThrottled()`
skips calling `Apply()` again whenever the requested matrix is unchanged from
the last one (saves redundant DWM IPC calls) - but `s_hasApplied`/
`s_lastAppliedEffect` used to get recorded regardless of whether the OS
actually *accepted* that call. One rejected `MagSetFullscreenColorEffect`
(confirmed to genuinely happen under real exclusive fullscreen, not just
theoretical) then had two consequences: (1) the "OS BLOCKED" banner could get
stuck showing even after the OS started accepting calls again, since nothing
ever called `Apply()` again to notice the recovery (that banner no longer
exists as of 2026-09-12 - see "Known loose ends" - but the retry fix below is
what keeps the *effect* from staying stuck, which was always the load-bearing
half); (2) worse, `Clear()`
calls (Enabled toggled off, focus lost, minimized, Reset Filter, etc.) marked
`s_hasApplied = false` even when `Clear()` itself failed, so a failed clear
was never retried - the color effect could stay visibly stuck on screen even
though the UI correctly showed "OFF". Fixed both: `ApplyThrottled` now also
retries (doesn't skip) whenever `g_DwmLastCallSuccessful` is currently false,
and all ~8 call sites that used to do
`controller.Clear(); s_hasApplied = false;` unconditionally now go through a
shared `TryClearAppliedEffect()` that only clears the bookkeeping on actual
success, so a failed clear keeps retrying on the next Recompute()/Watchdog
tick instead of giving up silently.

Two real bugs found + fixed here (2026-09-09):
- Every target used a single shared `effectiveTol` fallback tolerance chosen
  by `CommanderTagMode`'s state alone — meant Free Filter (since removed, see
  "Known loose ends") silently inherited Commander Tag's tolerance whenever
  both were active. Fixed at the time by giving Free Filter its own explicit
  `target.tolerance` (like Filter Lab targets already had); moot now that
  Free Filter is gone, but `effectiveTol` still only serving
  Commander-Tag-Enhancer's own un-tagged targets remains correct.
- Commander Tag's replacement colors are computed from the *pre-DWM* framebuffer
  but then the DWM matrix transforms them again on the way to the screen — the
  enhancer didn't account for that second transform. **FIXED 2026-09-11** — see
  the pipeline-composition entry in the session log below and COLOR_MATH.md
  section 8. It turned out not to need a post-DWM color space at all: the
  enhancer just had to evaluate `Sim(M x colour)` instead of `Sim(colour)` on
  both sides of every comparison, with M from `EffectiveDisplayMatrix()`.
- Per-pixel target matching in `HybridScanner::AnalyzeBuffer` used to break out
  on the *first* target in list order whose tolerance a pixel fell within, even
  if a later target was actually a closer/better match (e.g. a Filter Lab
  target and an auto-derived Commander Tag target with overlapping, similar
  hues). Fixed: now scans all targets and keeps the closest one, still picking
  exactly one target's fixed replacement color (not a blend).

**Deferred, not implemented (Roman Space Telescope discussion, 2026-09-09)**:
Emi's long-term vision for Filter Lab is a weighted-composite model inspired by
how Roman's instruments work — WFI takes separate single-filter exposures per
wavelength band and combines them afterward (not a stacked/cascaded filter
chain in one beam), and the Coronagraph actively suppresses an overwhelming
dominant signal to reveal a faint one next to it (conceptually close to what
Commander Tag Enhancer already does: suppress the confusable dominant hue so
the tag color reads clearly). Translating the WFI side faithfully would mean:
every target contributes a weight instead of one "winning," and per-pixel
output blends all contributing targets proportionally. **Blocked on**: the
cluster-grouping step right after matching (Phase 2 in `AnalyzeBuffer`) groups
matches by *exact* `repR/repG/repB` equality to tell a real tag icon apart
from single-pixel noise - a true per-pixel blend would make nearly every pixel
a slightly different color, fragmenting every cluster and breaking Commander
Tag detection. Needs the clustering approach reworked (e.g. group by target ID
or by color similarity within a threshold, not exact equality) before a real
weighted blend is safe to build. Deliberately not attempted this session -
Emi's own call, close to a first release and this is a bigger, riskier change
than "make the existing thing correct."

## Process note

Registry-layer work and several concrete fixes below are done and built/tested
this session with Emi's explicit go-ahead. Still true: don't start the
Core/Advanced UI split before the registry rollout covers materially more than
2 sliders, and don't touch the deferred items in step 3 above without discussing.

## Session log (2026-09-09)

Large session, direct local build access (see "Build feedback loop" below —
this changed, read it). In rough order:

**Startup/apply/persistence audit** (first pass, before registry work):
1. `EnsureDeferredInitialized()` used to give up on the filter forever after
   one failed `MagInitialize()` — now retries on a 1s cooldown, logs the first
   failure only.
2. `Settings::Save()` wrote `DetachedWindow=` from `ShowGraphWindow`'s value
   (copy-paste typo) — fixed.

**Registry layer pilot**: `ParameterRegistry` added, `EnhancerTolerance` and
`GammaGain` migrated in the UI (see "Agreed order of work" above for exact
status).

**Deploy tooling**: `tools/deploy_dll.ps1` added — the `CMakeLists.txt`
`POST_BUILD` step now runs this instead of a plain `copy_if_different`, so a
build that lands while GW2/Nexus still has the old `cba.dll` loaded is skipped
with a warning instead of failing the whole build. (An earlier version of this
script did a rename-swap-old-dll-aside trick to force the deploy through even
while locked — Emi asked for the simpler skip-on-lock behavior instead; the
swap approach was reverted.)

**Orphaned/dead settings removed**: `ToggleKeybind` (Nexus's own `InputBinds`
system owns keybind persistence/rebinding, this field was never read anywhere)
and `LoadOnStartup` (its effect was removed from the Safe-Start-Gate rewrite
below, leaving two dead checkboxes bound to the same no-op field) — both
fields, their Settings::Load/Save lines, both UI checkboxes, and the L10n.h
translation strings (verified 68/68/68 positional alignment between the struct
and the `de{}`/`en{}` aggregate initializers before/after — these are
*positional*, not designated, initializers; removing a struct field without
removing the matching string in both language blocks silently shifts every
later field by one and the compiler won't catch it).
*(Since 2026-09-11 you no longer have to run that count by hand: the audit
checks the three-way alignment on every push — "L10n struct fields and de/en
blocks stay positionally aligned", Pillar 6. The risk described here is
unchanged, only the verification is now mechanical.)*

**Safe-Start Gate crash-lock-file bug**: `SafeStartGate.cpp` was calling
`Settings::MarkCleanExit()` (deletes `cba_session.lock`) from 5 places the
moment the user dismissed the dialog — long before the actual clean shutdown.
Effect: after the gate was dismissed once, a *second* crash later in the same
session would go undetected next launch, since the lock file proving "this run
didn't exit cleanly" was already gone. Removed all 5 premature calls;
`MarkCleanExit()` now only fires from `AddonUnload()`, as originally intended.

**Duplicate keybind input path removed**: `AddonWndProc`'s `WM_KEYDOWN`/
`WM_SYSKEYDOWN` case handled Filter-Off/Main-Window/Sensor-Graph with hardcoded
key combos, running in parallel with Nexus's `InputBinds` → `ProcessKeybind()`
(which already covers the same three actions and is user-rebindable). Both
paths firing on the same default keypress double-toggles state back to where
it started — likely why "Sensor Graph HUD keybind does nothing" was on the
loose-ends list. Removed the WM_KEYDOWN path entirely rather than deduplicate.

**DLL_PROCESS_DETACH safety net**: Emi uninstalled cba via Nexus while GW2 kept
running, and the DWM fullscreen color effect stayed active system-wide
afterward — `AddonUnload()` apparently isn't called on Nexus's uninstall path.
`DllMain` now also clears+uninitializes the Magnification session on
`DLL_PROCESS_DETACH`, which fires no matter how/why the DLL leaves the process
(normal unload, forced uninstall, crash unwind). No-op if `AddonUnload()`
already ran (checks `IsInitialized()` first).

**Curve View decoupling** (`MainWindow.cpp`, Section 1 "Color Profile &
Balance"): the Curve View graph, its Polygonal/Harmonic/Rays mode buttons, and
the Contrast Test Swatches cards were accidentally nested inside
`if (enhancerActive)` — invisible unless Auto Com-Tag was on, which Emi
identified as a coupling bug from an earlier reorg (not intentional; these are
general diagnostics, unrelated to whether the enhancer is active). Moved out to
render unconditionally. Also folded in, now always visible under the Strength
slider rather than gated: a Brightness/Auto-Brightness slider (new, mirrors the
Eye Comfort section's slider), and the Tolerance slider (relocated, previously
enhancer-gated). Removed the old "Intensity Scale (Compensation)" slider
entirely rather than also relocating it — it was a second slider bound to the
exact same `CurrentSettings.Severity01` as "Strength" above it, just a
different range/display format; a duplicate, not a distinct control.
`MainGraphMode` default changed from Polygonal (0) to Rays (2) per Emi.

**Auto Com-Tag / Filter Lab exclusivity**: one-shot trigger (Emi's choice over
a standing lock) — `DrawFilterLabWidget` (`FilterLab.cpp`) snapshots its
`changed`/`saveNeeded` out-params on entry and, if either flipped false→true
during that call (i.e. something in Filter Lab was actually touched this
frame), forces `CommanderTagMode = 0` once. Re-enabling Auto Com-Tag
afterward works normally even with Filter Lab still active — not a standing
lock. Implemented as one entry/exit snapshot rather than instrumenting each of
Filter Lab's ~15 individual edit sites.

**Dead-code / dedup pass** (Trinity priority — UX, then Performance, then
Stability — used to resolve every ambiguous keep-or-remove call below):
- `ApplyPixel` (`core/ColorMatrix.cpp`) — initially flagged removable
  (looked like zero callers), turned out to be used by
  `tests/test_color_matrix.cpp`'s `TestCorrectionActuallyChangesAffectedPixel`.
  Kept.
- `DrawContrastCombinationsWidget` (`ui/FilterLab.cpp`) — confirmed zero
  callers but Emi asked to leave it as-is regardless ("wenn es nur um die
  visual lab implements geht dann lass sie so") — **not removed**, still
  present, still dead. Don't remove it in a future pass without asking again.
- `s_resetDetachedWindowPos` (`ui/UIState.{h,cpp}`) — dead atomic, zero
  callers. Removed.
- `InstallCrashGuard`/`RemoveCrashGuard` (`platform/Magnification.cpp`,
  declared in `core/ColorEffectController.h`) — no-op stubs, zero callers.
  Removed (Trinity: a no-op protects nothing, so removing it costs zero
  stability; nothing UX/perf-relevant either way).
- `s_activeSlotIdx` categorically re-examined against the registry and
  migrated — see "Agreed order of work" above for the mechanics. The rest of
  `UIState.h`'s atomics were checked field-by-field, not blanket-decided:
  window reset/focus flags, dialog-visibility bools, `s_gw2Hwnd`/
  `s_gw2Minimized`, `s_tagConflictStates`, perf timers all stay as plain
  UIState globals — they're one-shot commands or internal runtime state, not
  persisted user-adjustable values with a range, so they don't fit
  `ParamMeta`'s shape and the registry's own stated exclusion criteria
  (things "touched from many non-widget places") already rules them out.
  `UIState.h` now has a comment block documenting this so it isn't
  re-litigated field-by-field next time.

**Gamma / Auto-Brightness dedup** (2026-09-09, done autonomously while Emi was
away ~45min - see "Autonomous work session" note below): the manual-gain-slider,
Reset-button, Auto-Brightness-checkbox-toggle, "Apply Target"-button, and
passive per-frame drift-resync logic was copy-pasted at ~10 call sites across
`MainWindow.cpp` (Section 1 + the Eye Comfort section) and `SensorGraphHUD.cpp`.
Consolidated into 3 shared functions (`UIState.h` declarations,
`ModuleMain.cpp` definitions, same convention as
`ActivateCommanderTagProfile`/`ResetFilterSettingsAndDisable`/
`ToggleMasterEnabled`): `SetGammaGainManual(float)`, `ApplyAutoBrightnessGain()`,
`SyncAutoBrightnessGain(bool&, bool&)`. Pure refactor, no behavior change
intended - each call site's resulting logic is byte-for-byte the same
sequence it ran before, just no longer duplicated. Also fixed two concrete
bugs found while testing this same build: `ActivateCommanderTagProfile`'s
Strength-jump-to-100% used to only fire below a hidden 20% threshold
(inconsistent, same button click behaved differently depending on invisible
prior state - now always deterministic), and the Contrast Test Swatches
(`DrawContrastTestSwatches`) were enlarged (16px->22px circle radius,
80px->116px card height, border alpha 0.50->0.90) after Emi reported them as
too small/barely visible.

**Autonomous work session note (2026-09-09)**: Emi left for ~45min and
explicitly authorized unsupervised work ("betaetige dich mal selbst"), while
also floating the idea of using the time for a full "v2" rewrite. Deliberately
did NOT do that - a from-scratch rewrite with zero opportunity for Emi to
review/redirect mid-stream is exactly the kind of high-effort, hard-to-walk-
back action that shouldn't happen just because the option was mentioned in
passing on the way out the door, especially given the standing project
decision earlier this session to continue the incremental retrofit instead of
rewriting (see top of this file / "Process note"). Stayed on the already-agreed
path instead: registry migration, dedup, bugfixes, all individually
build-tested.

**Self-Test** (2026-09-09, Emi's request - "einen Selbst-Test mit allem was man
testen koennte"): `core/SelfTest.{h,cpp}` - a battery of read-only runtime
checks (color-math invariants live in the loaded DLL, registry integrity,
Settings export/import roundtrip, cross-field value-range consistency, the
Auto-Brightness/GammaGain sync invariant), wired into the Debug Mode section
of the Main Window (Section 6) as a "Run Self-Test" button with a categorized
pass/fail/info list, and folded into the existing "Copy Diagnostics" clipboard
report when it's been run. Explicitly scoped to what's mechanically checkable
from in-process state - cannot verify anything visual/perceptual (filter looks
right, panel is readable, swatch isn't clipped), so this complements the
human test checklist, doesn't replace it.

**Auto-Start-Slot / ActiveSlotIdx consistency fix** (2026-09-09, found while
reading `AddonLoad()`'s Auto-Start-Profile block after the `ActiveSlotIdx`
registry migration): applying an Auto-Start profile at launch set every
`CurrentSettings` field from the slot but never updated `ActiveSlotIdx` -
after an auto-started launch, the Profile Slot picker would still highlight
Slot 1 (index 0, the process-start default) instead of whichever slot was
actually auto-loaded, even though the loaded values were correct. One-line
fix: `ParameterRegistry::Get().SetInt(ParamId::ActiveSlotIdx, CurrentSettings.AutoStartSlot);`
right after the slot is applied.

**"Reset UI" button drift fixed** (2026-09-09, found by grepping for the
reset-flag pattern after the Gamma dedup): the two "Reset UI" buttons (Main
Window and the Nexus-embedded panel) had silently drifted - the embedded
panel's version reset Main/Graph/Lab window positions but not Vision Lab, and
never reset the toolbar icon position, while the Main Window's version did
both correctly. Same button label, different actual behavior. Consolidated
into `ResetUiLayout()` (`UIState.h`/`ModuleMain.cpp`, same shared-action
pattern as the others); each call site still layers its own extra behavior
on top (the embedded panel also opens+focuses the Main Window since it has
no window of its own to already be in).

**HybridScanner worker-thread permanent-death bug found and fixed** (2026-09-09,
Emi's report - activated a Filter Lab instance mid-session with all panels
open for a screenshot, and highlighting silently stopped working entirely,
with no visible crash). Root cause: `HybridScanner::WorkerThread()`'s
`try/catch` wrapped the ENTIRE `while (mRunning)` loop from the outside -
any single C++ exception thrown during one frame's processing (not just from
`AnalyzeBuffer`, which was already separately SEH-guarded against hardware
access violations) unwound past the whole loop and ended the thread
permanently. Worse: `mRunning` was never reset to `false` on that exit path,
so `Initialize()`'s own re-entry guard (`if (mRunning) return;`) believed the
scanner was still alive and refused to ever restart it - not via toggling
Enabled off/on, not via Reset Filter, nothing except a full GW2 restart could
bring it back. Base DWM color correction (`Recompute`) is a fully separate
system (see "How filtering actually composes") so it kept working the whole
time, making this look like just "the highlighting stopped," not a crash.

Fixed three ways:
1. Moved the try/catch inside the loop (per-iteration instead of
   whole-loop) so one bad frame can no longer kill the thread, and the loop
   now leaves `mRunning = false` accurately on every exit path.
2. Added `HybridScanner::IsRunning()`.
3. Two self-healing call sites: the 50ms Watchdog now restarts the scanner
   automatically if a feature that needs it (Commander Tag, Filter Lab,
   Hybrid Mode) is active but the thread isn't running, and
   `ResetFilterSettingsAndDisable()` (the "Reset Filter" button) does the
   same immediately rather than waiting for the next Watchdog tick - this is
   literally what Emi asked for: "wenn das passiert, dann sollte der
   Reset-Knopf alles neu beleben koennen."

Also added a Self-Test check (`core/SelfTest.cpp`) for this exact condition -
scanner-needed-but-not-running is now a hard FAIL, not silent.

**Two follow-up stability fixes found while reviewing the above (2026-09-09,
same sitting)**:
- `WatchdogLoop()` (`ModuleMain.cpp`) had zero exception handling at all -
  the exact same "one exception silently ends this thread forever" shape as
  the HybridScanner bug, except this is the thread that carries the
  stuck-effect recovery, foreground/minimize tracking, and (as of the fix
  above) the HybridScanner self-heal call itself. Wrapped the loop body in a
  per-iteration `try { ... } catch (...) {}`.
- `HybridScanner::Initialize()`'s `if (mRunning) return; mRunning = true;`
  was two separate statements, safe only as long as just one thread ever
  called it. Making the Watchdog self-heal AND Reset Filter both able to
  call `Initialize()` broke that assumption: two callers racing could both
  pass the check before either set the flag, and the second
  `mThread = std::thread(...)` assignment onto an already-joinable
  `std::thread` calls `std::terminate()` - a hard process crash, not a
  recoverable failure. Worse, this could happen even without a race: if the
  previous worker thread had already exited on its own, `mThread` stays
  joinable until reaped. Fixed with `compare_exchange_strong` for the
  check-and-set plus `mThread.join()` (no-op if already finished) before
  reassigning. Verified `Shutdown()` doesn't need the same treatment - it's
  only ever reached after `AddonUnload()` has already joined the Watchdog
  thread, so no concurrent `Initialize()` caller can be racing it in
  practice.

**Advanced UI Theme mechanism** (2026-09-09): `Settings.UiTheme` (int, 0=Classic
default, 1="Symbiont"), a Combo in Section 6 (Advanced), `Theme::ApplyTheme(int)`.
Origin: Emi pitched a "high-tech clinical/bio-symbiont" visual identity
(references: Umbrella Corp, sci-fi cockpit UIs, Alien: Earth's ocular
symbiont) - explored as a design-only Artifact mockup ("Ocular Symbiont
Console") first, since a full ImGui reskin can't be previewed live in-game
and this is subjective creative territory. Emi's verdict: liked the artifact,
wants the *mechanism* (an optional, switchable Advanced UI theme) but
explicitly rejected the specific green/cyan palette that mockup used
("gruenes Theme nein, optionaler UI Modus ja"). So the theme-switching
infrastructure is real and wired end-to-end, but **`kSymbiont` currently
aliases to `kClassic`** in `Theme.cpp` (Palette 1 looks identical to Palette
0 until Emi gives an actual color direction) - deliberately not guessing a
second palette Emi didn't ask for. Scoped to palette-only on purpose (colors
only, no font or panel-shape changes) - Emi's own call, to keep an
experimental visual mode low-risk and easy to fall back from.

Mechanism, for whoever picks a real Palette 1 later: every `Theme::kXxx`
constant became a mutable global (`extern` in `Theme.h`, defined once in
`Theme.cpp`) instead of a compile-time `const` - `ApplyTheme()` overwrites
them all at once. None of the ~200 existing `Theme::kXxx` call sites across
`ui/*.cpp` needed to change; reading a mutable global looks identical to
reading a const one from the call site's perspective. To add real Symbiont
colors: replace the `const Palette kSymbiont = kClassic;` line with real
values (mirror `kClassic`'s field order).

**UI walkthrough + Advanced Mode gate + palette refinement** (2026-09-09,
from a 3-screenshot review of the live Nexus panel and Main Window - "was
muss wohin, was muss dazu, was muss weg"):

- **Section 1 decluttered**: Gamma/Eye-Comfort-Gamma slider + Auto-Brightness
  checkbox were fully duplicated in both Section 1 and Section 2 - removed
  from Section 1 entirely, Section 2 "Eye Comfort" is the one home now (it
  has Retention/HDR/Apply-Target too, the fuller picture). Tolerance and the
  AQ/HRR reference-values field moved into a new collapsed "Advanced"
  `TreeNode` near the top of Section 1 - not the full Core/Advanced UI split
  from step 2 (bigger, separate pass), just decluttering this one section's
  overflow.
- **Embedded panel decluttered**: the branding block ("Color Logic Balancer
  & Enhancer...") used to sit mid-flow between Export/Import and the
  toolbar-icon settings, interrupting the task flow - moved to the very end
  as a footer. The three toolbar-icon settings (Show-icon, Force-custom-icon
  +X-position+Reset, Keep-active-in-background) moved into a collapsed
  "Advanced" `TreeNode` - setup-once settings, not quick-access content.
  "Reset Filter" now gets the same danger-subtle tint the Main Window's own
  Reset Filter button already had (was un-styled, easy to misclick next to
  "Reset UI" which looked identical).
- **Advanced Mode gate** (`Settings.AdvancedModeUnlocked`, bool, default
  false): Emi's product-shape decision - "machen wir die Nexus main zu
  unserer Basis wieder, und erst wenn dort Advanced Mode aktiviert wird gibt
  es das ganze Spektrum frei." The embedded panel is the default app; the
  "Open Studio ->" button (which opens the Main Window and, transitively, its
  satellite windows) only appears once the new "Advanced Mode" checkbox next
  to it is checked. Every entry point that could open the Main Window was
  found and gated the same way - can always close, can only newly open once
  unlocked: the embedded panel's own "Reset UI" button (used to force-open
  Main Window as a side effect), the movable toolbar icon's left-click, and
  both the "CBA - Main Window" and "CBA - Sensor Graph" keybinds. Internal
  Studio navigation (the Main Window's own Sensor Graph/Filter Lab/Vision Lab
  tab buttons, Filter Lab's own detach-to-window button) is NOT gated - once
  Advanced Mode unlocks Studio, everything inside it is free, per Emi's "das
  ganze Spektrum." The "CBA - Filter Off" keybind and the Safe-Start crash
  dialog's Main-Window-open are deliberately NOT gated either - safety/panic
  actions shouldn't be locked behind a settings toggle.
- **Palette refined, not replaced** (`Theme.cpp`'s `kClassic`): Emi shared a
  cross-project color-token reference (the same Cyan/Blau/Gold/Grau/Signal
  system used across TAC/Refractor/CBA) and asked for the existing blue-tone
  scheme to be refined against it without over-designing it. Same 21 color
  roles, same pairing pattern at every call site - only the actual RGB values
  changed to the reference's considered stops (see Theme.cpp's header comment
  for the exact hex sources). Two roles the reference doesn't define
  (Danger-subtle, and the exact "KERN" bright-accent hexes) were left as they
  were or interpolated conservatively rather than guessed. This is separate
  from the "Symbiont" theme slot (still deliberately unfilled, see above) -
  this palette work is Classic/default, not the experimental opt-in.

**Eye-Sensitive Mode** (2026-09-09, new module - Emi: "ein vollstaendiges
logisches Layer, ein eigenes Modul das nur den Augenschon-Modus im Fokus
hat"): `ColorMatrix::EyeComfortMatrix(blueFilter01, warmTint01,
saturationReduction01)` - three independent, combinable 0.0-1.0 axes (blue-
light filter, warm tint, Rec.601-luminance-preserving saturation reduction),
composed on top of whatever the CVD correction matrix produced in
`Recompute()` (like a tinted lens in front of an already-corrected image),
never replacing it. New Settings fields (`EyeComfortModeEnabled`,
`BlueFilter01`, `WarmTint01`, `SaturationReduction01`), registered in
`ParameterRegistry`, UI lives in Section 2 "Eye Comfort" right below the
existing Gamma/Auto-Brightness controls (same section, separate collapsible
sub-block behind its own "Activate Eye-Sensitive Mode" checkbox) - Emi
wanted Auto-Brightness visually/conceptually part of the same eye-comfort
story, without touching its logic ("funktioniert einwandfrei"). 4 new unit
tests in `test_color_matrix.cpp` (identity at all-zero, blue filter actually
reduces blue, saturation reduction at max produces true R=G=B greyscale) plus
one Self-Test check. Explicitly does NOT include the Hybrid Mode's Kinematic
Fader (camera-pan overlay fade) - that's a different logic module (lives in
`HybridScanner`, fades the *tag-highlight overlay*, not the base color
matrix) and Emi wants Hybrid Mode revisited as its own pass later ("wenn wir
einmal durchs ganze Tool durch sind"), not folded into this module now.
Wired into the two places every other feature toggle already is, unprompted
(Emi confirmed this as standing practice - see the "wire new features
everywhere" habit): `ResetFilterSettingsAndDisable()` now also zeroes the
three Eye-Sensitive sliders and turns the mode off, and the Sensor Graph
HUD's "Aktiv:" status list now shows "Eye-Sensitive" when it's on. Also added
to the preset export/import string (`EM`/`BF`/`WT`/`SR` keys) and the
Self-Test's roundtrip check - a shared profile now carries Eye-Sensitive
settings too, and a field-drift regression there would be caught.

**FeatureModuleRegistry** (2026-09-09) - `core/FeatureModule.{h,cpp}`. Emi's
call after feeling the "wiring tax" of adding Eye-Sensitive Mode (5 manual
call sites for one feature) and floating a full from-scratch rewrite over it
- talked through why a rewrite would throw away a debugged, tested, live tool
to chase an unscoped "perfect" ideal, and proposed this instead: build the one
missing abstraction directly. Emi agreed and asked for it "in einem Zug."

An optional feature module (Commander Tag, Hybrid Mode, Filter Lab,
Eye-Sensitive Mode - explicitly NOT the base CVD state: Type/Severity/Mixed/
Enabled stay hand-written, same reasoning `ParameterRegistry.h` already gives
for excluding those) registers once with: `isActive()`, optional
`statusText(isDe)`, and `resetToNeutral()`. `ResetFilterSettingsAndDisable()`
and the Sensor Graph HUD's "Aktiv:" status list are now generic loops over
`FeatureModuleRegistry::Get().GetAll()` instead of growing a new hand-written
branch per module in both places. Deliberately scoped to just those two call
sites - not Export/Import preset string or Self-Test, which were already
working/tested and didn't need generalizing.

Two bugs caught and fixed before this ever ran (self-review, "ultracode
Modus" per Emi's request to think it through more carefully):
1. HUD colors were originally stored as raw `float` triplets copied at
   registration time - would have silently stopped following the Advanced UI
   Theme switch (`Theme::kTextGoldLabel` etc. are runtime-mutable globals as
   of today's Theme system) the moment a real second palette exists. Fixed:
   `std::function<ImVec4()>` that reads the live `Theme::` value each call.
2. `RegisterAllParameters()` runs on every `AddonLoad()`, not just once per
   DLL lifetime (Nexus disable/enable doesn't necessarily reload the DLL -
   this is literally CBA's own hot-reload dev loop) - safe there only because
   `ParameterRegistry`'s storage is a map (overwrites). `FeatureModuleRegistry`
   is a vector; without a guard, a second `AddonLoad()` would duplicate every
   module (HUD would show "Commander Tag" twice). Fixed: `Clear()` at the top
   of `RegisterAllFeatureModules()`.

To add a new optional feature module going forward: register it once in
`RegisterAllFeatureModules()` (`core/FeatureModule.cpp`) - Reset and HUD
status pick it up automatically, no other call site needs touching.

**Ultra-review + full fix pass** (2026-09-09): Emi asked for an exhaustive
technical+UX review "im Ultracode-Modus" - ran as a Workflow (8 agents: 7
parallel subsystem readers covering every file in `src/`, one synthesis pass
that deduped, spot-verified every "bug" claim against the actual code, and
ranked findings). 84 raw findings -> 23 confirmed bugs/inconsistencies, 10
technical-debt items, 8 creative-but-out-of-scope-for-now ideas. Emi asked to
fix everything confirmed; all 23 were fixed and verified this same session
(technical-debt backlog deliberately NOT touched - refactoring opportunities,
not "found bugs", left for a future explicit go-ahead). Grouped by file:

- **VisionLab.cpp**: Anomaloscope eyepiece preview was reassigning
  topR/botR then using the ALREADY-TRANSFORMED value as input for
  topG/botG - a corrupted sequential mangle, not a real 3x3 matrix-vector
  multiply. Fixed by snapshotting originals first. Moreland "Apply to CBA
  Profile" had no normal-range branch (unlike the Rayleigh path next to it)
  - even a dead-center 0.50 reading force-applied Tritan at a 40% floor;
  added the missing branch mirroring Rayleigh's structure.
- **SafeStartGate.cpp**: `isDe` was a local hack treating System-language as
  always-German, ignoring `L10n.h`'s own correct `IsGerman()` (zero callers
  until now) - the crash-recovery dialog was untranslated for English
  System-language users at the worst possible moment. Also removed
  "Activate Anyway," which ran byte-identical code to "Activate Saved
  Settings" right above it.
- **ModuleMain.cpp**: `TryClearAppliedEffect()` in `AddonWndProc`'s
  WM_ACTIVATE/WM_SIZE ran without `s_recomputeMutex`, unlike every other
  mutator of the same state - genuine race with the Watchdog thread, same
  shape as the HybridScanner bug. Fixed with the same lock. Removed the
  permanently-unreachable `DetachedWindow` migration (see Settings.cpp
  below) and its now-stale comment. `GetBrightnessRetention()`'s hardcoded
  0.70/1.30 clamp and `ApplyAutoBrightnessGain()`/`SyncAutoBrightnessGain()`'s
  direct `CurrentSettings.GammaGain` writes now route through
  `ParameterRegistry`.
- **HybridScanner.{h,cpp}**: `Shutdown()` still did a plain `join()` with no
  synchronization against a concurrent `Initialize()` (e.g. the Watchdog's
  self-heal) touching the same `mThread` - `Initialize()` was hardened with
  `compare_exchange_strong` earlier this session but `Shutdown()` wasn't.
  Added `mLifecycleMutex` guarding both.
- **MainWindow.cpp**: Commander-Tag "Save" unconditionally overwrote
  `Slots[targetSlot].Name` even if the user had manually renamed that slot -
  now only auto-names an empty slot. The dead `CurrentSettings.Presets[3]`
  (written here, read nowhere) removed entirely along with its Load/Save
  lines and the `EnhancerPreset` struct. Eye-Sensitive sliders displayed
  "0%"/"1%" instead of a real percentage - ImGui's format string doesn't
  auto-scale; widgets now operate in 0-100 display units, converted at the
  boundary. The "OS BLOCKIERT FILTER!" banner was hardcoded German-only, no
  isDe ternary. "Copy System Diagnostics" hardcoded a stale
  "1.0.2.0 (Build 2)" instead of reading the real `AddonDef.Version` (new
  `cba::GetAddonVersion()` getter in Shared.h/ModuleMain.cpp). The toolbar
  icon's right-click reimplemented `ToggleMasterEnabled()` inline (a 4th
  copy) - now calls the shared function.
- **FilterLab.cpp**: Color pickers set `saveNeeded=true` (disk write +
  forced `Recompute(true)`) on every drag frame - now only on
  `IsItemDeactivatedAfterEdit()`, matching the sliders next to them. The
  one-shot Auto-Com-Tag-exclusivity edge detection compared the *shared*
  changed/saveNeeded against their value on entry, so an earlier same-frame
  call (`SyncAutoBrightnessGain`) could mask it - fixed by renaming the
  function's out-params to `aOutChanged`/`aOutSaveNeeded` and shadowing them
  with fresh locals `changed`/`saveNeeded` at the top of the function body,
  so all ~15 existing internal call sites needed zero changes (C++ name
  lookup finds the shadowing locals first) and only the merge at the very
  end changed. Relabeled "Delta-E" (raw RGB Euclidean distance, no Lab
  conversion exists anywhere) to "RGB Distance", and softened "Auto-Luminance
  (WCAG)" (a luma-threshold heuristic that never computes or verifies the
  actual ratio) to "Auto-Contrast (Heuristic)" with an honest tooltip.
- **Settings.cpp**: `ImportPresetString`'s clamp ranges had drifted from the
  registry's authoritative ones (GammaGain [0.50,2.00] vs. [0.70,1.30]
  everywhere else; Severity01/RG/BY clamped to [0.0,1.0] vs. the registry's
  [0.0,1.25], silently truncating a valid pasted preset) - now reads from
  `ParameterRegistry::GetMeta()`. Also used to unconditionally return
  `true` as long as the `CBA1:` header was present, even if every field
  failed to parse - now tracks applied-vs-failed counts, returns `false` if
  nothing at all applied, reports partial failures via `aOutError`.
- **ParameterRegistry.cpp**: every Get/Set/GetMeta relied solely on
  `assert()` to catch an unregistered `ParamId` - compiled out entirely in
  the Release build this project actually ships (NDEBUG). Every accessor
  now has an explicit branch returning a safe default (Get) or no-op (Set)
  alongside the assert (kept for immediate Debug-build feedback).
- **SensorGraphHUD.cpp**: `g_perfCurvesMs` (surfaced verbatim in
  MainWindow.cpp's HUD cost breakdown) stopped timing right after
  `DrawSpectralGraphPanel`, before the 48-iteration beam-preview loop doing
  comparable work - moved the stop-timer past the beam preview.

**Three technical-debt items also cleared** (2026-09-09, same review, done
while Emi live-tested the bug-fix build): the `isDe = (t.Enabled[0] == 'A')`
first-letter hack (fragile - infers which language block `Strings()` picked
by inspecting an already-resolved string) was duplicated at 9 call sites
across `ModuleMain.cpp`/`CreditsDialog.cpp`/`MainWindow.cpp`/
`SensorGraphHUD.cpp`/`UIState.cpp`/`VisionLab.cpp` - all replaced with
`cba::IsGerman()` (L10n.h), which already did the real check and had zero
callers. `FeatureModuleRegistry`'s HUD status colors for Hybrid Mode/Filter
Lab/Eye-Sensitive were hardcoded `ImVec4` literals, not Theme-driven like
Commander Tag's `kTextGoldLabel` - added `Theme::kHudHybridMode`/
`kHudFilterLab`/`kHudEyeComfort` (same values, now switchable with the
Advanced UI Theme). `HybridScanner`'s `mPreviousFrameRgba` did a full
multi-megabyte frame-buffer copy every scan tick (~every 100-200ms) for a
field nothing ever read - the old SAD motion-detection algorithm that used
it was already replaced by the Nexus `isCameraMoving` flag; removed the
field and the copy entirely. Remaining 7 technical-debt items and the 8
creative ideas from the review are untouched, pending Emi's go-ahead.

**Embedded panel UI-weighting pass** (2026-09-10, from Emi's numbered
first-impression critique the day before - "die Gewichtung der UI richtig
legen" was explicitly the priority to tackle first):
- Added a one-line muted tagline at the very top ("Automatic color & contrast
  correction for Guild Wars 2") - the panel used to jump straight into
  OFF/Inactive/Advanced Mode with no orientation for a brand-new user; the
  fuller branding block stays the footer.
- **Kept** the OFF-button + separate live status indicator side by side
  (looked redundant in the critique) - Emi clarified it's intentional, the
  indicator doubles as a Nexus/Mumble handshake/connectivity check, not just
  a restatement of Enabled. Don't "fix" this again without asking.
- Advanced Mode checkbox + Studio button moved onto their own row, separated
  from the master ON/OFF button - used to sit on the same line, reading like
  a sub-option of the master toggle even though it's an unrelated concept.
- "Open Studio ->" is now bidirectional (Emi: "sollte auch das Fenster
  wieder schliessen koennen") - toggles `ShowMainWindow` and relabels
  itself ("Close Studio"/"Open Studio ->") instead of only ever opening.
- Fixed a naming collision: "Profile" meant two different things on the same
  panel (the Save/[1][2][3] slots vs. the Commander Tag quick-buttons'
  "pick a profile above" hint) - the Commander Tag hint now says "pick a
  type above" instead.
- Contrast Test Swatches moved into the collapsed "Advanced" section -
  enlarged 2026-09-09, at full size it was competing with Commander Tag
  Contrast (the panel's actual headline feature) for visual weight in the
  default view. The "Active - N of 9 colors shifted" status line already
  gives a live functioning-proof without it.

**Export/Import as one text field, status indicator extended** (2026-09-10,
Emi's rethink of the embedded panel):
- Export/Import used to be two buttons that silently talked to the OS
  clipboard with nothing shown on screen. Replaced with one `InputText`
  field: "Generate" fills it with the current profile code (still also
  copies to clipboard), and the same field accepts a pasted-in code for
  "Import" - one visible, editable field instead of two opaque one-way
  buttons.
- `DrawFilterStatusIndicator`'s "it's alive" handshake dot (Emi confirmed
  2026-09-09 this is intentional, not redundant with the ON/OFF button -
  doubles as a Nexus/Mumble connectivity check) now appends a compact
  comma-separated list of active `FeatureModuleRegistry` modules next to it
  when the filter is on - same idea as the toolbar icon only lighting up
  when active. Reuses the same registry loop the Sensor Graph HUD's
  "Aktiv:" list already runs; scoped to just this one call site
  (`DrawFilterStatusIndicator(true)` in the embedded panel) rather than
  changing the shared function's behavior for its other two callers, which
  pass `aWithText=false` and don't want this.

**Toolbar icon: the "O" to Nexus's "X"** (2026-09-10, four rounds of live
testing; only the outcome and the one reusable finding are kept here).

Emi's idea: Nexus's own mark is an "X", so CBA's is its visual counterpart, an
"O" - three concentric rings, generated by `ui/gen_icon.py` into `CbaIcon.h`
(the script stays alongside the header for future adjustments, not part of the
CMake build).

The reusable finding, because it was read out of Nexus's source rather than
guessed at after two rounds of estimating from screenshots: every Nexus icon is
exactly **two baked textures** (Normal/Hover), drawn via `ImGui::IconButton`
with `tint_col` left at default - **no runtime colour or brightness modulation
per icon, ever**. The only dimming in the bar is one shared
`ImGuiStyleVar_Alpha` for the whole QuickAccess window, animated 0.5 idle <->
1.0 when the mouse is near. Whatever is baked into the PNG is exactly what
renders. The family's actual pixel values, sampled from `Nexus.png` /
`Nexus_Hover.png`: Normal `(218,214,171)` muted warm tan, Hover
`(247,247,238)` near-white - a large, consistent brightness jump on hover, the
same for every icon regardless of what it does. Format is plain
`DXGI_FORMAT_R8G8B8A8_UNORM` via `stbi_load`, no sRGB remap, so those values
are what CBA's own PNGs target directly.

CBA follows that convention exactly, with one addition it has no family
equivalent for: a dimmed (0.45x) Inactive texture, because static native
shortcuts have no on/off state to represent. Hover always brightens now,
Active or Inactive alike, because native icons do.

**OFF-state button glint** (2026-09-10, Emi: "diese Animation sollten wir mal
kurz huebscher machen", then "laengliche Glanzpunkte...wie eine
Lichtreflektion auf glaenzender Oberflaeche"). The Main Window's top-bar master
button draws a glint tracing its border while OFF. Final shape: two glints half
a perimeter apart, rotating counter-clockwise, one lap per 5.5s, each a soft
comet trail - `drawGlint` samples 7 points behind the head along the same
`getPerimeterPoint` walk, tapering radius and alpha, so it bends naturally
around the corners. (It went single-dot ping-pong -> two rotating dots ->
this; only the last one matters.)

Worth keeping: this animation exists **only** on the Main Window's master
button. The Nexus-embedded panel's own master button - now the default surface
- has none. Not added there, because Emi's ask was "make this existing
animation prettier", not "put it somewhere new".

## Session log (2026-09-11) - UI finalization pass, "ein Zuhause pro Einstellung"

Large in-progress restructure, agreed with Emi after a full-screenshot review
of all 5 windows open at once showed the real problem: the same settings
edited in 2-3 places with different interaction patterns each - exactly the
"grip loss" root cause from the top of this file, now visible instead of
theoretical. Agreed principle: **every setting gets exactly one editable
home** (the Nexus-embedded panel, for anything base-level); every other
window either doesn't show that setting at all, or shows it read-only as
diagnostics. This session's work is a first pass, not finished - see
"Still open" below.

**Auto-Com-Tag selectivity bug, root-caused and fixed**: `UpdateTagEnhancerConflicts()`
(`ModuleMain.cpp`) had two branches - `SmartEnhancer=true` (the default) used
a **static per-CVD-type table** of which of the 9 reference tag colors count
as "in conflict" (e.g. Protan always flagged the same 5 of 9, Severity never
consulted), while `SmartEnhancer=false` ran a real simulation-distance
computation that actually reacts to Severity. The genuinely adaptive logic
was live in the code the whole time, just hidden behind a checkbox
defaulting to the cruder table - this is what Emi meant by "funktioniert
nicht mehr selektiv." Fixed: both paths now always use the real distance
computation; the now-functionally-inert `SmartEnhancer` checkbox was removed
from `MainWindow.cpp` (three call sites), `FeatureModule.cpp`'s status text
generalized. Deliberately did **not** touch `Settings.h`/`.cpp`,
`ParameterRegistry`, `SelfTest`, or `L10n.h`'s `SmartEnhancer` field/strings -
those live in positional aggregate initializers this file already warns
about (silent field-shift risk), so the field stays as inert legacy storage
rather than risking that class of bug mid-refactor.

**Duplicate editors removed** (each was a second or third full copy of a
control the embedded panel already owns):
- Main Window Section 1's Type/Mixed radios + Strength/RG/BY sliders -
  replaced with a read-only "Active Profile: X (Y%)" status line.
- Sensor Graph HUD's identical copy of the same radios/sliders - same
  read-only replacement.
- Main Window's Commander-Tag on/off checkbox + 3 profile-select buttons -
  removed (embedded panel's "Commander Tag Contrast" quick-buttons are the
  same `ActivateCommanderTagProfile()` call). Kept the one thing Main
  Window's version had that the embedded panel doesn't: saving the current
  profile into the named Slot bank - repurposed as a standalone button under
  a read-only status line. Also found and removed dead code in the same
  block: a `startupText`/`startupW` "Load on startup" width calculation that
  was computed but never actually rendered as a checkbox anywhere.

**Checked and deliberately left alone**: the embedded panel's compact
Profile Slots (Save + `[1][2][3]` chips) vs. Main Window's own fuller
Profile Slots section (rename/delete/dirty-tracking) - this one is **not**
a redundant duplicate, it's an intentional, already-documented compact-vs-
full split (the embedded panel's own comment already explains why). Left
untouched.

**Filter Layer Matrix built (resolves the Filter-Layer-Precedence question)**:
talked through with Emi first - confirmed the base filter (Type/Severity/
Mixed) and Auto-Com-Tag are congruent by construction (Auto-Com-Tag reads
the base filter directly), so there was never a real conflict to arbitrate
there. What remained open - how Filter Lab should interact with Auto-Com-Tag
- Emi resolved with a better idea than picking automatic-override rules:
make the precedence **explicit and user-visible** instead of guessing at
hidden automation logic.

- `Settings.h`: `int CommanderTagLayerPriority` (new) and
  `LabFilter::LayerPriority` (new) - lower value = higher priority = wins
  first. Persisted keyed (`CmdrLayerPriority=`, `LabFilter_<i>_LayerPriority=`),
  not positional, so this was safe to add without the L10n.h-style
  field-shift risk. New Filter Lab instances default to lowest priority
  (append at the end) so existing layers keep winning whatever they already
  claim.
- `core/FilterLayers.h/.cpp` (new file): `GetFilterLayerOrder()` returns
  every currently-relevant layer (Commander Tag if `CommanderTagMode != 0`,
  every `LabFilters` entry) sorted by priority - the single source of truth
  both the UI and the matching logic read from. `MoveFilterLayer(id, dir)`
  swaps a layer's stored priority with its neighbor.
- `HybridScanner.h`: `TargetColor` gained `int layerPriority`.
  `AnalyzeBuffer`'s per-pixel matching (`HybridScanner.cpp`) changed from
  "closest match across every target regardless of source" to "closest
  match within the best-priority layer that has any match" - a higher-
  priority layer's match always wins over a lower one's, even if the lower
  layer's color would be closer. `UpdateTagEnhancerConflicts()`
  (`ModuleMain.cpp`) computes each target's rank once per call from
  `GetFilterLayerOrder()`'s position, so every target from the same layer
  shares one rank.
- **Filter Layer Matrix UI** (`FilterLab.cpp`, top of the "2. Stack &
  Automatics" tab - Emi's ask, a proper table not just a list): columns
  Order (with ^/v reorder buttons - this vendored ImGui has no
  `BeginDisabled`, so boundary rows show plain disabled-look text instead
  of a greyed button, `MoveFilterLayer` itself is already a safe no-op
  there regardless), Layer name (Lab Filter rows are selectable, jump the
  detail editor in Tab 1 to that filter), Active checkbox, Target->Replace
  color swatches, Tolerance. Commander Tag's row shows its 9-auto-target
  count and `EnhancerTolerance` instead of a single swatch pair, since it
  has no single target color of its own.
- Deliberately did **not** add this to the Sensor Graph HUD's "Aktiv:" list
  or the embedded panel's handshake status line - those already show
  *which whole features* are on (`FeatureModuleRegistry`, coarser
  granularity); the Filter Layer Matrix is one level deeper (individual
  target layers within the targeting system, with real order), scoped
  deliberately to Filter Lab only so it doesn't become a third "what's
  active" list.

**"Glass" Contrast Swatches bug found and fixed, same day it was added**:
Emi's live-testing verdict on the 2026-09-11 Glass toggle above: checking it
made the cards *more* opaque, not transparent ("macht nur den Hintergrund
komplett schwarz, noch schwaerzer als vorher"). Root cause: the swatch cards
are `BeginChild` windows nested inside this *window's own* already-opaque
background (Theme's `WindowBg`) - dropping the child's own tint to alpha 0.10
didn't reveal the live game behind the panel, it just revealed the flatter,
darker window background underneath, which reads blacker than the tinted
card fill it replaced. A real "see the game through this" effect would need
the swatches drawn on `GetBackgroundDrawList()` instead of inside a window (a
bigger change, not attempted). Emi's own fix suggestion, taken directly:
drop the card concept entirely and match Vision Lab's Anomaloscope circle -
plain shapes straight on the panel's own background, no separate dark layer.
`DrawContrastTestSwatches` (`MainWindow.cpp`) now uses
`ImGuiWindowFlags_NoBackground` with no border instead of a `ChildBg`
push/toggle; `Settings.GlassContrastCards` (added and broken the same
session) removed outright - keyed field, safe to drop with zero migration
risk.

**Compact "Original -> Contrast" row added to the Nexus-embedded panel**
(2026-09-11, Emi's ask: a small always-visible proof of what Auto-Com-Tag
actually does, "in Form von einer Reihe aller Commander-Tag-Farben"): right
below the "Active - N of 9 colors shifted" status line, a row of 9 tiny
two-circle overlap swatches (original tag color vs. `s_tagConflictStates[i]`'s
already-computed replacement color, one per Commander Tag reference color).
Deliberately reuses the existing per-tag conflict state instead of
recomputing anything - for a tag with no conflict, `rep*` already equals the
original, so its pair of circles fully coincide and reads as one plain dot
automatically, no extra branch needed to distinguish "safe" from "shifted"
tags. Only shown while Auto-Com-Tag is on.

**Still open / deliberately not touched this pass**:
- Two Curve-View/spectrum-graph widgets (Main Window's static transfer-curve
  view vs. Sensor Graph HUD's live filtered-spectrum view) share the same
  Polygonal/Harmonic/Rays selector look even though the underlying data
  differs - flagged as a visual-distinction polish item, not a functional
  duplicate (both are already read-only diagnostics), lower priority than
  the structural work above.
- Commander Tag pre/post-DWM double-transform, slider-overscaling,
  `DisableHotloading` CI split, and the D-tier items (ungoverned
  `Enabled=true` sites, OS-banner sync, `cba_session.lock`,
  `FreeFilterEnabled`) are all still queued from the agreed A-D punch list,
  not started yet this pass.
  *(Status as of 2026-09-12, so this dated entry is not read as current: the
  double-transform was fixed the same evening; the ungoverned-`Enabled` list
  became an audit ratchet; the OS banner was deleted rather than synced;
  `FreeFilterEnabled` was already gone. Slider-overscaling, the
  `DisableHotloading` split and `cba_session.lock` are genuinely still open.
  This is the one place a dated log needed a pointer forward — everything else
  in these logs stays true by being past tense.)*

Built and unit-tested after every logical chunk (not after every single
edit, per Emi's explicit ask to batch builds on large tasks) - 24/24 passing
throughout.

## Session log (2026-09-11, later) - pipeline composition made explicit

Emi handed over an autonomous pass with only the Trinity ruleset (UX ->
Performance -> Stability) as a constraint, plus three framing principles
stated during the work, which shaped every decision below:
1. The Filter Layer Matrix exists to **list the pipeline and avoid hidden
   automation** - visualize what's active instead of adding clever logic.
2. Modes stay cleanly separated and **nothing gets suppressed** unless the
   user wants it (Hybrid = overlay mode, Filter Lab = free experimentation
   / wavelength elimination, Vision Lab = the clinically grounded one).
3. The whole point of the "automatics" was **to stop other settings from
   corrupting them**.

**The root finding (one bug, two symptoms).** The screen-wide DWM stage was
never represented anywhere as data - only spelled out inline inside
`Recompute()`. Two higher-level automatics therefore modelled a pipeline
that omitted it:
- *Commander Tag enhancer*: scored candidate replacement colours as
  `Sim(colour)`, but the user perceives `Sim(M x colour)` - the DWM matrix
  hits the scanner's own overlay markers too. It was optimising a stage that
  never exists in isolation. This is exactly principle 3 violated from the
  inside: the base correction was silently corrupting the automatic.
- *Auto-Brightness*: `GetBrightnessRetention()` measured luminance loss of
  the CVD matrix only, so Eye-Sensitive Mode's very real luminance cost
  (blue filter, desaturation) went uncompensated - Emi's own "die
  Helligkeitsregelung ist genial aber noch nicht vollstaendig in der Logik".

**Fix**: `core/FilterLayers.*` (already the "which layers are active" module)
now also owns the composition math, so display order and display maths live
in one place: `ActiveCorrectionMatrix()` (the `if (Mixed)...` pattern that
was copy-pasted at 7 sites), `ColorStackMatrix()` (CVD + Eye-Sensitive,
deliberately WITHOUT GammaGain) and `EffectiveDisplayMatrix()` (the full
`g * (E x C)`, identity while the master filter is off - a real state, the
highlighter is not gated on `Enabled`). `Recompute()` now calls the last of
these rather than keeping its own copy. The GammaGain/ColorStack split is
load-bearing, not cosmetic: Auto-Brightness solves FOR the gain, so feeding
a gain-containing matrix back into its own measurement would be a feedback
loop. Full derivation: COLOR_MATH.md section 8.

**Expect fewer shifted tags, and that is correct.** With M included, tags the
base correction already separates no longer register as conflicts, so
"N of 9 shifted" will read lower than before. Per Fidaner et al. (2005) that
is the intended behaviour, not a regression - unit test
`TestCorrectionIncreasesPerceivedSeparation` pins the underlying property
(correction must increase a deutan's perceived red/green tag separation),
and `TestOmittingTheDisplayMatrixChangesTheAnswer` pins that omitting M is a
different answer, not an approximation. Nothing is suppressed: the base
correction still runs in full, the enhancer merely accounts for it.

**Pipeline made visible** (principle 1): the Filter Lab widget is now
"Filter Pipeline" - a read-only *Stage 1 - screen-wide (DWM)* table (base CVD
correction / Eye-Sensitive / Brightness, each with live status) above the
existing reorderable *Stage 2 - target layers*. Pure visualization, no new
automation, and it is what makes a low "N of 9 shifted" legible instead of
looking broken. Read-only on purpose - one editable home per setting.

**Also fixed / found while in there**:
- `FilterLab.cpp` used `corrMat[1][0]` twice in the green row instead of
  `[1][1]` - a real transcription bug, latent only because it sits in the
  callerless `DrawContrastCombinationsWidget`. All 7 hand-expanded matrix
  multiplies now go through the tested, clamping `ColorMatrix::ApplyPixel`.
- Doc drift corrected: the ungoverned-`Enabled` list was 5, is now 4 (see
  "Known loose ends"); the file map was missing 4 core modules; the
  "local build is around v1.0.2-pre" line predated v1.11.0.

**Deliberately NOT changed, needs Emi's call**: `GetBrightnessRetention()`
samples `kGw2TagRefs[0..7]` - 8 of 9, skipping White. Reads like an
off-by-one against a `[9]` array, but has a defensible reading (white is
invariant under the correction, and neutrals are already represented by the
ambient "Stein" sample). "Fixing" it would lower everyone's recommended gain,
i.e. visibly change brightness - so it is flagged in-code and left alone.

## Session log (2026-09-11, evening) - base panel rebuilt around one job

Driven by `PRODUCT_CONCEPT.md` (written this session, read it first). Emi's
framing: the base product should be **one usable feature done completely**,
with everything else clearly in the extended tier, where "half diffuse" is an
acceptable state for a workshop but not for the entry point.

**Chosen base feature: Commander Tag contrast.** Deliberately not AoE circles
despite them arguably being the bigger GW2 complaint - an AoE ring is
alpha-blended over grass/stone/snow, so its actual pixel colours vary wildly,
while a tag is a discrete icon with constant colours. For a colour-distance
matcher the tag is a reliable target and the ring is not. AoE belongs in the
extended tier where experimenting is the point.

**Guided entry replaces the three type buttons.** "Protan / Deutan / Tritan"
asks for a diagnosis most players have never had. The panel now asks what the
user can SEE: which pair is hardest to separate, then - only on the red-green
axis - whether the red reads much darker (the one protan/deutan discriminator
a person can actually answer, since protanopia genuinely lowers luminance
response to long wavelengths). Severity is set the same way: the pair is shown
AS CORRECTED and the user answers "can you tell them apart now?". Direct type
buttons still exist for anyone who knows their diagnosis - under Advanced.

**Hold-to-compare** (`CTRL+SHIFT+V`, remappable): suspends both filter stages
while physically held, so "is this doing anything?" gets answered against live
game content. Nexus's `KEYBINDS_PROCESS` already delivers the release edge
(`aIsRelease`), so this needed no second raw-input path - the deliberately
removed `WM_KEYDOWN` path stays removed. The Watchdog clears the flag when GW2
loses focus or is minimized, because a lost release event would otherwise
suppress the filter indefinitely.

**Base panel reduced**: profile slots gated behind Advanced Mode (empty on a
fresh install = pure noise), profile-code field moved into Advanced. Nothing
removed, everything still reachable.

**Eye Comfort promoted into the base panel.** Emi reported it as the feature
that actually keeps him using the tool while playing - and checking revealed
its entire UI lived in Main Window Section 2, i.e. behind the Advanced gate.
Same structural mistake as Vision Lab's, at the most expensive possible spot.
**Rule recorded: an acquisition feature may sit behind a gate, a retention
feature may not.** The compact block (activate + three sliders) is a second
*binding*, not a duplicate editor - all three values are
ParameterRegistry-backed, so storage and clamp are shared with Section 2,
which is literally the registry's own stated litmus test.

**"The filter cannot work right now" banner added to the base panel.** Both
conditions (exclusive fullscreen, DWM rejecting) were previously reported only
in the Main Window - so a base-panel user in exclusive fullscreen saw "ON", a
live status dot and "Active - N of 9 shifted" while nothing whatsoever
happened on screen. That is not a missing warning, it is the panel supplying
false evidence, which the concept exists to prevent.

**Zero-shifted is now spelled out as success.** After the pipeline fix earlier
today the enhancer legitimately shifts fewer tags, often none. A bare
"0 of 9" reads as broken and would send someone hunting a non-existent fault.

**Self-review found five defects in the same session's own UI code** (written
without any ability to run it - worth repeating that discipline):
- "I can tell them all apart" was a no-op that re-entered question 1 forever.
- "Off" had become a one-way door once the type buttons moved to Advanced.
- The step-3 preview briefly swapped `CurrentSettings` fields to borrow
  `ActiveCorrectionMatrix()`, racing the Watchdog's 50ms `Recompute()`.
- The compare-hold safety net skipped the minimized case - the likeliest way
  to lose a release event in the first place.
- `InvisibleButton` took `GetContentRegionAvail()` unchecked; inside Nexus's
  user-resizable window that can go non-positive, which is an ImGui assert.
- Plus one caught by reading before compiling: `Theme::kDotReadyCol` is an
  `ImU32` for draw-list calls and was being passed to `TextColored`.

**Note on the build environment**: a Visual Studio background update
deregistered the VS instance mid-session, so `cmake --build` failed with
"could not find specified instance of Visual Studio" while `vswhere` returned
nothing. Not a code problem and not something to work around by editing the
generator - it resolves when the installer finishes.

## Session log (2026-09-11, night) - the two checking systems, merged

**Two half-built systems found, unified.** `tools/audit_pro_review.py` (reads
source, runs in CI) and `core/SelfTest.{h,cpp}` (reads live in-process state,
runs on demand) had grown up unaware of each other with near-identical data
models. They were never competitors - they are split by *what they can
observe*. The gap between them turned out to be the cause of the doc rot:
SelfTest already had a third result state (`isInfo`), the audit only knew
pass/fail, so a fact that legitimately *varies* had no home in tooling and got
hand-written into this file instead.

The audit borrowed that vocabulary and gained `ratchet()` - a measurement
allowed to shrink but not grow, printed every run so it cannot go stale, and
failing only on a rise. Verified it actually fails when exceeded rather than
assuming it. New Pillar 6 moved three rotting facts out of prose. In the other
direction SelfTest gained assertions for the pipeline decomposition
(`EffectiveDisplayMatrix` must be identity while the filter is off; the colour
stack must equal the plain CVD correction while Eye-Sensitive is off) - i.e.
the exact drift whose absence caused the Commander Tag bug that morning.

**Both silent-field-shift traps removed rather than guarded.** We had built two
separate guards around hand-maintained positional orderings; guarding a fragile
design twice is worse than fixing it once.
- `ExportPresetString` was one `snprintf` whose placeholder order and argument
  list had to stay in sync by hand. The importer was always key-based, so the
  ordering served nothing - now built field by field, key next to its value.
- `L10n.h`'s language blocks are designated initializers (`.Field = "..."`).
  A wrong order is now a compile error - verified by deliberately swapping two
  entries and getting 8. The audit check stays for the one failure mode still
  legal C++: an *omitted* field compiles and leaves a null `const char*`,
  which reaches ImGui as a crash. Order is the compiler's job now,
  completeness is the audit's.

**SmartEnhancer fully removed** - Settings field, Load/Save, preset import
branch, `ParameterRegistry` registration and its `ParamId`. It had had no
behavioural reader since the selectivity fix and survived only because editing
`L10n.h` by hand was risky; once that edit became mechanically verified, the
reason to keep it disappeared. Removing the enum entry is safe despite the
"do not remove" note there: nothing serializes off the integer, and SelfTest
asserts enum/registry agreement, so a half-done removal fails loudly.

**Vision Lab: a normal result no longer applies a correction.** Both
"normal reading" branches of *Apply to CBA Profile* set 0.20 severity and left
`Type` untouched, so the outcome depended on what had been tested before -
run Moreland first, then measure normal on Rayleigh, and Apply switched on a
20% Tritan correction right after the panel said "Normal Trichromat".

**Not done on purpose while Emi was away:** no release tag. `main` was pushed
(reaches no users), but a tag triggers the GitHub release that Nexus serves to
players, and the entire day's UI had still never rendered on a screen. Cutting
that unattended was the one action that would have been hard to walk back.

## Session log (2026-09-12) - the manual path gets a home

Emi's first live look at build 30 with all five windows open. Verdict: the
guided entry works, but **the manual filter settings were simply gone** - and
the leftover "Advanced" fold in Main Window Section 1 (Tolerance + the AQ/HRR
reference field) read as fragmented from the contrast logic it actually
drives.

**What that exposed about the 2026-09-11 restructure.** "One editable home per
setting" was right, and removing the third duplicate editor was right - but
the setting whose home was removed had two *different kinds* of user, and only
one of them got a home. The Nexus panel asks what you can see and sets the
values for you; that is the correct entry point and the wrong tool for someone
who wants to dial a number in by hand. The rule was never wrong, the count was:
there are two paths, so there are two homes.

**Sensor Graph window is now the manual home.** Directly under the real-time
spectrum, as one module rather than scattered controls:
Protan/Deutan/Tritan/Mixed, the strength sliders back at their 125% ceiling,
the contrast tolerance and reference values moved in from Main Window
Section 1, and the brightness block that already lived there as the last
slider - Emi's own ordering. Main Window Section 1 keeps its read-only status
line and now names both homes ("Guided: Nexus Panel | Manual: Sensor Graph")
instead of only the one that deliberately has no sliders.

Not a straight revert of the deleted block: the three near-identical ~25-line
slider+Reset copies it used to carry (Strength, RG, BY) are one lambda, and
the sliders read/write through `ParameterRegistry` so the 1.25 ceiling comes
from `ParamMeta` rather than from three hand-written widget arguments.
Percent stays display-only in 0-125 units - ImGui does not scale a value to
match its format string, which is the Eye-Sensitive "0%/1%" bug already on
record here.

**Two defects fixed while moving the code rather than after:**
- A manual slider in an OFF filter stores a value and changes nothing on
  screen - the panel supplying false evidence, which is the one thing
  PRODUCT_CONCEPT.md exists to prevent. The module now says so and offers the
  single click that fixes it, via `ToggleMasterEnabled()` (no new direct
  `Enabled` write - see the ratchet).
- The reference-values text field set `saveNeeded` per keystroke, i.e. a
  `Settings::Save` plus a forced `Recompute(true)` for every letter typed -
  the same defect the Filter Lab colour pickers had. Now deferred to
  `IsItemDeactivatedAfterEdit()`. The per-keystroke *assignment* has to stay:
  the buffer is refilled from `DiagnosisHint` each frame, so skipping it would
  undo each character as it is typed.

Build 32, 26/26 unit tests, 24/24 audit + 1 informational.

**Second pass the same day, from Emi's live screenshot of the new module.**
Three asks, one of which turned out to be two bugs wearing one coat:

- *Reset buttons on the Eye Comfort sliders.* Without one the only routes back
  to neutral were dragging by eye to exactly zero or "Filter zuruecksetzen",
  which also wipes the colour profile. Added to the Nexus panel's three
  sliders, and to Main Window Section 2's copy of the same three - which had
  been three hand-copied blocks and is now one lambda, the same shape as the
  panel's and the Sensor Graph window's. Reset in one place and not the other
  is precisely the "Reset UI" drift already on record above.
- *Remove the "OS BLOCKIERT FILTER!" banner.* Emi: it is not correct anyway.
  He was right, and about more than he knew - see the loose-ends entry. It was
  wrong about the world (the flag reports "not foreground", not "blocked") and
  wrong about its own window: a full-width `BeginChild` dropped between the
  master button and the toolbar buttons that follow it via `SameLine()`. The
  child ends the row, so Sensor Graph / Filter Lab / Vision Lab / Reset UI /
  Reset Filter / Export / Import / language were all laid out past the right
  edge and vanished. That is the "**die ganze Tab-Leiste verschwindet, sobald
  ich das Snipping-Tool im Vordergrund habe**" report: one condition, two
  visible symptoms, and the screenshot tool was itself the trigger.
- It also silently stole the two ImGui "last item" queries that follow it: the
  master button's own tooltip and the `GetItemRectMin/Max` the OFF-state glint
  animation traces both read the *banner's* rect whenever it showed. Neither
  was reported by anyone; both are repaired by the same deletion.

The general lesson worth keeping: a conditional full-width `BeginChild` in the
middle of a `SameLine()` row is invisible in code review and invisible in
testing, because it only misbehaves in the state that makes it appear.

Build 33, 26/26 unit tests, 24/24 audit + 1 informational.

### Third pass: why the filter would not switch off

Emi: the filter is supposed to get out of the way when he leaves the game, and
it does not. He proposed two possible rules - "off when the game is in the
background", like GW2's own music setting, or the simpler "off when the window
is minimized" - and asked whether we could do it cleanly.

**The rule was already in the code and already correct.**
`!isMinimized && (isGw2Foreground || SystemWide)` says exactly what he asked
for, including minimize overriding "keep active in background". So the
interesting question was not what to build but why what exists does not work.

**The Nexus log answered it in one line.** Every rejection in
`addons/Nexus/Nexus.log` reads `MagSetFullscreenColorEffect (Clear) REJECTED`.
Not one `Apply` rejection in the whole file. The filter switched **on**
reliably and **off** unreliably - and a rejected clear is invisible from inside
the game, because by definition you are looking at something else.

The asymmetry follows the thread: `MagInitialize()` and almost every `Apply()`
run on the render thread; the clears on focus loss ran from the WndProc thread
or the Watchdog thread, at a moment when GW2 was already in the background.
Whether the OS refuses because of the thread or because of the foreground state
is not settled - but both readings have the same fix, so it did not need to be:
**evaluate the gate where the calls demonstrably succeed.**

What was built:
- `ShouldScreenEffectBeActive()` and `SyncScreenEffectToGate()` (declared in
  `ui/UIState.h`, defined in `ModuleMain.cpp`, same convention as
  `ToggleMasterEnabled` and friends). The gate condition had been written out
  by hand in three places - `Recompute`, `WatchdogLoop`, and
  `DrawFilterStatusIndicator`'s inverse - which is how a rule this small turns
  into three rules. One now.
- The **render callback** drives the gate, throttled to the same 50ms the
  Watchdog uses, so this moves *where* the call happens rather than how often.
  This is the actual fix for the alt-tab case: a windowed GW2 keeps presenting
  in the background, so the render thread is alive exactly when the clear is
  needed.
- `WM_SYSCOMMAND`/`SC_MINIMIZE` added, which arrives *before* the window is
  minimized while GW2 is still in front. `WM_SIZE` and the Watchdog stay as the
  safety net, because Win+D and Win+M do not necessarily route through
  `WM_SYSCOMMAND`.
- The WndProc cases stopped re-deriving half the rule each
  (`if (!SystemWide)` in one branch, a bare `Recompute()` in the other) and now
  just poke the gate. The mutex discipline the 2026-09-09 review added by hand
  lives inside the gate function now, so the call site that had forgotten it
  cannot forget it again.

**Made verifiable rather than asserted**: `g_DwmClearRejectCount` counts
refused clears since load, and SelfTest reports it alongside a hard check that
the effect is not installed while the gate says off. The claim "it switches off
now" is therefore something Emi can read off the panel after alt-tabbing, and
something the Nexus log will contradict if it is wrong. That is the point - the
previous version of this belief was a banner that stated the opposite of what
was happening.

Build 35, 26/26 unit tests, 24/24 audit + 1 informational.

Confirmed live the same evening: Emi's SelfTest read *"Clears the OS refused
since load - Every attempt to switch the screen-wide effect off went through"*,
and the filter dropped the moment the snipping tool took focus. The fix is
observed, not assumed.

### Fourth pass: the base panel gets a control field

Emi, after using it: the panel's own controls were scattered by accident of
history rather than by meaning. Three moves, all in `RenderEmbeddedOptions`:

- **"Filter auch im Hintergrund aktiv lassen" moved out of the Advanced fold**
  to sit directly under the master switch. It answers the very next question
  that switch raises - not "is the filter on" but "on *when*" - and until the
  screen-effect gate was fixed a few hours earlier it barely did anything
  observable, which is probably how it drifted somewhere nobody looks.
- **"UI zuruecksetzen" / "Filter zuruecksetzen" moved up** out of the bottom of
  the Eye Comfort section. Two panic buttons parked at the end of an unrelated
  feature are findable only by someone who already knows they are there - i.e.
  not by the person who needs them.
- Reading order of the base field is now: what is on -> when -> how much app
  (Advanced Mode / Studio) -> recovery.

**Brightness joined Eye Comfort** on the base panel: retention and recommended
gain, the one-shot "Optimalwert" button, the automatic checkbox, and a manual
slider with Reset - the same three controls as the Sensor Graph window's block,
in the same order. Deliberately *not* inside the "Aktivieren" gate above it:
the compensation works on the plain colour correction too, and hiding a working
control behind a checkbox that does not govern it is the exact mistake that
kept Eye Comfort itself out of sight until 2026-09-11. Every value is
ParameterRegistry-backed, so this is a second *binding*, not a second editor.

Build 37, 26/26 unit tests, 24/24 audit + 1 informational.

### Fifth pass: Nexus "Disable" left the filter on and the addon dead

Emi, immediately after: clicked **Disable** in Nexus, the colour effect kept
running, and clicking Enable again did nothing at all.

Two separate defects, both in `AddonUnload()`, and the second one is the more
embarrassing of the pair.

**1. The clear at unload is the same rejected call as everywhere else.**
`AddonUnload()` runs on Nexus's loader thread; `MagInitialize()` ran on the
render thread. One plain `Clear()`, rejected, and there is no code left in the
process to try again - the effect just stays on the display.

Two facts came out of this that were previously assumptions:
- **`MagUninitialize()` does not remove an installed fullscreen colour
  effect.** `Shutdown()` called it and the screen stayed tinted. A successful
  `MagSetFullscreenColorEffect` is the only way out. Worth knowing before
  anyone reaches for it as a cleanup shortcut again.
- The rejection is not specific to being in the background - Emi was looking at
  the Nexus window inside GW2 when he clicked Disable.

`ClearScreenEffectForShutdown()` now escalates: the plain call, then five
20ms-spaced retries, then a rebind of the Magnification session to the
unloading thread (`MagUninitialize` + `MagInitialize` there) and one more try.
Whichever step works is written to the Nexus log, so the next report answers
which reading was right instead of us arguing it again. If all of them fail it
logs the recovery path (Windows Settings > Accessibility > Colour filters,
toggle on then off) rather than leaving the user with a tinted screen and no
idea why.

**2. `s_deferredInitDone` never got reset, so re-enabling could not work.**
This file already recorded that Nexus's disable/enable does not necessarily
unload the DLL (see the `FeatureModuleRegistry` note from 2026-09-09) - and
then the same trap caught us one static further along. `Shutdown()` sets the
controller's own `_initialized` to false, but `s_deferredInitDone` stayed
**true**, so `EnsureDeferredInitialized()` returned at its first line and
`MagInitialize()` was never called again. Every `Apply()` and `Clear()` bailed
out at their `if (!_initialized)` guard. The addon was alive, with a dead
Magnification session and no path back to a live one.

`AddonUnload()` now resets the whole session state - `s_deferredInitDone`, the
init-retry cooldown (hoisted out of function-local statics for exactly this
reason), the apply bookkeeping, and the compare-hold flag.

**3. And the leftover effect from a *previous* session had nobody to clear it
either.** On a fresh load with the filter off, `s_hasApplied` is false, so
nothing thought there was anything to clear - while the display still carried
whatever the last session left behind. `EnsureDeferredInitialized()` now clears
explicitly when the filter is off, right after `MagInitialize()` succeeds:
the first moment in the process where the call is known to work, on the thread
where it is known to work, and therefore the right moment to insist that "off"
means a neutral screen.

Build 39, 26/26 unit tests, 24/24 audit + 1 informational.

### Sixth pass: startup behaviour, and what "enable in background" collides with

Emi asked for a general startup policy - everything off, all windows closed,
positions remembered the way arcdps does it - plus a legible link between the
three profile slots and "start automatically with GW2", and wanted the
interaction with "keep the filter active in the background" thought through.

**Most of the policy already existed. Verified live rather than assumed:**
`Settings::Load` forces `Enabled = false` and all four `Show*Window` flags to
false unconditionally, on every launch, with the reasoning already written
there. Window *positions* are persisted by ImGui through Nexus's own
`addons/Nexus/imgui.ini` (7 CBA entries with `Pos=`/`Size=` in Emi's, checked
directly) - so "remember where it was, but do not open it" was already the
behaviour. The Auto-Start profile is the single deliberate exception, and the
Safe-Start Gate can veto it after a crash.

**What was actually missing was legibility, not logic.** `AutoStartSlot` is one
int with four states, and it was edited two incompatible ways: a **right-click
on a slot chip** in the Nexus panel, with a one-character `*` as the only
feedback and nothing on screen saying the feature existed at all, and **three
per-slot checkboxes** in the Studio standing in for what is a single choice.
Both are gone. `DrawAutoStartControl()` is now the one control, drawn in both
places: a checkbox for "do I want this", then radio buttons over the three
slots for "which one", with empty slots drawn in place but not selectable.
They cannot desync - switching it on always picks a real slot (the active one
if it is saved, otherwise the first), and clearing a slot switches it off. With
no slot saved at all it says so in a sentence instead of offering a checkbox
that refuses to stay checked (this vendored ImGui has no `BeginDisabled`).

**The collision Emi asked about is real, and it was the startup case.**
`SystemWide` is persisted, so it is already on at the next launch - and at
launch the player is usually in a browser or on Discord waiting through the
loading screen and character select. An Auto-Start profile plus SystemWide
would therefore tint whatever they are actually looking at, minutes before they
ever see the game. `SystemWide` now only counts once GW2 has been the
foreground window at least once in this session (`s_gw2EverForeground`, reset
on unload): "keep the correction while I alt-tab away" presupposes having been
there first. Both language tooltips say so.

**New audit check, and what probing it found.** "Settings::Load forces a
neutral start" is now Pillar 6.4 - this exact guarantee has already been lost
once by accident (removing the dead `LoadOnStartup` opt-in in 2026-09-09 also
removed the only thing forcing `Enabled` back to false, and the symptom was the
filter re-arming itself at character select). A policy that can vanish as a
side effect of unrelated cleanup belongs in CI.

Worth recording that the check was wrong twice before it was right, and only
because it got probed instead of trusted:
1. It matched the text inside a commented-out line, so disabling the guarantee
   left it green. Now comment-stripped.
2. It still passed with the unconditional `s.Enabled = false;` removed, because
   the *same string* appears ~190 lines earlier inside the Safe-Start crash
   branch - a conditional write, which is exactly what this guarantee is not.
   It now anchors on the window-visibility group and requires the rest within
   the same stretch of source, so it pins one block rather than five sightings.
Both failure modes were silent passes. Same lesson as the ratchet: run the
check against a deliberately broken tree before believing it.

Build 40, 26/26 unit tests, 25/25 audit + 1 informational.

### Seventh pass: two questions about scope

**"Does the closed-on-start rule cover all windows?"** Yes, and checked rather
than restated: `Settings::Load` forces all four `Show*Window` flags false, the
new audit check 6.4 pins that, and Emi's live
`addons/cba/settings.ini` reads `ShowMainWindow=0 / ShowGraphWindow=0 /
ShowLabWindow=0 / ShowVisionLab=0` with `Enabled=0`. Vision Lab has no second
opening path - `RegisterCloseOnEscape` only ever writes false, and the render
gate is a plain `if (ShowVisionLabWindow)`. If it opened on a launch recently,
it was a build from before that policy, not a live hole.

**"Filter off when I look at the browser, cleanly back on afterwards."** That
is exactly what `SystemWide` **off** does, and it is the default - but Emi's
settings.ini had `SystemWide=1`. Worth understanding why rather than just
flipping it: until the screen-effect gate was fixed earlier the same day, this
checkbox had **no observable effect** - the filter stayed on in the background
either way. Any value sitting in a settings.ini today was therefore chosen
while the setting did nothing. It now decides whether the browser you alt-tab
into gets tinted, so the base panel spells the consequence out live underneath
it, in one line, instead of hiding it in a tooltip nobody opens.

The larger question it came with - "can we filter only the GW2 window" - is
recorded as its own decision entry under "Known loose ends" rather than
answered in a session log, because it is a standing architectural question and
not a thing that happened on a Friday.

Build 41, 26/26 unit tests, 25/25 audit + 1 informational.

## Branch log (v2-shader-core, opened 2026-09-12)

Not on `main`. This is the branch where the colour correction stops being a
system-wide Windows accessibility effect and becomes a pass on GW2's own
frame. Written up only after it had answered every question that could have
killed it, which is the bar I set before starting.

**The question that opened it.** Emi asked whether the filter could apply to
the GW2 window only - "wir teilen den Screen in 2 Layer". The literal idea has
no API: `MagSetFullscreenColorEffect` takes a matrix and nothing else, and DWM
composes every window into one surface before the effect applies. The two real
options were a magnifier overlay window (topmost, a fullscreen copy per
refresh, dead in exclusive fullscreen) and a post-process pass on the
swapchain. The second is the right one and was blocked only by AGENTS.md
section 4's "no D3D11 hooking" rule.

**Why that rule turned out not to be in the way.** Emi's reason for it is
specific and good: a 20-year-old GW2 account he will not risk, and no wish to
trip a heuristic scanner. So this was checked in his own Nexus checkout rather
than reasoned about:
- Nexus already installs the only hook involved - a MinHook vtable detour on
  `IDXGISwapChain::Present`, `Core/Hooks/Hooks.cpp:81`.
- Nexus already binds the game's backbuffer as a render target and draws ImGui
  into it, `UI/UiContext.cpp:348/490`. Every CBA window is already pixels in
  GW2's frame.
- CBA already takes the device off the swapchain, creates textures,
  `CopyResource`s the rendered frame and `Map`s it for CPU reading
  (`HybridScanner.cpp:489-608`), and already draws a display-sized textured
  quad over the game (`ModuleMain.cpp:1159`).
The pass adds one draw call and a compiled shader. No new hook, no new module,
no process or memory access. The rule means "do not inject or hook like
ReShade"; it never meant "never touch D3D", and CBA had been past that line
for weeks in the more invasive direction.

**What the branch answers, with numbers rather than argument:**

| Question | Answer | How |
|---|---|---|
| Does it work? | yes | live, Emi |
| Does HDR10 break the maths? | no | backbuffer is `R8G8B8A8_UNORM`, normalised 0..1 - SelfTest reports the format |
| What does it cost? | 0.032-0.061 ms CPU | `g_perfShaderPassMs`, in the report and the Performance Watchdog |
| Do the two backends agree? | to 1e-9 | unit test `TestBothBackendsApplyTheSameTransform` |
| Does the correction tint white? | no, spread 3.9e-5 | unit test `TestCorrectionKeepsWhiteNeutral`, plus a live SelfTest check |

**The mistake worth keeping.** The pass first went into
`ERenderType_PreRender`, which corrects the game before ImGui and therefore
left CBA's own interface true colour. That is genuinely desirable - under DWM
even Vision Lab's anomaloscope was viewed through the correction it exists to
measure - and it was advertised as a benefit. It also silently broke the
invariant the entire Commander Tag derivation rests on ("everything on screen
gets M"): `HybridScanner::ScanFrame` runs in `ERenderType_Render`, i.e. after,
so it would have matched an already-corrected frame against raw reference tag
colours; and the tag overlay, also drawn in Render, would never have received
M while the enhancer chose its colours assuming it would. It would have shown
up only with Commander Tag Contrast on, only as "the colours look a bit off" -
the hardest class of bug to attribute, with the cause written in the docs as a
feature. `ERenderType_PostRender` restores the invariant exactly: the pass sees
game plus overlay plus UI, the same content DWM saw, still before Present.

Recorded because the lesson is not "check the frame order". It is that a
pleasing side effect discovered during a refactor deserves more suspicion than
a planned one, not less.

**The sensor (2026-09-12).** Emi's ask: a way to measure what the filter
really does, "im Rahmen unserer Moeglichkeiten". The shader path is what made
it possible — inside the pass both halves of the same frame exist, the copy
taken before the correction and the backbuffer after it, so their difference is
the effect on the actual image. `platform/FilterSensor` takes the frame-wide
mean through the GPU's own mip chain (copy mip 0, GenerateMips, read the 1x1
top: three calls and four bytes back, versus the eight million pixels
HybridScanner has to map), with `DO_NOT_WAIT` because a sensor that stalls the
frame it measures is measuring something it caused, throttled to 5 Hz, and
allocated only while something is reading it.

It earned its keep immediately and then caught me out twice in one reading:
- The comparison row compared `retentionRatio` (which deliberately excludes
  GammaGain) against a measurement that includes it, and reported the unit
  mismatch as a disagreement. Same false-evidence failure as the removed "OS
  blocked" banner, in a block whose entire purpose is supplying evidence.
  Fixed by comparing retention x gain against the measurement.
- What survived that fix is a genuine finding: at severity 0 with a 1.25x gain
  the screen got +16.2% brighter, not +25%. The shader clamps, so a gain above
  1.0 cannot brighten pixels already at maximum. The matrix cannot know that.

**Auto-Brightness can now be steered by measurement** rather than prediction,
in two modes that are different jobs rather than variants:
- *Filter neutral halten* — drive measured_after towards measured_before, i.e.
  hold the correction to zero brightness cost. Scene-independent, because it
  steers on a ratio. The gain is divided out of the measurement before use
  (`g_new = g * before / after`), which is what makes it a one-step correction
  instead of the feedback loop this file already warned about.
- *Niveau halten* — drive measured brightness towards a captured target. This
  is auto-exposure and it works AGAINST the game's own lighting; the tooltip
  says so, because a cave and a desert are supposed to differ. It exists
  because Emi asked the real question behind it: if the game's own output is
  too bright, CBA should be able to pull it back.
Guards on both: deadband, damping, and a freshness window — the pass stops
sampling whenever it stops running, and a frozen reading would steer on an old
frame.

**The "only two positions" opacity slider** turned out not to be the slider.
`UiOpacity` lives in 0.10..1.00 and the format string was `"%.0f%%"`, so
everything below 0.5 printed "0%" and everything above printed "1%". The handle
moved continuously the whole time; the number beside it had two states, which
is what a person sees and therefore what the control is. Third instance of the
identical defect (Eye-Sensitive 2026-09-09, the strength sliders 2026-09-12) —
**ImGui does not scale a value to match its format string**, and the fix is
always the same: run the widget in display units and convert at the boundary.
The control is also named for its job again ("Durchsicht"), having drifted to
"Mischpult", which names furniture rather than what it does.

**Still open on this branch:**
- The true-colour UI is not abandoned, it is deferred. Doing it properly means
  giving the enhancer two matrices - one for game pixels, one for overlay
  pixels - which is a colour-science change and belongs in its own step.
- The DWM path stays until Emi has lived with the shader for a while. Both are
  selectable at runtime; SelfTest asserts only one is painting, because both at
  once would square the correction rather than double it.
- `Magnification session: initialized` still appears in shader mode - harmless
  dead weight that falls out with the DWM path, not before, because the
  comparison needs it.
- Emi's framing for where this goes: every logic the tool already has becomes
  *optionally verified against a measurement*. Auto-Brightness was the right
  first one because it is the only one that already states a number the sensor
  can contradict. The obvious next candidate is the Commander Tag enhancer,
  which claims "N of 9 colours shifted" - with a before/after frame it could be
  measured whether the perceived separation actually increased, instead of
  asserted from the model that chose the colours.
- The sensor measures the frame-wide mean, CBA's own UI and the sensor window
  included. Stated on screen. A region of interest (game area only, or a
  user-placed probe rectangle) would be the honest next refinement if the
  numbers ever need to be precise rather than indicative.

## Build feedback loop

**This changed from earlier sessions**: Claude now has direct local access
(this is a normal Claude Code session on Emi's actual Windows machine, not the
old sandboxed cloud bridge) — MSVC (VS 2026 Community) and CMake are on the
machine, so builds happen directly: `cmake --build build --config Release`
from `plugins/nexus`, no more relying on a separate watch script or asking Emi
to build manually. `tools/watch_build.ps1` still exists for Emi's own use if
wanted, but isn't the primary feedback loop anymore.

The CMake `POST_BUILD` step auto-deploys `cba.dll` to
`C:/GAMES/Guild Wars 2/addons/cba.dll` via `tools/deploy_dll.ps1` (skips with a
warning, doesn't fail the build, if the file is locked by a running
GW2/Nexus). Emi hot-reloads via Nexus's addon disable/enable rather than
restarting the game — fast iteration loop for UI/logic changes that don't
touch startup/crash-recovery paths, which need an actual GW2 restart to test
properly.
