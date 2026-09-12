# AGENTS.md — Development Guidelines & Core Rules for CBA (cba4gw2)

This document contains mandatory architectural guidelines, UI rules, and best practices for all AI agents and developers working on the **cba4gw2** (ColorBalanceAssist for Guild Wars 2) codebase.

---

## 1. Dynamic ImGui Sizing (The `CalcTextSize` Rule)

### The Problem:
Hardcoding static pixel widths (e.g. `ImVec2(148.0f, 24.0f)` or `isDe ? 72.0f : 52.0f`) creates fragile UIs that break whenever:
- Text is translated between German and English (e.g., `"Deutan (Gruen)"` vs. `"Deutan (Green)"`).
- Font scale, UI scale, or system DPI changes.
- Text strings are refined in localization files (`L10n.h`).

### The Rule:
1. **Auto-Sizing by Default**:
   - For standalone buttons, pass `0.0f` for width (`ImVec2(0.0f, height)` or `ImVec2(0.0f, 0.0f)`). Dear ImGui will automatically calculate the exact width based on text size plus `style.FramePadding.x * 2.0f`.
2. **Dynamic Width Calculation with `ImGui::CalcTextSize()`**:
   - When an explicit width is needed (e.g., button groups, aligned rows, or responsive layouts), always compute it dynamically:
     ```cpp
     float padX = ImGui::GetStyle().FramePadding.x * 2.0f;
     float btnW = ImGui::CalcTextSize(label).x + padX + extraMargin;
     ```
3. **Uniform Button Groups (e.g., Profile 1 / 2 / 3)**:
   - When a row of buttons should share an identical width, compute the `std::max` over all labels in the group:
     ```cpp
     float w1 = ImGui::CalcTextSize(lbl1).x + padX + 6.0f;
     float w2 = ImGui::CalcTextSize(lbl2).x + padX + 6.0f;
     float w3 = ImGui::CalcTextSize(lbl3).x + padX + 6.0f;
     float uniformBtnW = std::max({ w1, w2, w3 });
     ```
4. **Responsive Row Layouts**:
   - Never compare `availWidth` against a hardcoded magic number like `440.0f`.
   - Sum the dynamically measured widths of all elements + item spacings, and compare against `ImGui::GetContentRegionAvail().x`:
     ```cpp
     float totalNeeded = (uniformBtnW * 3.0f) + saveBtnW + checkboxW + (4.0f * spacingX);
     bool fitsSingleLine = (ImGui::GetContentRegionAvail().x >= totalNeeded);
     ```

---

## 2. String Encoding & Glyph Safety (ASCII-Clean ImGui Strings)

- Dear ImGui in Nexus addons uses a font atlas that may not contain the full extended Latin, umlauts, or Greek glyph sets.
- **Rule**: All Dear ImGui labels, button texts, combo items, and tooltips must be strictly **ASCII-clean**:
  - Use `ae`, `oe`, `ue`, `ss` instead of `ä`, `ö`, `ü`, `ß` in UI strings (e.g., `"Gruen"`, `"Ueber CBA"`, `"Zuruecksetzen"`).
  - Use `Delta-E` or `dE` instead of `ΔE`.
  - Never place raw UTF-8 multi-byte characters in ImGui calls to prevent `?` glyph rendering glitches in the game overlay.

---

## 3. Filter Lifecycle & Deferred Initialization

- Windows Magnification API (`MagInitialize`) must be safely deferred during game startup to allow GW2, ArcDPS, and NVIDIA overlays to establish stable swapchains.
- **Rule**: Any UI button or toggle that arms or switches a filter mode (`Com-Tag Profile 1/2/3`, `Active` checkbox, `Smart-Auto`, `Reset to Neutral`) must invoke:
  ```cpp
  EnsureDeferredInitialized();
  // Set settings flags ...
  Recompute(/*aForce=*/true);
  ```
- This guarantees that cold-start initialization lag is eliminated and the DWM matrix is updated immediately without requiring the user to touch other settings first.

---

## 4. Game Memory Safety & ArenaNet Policy Compliance

- **Rule**: CBA is strictly a visual presentation-layer assistant (Windows Magnification DWM API / DirectX swapchain post-processing).
- **Zero Game Memory Interaction**: Under no circumstances should the addon read or write Guild Wars 2 process memory, hook game internal functions, or automate player actions.
- Maintain the official policy notice in README and About dialog:
  > *"Third-party addon, used at your own risk per ArenaNet's Third-Party Programs Policy — no automation, no game-memory access, visual-only."*

---

## 5. Architectural Separation of Concerns

Keep the modular folder structure established in `plugins/nexus/src/`:
- `core/`: Pure mathematical logic, matrices, settings serialization, zero ImGui dependency.
- `platform/`: OS interop, Windows Magnification API, Window mode detection, DirectX capture.
- `ui/`: Dear ImGui windows, widgets, and theme styling.
- `ModuleMain.cpp`: Nexus addon lifecycle, WndProc, and keybind dispatch.

---

## 6. Build Deployment & Hot-Reload Workflow

- **Automatic Post-Build Deployment**:
  - `CMakeLists.txt` is configured to automatically copy `cba.dll` directly to the Guild Wars 2 addons directory (`C:/GAMES/Guild Wars 2/addons/cba.dll`) as a post-build step.
  - Every agent or developer must ensure that any compiled build is deployed to `C:\GAMES\Guild Wars 2\addons\cba.dll` so that hot-reloading in the running game via Nexus works seamlessly and reliably.


# CLAUDE.md — working rules for CBA (ColorBalanceAssist)

Standing brief for any AI session on this repo. Kept short on purpose: it is
loaded into every session, so every line here costs attention on every task.

Companions: `AGENTS.md` (coding/UI conventions), `COLOR_MATH.md` (the colour
science and its references), `PRODUCT_CONCEPT.md` (who this is for and why the
UI is shaped the way it is — **read before proposing UI or feature work**).
Anything older lives in `docs/PROJECT_HISTORY.md.old` — archived, not
maintained, and deliberately not a `.md` so nothing treats it as live docs.
Go there only when you need the reasoning behind a past decision. Code comments
citing "CLAUDE.md" for a dated finding mean that file; they predate the split
and are not worth a mass edit. It is also in git history
(`git show 29f790c:CLAUDE.md`) if the file ever goes.

---

## What CBA is, in ten lines

Native C++ Nexus addon for Guild Wars 2 (`plugins/nexus` → `cba.dll`). It
corrects colour for colour-vision deficiency and reduces eye strain. The
correction is one 3×3 matrix (`EffectiveDisplayMatrix`) applied to the whole
picture, plus a separate tag/target highlighting layer that replaces individual
pixel colours via a DXGI readback (`HybridScanner`).

The matrix reaches the screen one of two ways, chosen by
`Settings.RenderBackend`:

- **`1` — a pixel shader on GW2's own backbuffer** (`ShaderColorPipeline`,
  `ERenderType_PostRender`). The default. Touches nothing outside the game.
- **`0` — the Windows Magnification API**, screen-wide via DWM. Kept as a
  complementary path: screen-wide where the shader is game-only.

`FilterSensor` measures the frame before and after the correction — the one
thing this tool can honestly measure about its own effect.

---

## The rules

### Safety — these are not trade-offs

1. **Emi's GW2 account is 20 years old and irreplaceable.** No injection, no
   hooking, no reading or writing game memory. Nexus's own addon API is the
   only route into the process. If something needs more than that, it does not
   get built.
2. **No input automation into GW2.** Not keystrokes, not clicks, not ever.
3. **Before anything new touches D3D or the process, read Nexus's source and
   record the line references.** "It is probably fine" is not an answer to a
   question about that account. (Emi's checkout:
   `C:/Users/Emi/Desktop/Nexus/src`.)
4. **Screenshots and captures are scoped to the GW2 window.** Never the
   desktop. Work out what a capture will contain *before* running it — viewing
   is irreversible.
5. **Never handle credentials or logins.**

### Attribution

6. **No `Co-Authored-By`, no AI-attribution trailer, anywhere** — commits, PRs,
   release notes. Emi is the sole author, whatever any tool default says.

### Code

7. **ASCII only inside string literals.** CI fails otherwise.
8. **Widgets run in display units.** ImGui does not scale a value to match its
   format string; a 0–1 value with `"%.0f%%"` shows only "0%" and "1%". Convert
   at the boundary. This has cost three separate debugging sessions.
9. **One editable home per setting.** A second binding is allowed only through
   `ParameterRegistry`, so storage and clamp are shared.
10. **A shared action gets exactly one implementation.** Reset, toggle, apply —
    every duplicate has eventually drifted.
11. **Check return values.** A silent `false` becomes "the filter is stuck"
    with nothing anywhere saying why. Count failures somewhere a person can
    read them.
12. **Worker loops get per-iteration `try/catch`** and an accurate running
    flag. A thread that dies silently takes its safety net with it.
13. **Reset module-scoped statics in `AddonUnload`.** A Nexus disable/enable
    does not necessarily reload the DLL.
14. **Bound anything sized from file input.** A hand-edited ini is untrusted.
15. **Never state on the panel what you have not checked.** The tool's job is
    supplying evidence; a confident wrong readout is worse than silence.

### Before you say it works

16. **Build:** `cmake --build build --config Release` from `plugins/nexus`.
    Unit tests run as part of it; the DLL auto-deploys unless GW2 has it open.
17. **Audit:** `python tools/audit_pro_review.py` must be green.
18. **Report what you actually saw** — build number, test count, audit count.
    Never a state you did not observe.
19. **A new check is not done until you have broken the thing it checks** and
    watched it fail. Two checks written here passed against a deliberately
    broken tree before this rule existed.
20. **Where a check belongs:** `tools/audit_pro_review.py` reads source text
    (CI), `plugins/nexus/tests/` reads pure maths (every build),
    `core/SelfTest.{h,cpp}` reads live in-process state (on demand).
    `FilterSensor` reads the rendered image, but it measures the *filter*, not
    the project — an assertion that follows from a reading still belongs in
    SelfTest.

### Writing things down

21. **Write the command, not the count.** Anything the code can invalidate —
    counts, line numbers, "currently registered" lists — goes in as the way to
    re-derive it. A count rots; a `grep` does not.
22. **Re-derive before relying on it, and only write "X is in place" after
    looking.** Citing this file as evidence is the failure mode.

---

## Open work

Lives in `ROADMAP.md` — all of it, so there is one list rather than two that
drift apart. This file holds rules; the roadmap holds what is left to do.

## Where Filter Lab is going

Filter Lab is a precursor, not a feature. Emi's intent for it (2026-09-12): a
**modular stack of filter layers, orderable, composed on top of each other to
eliminate specific wavelengths** — and it is meant to be the centrepiece once
the rest is solid. Do not treat its current contents as the design.

The reference is the Nancy Grace Roman Space Telescope
(<https://en.wikipedia.org/wiki/Nancy_Grace_Roman_Space_Telescope>). Two ideas
from it that shaped this, worth keeping straight:

- Its Wide Field Instrument takes **separate exposures through single filters**
  per wavelength band and combines them afterwards — not a cascade of filters
  stacked in one beam. That distinction is the whole point: the target model is
  every layer contributing a weight, not each one narrowing what the next sees.
- Its Coronagraph **suppresses an overwhelming dominant signal** to reveal a
  faint one beside it — conceptually what Commander Tag contrast already does
  when it pushes back a confusable hue.

Known blocker, so nobody rediscovers it: `HybridScanner::AnalyzeBuffer`'s
cluster step groups matches by *exact* replacement-colour equality to tell a
real tag icon from single-pixel noise. A true weighted blend makes nearly every
pixel a slightly different colour and fragments every cluster. Grouping by
target ID or colour similarity has to come first.

---

## Build environment

MSVC (VS 2026 Community) and CMake are on this machine; build directly. The
`POST_BUILD` step deploys `cba.dll` to `C:/GAMES/Guild Wars 2/addons/` and
skips with a warning if GW2 has it loaded — Emi then disables the addon in
Nexus so the next build can deploy. Local dev builds carry a +1-per-build
counter (`plugins/nexus/tools/.build_counter`) so the version Nexus displays
proves which compile is loaded. Releases are git tags (`v*`); latest is
v1.11.0, everything since is unreleased. Semver: Major breaks, Minor features,
Patch fixes.

Current work is on the `v2-shader-core` branch.
