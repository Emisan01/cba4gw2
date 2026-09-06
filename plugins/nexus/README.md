# cba4gw2 Nexus Plugin

This directory contains the source code for `cba.dll`, a standalone, hardware-accelerated color assistance plugin for the [GW2 Nexus](https://raidcore.gg/Nexus) addon loader.

---

## Features

- **Fixed Header Bar & Quick Controls:**
  - High-visibility **Master ON / OFF Toggle** with animated pulse indicator.
  - **3-Slot Profile Quickbar** with real-time state badges (dim grey = empty, green = saved/active, bright orange = slot full).
  - Quick **Reset UI** button to instantly snap all detached windows back to standard positions.
- **Scientific Color Correction (Weg A — 100% Hookless):**
  - Clinical Daltonization profiles: **Protanopia / Protanomaly**, **Deuteranopia / Deuteranomaly**, **Tritanopia / Tritanomaly**, and **Mixed** mode.
  - Mathematically grounded LMS cone projection (Viénot, Brettel & Mollon 1999) and contrast redistribution (Fidaner, Lin & Özgüven 2005).
  - Zero game hooks, zero shader injection, zero account risk via the Windows Magnification DWM pipeline.
- **Detachable Filter Laboratory (`Filter-Labor`):**
  - Interactive **XY Color Ray Matrix** with real-time tone mapping.
  - Stackable custom filter instances with adjustable tone tolerance radius ($\pm 1$ to $\pm 32$ tones) and soft diffusion feathering.
  - Selective actions: Signal Color Replacement, Auto-Complementary, Invert, Luminance Boost.
- **Commander Tag & Squad UI Enhancer:**
  - Target-specific contrast enhancement for squad commander tags (Blue, Red, Green) in large Zergs and WvW encounters.
- **Adaptive Eye Comfort & Auto-Brightness:**
  - Dynamic gamma and luminance adaptation to protect eyesight during rapid transitions between dark dungeons and bright snowy environments.
- **Triple Spectral Visualization:**
  - 3 selectable graph rendering modes: **Polygonal (PWL)**, **Harmonisch (Gauss/LMS)**, and **Strahlen (Ray Scope)**.
  - Real-time 32-sample RGB transfer curves and full-spectrum filtered beam display.
- **Smart Event-Driven Auto-Sleep (`WndProc`):**
  - Instant neutral color restoration on Alt-Tab (`WM_ACTIVATE` / `WA_INACTIVE`) or minimize.
  - Automatic restoration when returning to Guild Wars 2.
- **Ergonomic Default Hotkeys (Nexus Managed):**
  - `Ctrl + Shift + C`: Toggle CBA Main Options Window
  - `Ctrl + Shift + G`: Toggle Sensor Graph Window
  - `Ctrl + Shift + O`: Filter Emergency Off (instantly deactivates active correction)
- **100% Clean ASCII Typography:**
  - Optimized for Dear ImGui default font rendering with centered button labels and guaranteed zero missing glyphs (`?`).
  - Bilingual German / English language support.

---

## Build from Source

Requirements: **Visual Studio 2022 (MSVC v143)**, **CMake ≥ 3.20**, Windows 10/11 SDK.

```powershell
cd plugins/nexus
cmake -S . -B build -A x64
cmake --build build --config Release
```

The resulting `cba.dll` is compiled to `build/bin/Release/cba.dll`. All dependencies (Dear ImGui, Nexus API headers) are vendored in `thirdparty/` — zero external package downloads needed.

---

## Installation

1. Download `cba.dll` from [Releases](https://github.com/Emisan01/cba4gw2/releases).
2. Copy `cba.dll` into your Nexus addons folder: `<Guild Wars 2>/addons/cba.dll`.
3. Launch Guild Wars 2 through Nexus.
4. Press `Ctrl + Shift + C` or click the CBA icon in the Nexus top bar to open options.
5. Ensure Guild Wars 2 is set to **Windowed** or **Windowed Fullscreen (Borderless)** in Graphics Options (Exclusive Fullscreen bypasses Desktop Window Manager filters).
