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
