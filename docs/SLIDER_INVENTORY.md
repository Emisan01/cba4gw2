# CBA — Regler-Bestandsaufnahme (Phase 1)

Stand 2026-09-14. Antwort auf die Frage "Regler den Modulen zuordnen." Dies
ist die Bestandsaufnahme, keine Entscheidung — wo ein Regler künftig hinsoll,
klärt Emi noch. Jede Zeile hier ist durch Lesen des aktuellen Codes
entstanden (Dateien + Zeilen angegeben), nicht geraten.

---

## Modul: Farbkorrektur-Typ & Stärke (Core)

**`CurrentSettings.Type` (Protan/Deutan/Tritan) + `Mixed`** — drei Stellen
schreiben das live:

- Geführter Setup-Wizard (einmalig, Fragen beantworten):
  [MainWindowDashboard.cpp:520-559](../plugins/nexus/src/ui/MainWindowDashboard.cpp)
- Direkt-Regler (`typeBtnHUD` + Mixed-Radio):
  [SensorGraphHUD.cpp:841-872](../plugins/nexus/src/ui/SensorGraphHUD.cpp) —
  laut Codekommentar 2026-09-12 hierher verschoben ("Manual Direct Controls")
- Diagnose → Anwenden (Rayleigh/Moreland-Test im Okular, Button "Als
  CBA-Profil übernehmen"):
  [VisionLab.cpp:513-560](../plugins/nexus/src/ui/VisionLab.cpp)
- Schnellauswahl-Paare (`pairOption` → `applyDerivedProfile`):
  [MainWindowDashboard.cpp:442-459](../plugins/nexus/src/ui/MainWindowDashboard.cpp)

Kein Regel-9-Verstoß (jeder Pfad ist ein bewusst anderer Einstieg: geführt /
manuell / diagnostiziert), aber die Antwort auf "wo stelle ich den Typ ein"
hängt davon ab, in welchem Fenster man gerade ist.

**`Severity01` / `MixedRgSeverity01` / `MixedBySeverity01` (Stärke)** —
sauberer:

- Der einzige Live-Regler (`severitySlider`, Registry-gebunden über
  `ParameterRegistry`): [SensorGraphHUD.cpp:889-931](../plugins/nexus/src/ui/SensorGraphHUD.cpp)
- Alles andere setzt nur einen Zielwert (Dashboard-Presets, Vision-Lab-Apply,
  Setup-Wizard-Abschluss) — kein zweiter Regler, nur Ergebnis-Zuweisungen.

**`EnhancerTolerance` (Erkennungsradius):**
[SensorGraphHUD.cpp:940-959](../plugins/nexus/src/ui/SensorGraphHUD.cpp) —
laut Codekommentar von "Main Window Section 1" hierher verschoben, 2026-09-12.

**Profil-Slots (Speichern/Laden/Aktiv-Anzeige):** ausschließlich
[MainWindowDashboard.cpp:660ff](../plugins/nexus/src/ui/MainWindowDashboard.cpp).

**Commander-Tag-Konflikt-Anzeige (9 Kreise, read-only):**
[MainWindowDashboard.cpp:389-431](../plugins/nexus/src/ui/MainWindowDashboard.cpp).

---

## Modul: Eye Comfort (Advanced)

- **`GammaGain`:** Haupt-Zuhause
  [MainWindowEyeComfort.cpp:87](../plugins/nexus/src/ui/MainWindowEyeComfort.cpp),
  zusätzlich Registry-gebundenes Zweit-Zuhause
  [SensorGraphHUD.cpp:1084-1097](../plugins/nexus/src/ui/SensorGraphHUD.cpp)
  ("Manuelle Helligkeit", nur wenn Auto-Sync aus) — laut Codekommentar
  bewusst über `ParameterRegistry` geteilt, damit Speicher und Clamp eine
  Quelle bleiben. Das ist die von Regel 9 erlaubte Ausnahme, kein Fund.
- **`BlueFilter01` / `WarmTint01` / `SaturationReduction01`:** ausschließlich
  [MainWindowEyeComfort.cpp:211-213](../plugins/nexus/src/ui/MainWindowEyeComfort.cpp)
  über den `eyeSlider`-Helper — ein sauberes Zuhause.
- **`EyeComfortModeEnabled`, `AutoBrightness`:** Checkboxen in
  MainWindowEyeComfort.cpp (Haupt) und SensorGraphHUD.cpp (Auto-Sync,
  gleiches Feld `AutoBrightness`).

---

## Modul: Mini-HUD Darstellung

- **`ShowMiniHUD`, `MiniHudBgAlpha`, `MiniHudTitleBar`, `MiniHudBorders`:**
  alle in [MainWindowSystem.cpp:127-142](../plugins/nexus/src/ui/MainWindowSystem.cpp)
  — ein Zuhause, aber fachlich unter "System" einsortiert, nicht unter einem
  eigenen Mini-HUD-Modul.
- **`CurrentSettings.UiOpacity`** ("Fenster-Deckkraft", das CBA-Hauptfenster
  selbst, nicht Mini-HUD):
  [SensorGraphHUD.cpp:471-479](../plugins/nexus/src/ui/SensorGraphHUD.cpp).
  Heißt im UI-Text ähnlich wie `MiniHudBgAlpha` ("Opacity"/"Deckkraft"),
  ist aber ein anderes Setting für ein anderes Fenster — leicht verwechselbar.

---

## Modul: System / Integration

Alle in [MainWindowSystem.cpp](../plugins/nexus/src/ui/MainWindowSystem.cpp):
`SystemWide` (DWM screen-wide), `SyncArcDpsTheme`, `EnableHybridMode`,
`DebugMode`, UI-Theme-Combo (Zeilen 85-187).

---

## Modul: Sichtbarkeit / Zugriff

- **`Enabled`, `ShowQuickAccessIcon`:** im Container-Fenster selbst, nicht in
  einem Tab — [MainWindow.cpp:50-59](../plugins/nexus/src/ui/MainWindow.cpp).
- **Autostart (Checkbox + `AutoStartSlot`-Radios):**
  [MainWindowDashboard.cpp:100-143](../plugins/nexus/src/ui/MainWindowDashboard.cpp).

---

## Modul: Filter Lab (Phase-4-Vorstufe)

Pro Layer, nicht Registry-gebunden (eigene Struktur `curF`), alle in
[FilterLab.cpp](../plugins/nexus/src/ui/FilterLab.cpp): `Enabled` (333),
`ReplaceRgb` via ColorEdit3 (384), `ToleranceTones` (509), `Diffusion` (526).
Dazu Kontrast-Paar-Combo (53) + Kompensations-Slider (160), unabhängig von
den Filter-Layern.

---

## Modul: Vision Lab (Sandbox — keine Live-Regler)

Alle folgenden sind lokale `static`-Variablen in VisionLab.cpp, wirken nur
auf die Simulation im Fenster selbst, **nicht** auf `CurrentSettings`:
`s_rayleighMix`, `s_rayleighLuma`, `s_morelandMix`, `s_morelandLuma`,
`s_ambientLight`, `s_repType`, `s_repSeverity`, `s_plateType`, `s_sceneIdx`,
`s_anomPreviewFilter`, `s_plateFilter`. Einzige Ausnahme: der Button "Als
CBA-Profil übernehmen" (siehe oben, Modul Farbkorrektur-Typ). Für die
Modulzuordnung kein Handlungsbedarf, der Vollständigkeit halber gelistet.

---

## Offene Fragen für Emi (nicht von mir zu entscheiden)

1. **Type/Mixed/Severity manuell einstellen:** bleibt das im Sensor Graph
   HUD, oder soll es zurück ins Hauptfenster? Aktuell hat "wo stelle ich das
   ein" drei verschiedene Antworten je nach Fenster (siehe oben).
2. **Mini-HUD-Anzeigeoptionen** sitzen unter "System" — eigenes
   Mini-HUD-Modul sinnvoller?
3. **`UiOpacity` vs. `MiniHudBgAlpha`:** beide heißen im UI-Text
   "Deckkraft"/"Opacity", sind aber unterschiedliche Settings für
   unterschiedliche Fenster. Umbenennen zur Klarheit?
