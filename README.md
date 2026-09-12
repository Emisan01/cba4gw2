# cba4gw2 — Color Balance Assist for Guild Wars 2

**Tell commander tags apart in a zerg. Take the strain off your eyes on long sessions.**

A 100% hookless Guild Wars 2 addon for the [Nexus](https://raidcore.gg/Nexus) addon loader. It corrects colour and contrast in real time through the Windows Magnification API — no DirectX hooks, no shader injection, no game-memory access.

📦 **[All Releases](https://github.com/Emisan01/cba4gw2/releases)** · ⬇️ **[Download cba.dll (latest)](https://github.com/Emisan01/cba4gw2/releases/latest/download/cba.dll)** — always points at the current release.

---

## 👁️ What it actually does for you

**Commander tags you can tell apart.** In a big zerg the squad tag colours sit right next to each other. If you have a red-green or blue-yellow deficiency, several of them collapse into the same colour. CBA measures which of the nine GW2 tag colours are genuinely confusable *for your vision*, and shifts only those — the ones you can already distinguish are left untouched.

**Eye comfort on long evenings.** An independent blue-light filter, warm tint and saturation reduction, applied on top of everything else. This one has nothing to do with colour blindness — it is simply the part most people end up leaving on.

**A colour correction for the whole screen**, based on published clinical models rather than a "make it more colourful" slider. See the science section below if that matters to you.

---

## 📦 Installation

1. Download [`cba.dll`](https://github.com/Emisan01/cba4gw2/releases/latest/download/cba.dll) from the [latest release](https://github.com/Emisan01/cba4gw2/releases/latest)
2. Drop it into your Nexus addons folder: `<Guild Wars 2>/addons/`
3. Start Guild Wars 2 through Nexus
4. Open **Nexus → Addons → cba4gw2 → Options**

---

## 🚀 Setting it up (about a minute)

You do **not** need to know whether you are protan, deutan or tritan. Most people never had that measured, and it is the wrong question to start from anyway.

Instead the panel asks what you can *see*:

1. **Which colour pair is hardest for you to tell apart?** — real GW2 tag colours, shown as they are. Your own eyes do the work.
2. *(red-green only)* **Does the red look much darker than the green?** — this is the one difference between protan and deutan that a person can actually answer about themselves.
3. **Can you tell them apart now?** — the same pair is shown *as the correction will render it*. If not, turn it up. That is how strength gets set: a yes/no you can answer, not a percentage you would have to guess.

If you already know your diagnosis, the direct type buttons are under **Advanced**.

**Hold `Ctrl+Shift+V`** at any time to suspend the filter while the key is down. That is the honest way to check whether it is doing anything — against the actual game, not a swatch. Every keybind is remappable in Nexus's own keybind settings.

---

## 🔧 Going further (optional)

Tick **Advanced Mode** to unlock the Studio: profile slots and shareable profile codes, a Filter Lab for building your own colour-replacement layers, a Vision Lab with a Nagel/Moreland anomaloscope and Anomalous Quotient calculation, live spectrum and transfer-curve graphs, and a filter-pipeline view showing exactly which stages are active and in what order.

None of it is needed for the two things above. It is there because some people want to see the machinery.

---

## 🔬 Scientific foundation

CBA implements a published, mathematically verifiable correction pipeline rather than an ad-hoc colour tweak:

* **Colour space:** sRGB is transformed into the physiological **LMS cone response space** using the **Hunt-Pointer-Estévez (HPE)** matrix.
* **Dichromacy simulation:** missing cone channels are projected per **Viénot, Brettel & Mollon (1999)**, which keeps the equi-energy neutral axis intact — white, grey and black stay exactly where they are.
* **Daltonization:** lost contrast is measured and redistributed into channels you can still see, using type-specific shift matrices per **Fidaner, Lin & Özgüven (2005)**.
* **Contrast reference:** ITU-R BT.709 / **WCAG 2.1** relative luminance, with HRR and Farnsworth-Munsell scales used for the clinical readouts.

Every claim there is checked by unit tests that assert the *properties* (white-point invariance, identity at zero severity, that the correction genuinely increases a dichromat's perceived separation) rather than re-encoding the implementation's own arithmetic.

👉 Full derivations, matrices and proofs: [`COLOR_MATH.md`](COLOR_MATH.md). The reasoning behind the UI: [`PRODUCT_CONCEPT.md`](PRODUCT_CONCEPT.md).

---

## 🛡️ Safety and ArenaNet policy

> Third-party addon, used at your own risk per ArenaNet's Third-Party Programs Policy — **no automation, no game-memory access, visual-only.**

CBA does not read or write Guild Wars 2 process memory, does not hook game functions, does not generate input, and does not touch the ArenaNet or ArcDPS crash reporters. It is a presentation-layer tool: it changes what the *screen* shows, through an official Windows API. See [`SECURITY_COMPLIANCE.md`](SECURITY_COMPLIANCE.md).

---

## 🛠️ Building from source

Requirements: **Visual Studio (MSVC v143 or newer)**, **CMake ≥ 3.20**, Windows SDK.

```powershell
cd plugins/nexus
cmake -S . -B build -A x64
cmake --build build --config Release
# Output: plugins/nexus/build/bin/Release/cba.dll
```

All dependencies (ImGui, Nexus API headers) are vendored in `thirdparty/` — nothing is downloaded during the build.

---

## 📜 License

MIT License — see [LICENSE](LICENSE).
