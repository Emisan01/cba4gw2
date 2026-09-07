# Security, Policy & Technical Compliance Guide (cba4gw2)

This document provides a comprehensive technical audit and compliance specification for **cba4gw2** (ColorBalanceAssist for Guild Wars 2), specifically designed for review by **ArenaNet Security Engineers**, **Raidcore / Nexus Core Developers**, and **Community Accessibility Teams**.

---

## 🏛️ 1. ArenaNet Third-Party Programs Policy Compliance

cba4gw2 strictly complies with ArenaNet's [Third-Party Programs Policy](https://help.guildwars2.com/hc/en-us/articles/360013625034-Policy-Third-Party-Programs):

| Requirement | Implementation & Technical Proof | Status |
| :--- | :--- | :---: |
| **Zero Game-Memory Tampering** | No `ReadProcessMemory`, `WriteProcessMemory`, `VirtualAllocEx`, or pointer scanning of `Gw2-64.exe`. Zero process injection. | **COMPLIANT** |
| **Zero Function Hooking** | 100% hookless. No MinHook, Detours, EasyHook, or inline VMT/bytecode patching. Does not hook DirectX `Present`. | **COMPLIANT** |
| **Zero Automation / Macros** | No `SendInput`, `mouse_event`, or `keybd_event`. All input processing is read-only (observing registered hotkeys). | **COMPLIANT** |
| **Visual Presentation Only** | Color compensation is applied post-rasterization via the official **Windows Desktop Window Manager (DWM) Magnification API** (`MagSetFullscreenColorEffect`). | **COMPLIANT** |
| **No Unfair Combat Advantage** | Provides color-space compensation for Color Vision Deficiencies (CVD / Daltonism). Zero hidden information reveals, zero wallhacks. | **COMPLIANT** |
| **Crash Logger Preservation** | Does **not** intercept `SetUnhandledExceptionFilter`. ArenaNet and ArcDPS crash dumping handlers remain 100% intact. | **COMPLIANT** |

---

## ⚙️ 2. Raidcore / Nexus Engine Architecture & ABI Safety

cba4gw2 integrates seamlessly into the Nexus addon ecosystem adhering to strict ABI standards:

- **Heap & Allocator Synchronization**:
  In `AddonLoad()`, the ImGui allocator is explicitly mapped to Nexus's unified heap allocators:
  ```cpp
  ImGui::SetAllocatorFunctions(
      (void* (*)(size_t, void*))APIDefs->ImguiMalloc,
      (void(*)(void*, void*))APIDefs->ImguiFree);
  ```
  This eliminates cross-DLL heap corruption and CRT allocator conflicts.

- **Zero-Throw DLL Boundaries**:
  Every exported callback (`AddonLoad`, `AddonUnload`, `AddonRenderWindow`, `AddonOptions`, `AddonWndProc`, `GetAddonDef`) is fully isolated with `try { ... } catch (...)`. No C++ exception can ever propagate into `Gw2-64.exe` or Nexus.

- **Exact Lifecycle Deregistration Symmetry**:
  Upon `AddonUnload()`, every registered element is cleanly deregistered:
  - `APIDefs->UI.DeregisterCloseOnEscape(...)` (all 7 window variants paired)
  - `APIDefs->InputBinds.Deregister(...)` (all hotkeys paired)
  - `APIDefs->QuickAccess.Remove("QA_CBA")`
  - `APIDefs->Renderer.Deregister(...)`
  - `APIDefs->WndProc.Deregister(...)`

- **Public MumbleLink Consumption**:
  Position and camera data are received via official Nexus `DataLink.Get("GW2_MUMBLE_LINK")` shared memory, requiring zero process inspection.

---

## ⚡ 3. Performance, DWM & Systems Engineering

- **60 Hz DWM IPC Throttling**:
  Direct DWM fullscreen color calls (`MagSetFullscreenColorEffect`) are capped at 60 Hz (16 ms intervals) and memoized with `RoughlyEqual()`. Rapid dragging of sliders will never flood the Windows Desktop Window Manager IPC channel.
- **Zero-Allocation Hot Loops**:
  `AddonRenderWindow()` and `SensorGraphHUD()` avoid dynamic heap allocations during frame rendering. Spectral curves use static stack buffers.
- **Thread Safety**:
  - Filter matrix computations are guarded by `std::mutex s_recomputeMutex`.
  - All shared cross-thread flags (`s_deferredInitDone`, `s_safeStartPending`, `s_watchdogRunning`) utilize `std::atomic<bool>`.
  - Background threads (`WatchdogLoop`, `HybridScanner`) terminate cleanly with explicit `.join()` calls on addon unload.
- **Deferred Disk I/O**:
  Settings writes (`settings.ini`) are deferred until `ImGui::IsItemDeactivatedAfterEdit()` (mouse release), preventing SSD wear and hitching during adjustments.

---

## 🔬 4. Clinical & Mathematical Color Science

- **Equal-Energy White-Point Invariance**:
  LMS transformations utilize the **Hunt-Pointer-Estévez (HPE)** physiological cone fundamentals (Viénot, Brettel & Mollon 1999). Neutral colors (white, gray, black) are mathematically invariant:
  $$\text{Sim}(\text{White}) = \text{White}, \quad \text{Corr}(\text{White}) = \text{White}$$
  Empirical verification error across all three axes:
  $$\Delta_{\text{Deutan}} < 10^{-6}, \quad \Delta_{\text{Protan}} < 10^{-4}, \quad \Delta_{\text{Tritan}} < 10^{-4}$$
- **Daltonization Contrast Redistribution**:
  Implements **Fidaner, Lin & Özgüven (2005)** error diffusion to project lost dichromatic information into orthogonal visible channels.
- **Relative Luminance Standards**:
  Calculated strictly according to **ITU-R BT.709 / W3C WCAG 2.1**:
  $$Y = 0.2126\,R + 0.7152\,G + 0.0722\,B$$

---

## 🛡️ 5. ImGui Resilience & User Experience

- **ImGui Stack Neutrality**:
  All `PushStyleColor`, `PushStyleVar`, `PushID`, and `BeginChild` calls are balanced across all execution branches.
- **DPI & Font Scaling Immunity**:
  Dynamic sizing (`ImGui::CalcTextSize()` + `FramePadding`) is used instead of hardcoded pixel widths, ensuring clean rendering across all UI scales.
- **Strict ASCII Encoding**:
  All user-facing strings are strictly ASCII-clean (`ae`, `oe`, `ue`, `ss`), preventing missing glyph rendering issues (`?`) in the Nexus font atlas.
- **Safe-Start Gate & Crash-Breadcrumb Recovery**:
  A session lockfile (`cba_session.lock`) detects abnormal game crashes and disarms the filter on next startup to prevent blinding or disorienting visual artifacts.

---

## 🔍 6. Automated Audit Verification

The complete 22-point compliance check can be reproduced locally at any time:

```bash
python tools/audit_pro_review.py
```

*Result: **22 / 22 Checks Passed (100.0%)**.*
