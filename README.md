# cba4gw2 — Color Balance Assist for Guild Wars 2 (v1.0)

A high-performance, 100% hookless Guild Wars 2 addon for the [Nexus](https://raidcore.gg/Nexus) addon loader. Applies a real-time, hardware-accelerated color balance and contrast assistance filter [...]

---

## 🔬 Scientific & Mathematical Foundation

cba4gw2 v1.0 implements a mathematically verified, clinically grounded color correction pipeline:

* **Color Space:** Standard sRGB is transformed into the physiological **LMS (Long, Medium, Short) cone response space** using the **Hunt-Pointer-Estévez (HPE)** conversion matrix.
* **Dichromacy Simulation:** Missing cone channels are projected according to **Viénot, Brettel & Mollon (1999)**, ensuring that the equi-energy neutral axis (white, gray, black) remains complete[...]
* **Daltonization Compensation:** Lost contrasts are calculated and redistributed into visible channels using type-specific shift matrices based on **Fidaner, Lin & Özgüven (2005)**.
* **Contrast Standards:** Optimized in accordance with **W3C WCAG 2.1** color contrast recommendations and **HRR / Farnsworth-Munsell** clinical scales.

👉 **For complete formulas, matrix proofs, and mathematical derivations, see [`COLOR_MATH.md`](COLOR_MATH.md).**

---

## ⚡ Key Features (v1.0)

- **100% Hookless & Safe (Weg A):** Zero DirectX / D3D11 present hooks, zero shader injection, zero game memory tampering. Applies strictly via the Windows Magnification API (`MagSetFullscreenColo[...]
- **Color Balance Profiles:**
  - **Protan** (Rot-Fokus / Red Contrast Focus)
  - **Deutan** (Grün-Fokus / Green Contrast Focus)
  - **Tritan** (Blau-Fokus / Blue Contrast Focus)
  - **Mixed Mode** (Unabhängige Rot-Grün- und Blau-Gelb-Farbbalance)
- **Event-Driven Auto-Sleep (`WndProc`):**
  - Instant neutral color restoration when tabbing out (`WM_ACTIVATE` / `WA_INACTIVE`) or minimizing (`WM_SIZE` / `SIZE_MINIMIZED`).
  - Instant filter reactivation when returning to GW2.
  - Transparent message passthrough ensuring no mouse, keyboard, or context menu clicks are swallowed.
- **Performance & I/O Throttling:**
  - **DWM IPC Capped at 60 Hz:** Sliders can be dragged rapidly without causing Desktop Window Manager stutter.
  - **Deferred Disk Writes:** Settings are written to disk only when sliders are released or buttons are clicked, preventing unnecessary SSD I/O.
- **Modern Interactive UI & Controls:**
  - **Fixed Header Bar:** Always-accessible Master ON/OFF toggle with animated pulse indicator, 3-slot Profile Quickbar (color-coded empty/saved/full states), and one-click `Reset UI` button.
  - **Detachable Filter Laboratory (`Filter-Labor`):** Interactive XY Color Ray Matrix with live tone mapping, stackable custom filter instances with tone radius ($\pm 1$ to $\pm 32$) and soft dif[...]
  - **Clinical Vision Lab (`Vision-Lab`):** Interactive Nagel & Moreland Anomaloscope with split eyepiece disc and live Anomalous Quotient (AQ) calculation, ICD-10 medical report translator, HRR/I[...]
  - **Commander Tag & Squad UI Enhancer:** Contrast amplification for squad commander tags (Blue/Red/Green) in large Zergs and WvW.
  - **Adaptive Eye Comfort:** Dynamic brightness and gamma compensation to prevent eye strain between bright and dark game environments.
  - **3 Scientific Graph Modes:** Polygonal (PWL), Harmonisch (Gauss/LMS), and Strahlen (Ray Scope) with 32-sample transfer curves and live filtered spectrum beam.
  - **Spacious Live-Feedback Status Card:** Displays active profile, exact percentages, and clinical HRR / Farnsworth severity classifications.
  - **Ergonomic Hotkeys:** `Ctrl+Shift+C` (Toggle Main UI), `Ctrl+Shift+G` (Toggle Sensor Graph), `Ctrl+Shift+O` (Filter Emergency Off).
  - **100% Clean ASCII Typography:** Fully centered button labels with generous margins and zero font glyph rendering glitches (`?`).
  - **Bilingual:** Automatic system detection for German and English with manual overrides.

---

## 📦 Installation

1. Download `cba.dll` from the latest Release: https://github.com/Emisan01/cba4gw2/releases/download/v1.0.2-pre/cba.dll (v1.0.2-pre)
2. Place `cba.dll` into your Nexus addons folder:
   `<Guild Wars 2>/addons/`
3. Launch Guild Wars 2 through Nexus.
4. In Nexus → **Addons** → **cba4gw2**, click **Options** to configure your profile.

> **Important:** Set Guild Wars 2 to **Windowed** or **Windowed Fullscreen (Borderless)** in Graphics Options. The Windows Magnification API cannot apply to Exclusive Fullscreen windows.
>
> **ArenaNet Policy Notice:** Third-party addon, used at your own risk per ArenaNet's Third-Party Programs Policy — no automation, no game-memory access, visual-only.

---

## 🛠️ Building from Source

Requirements: **Visual Studio 2022 (MSVC v143)**, **CMake ≥ 3.20**, Windows SDK.

```powershell
cd plugins/nexus
cmake -S . -B build -A x64
cmake --build build --config Release
# Output binary: plugins/nexus/build/bin/Release/cba.dll
```

All dependencies (ImGui, Nexus API headers) are vendored in `thirdparty/` — zero external package downloads required during build.

---

## 📋 Scope-Freeze & Roadmap Notes (v1.0)

To guarantee rock-solid stability and account safety, the following boundaries are enforced for v1.0:
* **In v1.0:** 100% Hookless Magnification API (Weg A), Viénot/HPE/Fidaner Daltonization, Live Curves, Live Beam, Live Feedback Status Card, DWM Throttling, Deferred Save, WndProc Auto-Sleep.
* **Deferred to future updates:** In-engine D3D11 Present hooks (Weg B), selective pixel-shader hue rotation (Smart Enhancer), pipette color pickers, and dynamic scene histogram analysis.

---

## 📜 License

MIT License — see [LICENSE](LICENSE).
