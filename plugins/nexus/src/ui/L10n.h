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
		const char* SmartEnhancer;
		const char* SmartEnhancerDesc;

		// Bottom Section
		const char* LoadOnStartup;
		const char* LoadOnStartupTooltip;
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
			"Aktiv",
			"Farbabgleich-Profil",
			"Protan",
			"Deutan",
			"Tritan",
			"Gemischt",
			"Staerke",
			"Rot-Gruen Staerke",
			"Blau-Gelb Staerke",
			"Fenstermodus",
			"Screenshots: GW2-intern wirkt vor dem Filter. PrintScreen / Win+PrintScreen und die meisten Display-Captures sehen den Filter.",
			"Sprache",
			"AQ/HRR Referenzwerte",
			"Kalibrierungs- und Benchmarkwerte fuer praezise Farbanpassung.",
			"Hybrid Modus (Beta)",
			"Kinematic Fader: Blendet das Overlay bei schnellen Kamerabewegungen automatisch sanft aus.\nLiest den GW2 Render-Buffer im Hintergrund, um WCAG-Fehler zu erkennen.",
			"Spektral-Farbkanalverlauf (R/G/B)",
			"Profil-Referenzwerte (AQ/HRR):",
			"Hinweis: Trage hier z.B. Farnsworth-Munsell Scores, HRR-Werte oder AQ-Indizes ein, um das Farbprofil exakt zu kalibrieren.",
			"Commander Tag Enhancer (Symbol-Unterscheidung)",
			"Ersetzt bestimmte GW2-Symbole durch extrem kontrastreiche Signalfarben.",
			"Enhancer Aktivieren",
			"Voreinstellungen (Metabattle)",
			"Tag Rot",
			"Tag Orange",
			"Tag Gelb",
			"Tag Gruen",
			"Tag Cyan",
			"Tag Blau",
			"Tag Lila",
			"Tag Magenta",
			"Tag Weiss",

			"Auto",
			"Smart-Enhancer: Symbole automatisch anpassen",
			"Passt Commander-Tags und Wegmarker automatisch an das oben gewaehlte Farbprofil an.",

			// Bottom Section
			" Beim Spielstart laden (Load on Startup)",
			"Aktiviert: Der gespeicherte Filterzustand wird beim Starten von GW2 geladen.\nDeaktiviert: Der Filter startet bei Spielstart immer inaktiv/neutral (kein ungewollter Farbstich).",
			"Profil-Zusammenfassung (Live-Feedback):",
			"Werte:",
			"Farbabgleich:",
			" Filter auch im Hintergrund aktiv lassen (z. B. bei Klick in Browser / 2. Monitor)",
			"Standard (Deaktiviert): Sobald GW2 den Fokus verliert (z. B. Klick in den Browser auf Monitor 2 oder Alt-Tab), pausiert der Filter sofort, damit andere Programme nicht beeinflusst werden.\n\nAktiviert: Laesst den Filter auch weiterlaufen, wenn ein anderes Fenster aktiv ist.\nHinweis: Bei Minimieren von GW2 pausiert der Filter in jedem Fall sofort.",
			"Fokus-Waechter: Filter ist exklusiv an GW2 gebunden und pausiert bei Alt-Tab/Klick auf 2. Monitor.",
			"Hintergrund-Modus: Filter bleibt auch bei Fokusverlust aktiv (pausiert nur bei Minimieren).",
			"Entwickler- & Debug-Modus (Performance Watchdog)",
			"Methodik & Referenzen:",
			"  - Daltonisierung: Fidaner et al. (2005)   - LMS-Dichromasie: Vienot, Brettel & Mollon (1999)\n  - Hunt-Pointer-Estevez (HPE) Farbraum   - W3C WCAG 2.1 Farbkontrast",

			// Eye Comfort Section
			"Eye Comfort (Helligkeit)",
			"Helligkeitsanpassung",
			"Helligkeitserhalt: %.1f%%  (Empfohlen: %.2fx)",
			"Empfehlung uebernehmen",
			"beeinflusst nur die Anzeige-Berechnung, nicht die Farbkorrektur selbst",

			// Windows, QuickAccess & Community Credits
			"Hauptfenster",
			"Oeffnet oder schliesst das CBA Hauptfenster (Strg+Shift+C)",
			"Sensor-Graph",
			"Oeffnet oder schliesst das Sensor- & Graph-HUD (Strg+Shift+G)",
			"CBA-Symbol in Schnellzugriff anzeigen",
			"Blendet das CBA-Icon in der oberen Nexus-Schnellstartleiste ein oder aus.",
			"Kompaktmodus: Einstellungen und Sensor-Graphen laufen in eigenen Fenstern.",
			"Mit Liebe fuer die Tyria-Community entwickelt - Barrierefreie Farboptimierung fuer GW2",
			"Credits <3",
			"Danksagung an die Community, Unterstuetzer & Raider",

			// Section Headers
			"1. Farbprofil & Balance",
			"2. Commander-Tag & Kontrast-Profile",
			"3. Eye Comfort (Helligkeit)",
			"4. Spiel- & Fenstermodus",
			"5. Hybrid Modus (Beta)",
			"6. Filter-Labor & Experimentierfeld",
			"7. Ueber CBA, Referenzen & Credits",
			""
		};

		static const L10n en{
			"Enabled",
			"Color Balance Profile",
			"Protan",
			"Deutan",
			"Tritan",
			"Mixed",
			"Strength",
			"Red-Green strength",
			"Blue-Yellow strength",
			"Window mode",
			"Screenshots: GW2's internal capture sees the scene before the filter. PrintScreen / Win+PrintScreen and most display captures see the filter.",
			"Language",
			"AQ/HRR Reference Values",
			"Calibration and benchmark values for precise color balancing.",
			"Hybrid Mode (Beta)",
			"Kinematic Fader: Automatically fades out the overlay during fast camera movements.\nReads the GW2 render buffer in the background to show WCAG errors.",
			"Color Profile Graph: Transfer function of R/G/B channels",
			"Profile Reference Values (AQ/HRR):",
			"Hint: Enter Farnsworth-Munsell scores, HRR values, or AQ indices to calibrate this color profile.",
			"Commander Tag Enhancer",
			"Replaces specific GW2 symbols with high-contrast signal colors.",
			"Enable Enhancer",
			"Presets",
			"Red Tag",
			"Orange Tag",
			"Yellow Tag",
			"Green Tag",
			"Cyan Tag",
			"Blue Tag",
			"Purple Tag",
			"Magenta Tag",
			"White Tag",

			"Auto",
			"Smart-Enhancer: Adjust symbols automatically",
			"Automatically adjusts Commander Tags and waymarkers based on the selected color profile above.",

			// Bottom Section
			" Load on Startup",
			"Enabled: Saved filter profile is restored when Guild Wars 2 launches.\nDisabled: Filter starts inactive/neutral at launch to prevent unintended color shifts.",
			"Profile Summary (Live Feedback):",
			"Values:",
			"Color Balance:",
			" Keep filter active in background (e.g. browser / 2nd monitor)",
			"Default (Disabled): As soon as GW2 loses focus (e.g. clicking browser on 2nd monitor or Alt-Tab), the filter pauses immediately to avoid tinting other applications.\n\nEnabled: Keeps the filter active even when another window has focus.\nNote: Minimizing GW2 always pauses the filter immediately.",
			"Focus Watchdog: Filter is bound exclusively to GW2 and pauses on Alt-Tab / 2nd monitor focus.",
			"Background Mode: Filter remains active when focus is lost (only pauses when minimized).",
			"Developer & Debug Mode (Performance Watchdog)",
			"Methodology & References:",
			"  - Daltonization: Fidaner et al. (2005)   - LMS Dichromacy: Vienot, Brettel & Mollon (1999)\n  - Hunt-Pointer-Estevez (HPE) Color Space   - W3C WCAG 2.1 Color Contrast",

			// Eye Comfort Section
			"Eye Comfort (Brightness)",
			"Brightness Adjust",
			"Brightness Retention: %.1f%%  (Recommended: %.2fx)",
			"Apply Recommendation",
			"affects display calculation only, not color correction itself",

			// Windows, QuickAccess & Community Credits
			"Main Window",
			"Opens or closes the CBA Main Window (Ctrl+Shift+C)",
			"Sensor Graph",
			"Opens or closes the Sensor & Graph HUD (Ctrl+Shift+G)",
			"Show CBA icon in quick access bar",
			"Shows or hides the CBA icon in the top Nexus quick access bar.",
			"Compact Mode: Settings and sensor graphs run in separate windows.",
			"Crafted with love for the Tyrian community - A Color Balance Assist and Enhancer for GW2",
			"Credits <3",
			"Credits to community, supporters & raiders",

			// Section Headers
			"1. Color Profile & Balance",
			"2. Commander Tag & Contrast Profiles",
			"3. Eye Comfort (Brightness)",
			"4. Game & Window Mode",
			"5. Hybrid Mode (Beta)",
			"6. Filter Lab & Experiment Field",
			"7. About CBA, References & Credits",
			""
		};

		if (CurrentSettings.Language == 2) return de; // Deutsch (explicit)
		if (CurrentSettings.Language == 0)            // System Language (Windows)
		{
			return (strcmp(DetectSystemLanguage(), "de") == 0) ? de : en;
		}
		return en; // English (1, default)
	}
}
