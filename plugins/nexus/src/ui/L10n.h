#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>
#include "Shared.h"

namespace cba
{
	struct L10n
	{
		const char* Enabled;
		const char* CorrectionProfile;
		const char* Protan;
		const char* Deutan;
		const char* Tritan;
		const char* Mixed;
		const char* Strength;
		const char* RgStrength;
		const char* ByStrength;
		const char* WindowMode;
		const char* ScreenshotNote;
		const char* Language;
		const char* Diagnosis;
		const char* DiagnosisHelp;
		const char* HybridMode;
		const char* HybridModeHelp;
		const char* ColorProfileGraph;
		const char* DiagnosticProfileRef;
		const char* DiagnosticHint;
		const char* CmdrEnhancer;
		const char* CmdrEnhancerDesc;
		const char* EnableEnhancer;
		const char* PresetsTitle;
		const char* TagRed;
		const char* TagOrange;
		const char* TagYellow;
		const char* TagGreen;
		const char* TagCyan;
		const char* TagBlue;
		const char* TagPurple;
		const char* TagPink;
		const char* TagWhite;

		const char* LangAuto;

		// Bottom Section
		const char* ProfileSummaryTitle;
		const char* ValuesLabel;
		const char* ClassificationLabel;
		const char* KeepActiveBackground;
		const char* KeepActiveBackgroundTooltip;
		const char* FocusWatchdogExclusive;
		const char* FocusWatchdogBackground;
		const char* DebugModeCheckbox;
		const char* MethodologyTitle;
		const char* MethodologyDesc;

		// Eye Comfort Section
		const char* EyeComfortHeader;
		const char* EyeComfortGammaSlider;
		const char* EyeComfortRetention;
		const char* EyeComfortApply;
		const char* EyeComfortHdrTooltip;

		// Windows, QuickAccess & Community Credits
		const char* OpenMainWindow;
		const char* OpenMainWindowTooltip;
		const char* OpenSensorGraph;
		const char* OpenSensorGraphTooltip;
		const char* ShowQuickAccess;
		const char* ShowQuickAccessTooltip;
		const char* CompactModeTip;
		const char* CommunityHeader;
		const char* CreditsBtn;
		const char* CreditsTooltip;

		// Collapsible Section Headers for Main Window
		const char* HeaderSection1;
		const char* HeaderSection2;
		const char* HeaderSection3;
		const char* HeaderSection4;
		const char* HeaderSection5;
		const char* HeaderSection6;
		const char* HeaderSection7;
		const char* HeaderSection8;
	};

	struct Gw2TagRef {
		const char* (*labelFunc)(const L10n&);
		float r, g, b; // Sampled from GW2 Tag Reference palette
	};

	static const Gw2TagRef kGw2TagRefs[9] = {
		{ [](const L10n& l) { return l.TagRed; },    0.851f, 0.275f, 0.235f }, // Rot #d9463c
		{ [](const L10n& l) { return l.TagOrange; }, 0.910f, 0.522f, 0.059f }, // Orange #e8850f
		{ [](const L10n& l) { return l.TagYellow; }, 0.910f, 0.753f, 0.125f }, // Gelb #e8c020
		{ [](const L10n& l) { return l.TagGreen; },  0.247f, 0.616f, 0.302f }, // Grün #3f9d4d
		{ [](const L10n& l) { return l.TagCyan; },   0.149f, 0.682f, 0.741f }, // Cyan #26aebd
		{ [](const L10n& l) { return l.TagBlue; },   0.212f, 0.439f, 0.800f }, // Blau #3670cc
		{ [](const L10n& l) { return l.TagPurple; }, 0.490f, 0.333f, 0.800f }, // Lila #7d55cc
		{ [](const L10n& l) { return l.TagPink; },   0.761f, 0.278f, 0.627f }, // Magenta #c247a0
		{ [](const L10n& l) { return l.TagWhite; },  0.902f, 0.902f, 0.902f }, // Weiss #e6e6e6
	};

	inline const char* DetectSystemLanguage()
	{
		LANGID lang = GetUserDefaultUILanguage();
		if (PRIMARYLANGID(lang) == LANG_GERMAN) return "de";
		if (PRIMARYLANGID(lang) == LANG_FRENCH) return "fr";
		if (PRIMARYLANGID(lang) == LANG_SPANISH) return "es";
		return "en";
	}

	inline const L10n& Strings()
	{
		static const L10n de{
			.Enabled = "Aktiv",
			.CorrectionProfile = "Farbabgleich-Profil",
			.Protan = "Protan",
			.Deutan = "Deutan",
			.Tritan = "Tritan",
			.Mixed = "Gemischt",
			.Strength = "Staerke",
			.RgStrength = "Rot-Gruen Staerke",
			.ByStrength = "Blau-Gelb Staerke",
			.WindowMode = "Fenstermodus",
			.ScreenshotNote = "Screenshots: GW2-intern wirkt vor dem Filter. PrintScreen / Win+PrintScreen und die meisten Display-Captures sehen den Filter.",
			.Language = "Sprache",
			.Diagnosis = "AQ/HRR Referenzwerte",
			.DiagnosisHelp = "Kalibrierungs- und Benchmarkwerte fuer praezise Farbanpassung.",
			.HybridMode = "Hybrid Modus (Beta)",
			.HybridModeHelp = "Kinematic Fader: Blendet das Overlay bei schnellen Kamerabewegungen automatisch sanft aus.\nLiest den GW2 Render-Buffer im Hintergrund, um WCAG-Fehler zu erkennen.",
			.ColorProfileGraph = "Spektral-Farbkanalverlauf (R/G/B)",
			.DiagnosticProfileRef = "Profil-Referenzwerte (AQ/HRR):",
			.DiagnosticHint = "Hinweis: Trage hier z.B. Farnsworth-Munsell Scores, HRR-Werte oder AQ-Indizes ein, um das Farbprofil exakt zu kalibrieren.",
			.CmdrEnhancer = "Commander Tag Enhancer (Symbol-Unterscheidung)",
			.CmdrEnhancerDesc = "Ersetzt bestimmte GW2-Symbole durch extrem kontrastreiche Signalfarben.",
			.EnableEnhancer = "Enhancer Aktivieren",
			.PresetsTitle = "Voreinstellungen (Metabattle)",
			.TagRed = "Tag Rot",
			.TagOrange = "Tag Orange",
			.TagYellow = "Tag Gelb",
			.TagGreen = "Tag Gruen",
			.TagCyan = "Tag Cyan",
			.TagBlue = "Tag Blau",
			.TagPurple = "Tag Lila",
			.TagPink = "Tag Magenta",
			.TagWhite = "Tag Weiss",

			.LangAuto = "Auto",

			// Bottom Section
			.ProfileSummaryTitle = "Profil-Zusammenfassung (Live-Feedback):",
			.ValuesLabel = "Werte:",
			.ClassificationLabel = "Farbabgleich:",
			.KeepActiveBackground = " Filter auch im Hintergrund aktiv lassen (z. B. bei Klick in Browser / 2. Monitor)",
			.KeepActiveBackgroundTooltip = "Standard (Deaktiviert): Sobald GW2 den Fokus verliert (z. B. Klick in den Browser auf Monitor 2 oder Alt-Tab), pausiert der Filter sofort, damit andere Programme nicht beeinflusst werden.\n\nAktiviert: Laesst den Filter auch weiterlaufen, wenn ein anderes Fenster aktiv ist.\nHinweis: Bei Minimieren von GW2 pausiert der Filter in jedem Fall sofort.",
			.FocusWatchdogExclusive = "Fokus-Waechter: Filter ist exklusiv an GW2 gebunden und pausiert bei Alt-Tab/Klick auf 2. Monitor.",
			.FocusWatchdogBackground = "Hintergrund-Modus: Filter bleibt auch bei Fokusverlust aktiv (pausiert nur bei Minimieren).",
			.DebugModeCheckbox = "Entwickler- & Debug-Modus (Performance Watchdog)",
			.MethodologyTitle = "Methodik & Referenzen:",
			.MethodologyDesc = "  - Daltonisierung: Fidaner et al. (2005)   - LMS-Dichromasie: Vienot, Brettel & Mollon (1999)\n  - Hunt-Pointer-Estevez (HPE) Farbraum   - W3C WCAG 2.1 Farbkontrast",

			// Eye Comfort Section
			.EyeComfortHeader = "Eye Comfort (Helligkeit)",
			.EyeComfortGammaSlider = "Helligkeitsanpassung",
			.EyeComfortRetention = "Helligkeitserhalt: %.1f%%  (Empfohlen: %.2fx)",
			.EyeComfortApply = "Empfehlung uebernehmen",
			.EyeComfortHdrTooltip = "beeinflusst nur die Anzeige-Berechnung, nicht die Farbkorrektur selbst",

			// Windows, QuickAccess & Community Credits
			.OpenMainWindow = "Hauptfenster",
			.OpenMainWindowTooltip = "Oeffnet oder schliesst das CBA Hauptfenster (Strg+Shift+C)",
			.OpenSensorGraph = "Sensor-Graph",
			.OpenSensorGraphTooltip = "Oeffnet oder schliesst das Sensor- & Graph-HUD (Strg+Shift+G)",
			.ShowQuickAccess = "CBA-Symbol in Schnellzugriff anzeigen",
			.ShowQuickAccessTooltip = "Blendet das CBA-Icon in der oberen Nexus-Schnellstartleiste ein oder aus.",
			.CompactModeTip = "Kompaktmodus: Einstellungen und Sensor-Graphen laufen in eigenen Fenstern.",
			.CommunityHeader = "Mit Liebe fuer die Tyria-Community entwickelt - Barrierefreie Farboptimierung fuer GW2",
			.CreditsBtn = "Credits <3",
			.CreditsTooltip = "Danksagung an die Community, Unterstuetzer & Raider",

			// Section Headers
			.HeaderSection1 = "1. Farbprofil & Balance",
			.HeaderSection2 = "2. Eye Comfort (Helligkeit)",
			.HeaderSection3 = "3. Spiel- & Fenstermodus",
			.HeaderSection4 = "4. Hybrid Modus (Beta)",
			.HeaderSection5 = "5. Filter-Labor & Experimentierfeld",
			.HeaderSection6 = "6. Ueber CBA, Referenzen & Credits",
			.HeaderSection7 = "",
			.HeaderSection8 = ""
		};

		static const L10n en{
			.Enabled = "Enabled",
			.CorrectionProfile = "Color Balance Profile",
			.Protan = "Protan",
			.Deutan = "Deutan",
			.Tritan = "Tritan",
			.Mixed = "Mixed",
			.Strength = "Strength",
			.RgStrength = "Red-Green strength",
			.ByStrength = "Blue-Yellow strength",
			.WindowMode = "Window mode",
			.ScreenshotNote = "Screenshots: GW2's internal capture sees the scene before the filter. PrintScreen / Win+PrintScreen and most display captures see the filter.",
			.Language = "Language",
			.Diagnosis = "AQ/HRR Reference Values",
			.DiagnosisHelp = "Calibration and benchmark values for precise color balancing.",
			.HybridMode = "Hybrid Mode (Beta)",
			.HybridModeHelp = "Kinematic Fader: Automatically fades out the overlay during fast camera movements.\nReads the GW2 render buffer in the background to show WCAG errors.",
			.ColorProfileGraph = "Color Profile Graph: Transfer function of R/G/B channels",
			.DiagnosticProfileRef = "Profile Reference Values (AQ/HRR):",
			.DiagnosticHint = "Hint: Enter Farnsworth-Munsell scores, HRR values, or AQ indices to calibrate this color profile.",
			.CmdrEnhancer = "Commander Tag Enhancer",
			.CmdrEnhancerDesc = "Replaces specific GW2 symbols with high-contrast signal colors.",
			.EnableEnhancer = "Enable Enhancer",
			.PresetsTitle = "Presets",
			.TagRed = "Red Tag",
			.TagOrange = "Orange Tag",
			.TagYellow = "Yellow Tag",
			.TagGreen = "Green Tag",
			.TagCyan = "Cyan Tag",
			.TagBlue = "Blue Tag",
			.TagPurple = "Purple Tag",
			.TagPink = "Magenta Tag",
			.TagWhite = "White Tag",

			.LangAuto = "Auto",

			// Bottom Section
			.ProfileSummaryTitle = "Profile Summary (Live Feedback):",
			.ValuesLabel = "Values:",
			.ClassificationLabel = "Color Balance:",
			.KeepActiveBackground = " Keep filter active in background (e.g. browser / 2nd monitor)",
			.KeepActiveBackgroundTooltip = "Default (Disabled): As soon as GW2 loses focus (e.g. clicking browser on 2nd monitor or Alt-Tab), the filter pauses immediately to avoid tinting other applications.\n\nEnabled: Keeps the filter active even when another window has focus.\nNote: Minimizing GW2 always pauses the filter immediately.",
			.FocusWatchdogExclusive = "Focus Watchdog: Filter is bound exclusively to GW2 and pauses on Alt-Tab / 2nd monitor focus.",
			.FocusWatchdogBackground = "Background Mode: Filter remains active when focus is lost (only pauses when minimized).",
			.DebugModeCheckbox = "Developer & Debug Mode (Performance Watchdog)",
			.MethodologyTitle = "Methodology & References:",
			.MethodologyDesc = "  - Daltonization: Fidaner et al. (2005)   - LMS Dichromacy: Vienot, Brettel & Mollon (1999)\n  - Hunt-Pointer-Estevez (HPE) Color Space   - W3C WCAG 2.1 Color Contrast",

			// Eye Comfort Section
			.EyeComfortHeader = "Eye Comfort (Brightness)",
			.EyeComfortGammaSlider = "Brightness Adjust",
			.EyeComfortRetention = "Brightness Retention: %.1f%%  (Recommended: %.2fx)",
			.EyeComfortApply = "Apply Recommendation",
			.EyeComfortHdrTooltip = "affects display calculation only, not color correction itself",

			// Windows, QuickAccess & Community Credits
			.OpenMainWindow = "Main Window",
			.OpenMainWindowTooltip = "Opens or closes the CBA Main Window (Ctrl+Shift+C)",
			.OpenSensorGraph = "Sensor Graph",
			.OpenSensorGraphTooltip = "Opens or closes the Sensor & Graph HUD (Ctrl+Shift+G)",
			.ShowQuickAccess = "Show CBA icon in quick access bar",
			.ShowQuickAccessTooltip = "Shows or hides the CBA icon in the top Nexus quick access bar.",
			.CompactModeTip = "Compact Mode: Settings and sensor graphs run in separate windows.",
			.CommunityHeader = "Crafted with love for the Tyrian community - A Color Balance Assist and Enhancer for GW2",
			.CreditsBtn = "Credits <3",
			.CreditsTooltip = "Credits to community, supporters & raiders",

			// Section Headers
			.HeaderSection1 = "1. Color Profile & Balance",
			.HeaderSection2 = "2. Eye Comfort (Brightness)",
			.HeaderSection3 = "3. Game & Window Mode",
			.HeaderSection4 = "4. Hybrid Mode (Beta)",
			.HeaderSection5 = "5. Filter Lab & Experiment Field",
			.HeaderSection6 = "6. About CBA, References & Credits",
			.HeaderSection7 = "",
			.HeaderSection8 = ""
		};

		if (CurrentSettings.Language == 2) return de; // Deutsch (explicit)
		if (CurrentSettings.Language == 0)            // System Language (Windows)
		{
			return (strcmp(DetectSystemLanguage(), "de") == 0) ? de : en;
		}
		return en; // English (1, default)
	}

	inline bool IsGerman()
	{
		if (CurrentSettings.Language == 2) return true;
		if (CurrentSettings.Language == 0) return (strcmp(DetectSystemLanguage(), "de") == 0);
		return false;
	}
}
