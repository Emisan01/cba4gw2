#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Shared.h"
#include "ColorMatrix.h"
#include "ColorEffectController.h"
#include "WindowMode.h"
#include "Settings.h"
#include "CbaIcon.h"
#include "HybridScanner.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <chrono>
#include <array>
#include <cstdio>
#include <string>
#include <cmath>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>

using namespace cba;

namespace Theme
{
	// ── Curated TAC-Inspired Palette (Cyan, Blau, Grau, Gold) ───────────────
	// Mittelwert Cyan / Blau (Deep Marine Teal) — Muted, harmonious button states
	const ImVec4 kBtnMittelwertIdle   = ImVec4(0.06f, 0.20f, 0.30f, 0.88f); // #0F334D (Struktur/Tief)
	const ImVec4 kBtnMittelwertHover  = ImVec4(0.10f, 0.28f, 0.40f, 0.95f); // #1A4766
	const ImVec4 kBtnMittelwertActive = ImVec4(0.04f, 0.15f, 0.24f, 1.00f);

	// Aktiver Zustand (Fenster geöffnet oder Filter AN — kein grelles Giftgrün mehr!)
	const ImVec4 kBtnStateActiveIdle  = ImVec4(0.02f, 0.32f, 0.36f, 0.92f); // #05525C (Deep Teal)
	const ImVec4 kBtnStateActiveHover = ImVec4(0.04f, 0.42f, 0.46f, 1.00f); // #0A6B75
	const ImVec4 kBtnStateActivePress = ImVec4(0.01f, 0.24f, 0.28f, 1.00f);

	// Neutral / Struktur (Grau / Slate)
	const ImVec4 kBtnNeutralIdle      = ImVec4(0.14f, 0.17f, 0.21f, 0.85f); // #242B36
	const ImVec4 kBtnNeutralHover     = ImVec4(0.20f, 0.24f, 0.30f, 0.95f); // #333D4D
	const ImVec4 kBtnNeutralPress     = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);

	// Subtle Reset / Danger
	const ImVec4 kBtnDangerSubtleIdle  = ImVec4(0.22f, 0.15f, 0.16f, 0.85f);
	const ImVec4 kBtnDangerSubtleHover = ImVec4(0.32f, 0.20f, 0.22f, 0.95f);
	const ImVec4 kBtnDangerSubtlePress = ImVec4(0.15f, 0.10f, 0.11f, 1.00f);

	// Text Farbstufen
	const ImVec4 kTextPrimary         = ImVec4(0.88f, 0.92f, 0.96f, 1.00f); // #E0EBF5 (hell)
	const ImVec4 kTextSecondary       = ImVec4(0.66f, 0.72f, 0.78f, 0.95f); // #A8B8C7 (inaktiv/sekundär)
	const ImVec4 kTextCyanLicht       = ImVec4(0.55f, 0.92f, 0.90f, 1.00f); // #8CEBE6 (Lichtakzent)
	const ImVec4 kTextBlauPeak        = ImVec4(0.72f, 0.85f, 0.97f, 1.00f); // #B8D8F8 (Blau Peak)
	const ImVec4 kTextGoldLabel       = ImVec4(0.98f, 0.85f, 0.25f, 1.00f); // #FAD840 (Gold Text / Label)
	const ImVec4 kTextDangerSubtle    = ImVec4(0.90f, 0.75f, 0.76f, 0.95f);

	// Signal Dots (Nur kleine Dots / Icons, nie als Fläche)
	const ImU32  kDotReadyCol         = IM_COL32(0, 210, 190, 255);         // Cyan / Emerald Ready
	const ImU32  kDotWarnCol          = IM_COL32(250, 216, 64, 255);        // Gold Warning
	const ImU32  kDotOffCol           = IM_COL32(110, 120, 130, 255);       // Gray Inactive

	// WCAG Relative Luminance helper: guarantees readable contrast for any button background
	inline ImVec4 GetContrastTextColor(const ImVec4& bgCol)
	{
		float lum = 0.2126f * bgCol.x + 0.7152f * bgCol.y + 0.0722f * bgCol.z;
		return (lum > 0.48f) ? ImVec4(0.08f, 0.10f, 0.14f, 1.00f) : ImVec4(0.92f, 0.96f, 1.00f, 1.00f);
	}
}

namespace
{
	AddonDefinition AddonDef{}; // empty definition
	// UI helper constants and functions
	static const float kPanelItemWidth = 240.0f;
	static void PanelHeader(const char* title) {
	    ImGui::PushStyleColor(ImGuiCol_Text, Theme::kTextBlauPeak);
	    ImGui::Separator();
	    ImGui::Spacing();
	    
        ImGui::TextUnformatted(title);
        ImGui::Spacing();
        ImGui::PopStyleColor();
    }
    static void PanelSpacing() { ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); }
};
	Settings        CurrentSettings{};
	std::string     AddonDir;
	NexusLinkData*  NexusLink = nullptr;

	struct MumbleContext {
		unsigned char serverAddress[28];
		uint32_t mapId;
		uint32_t mapType;
		uint32_t shardId;
		uint32_t instance;
		uint32_t buildId;
		uint32_t uiState;
		uint16_t compassWidth;
		uint16_t compassHeight;
		float compassRotation;
		float playerX;
		float playerY;
		float mapCenterX;
		float mapCenterY;
		float mapScale;
		uint32_t processId;
		uint8_t mountIndex;
	};

	struct GW2MumbleLink {
		uint32_t uiVersion;
		uint32_t uiTick;
		float fAvatarPosition[3];
		float fAvatarFront[3];
		float fAvatarTop[3];
		wchar_t name[256];
		float fCameraPosition[3];
		float fCameraFront[3];
		float fCameraTop[3];
		wchar_t identity[256];
		uint32_t context_len;
		MumbleContext context;
		wchar_t description[2048];
	};
	GW2MumbleLink* MumbleLinkData = nullptr;

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

	const char* DetectSystemLanguage()
	{
		LANGID lang = GetUserDefaultUILanguage();
		if (PRIMARYLANGID(lang) == LANG_GERMAN) return "de";
		if (PRIMARYLANGID(lang) == LANG_FRENCH) return "fr";
		if (PRIMARYLANGID(lang) == LANG_SPANISH) return "es";
		return "en";
	}

	const L10n& Strings()
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
			"2. Commander-Tag Enhancer",
			"3. Kontrast-Kombinationen (Ueberlappende Farbfelder)",
			"4. Eye Comfort (Helligkeit)",
			"5. Spiel- & Fenstermodus",
			"6. Hybrid Modus (Beta)",
			"7. Filter-Labor & Experimentierfeld",
			"8. Ueber CBA, Referenzen & Credits"
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
			"2. Commander Tag Enhancer",
			"3. Contrast Combinations (Overlapping Swatches)",
			"4. Eye Comfort (Brightness)",
			"5. Game & Window Mode",
			"6. Hybrid Mode (Beta)",
			"7. Filter Lab & Experiment Field",
			"8. About CBA, References & Credits"
		};

		if (CurrentSettings.Language == 2) return de; // Deutsch (explicit)
		if (CurrentSettings.Language == 0)            // System Language (Windows)
		{
			return (strcmp(DetectSystemLanguage(), "de") == 0) ? de : en;
		}
		return en; // English (1, default)
	}

	namespace
	{
		std::chrono::steady_clock::time_point s_lastApply{};
		MAGCOLOREFFECT s_lastAppliedEffect{};
		bool s_hasApplied = false;
		std::mutex s_recomputeMutex;

		std::atomic<bool> s_watchdogRunning{false};
		std::thread s_watchdogThread;
		std::atomic<HWND> s_gw2Hwnd{nullptr};
		std::atomic<bool> s_gw2Minimized{false};
		std::atomic<bool> s_resetMainWindowPos{false};
		std::atomic<bool> s_resetGraphWindowPos{false};
		std::atomic<bool> s_resetDetachedWindowPos{false};
		std::atomic<bool> s_resetLabWindowPos{false};
		std::atomic<bool> s_deferredInitDone{false};
		std::atomic<bool> s_focusMainWindow{false};
		std::atomic<bool> s_focusGraphWindow{false};
		std::atomic<bool> s_focusLabWindow{false};
		std::atomic<bool> s_safeStartPending{false};

		bool RoughlyEqual(const MAGCOLOREFFECT& a, const MAGCOLOREFFECT& b)
		{
			for (int i = 0; i < 5; i++)
				for (int j = 0; j < 5; j++)
					if (std::fabs(a.transform[i][j] - b.transform[i][j]) > 0.0005f)
						return false;
			return true;
		}

		void ApplyThrottled(const MAGCOLOREFFECT& aEffect, bool aForce = false)
		{
			if (!s_deferredInitDone.load())
				return;

			if (!aForce && s_hasApplied && RoughlyEqual(aEffect, s_lastAppliedEffect))
				return; // Identical, save DWM IPC call

			auto now = std::chrono::steady_clock::now();
			auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastApply).count();
			if (!aForce && elapsedMs < 16) // ~60 Hz DWM cap
				return;

			GetColorEffectController().Apply(aEffect);
			s_lastApply = now;
			s_lastAppliedEffect = aEffect;
			s_hasApplied = true;
		}
	}

	void EnsureDeferredInitialized();

	// ── Conflict Resolution & Replacement Color Calculation (Step 4) ────────
	static void RgbToHsv(float r, float g, float b, float& h, float& s, float& v)
	{
		float maxVal = r;
		if (g > maxVal) maxVal = g;
		if (b > maxVal) maxVal = b;

		float minVal = r;
		if (g < minVal) minVal = g;
		if (b < minVal) minVal = b;

		float delta = maxVal - minVal;
		v = maxVal;
		s = (maxVal > 1e-5f) ? (delta / maxVal) : 0.0f;
		if (delta < 1e-5f) {
			h = 0.0f;
		} else if (maxVal == r) {
			h = 60.0f * std::fmod(((g - b) / delta), 6.0f);
			if (h < 0.0f) h += 360.0f;
		} else if (maxVal == g) {
			h = 60.0f * (((b - r) / delta) + 2.0f);
		} else {
			h = 60.0f * (((r - g) / delta) + 4.0f);
		}
	}

	static void HsvToRgb(float h, float s, float v, float& r, float& g, float& b)
	{
		float c = v * s;
		float hPrime = std::fmod(h / 60.0f, 6.0f);
		if (hPrime < 0.0f) hPrime += 6.0f;
		float x = c * (1.0f - std::abs(std::fmod(hPrime, 2.0f) - 1.0f));
		float m = v - c;
		if (hPrime < 1.0f)      { r = c; g = x; b = 0; }
		else if (hPrime < 2.0f) { r = x; g = c; b = 0; }
		else if (hPrime < 3.0f) { r = 0; g = c; b = x; }
		else if (hPrime < 4.0f) { r = 0; g = x; b = c; }
		else if (hPrime < 5.0f) { r = x; g = 0; b = c; }
		else                    { r = c; g = 0; b = x; }
		r += m; g += m; b += m;
		r = std::clamp(r, 0.0f, 1.0f);
		g = std::clamp(g, 0.0f, 1.0f);
		b = std::clamp(b, 0.0f, 1.0f);
	}

	static float RelativeLuma(float r, float g, float b)
	{
		return 0.2126f * r + 0.7152f * g + 0.0722f * b;
	}

	struct TagConflictState
	{
		bool inConflict = false;
		float repR = 0.0f, repG = 0.0f, repB = 0.0f;
	};
	static std::array<TagConflictState, 9> s_tagConflictStates{};

	void UpdateTagEnhancerConflicts()
	{
		static bool s_lastActive = false;
		static float s_lastTolerance = -1.0f;
		static std::vector<TargetColor> s_lastTargets;

		bool anyActive = (CurrentSettings.CommanderTagMode != 0) || CurrentSettings.FreeFilterEnabled || CurrentSettings.LabModeEnabled;
		if (!anyActive)
		{
			if (s_lastActive)
			{
				GetHybridScanner().SetHighlighterParams(false, {}, CurrentSettings.EnhancerTolerance);
				s_lastActive = false;
				s_lastTargets.clear();
			}
			for (auto& st : s_tagConflictStates) st = {};
			return;
		}

		std::vector<TargetColor> targetColors;
		targetColors.reserve(10);

		if (CurrentSettings.CommanderTagMode != 0)
		{
			BalanceType defType = CurrentSettings.Mixed 
				? (CurrentSettings.MixedBySeverity01 > CurrentSettings.MixedRgSeverity01 ? BalanceType::Tritan : BalanceType::Deutan)
				: CurrentSettings.Type;
			double sev = CurrentSettings.Mixed 
				? (CurrentSettings.MixedRgSeverity01 > CurrentSettings.MixedBySeverity01 ? CurrentSettings.MixedRgSeverity01 : CurrentSettings.MixedBySeverity01)
				: CurrentSettings.Severity01;

			// 4a: Simulate each tag color under the user's balance profile and severity
			struct SimTag {
				float origR, origG, origB;
				float simR, simG, simB;
				float luma;
			};
			std::array<SimTag, 9> simTags{};
			for (int i = 0; i < 9; ++i)
			{
				simTags[i].origR = kGw2TagRefs[i].r;
				simTags[i].origG = kGw2TagRefs[i].g;
				simTags[i].origB = kGw2TagRefs[i].b;
				simTags[i].luma = RelativeLuma(kGw2TagRefs[i].r, kGw2TagRefs[i].g, kGw2TagRefs[i].b);

				double outR = 0.0, outG = 0.0, outB = 0.0;
				ColorMatrix::SimulatePixel(simTags[i].origR, simTags[i].origG, simTags[i].origB, defType, outR, outG, outB);

				// Interpolate with severity (sev = 0 -> original, sev = 1 -> full simulation, sev > 1 -> boosted)
				simTags[i].simR = (float)(simTags[i].origR + sev * (outR - simTags[i].origR));
				simTags[i].simG = (float)(simTags[i].origG + sev * (outG - simTags[i].origG));
				simTags[i].simB = (float)(simTags[i].origB + sev * (outB - simTags[i].origB));
			}

			// 4b: Canonical affected tags from GW2 WvW setup (Mockup Logic) + Pairwise Euclidean distance
			std::array<bool, 9> hasConflict{};
			if (CurrentSettings.SmartEnhancer)
			{
				if (defType == BalanceType::Protan) {
					// Protanopie: 5 von 9 verschoben (Rot, Orange, Grün, Blau, Lila)
					hasConflict[0] = true; // Rot
					hasConflict[1] = true; // Orange
					hasConflict[3] = true; // Grün
					hasConflict[5] = true; // Blau
					hasConflict[6] = true; // Lila
				} else if (defType == BalanceType::Deutan) {
					// Deuteranopie: 3 von 9 verschoben (Rot, Orange, Grün)
					hasConflict[0] = true; // Rot
					hasConflict[1] = true; // Orange
					hasConflict[3] = true; // Grün
				} else {
					// Tritanopie: 5 von 9 verschoben (Gelb, Grün, Cyan, Blau, Magenta)
					hasConflict[2] = true; // Gelb
					hasConflict[3] = true; // Grün
					hasConflict[4] = true; // Cyan
					hasConflict[5] = true; // Blau
					hasConflict[7] = true; // Magenta
				}
			}
			else
			{
				constexpr float kConflictThreshold = 0.16f;
				for (int i = 0; i < 9; ++i)
				{
					for (int j = i + 1; j < 9; ++j)
					{
						float dr = simTags[i].simR - simTags[j].simR;
						float dg = simTags[i].simG - simTags[j].simG;
						float db = simTags[i].simB - simTags[j].simB;
						float dist = std::sqrt(dr * dr + dg * dg + db * db);
						if (dist < kConflictThreshold)
						{
							hasConflict[i] = true;
							hasConflict[j] = true;
						}
					}
				}
			}

			// 4d: For each conflicting tag, find replacement color
			for (int i = 0; i < 9; ++i)
			{
				s_tagConflictStates[i].inConflict = hasConflict[i];
				if (!hasConflict[i])
				{
					s_tagConflictStates[i].repR = simTags[i].origR;
					s_tagConflictStates[i].repG = simTags[i].origG;
					s_tagConflictStates[i].repB = simTags[i].origB;
					continue;
				}

				float bestR = simTags[i].origR, bestG = simTags[i].origG, bestB = simTags[i].origB;
				float bestScore = -1.0f;

				float h = 0, s = 0, v = 0;
				RgbToHsv(simTags[i].origR, simTags[i].origG, simTags[i].origB, h, s, v);

				for (int step = 1; step <= 23; ++step)
				{
					float testH = std::fmod(h + step * 15.0f, 360.0f);
					float r = 0, g = 0, b = 0;
					HsvToRgb(testH, s, v, r, g, b);

					float newLuma = RelativeLuma(r, g, b);
					float minLuma = simTags[i].luma * 0.85f;
					if (newLuma < minLuma && newLuma > 1e-4f)
					{
						float scale = minLuma / newLuma;
						r = std::clamp(r * scale, 0.0f, 1.0f);
						g = std::clamp(g * scale, 0.0f, 1.0f);
						b = std::clamp(b * scale, 0.0f, 1.0f);
					}

					double candSimR = 0, candSimG = 0, candSimB = 0;
					ColorMatrix::SimulatePixel(r, g, b, defType, candSimR, candSimG, candSimB);
					candSimR = r + sev * (candSimR - r);
					candSimG = g + sev * (candSimG - g);
					candSimB = b + sev * (candSimB - b);

					float minDist = 999.0f;
					for (int j = 0; j < 9; ++j)
					{
						if (i == j) continue;
						float dr = (float)candSimR - simTags[j].simR;
						float dg = (float)candSimG - simTags[j].simG;
						float db = (float)candSimB - simTags[j].simB;
						float d = std::sqrt(dr * dr + dg * dg + db * db);
						if (d < minDist) minDist = d;
					}

					if (minDist > bestScore)
					{
						bestScore = minDist;
						bestR = r;
						bestG = g;
						bestB = b;
					}
				}

				s_tagConflictStates[i].repR = bestR;
				s_tagConflictStates[i].repG = bestG;
				s_tagConflictStates[i].repB = bestB;

				TargetColor tc;
				tc.r = simTags[i].origR;
				tc.g = simTags[i].origG;
				tc.b = simTags[i].origB;
				tc.repR = (uint8_t)(std::clamp(bestR * 255.0f, 0.0f, 255.0f));
				tc.repG = (uint8_t)(std::clamp(bestG * 255.0f, 0.0f, 255.0f));
				tc.repB = (uint8_t)(std::clamp(bestB * 255.0f, 0.0f, 255.0f));
				targetColors.push_back(tc);
			}
		}
		else
		{
			for (auto& st : s_tagConflictStates) st = {};
		}

		// 4e: If Free-Filter is enabled, add its high-precision targeted color to the scanner
		if (CurrentSettings.FreeFilterEnabled)
		{
			TargetColor freeTc;
			freeTc.r = CurrentSettings.FreeFilterTargetRgb[0];
			freeTc.g = CurrentSettings.FreeFilterTargetRgb[1];
			freeTc.b = CurrentSettings.FreeFilterTargetRgb[2];
			freeTc.repR = (uint8_t)(std::clamp(CurrentSettings.FreeFilterReplaceRgb[0] * 255.0f, 0.0f, 255.0f));
			freeTc.repG = (uint8_t)(std::clamp(CurrentSettings.FreeFilterReplaceRgb[1] * 255.0f, 0.0f, 255.0f));
			freeTc.repB = (uint8_t)(std::clamp(CurrentSettings.FreeFilterReplaceRgb[2] * 255.0f, 0.0f, 255.0f));
			targetColors.push_back(freeTc);
		}

		// 4f: If Filter-Labor is enabled, add all active lab filters to the scanner
		if (CurrentSettings.LabModeEnabled)
		{
			for (const auto& filter : CurrentSettings.LabFilters)
			{
				if (!filter.Enabled) continue;
				TargetColor labTc;
				labTc.r = filter.TargetRgb[0];
				labTc.g = filter.TargetRgb[1];
				labTc.b = filter.TargetRgb[2];
				labTc.repR = (uint8_t)(std::clamp(filter.ReplaceRgb[0] * 255.0f, 0.0f, 255.0f));
				labTc.repG = (uint8_t)(std::clamp(filter.ReplaceRgb[1] * 255.0f, 0.0f, 255.0f));
				labTc.repB = (uint8_t)(std::clamp(filter.ReplaceRgb[2] * 255.0f, 0.0f, 255.0f));
				labTc.tolerance = (filter.ToleranceTones / 255.0f) * 1.732f;
				labTc.diffusion = filter.Diffusion;
				labTc.actionType = filter.ActionType;
				targetColors.push_back(labTc);
			}
		}

		float effectiveTol = (CurrentSettings.CommanderTagMode != 0) 
			? CurrentSettings.EnhancerTolerance 
			: ((CurrentSettings.FreeFilterToleranceTones / 255.0f) * 1.732f);

		bool paramsChanged = (!s_lastActive) || (effectiveTol != s_lastTolerance) || (targetColors.size() != s_lastTargets.size());
		if (!paramsChanged)
		{
			for (size_t k = 0; k < targetColors.size(); ++k)
			{
				if (targetColors[k].r != s_lastTargets[k].r || targetColors[k].g != s_lastTargets[k].g || targetColors[k].b != s_lastTargets[k].b ||
					targetColors[k].repR != s_lastTargets[k].repR || targetColors[k].repG != s_lastTargets[k].repG || targetColors[k].repB != s_lastTargets[k].repB)
				{
					paramsChanged = true;
					break;
				}
			}
		}

		if (paramsChanged)
		{
			s_lastActive = true;
			s_lastTolerance = effectiveTol;
			s_lastTargets = targetColors;
			GetHybridScanner().SetHighlighterParams(true, targetColors, effectiveTol);
		}
	}

	struct BrightnessRetentionResult
	{
		float retentionRatio = 1.0f;  // e.g. 0.92f (92%)
		float recommendedGain = 1.0f; // clamp(1.0f / retentionRatio, 0.70f, 1.30f)
	};

	// Computes brightness retention of the active correction profile over a 13-color reference palette
	// (8 GW2 tag colors + 5 ambient environment colors).
	BrightnessRetentionResult GetBrightnessRetention()
	{
		// 5 placeholder ambient colors (Erdbraun, Laub, Himmel, Stein, Sonnenlicht)
		// TODO: kalibrieren
		static const struct { float r, g, b; } kAmbientColors[5] = {
			{ 0.45f, 0.32f, 0.20f }, // Erdbraun    (TODO: kalibrieren)
			{ 0.22f, 0.48f, 0.20f }, // Laub         (TODO: kalibrieren)
			{ 0.35f, 0.60f, 0.85f }, // Himmel       (TODO: kalibrieren)
			{ 0.52f, 0.52f, 0.52f }, // Stein        (TODO: kalibrieren)
			{ 0.95f, 0.90f, 0.70f }  // Sonnenlicht  (TODO: kalibrieren)
		};

		double m3x3[3][3];
		if (CurrentSettings.Mixed)
		{
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01,
				m3x3);
		}
		else
		{
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, m3x3);
		}

		float sumOrig = 0.0f;
		float sumTrans = 0.0f;

		auto processColor = [&](float r, float g, float b) {
			float origLuma = RelativeLuma(r, g, b);
			sumOrig += origLuma;

			float trR = (float)(m3x3[0][0] * r + m3x3[0][1] * g + m3x3[0][2] * b);
			float trG = (float)(m3x3[1][0] * r + m3x3[1][1] * g + m3x3[1][2] * b);
			float trB = (float)(m3x3[2][0] * r + m3x3[2][1] * g + m3x3[2][2] * b);

			trR = std::clamp(trR, 0.0f, 1.0f);
			trG = std::clamp(trG, 0.0f, 1.0f);
			trB = std::clamp(trB, 0.0f, 1.0f);

			float transLuma = RelativeLuma(trR, trG, trB);
			sumTrans += transLuma;
		};

		for (int i = 0; i < 8; ++i)
		{
			processColor(kGw2TagRefs[i].r, kGw2TagRefs[i].g, kGw2TagRefs[i].b);
		}
		for (int i = 0; i < 5; ++i)
		{
			processColor(kAmbientColors[i].r, kAmbientColors[i].g, kAmbientColors[i].b);
		}

		BrightnessRetentionResult res{};
		if (sumOrig > 1e-4f)
		{
			res.retentionRatio = sumTrans / sumOrig;
		}
		else
		{
			res.retentionRatio = 1.0f;
		}

		float inv = (res.retentionRatio > 1e-4f) ? (1.0f / res.retentionRatio) : 1.0f;
		res.recommendedGain = std::clamp(inv, 0.70f, 1.30f);
		return res;
	}

	// Rebuilds the MAGCOLOREFFECT from CurrentSettings and either applies or
	// clears it. Live math is computed immediately; DWM calls are throttled.
	void Recompute(bool aForce = false)
	{
		std::lock_guard<std::mutex> lock(s_recomputeMutex);
		auto& controller = GetColorEffectController();

		if (!s_deferredInitDone.load())
		{
			return;
		}

		UpdateTagEnhancerConflicts();

		if (!CurrentSettings.Enabled)
		{
			controller.Clear();
			s_hasApplied = false;
			return;
		}

		HWND fg = GetForegroundWindow();
		DWORD fgPid = 0;
		if (fg) GetWindowThreadProcessId(fg, &fgPid);
		bool isGw2Foreground = (fg && fgPid == GetCurrentProcessId());
		bool isMinimized = s_gw2Minimized.load() || (s_gw2Hwnd.load() && IsIconic(s_gw2Hwnd.load()));

		bool shouldBeActive = (!isMinimized && (isGw2Foreground || CurrentSettings.SystemWide));

		if (!shouldBeActive)
		{
			controller.Clear();
			s_hasApplied = false;
			return;
		}

		double m3x3[3][3];
		if (CurrentSettings.Mixed)
		{
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01,
				m3x3);
		}
		else
		{
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, m3x3);
		}

		// Eye Comfort: Linear brightness scaling (GammaGain, range 0.70 - 1.30)
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				m3x3[r][c] *= CurrentSettings.GammaGain;
			}
		}

		MAGCOLOREFFECT effect = ColorMatrix::ToMagColorEffect(m3x3);
		ApplyThrottled(effect, aForce);
	}

	namespace
	{
		void WatchdogLoop()
		{
			while (s_watchdogRunning)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				if (!s_deferredInitDone.load())
				{
					continue;
				}

				if (!CurrentSettings.Enabled)
				{
					if (s_hasApplied)
					{
						std::lock_guard<std::mutex> lock(s_recomputeMutex);
						GetColorEffectController().Clear();
						s_hasApplied = false;
					}
					continue;
				}

				HWND fg = GetForegroundWindow();
				DWORD fgPid = 0;
				if (fg) GetWindowThreadProcessId(fg, &fgPid);
				bool isGw2Foreground = (fg && fgPid == GetCurrentProcessId());
				bool isMinimized = s_gw2Minimized.load() || (s_gw2Hwnd.load() && IsIconic(s_gw2Hwnd.load()));

				bool shouldBeActive = (!isMinimized && (isGw2Foreground || CurrentSettings.SystemWide));

				if (!shouldBeActive)
				{
					if (s_hasApplied)
					{
						std::lock_guard<std::mutex> lock(s_recomputeMutex);
						GetColorEffectController().Clear();
						s_hasApplied = false;
					}
				}
				else
				{
					if (!s_hasApplied)
					{
						Recompute(/*aForce=*/true);
					}
				}
			}
		}
	}

	UINT AddonWndProc(HWND aWnd, UINT aMsg, WPARAM aWParam, LPARAM aLParam)
	{
		if (aWnd) s_gw2Hwnd = aWnd;
		switch (aMsg)
		{
			case WM_ACTIVATE:
			{
				WORD state = LOWORD(aWParam);
				if (state == WA_INACTIVE)
				{
					if (!CurrentSettings.SystemWide)
					{
						GetColorEffectController().Clear();
						s_hasApplied = false;
					}
				}
				else
				{
					s_gw2Minimized = false;
					Recompute(/*aForce=*/true); // Gained focus -> restore filter immediately
				}
				break;
			}
			case WM_SIZE:
			{
				if (aWParam == SIZE_MINIMIZED)
				{
					s_gw2Minimized = true;
					GetColorEffectController().Clear();
					s_hasApplied = false;
				}
				else if (aWParam == SIZE_RESTORED || aWParam == SIZE_MAXIMIZED)
				{
					s_gw2Minimized = false;
					Recompute(/*aForce=*/true);
				}
				break;
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				bool altDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
				bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
				bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

				// Filter Off: Strg + Shift + O (or legacy Strg + Shift + Q)
				if ((aWParam == 'O' || aWParam == 'Q') && ctrlDown && shiftDown)
				{
					CurrentSettings.Enabled = false;
					CurrentSettings.Save(AddonDir);
					{
						std::lock_guard<std::mutex> lock(s_recomputeMutex);
						GetColorEffectController().Clear();
						s_hasApplied = false;
					}
					Recompute(/*aForce=*/true);
					return aMsg;
				}
				// Main Window: Strg + Shift + C (or Ctrl + Alt + C)
				else if (aWParam == 'C')
				{
					if ((ctrlDown && shiftDown) || (ctrlDown && altDown))
					{
						EnsureDeferredInitialized();
						CurrentSettings.ShowMainWindow = !CurrentSettings.ShowMainWindow;
						if (CurrentSettings.ShowMainWindow) s_focusMainWindow = true;
					}
				}
				break;
			}
		}
		return aMsg;
	}

	void ProcessKeybind(const char* aIdentifier, bool aIsRelease)
	{
		if (aIsRelease) return;

		if (strcmp(aIdentifier, "CBA - Filter Off") == 0 || strcmp(aIdentifier, "CBA - Not-Aus") == 0 || strcmp(aIdentifier, "KB_CBA_PANIC") == 0)
		{
			CurrentSettings.Enabled = false;
			CurrentSettings.Save(AddonDir);
			{
				std::lock_guard<std::mutex> lock(s_recomputeMutex);
				GetColorEffectController().Clear();
				s_hasApplied = false;
			}
			Recompute(/*aForce=*/true);
		}
		else if (strcmp(aIdentifier, "CBA - Main Window") == 0 || strcmp(aIdentifier, "KB_CBA_WINDOW") == 0)
		{
			EnsureDeferredInitialized();
			CurrentSettings.ShowMainWindow = !CurrentSettings.ShowMainWindow;
			if (CurrentSettings.ShowMainWindow) s_focusMainWindow = true;
		}
		else if (strcmp(aIdentifier, "CBA - Sensor Graph") == 0 || strcmp(aIdentifier, "KB_CBA_GRAPH") == 0)
		{
			EnsureDeferredInitialized();
			CurrentSettings.ShowGraphWindow = !CurrentSettings.ShowGraphWindow;
			if (CurrentSettings.ShowGraphWindow) s_focusGraphWindow = true;
		}
	}

	void UpdateQuickAccessIcon()
	{
		if (!APIDefs) return;
		if (CurrentSettings.ShowQuickAccessIcon)
		{
			if (APIDefs->QuickAccess.Add)
			{
				APIDefs->QuickAccess.Add("QA_CBA", "CBA_ICON", "CBA_ICON", "CBA - Main Window", "cba4gw2 (Strg+Shift+C / Filter Off: Strg+Shift+O)");
			}
		}
		else
		{
			if (APIDefs->QuickAccess.Remove)
			{
				APIDefs->QuickAccess.Remove("QA_CBA");
			}
		}
	}

	// ── C64 Retro 8-Bit Credits Overlay & Audio System ────────────────────────
	static std::atomic<bool> s_showC64Credits{false};
	static std::atomic<bool> s_c64SoundEnabled{true};
	static std::atomic<bool> s_c64AudioRunning{false};

	void StartC64Audio()
	{
		if (s_c64AudioRunning.load()) return;
		s_c64AudioRunning.store(true);
		std::thread th([]() {
			// Classic 8-bit chiptune melody (C major / G / Am / F arpeggios)
			const struct Note { DWORD freq; DWORD dur; } kTrack[] = {
				{ 523, 85 }, { 659, 85 }, { 784, 85 }, { 1046, 110 }, { 784, 80 }, { 659, 80 },
				{ 587, 85 }, { 698, 85 }, { 880, 85 }, { 1175, 110 }, { 880, 80 }, { 698, 80 },
				{ 440, 85 }, { 523, 85 }, { 659, 85 }, { 880,  110 }, { 659, 80 }, { 523, 80 },
				{ 349, 85 }, { 440, 85 }, { 523, 85 }, { 698,  110 }, { 523, 80 }, { 440, 80 },
				{ 392, 95 }, { 494, 95 }, { 587, 95 }, { 784,  140 }, { 587, 80 }, { 494, 80 }
			};
			const size_t kTrackLen = sizeof(kTrack) / sizeof(kTrack[0]);
			size_t noteIdx = 0;
			while (s_c64AudioRunning.load() && s_showC64Credits.load())
			{
				if (s_c64SoundEnabled.load())
				{
					Beep(kTrack[noteIdx].freq, kTrack[noteIdx].dur);
					noteIdx = (noteIdx + 1) % kTrackLen;
					std::this_thread::sleep_for(std::chrono::milliseconds(15));
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}
			s_c64AudioRunning.store(false);
		});
		th.detach();
	}

	void StopC64Audio()
	{
		s_c64AudioRunning.store(false);
	}

	void RenderC64CreditsOverlay()
	{
		if (!s_showC64Credits.load() || !ImGui::GetCurrentContext()) return;

		ImGuiIO& io = ImGui::GetIO();
		ImVec2 disp = io.DisplaySize;
		float winW = std::clamp(disp.x * 0.75f, 480.0f, 680.0f);
		float winH = std::clamp(disp.y * 0.80f, 420.0f, 540.0f);
		float posX = (disp.x - winW) * 0.5f;
		float posY = (disp.y - winH) * 0.5f;

		ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

		// Commodore 64 Palette
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.24f, 0.98f));
		ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(0.40f, 0.35f, 0.85f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_Text,     ImVec4(0.65f, 0.62f, 1.00f, 1.00f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

		bool open = true;
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		if (ImGui::Begin("**** COMMODORE 64 BASIC V2 - CBA4GW2 CREDITS ****", &open, flags))
		{
			if (!open || ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				s_showC64Credits.store(false);
				StopC64Audio();
			}

			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImVec2 p0 = ImGui::GetWindowPos();
			ImVec2 p1(p0.x + winW, p0.y + winH);

			// Animated C64 Copper Raster Bars effect across top and bottom
			static float s_time = 0.0f;
			s_time += io.DeltaTime;
			for (int r = 0; r < 4; ++r)
			{
				float tOffset = s_time * 2.5f + r * 0.4f;
				float colR = 0.5f + 0.45f * std::sin(tOffset);
				float colG = 0.5f + 0.45f * std::sin(tOffset + 2.094f);
				float colB = 0.5f + 0.45f * std::sin(tOffset + 4.188f);
				ImU32 barCol = IM_COL32((int)(colR * 255), (int)(colG * 255), (int)(colB * 255), 180);
				float barYTop = p0.y + 26.0f + r * 3.0f;
				dl->AddLine(ImVec2(p0.x + 4.0f, barYTop), ImVec2(p1.x - 4.0f, barYTop), barCol, 2.0f);
				float barYBot = p1.y - 42.0f + r * 3.0f;
				dl->AddLine(ImVec2(p0.x + 4.0f, barYBot), ImVec2(p1.x - 4.0f, barYBot), barCol, 2.0f);
			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "    **** COMMODORE 64 BASIC V2 ****");
			ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), " 64K RAM SYSTEM  38911 BASIC BYTES FREE");
			ImGui::Spacing();
			ImGui::Text("READY.");
			ImGui::Text("LOAD \"CBA4GW2\",8,1");
			ImGui::Text("SEARCHING FOR CBA4GW2... FOUND.");
			ImGui::Text("LOADING... READY.");
			ImGui::Text("RUN");
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Endless Auto-Scrolling Credits Container
			float scrollAreaH = winH - 240.0f;
			if (ImGui::BeginChild("##c64_scroller", ImVec2(-1.0f, scrollAreaH), true, ImGuiWindowFlags_NoScrollbar))
			{
				static float s_scrollPos = 0.0f;
				s_scrollPos += io.DeltaTime * 32.0f; // Scroll speed (32 px/sec)

				// Loop seamlessly
				float maxScroll = 680.0f;
				if (s_scrollPos > maxScroll)
				{
					s_scrollPos = 0.0f;
				}

				ImGui::SetScrollY(s_scrollPos);

				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "========================================================");
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "        COLOR BALANCE ASSIST FOR GUILD WARS 2           ");
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "                    CBA4GW2 v1.0.2                      ");
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "========================================================");
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.75f, 1.0f), "     \"FIGHT THE ELDER DRAGONS, NOT YOUR MONITOR!\"     ");
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::TextColored(ImVec4(1.0f, 0.60f, 0.80f, 1.0f), "[ CONCEPT, VISION & SYSTEM ]");
				ImGui::Text("  Emisan01 & the Guild Wars 2 Accessibility Initiative");
				ImGui::Spacing();

				ImGui::TextColored(ImVec4(1.0f, 0.60f, 0.80f, 1.0f), "[ MATHEMATICAL COLOR MODELS & RESEARCH ]");
				ImGui::Text("  - LMS-Dichromacy & Daltonization: Fidaner et al. (2005)");
				ImGui::Text("  - Computerized Dichromat Simulation: Vienot, Brettel & Mollon (1999)");
				ImGui::Text("  - Hunt-Pointer-Estevez (HPE) Cone Transformation");
				ImGui::Text("  - W3C Web Content Accessibility Guidelines (WCAG 2.1)");
				ImGui::Spacing();

				ImGui::TextColored(ImVec4(1.0f, 0.60f, 0.80f, 1.0f), "[ PLATFORM, ENGINES & COMMUNITY HEROES ]");
				ImGui::Text("  - ArenaNet: For creating Guild Wars 2 and 12+ years of Tyrian joy!");
				ImGui::Text("  - Nightmoore & Raidcore: For the incredible Nexus Addon Engine");
				ImGui::Text("  - DeltaConnected & ArcDPS Team: For pioneering GW2 modding");
				ImGui::Text("  - Omar Cornut: For the Dear ImGui interface library");
				ImGui::Spacing();

				ImGui::TextColored(ImVec4(1.0f, 0.60f, 0.80f, 1.0f), "[ GREETINGS & DANKSAGUNG TO ALL PLAYERS ]");
				ImGui::Text("  * To all Commanders who lead epic Zergs through WvW!");
				ImGui::Text("  * To all Raiders who master Dhuum, Qadim, Samarog & Cerus!");
				ImGui::Text("  * To all Fractal runners and Strike Mission squads!");
				ImGui::Text("  * And to every player who values clear contrast and balance:");
				ImGui::TextColored(ImVec4(0.40f, 1.00f, 0.50f, 1.0f), "    May your greens and reds never blend,");
				ImGui::TextColored(ImVec4(0.40f, 1.00f, 0.50f, 1.0f), "    may commander tags shine bright in every zerg,");
				ImGui::TextColored(ImVec4(0.40f, 1.00f, 0.50f, 1.0f), "    and may your drops forever be Precursor Gold!");
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "+++ ENDLESS RETRO CREDITS LOOPING... THANK YOU ALL! +++");
				ImGui::Spacing();
				ImGui::Spacing();

				ImGui::EndChild();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Bottom Buttons (Sound Toggle & Close)
			bool isDe = (Strings().Enabled[0] == 'A');
			bool sound = s_c64SoundEnabled.load();
			if (sound)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.52f, 0.28f, 0.90f));
				if (ImGui::Button("[ 8-BIT AUDIO: ON ]", ImVec2(170.0f, 26.0f)))
				{
					s_c64SoundEnabled.store(false);
				}
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.38f, 0.90f));
				if (ImGui::Button("[ 8-BIT AUDIO: OFF ]", ImVec2(170.0f, 26.0f)))
				{
					s_c64SoundEnabled.store(true);
					StartC64Audio();
				}
				ImGui::PopStyleColor();
			}

			ImGui::SameLine(0, 20.0f);
			if (ImGui::Button(isDe ? "Zurueck zum Spiel (ESC)" : "Return to Game (ESC)", ImVec2(220.0f, 26.0f)))
			{
				s_showC64Credits.store(false);
				StopC64Audio();
			}
		}
		ImGui::End();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
	}

	// ── Live Status Dot & Filter State Indicator ─────────────────────────────
	static void DrawFilterStatusIndicator(bool aWithText)
	{
		const L10n& t = Strings();
		bool isDe = (t.Enabled[0] == 'A');

		// Determine current filter state
		bool isEnabled = CurrentSettings.Enabled;
		WindowMode mode = DetectWindowMode(APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);
		bool isExclusive = (mode == WindowMode::ExclusiveFullscreen);

		HWND fg = GetForegroundWindow();
		DWORD fgPid = 0;
		if (fg) GetWindowThreadProcessId(fg, &fgPid);
		bool isGw2Foreground = (fg && fgPid == GetCurrentProcessId());
		bool isMinimized = s_gw2Minimized.load() || (s_gw2Hwnd.load() && IsIconic(s_gw2Hwnd.load()));
		bool isPaused = (!isMinimized && !isGw2Foreground && !CurrentSettings.SystemWide);

		ImU32 dotColor;
		ImU32 glowColor = 0;
		const char* statusText = "";
		const char* tooltipText = "";

		if (!isEnabled)
		{
			dotColor = Theme::kDotOffCol;
			statusText = isDe ? "Inaktiv" : "Inactive";
			tooltipText = isDe ? "CBA Status: Farbfilter ist ausgeschaltet (OFF).\nKlicke auf [ON], um den Filter zu aktivieren." 
			                   : "CBA Status: Color filter is OFF.\nClick [ON] to activate the filter.";
		}
		else if (isExclusive)
		{
			dotColor = Theme::kDotWarnCol;
			statusText = isDe ? "Blockiert (Vollbild)" : "Blocked (Fullscreen)";
			tooltipText = isDe ? "CBA Status: Windows DWM-Farbfilter wird durch exklusives Vollbild blockiert!\nBitte in GW2 Grafikoptionen auf 'Fenster-Vollbild' (Borderless) umschalten."
			                   : "CBA Status: Windows DWM filter blocked by exclusive fullscreen!\nPlease switch GW2 graphics to 'Windowed Fullscreen' (Borderless).";
		}
		else if (isMinimized || isPaused)
		{
			dotColor = Theme::kDotWarnCol;
			statusText = isDe ? "Pausiert" : "Paused";
			tooltipText = isDe ? "CBA Status: GW2 ist im Hintergrund oder minimiert.\nFilter pausiert automatisch zum Schutz anderer Anwendungen.\n('Im Hintergrund aktiv lassen' fuer Dauerbetrieb)."
			                   : "CBA Status: GW2 is in background or minimized.\nFilter pauses automatically.\n('Keep active in background' to keep active).";
		}
		else
		{
			// Active & running: soft cyan-emerald pulse
			float time = (float)ImGui::GetTime();
			float pulse = 0.70f + 0.30f * std::sin(time * 3.5f);
			dotColor = Theme::kDotReadyCol;
			glowColor = IM_COL32(0, 210, 190, (int)(pulse * 90.0f));
			statusText = isDe ? "Aktiv (DWM)" : "Active (DWM)";
			tooltipText = isDe ? "CBA Status: Farbfilter ist aktiv und an Guild Wars 2 gebunden.\nWindows Magnification DWM-Hardwarebeschleunigung laeuft stabil."
			                   : "CBA Status: Color filter active and bound to Guild Wars 2.\nWindows Magnification DWM hardware acceleration active.";
		}

		ImVec2 p = ImGui::GetCursorScreenPos();
		float radius = 5.0f;
		float h = ImGui::GetTextLineHeight();
		ImVec2 center(p.x + radius + 2.0f, p.y + h * 0.5f);
		ImDrawList* dl = ImGui::GetWindowDrawList();

		if (glowColor != 0)
		{
			dl->AddCircleFilled(center, radius + 3.0f, glowColor);
		}
		dl->AddCircleFilled(center, radius, dotColor);
		dl->AddCircle(center, radius, IM_COL32(20, 30, 40, 200), 0, 1.0f);

		float itemW = radius * 2.0f + 4.0f;
		ImGui::Dummy(ImVec2(itemW, h));
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltipText);
		}

		if (aWithText)
		{
			ImGui::SameLine(0, 4.0f);
			ImGui::TextColored(ImColor(dotColor), "%s", statusText);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", tooltipText);
			}
		}
	}

	// Registered as ERenderType::OptionsRender — appended into Nexus's own
	// options window under this addon's name, no separate window needed.

	// Draws R/G/B correction curves (neutral-grey input ramp) into a dark panel.
	// All three channel curves on one graph gives an immediate visual sense of
	// how much the filter shifts each colour channel.
	// Draws R/G/B correction curves into an aesthetic, clipped panel.
	// Samples the transfer function across 32 steps with antialiased glow lines
	// Draws R/G/B correction curves into an aesthetic panel.
	// Samples the spectral response across the full color spectrum (Hue 0° to 360°)
	// aligned directly with the color beam below, showing live channel boosts and separation.
	static void DrawCurvePanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                           const double aM[3][3], bool aIsDetached, float aOpacity)
	{
		const float pad = 8.0f;
		const float labelSpaceLeft = 28.0f;
		const float labelSpaceBottom = 20.0f;
		const float badgeSpaceRight = 6.0f;

		const float plotX = aOrigin.x + pad + labelSpaceLeft;
		const float plotY = aOrigin.y + pad + 4.0f;
		const float plotW = aW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		const float plotH = aH - pad * 2 - labelSpaceBottom - 4.0f;

		// Panel background: transparent glass when detached (showing GW2), deep slate when docked
		float panelA = aIsDetached ? std::clamp(aOpacity * 0.92f, 0.0f, 0.92f) : 0.92f;
		int bgAlpha = (int)(panelA * 255.0f);
		int borderAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f, 20.0f, 150.0f)) : 150;

		aDraw->AddRectFilled(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(12, 15, 22, bgAlpha), 6.0f);
		aDraw->AddRect(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(65, 85, 125, borderAlpha), 6.0f, 0, 1.2f);

		// Grid with axis tick labels
		int gridAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 65.0f, 15.0f, 65.0f)) : 65;
		int textAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f + 60.0f, 60.0f, 210.0f)) : 210;

		// Horizontal grid lines (0%, 50%, 100% signal)
		for (int k = 0; k <= 2; ++k) {
			float frac = k / 2.0f;
			float yg = plotY + plotH * (1.0f - frac);
			if (k > 0 && k < 2) {
				aDraw->AddLine(ImVec2(plotX, yg), ImVec2(plotX + plotW, yg), IM_COL32(50, 65, 95, gridAlpha));
			}
			char buf[16];
			std::snprintf(buf, sizeof(buf), "%d%%", (int)(frac * 100.0f));
			aDraw->AddText(ImVec2(plotX - 26.0f, yg - 6.0f), IM_COL32(125, 140, 175, textAlpha), buf);
		}

		// Vertical grid lines & Spectrum Landmarks (R, Y, G, C, B, M, R)
		struct SpecMark { float u; const char* name; ImU32 col; };
		static const SpecMark kMarks[] = {
			{ 0.000f, "R", IM_COL32(255, 80, 80, 255) },
			{ 0.167f, "Y", IM_COL32(255, 230, 70, 255) },
			{ 0.333f, "G", IM_COL32(70, 240, 110, 255) },
			{ 0.500f, "C", IM_COL32(60, 220, 240, 255) },
			{ 0.667f, "B", IM_COL32(80, 170, 255, 255) },
			{ 0.833f, "M", IM_COL32(240, 90, 230, 255) },
			{ 1.000f, "R", IM_COL32(255, 80, 80, 255) }
		};

		for (const auto& m : kMarks) {
			float xg = plotX + plotW * m.u;
			if (m.u > 0.01f && m.u < 0.99f) {
				aDraw->AddLine(ImVec2(xg, plotY), ImVec2(xg, plotY + plotH), IM_COL32(50, 65, 95, gridAlpha));
			}
			aDraw->AddText(ImVec2(xg - 4.0f, plotY + plotH + 4.0f), m.col, m.name);
		}

		// Inner plot border
		aDraw->AddRect(ImVec2(plotX, plotY), ImVec2(plotX + plotW, plotY + plotH), IM_COL32(70, 90, 130, borderAlpha), 0.0f, 0, 1.0f);

		// Clip curves strictly within plot box
		aDraw->PushClipRect(ImVec2(plotX - 0.5f, plotY - 0.5f), ImVec2(plotX + plotW + 0.5f, plotY + plotH + 0.5f), true);

		constexpr int kSteps = 48;
		static const ImU32 kChanCol[3] = {
			IM_COL32(255, 75, 75, 255),   // Red
			IM_COL32(65, 240, 110, 255),  // Green
			IM_COL32(75, 170, 255, 255)   // Blue
		};
		static const ImU32 kChanGlow[3] = {
			IM_COL32(255, 75, 75, 50),
			IM_COL32(65, 240, 110, 50),
			IM_COL32(75, 170, 255, 50)
		};

		auto sampleSpectrumRGB = [](float u, float& r0, float& g0, float& b0) {
			float h = u * 6.0f;
			float x = 1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f);
			if (h < 1.0f)      { r0 = 1.0f; g0 = x;    b0 = 0.0f; }
			else if (h < 2.0f) { r0 = x;    g0 = 1.0f; b0 = 0.0f; }
			else if (h < 3.0f) { r0 = 0.0f; g0 = 1.0f; b0 = x;    }
			else if (h < 4.0f) { r0 = 0.0f; g0 = x;    b0 = 1.0f; }
			else if (h < 5.0f) { r0 = x;    g0 = 0.0f; b0 = 1.0f; }
			else               { r0 = 1.0f; g0 = 0.0f; b0 = x;    }
		};

		ImVec2 prevPts[3];
		for (int step = 0; step <= kSteps; ++step) {
			float u = (float)step / (float)kSteps;
			float r0, g0, b0;
			sampleSpectrumRGB(u, r0, g0, b0);

			double cr = std::clamp(aM[0][0]*r0 + aM[0][1]*g0 + aM[0][2]*b0, 0.0, 1.0);
			double cg = std::clamp(aM[1][0]*r0 + aM[1][1]*g0 + aM[1][2]*b0, 0.0, 1.0);
			double cb = std::clamp(aM[2][0]*r0 + aM[2][1]*g0 + aM[2][2]*b0, 0.0, 1.0);

			float curX = plotX + u * plotW;
			ImVec2 curPts[3] = {
				ImVec2(curX, plotY + plotH - (float)cr * plotH),
				ImVec2(curX, plotY + plotH - (float)cg * plotH),
				ImVec2(curX, plotY + plotH - (float)cb * plotH)
			};

			if (step > 0) {
				for (int ch = 0; ch < 3; ++ch) {
					aDraw->AddLine(prevPts[ch], curPts[ch], kChanGlow[ch], 4.5f);
					aDraw->AddLine(prevPts[ch], curPts[ch], kChanCol[ch], 2.0f);
				}
			}

			for (int ch = 0; ch < 3; ++ch) {
				prevPts[ch] = curPts[ch];
			}
		}

		aDraw->PopClipRect();
	}

	// ── Mode 2: Harmonische Resonanz (Gauß / Sinusoidale LMS-Wellen) ──────────
	// Basiert auf stetig differenzierbaren LMS-Zapfen-Absorptionsspektren (CIE Standard Observer / Stockman & Sharpe).
	// Erzeugt weiche, fließende, organische Wellenkurven (genau wie in den UI-Mockups).
	static void DrawHarmonicCurvePanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                                   const double aM[3][3], bool aIsDetached, float aOpacity)
	{
		const float pad = 8.0f;
		const float labelSpaceLeft = 28.0f;
		const float labelSpaceBottom = 20.0f;
		const float badgeSpaceRight = 6.0f;

		const float plotX = aOrigin.x + pad + labelSpaceLeft;
		const float plotY = aOrigin.y + pad + 4.0f;
		const float plotW = aW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		const float plotH = aH - pad * 2 - labelSpaceBottom - 4.0f;

		float panelA = aIsDetached ? std::clamp(aOpacity * 0.92f, 0.0f, 0.92f) : 0.92f;
		int bgAlpha = (int)(panelA * 255.0f);
		int borderAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f, 20.0f, 150.0f)) : 150;

		aDraw->AddRectFilled(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(12, 15, 22, bgAlpha), 6.0f);
		aDraw->AddRect(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(65, 85, 125, borderAlpha), 6.0f, 0, 1.2f);

		int gridAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 65.0f, 15.0f, 65.0f)) : 65;
		int textAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f + 60.0f, 60.0f, 210.0f)) : 210;

		// Horizontal grid lines (0%, 50%, 100%)
		for (int k = 0; k <= 2; ++k) {
			float frac = k / 2.0f;
			float yg = plotY + plotH * (1.0f - frac);
			if (k > 0 && k < 2) {
				aDraw->AddLine(ImVec2(plotX, yg), ImVec2(plotX + plotW, yg), IM_COL32(50, 65, 95, gridAlpha));
			}
			char buf[16];
			std::snprintf(buf, sizeof(buf), "%d%%", (int)(frac * 100.0f));
			aDraw->AddText(ImVec2(plotX - 26.0f, yg - 6.0f), IM_COL32(125, 140, 175, textAlpha), buf);
		}

		// Vertical landmarks
		struct SpecMark { float u; const char* name; ImU32 col; };
		static const SpecMark kMarks[] = {
			{ 0.000f, "R", IM_COL32(255, 80, 80, 255) },
			{ 0.167f, "Y", IM_COL32(255, 230, 70, 255) },
			{ 0.333f, "G", IM_COL32(70, 240, 110, 255) },
			{ 0.500f, "C", IM_COL32(60, 220, 240, 255) },
			{ 0.667f, "B", IM_COL32(80, 170, 255, 255) },
			{ 0.833f, "M", IM_COL32(240, 90, 230, 255) },
			{ 1.000f, "R", IM_COL32(255, 80, 80, 255) }
		};
		for (const auto& m : kMarks) {
			float xg = plotX + plotW * m.u;
			if (m.u > 0.01f && m.u < 0.99f) {
				aDraw->AddLine(ImVec2(xg, plotY), ImVec2(xg, plotY + plotH), IM_COL32(50, 65, 95, gridAlpha));
			}
			aDraw->AddText(ImVec2(xg - 4.0f, plotY + plotH + 4.0f), m.col, m.name);
		}

		aDraw->AddRect(ImVec2(plotX, plotY), ImVec2(plotX + plotW, plotY + plotH), IM_COL32(70, 90, 130, borderAlpha), 0.0f, 0, 1.0f);
		aDraw->PushClipRect(ImVec2(plotX - 0.5f, plotY - 0.5f), ImVec2(plotX + plotW + 0.5f, plotY + plotH + 0.5f), true);

		constexpr int kSteps = 64;
		static const ImU32 kChanCol[3] = {
			IM_COL32(255, 75, 75, 255),
			IM_COL32(65, 240, 110, 255),
			IM_COL32(75, 170, 255, 255)
		};
		static const ImU32 kChanGlow[3] = {
			IM_COL32(255, 75, 75, 50),
			IM_COL32(65, 240, 110, 50),
			IM_COL32(75, 170, 255, 50)
		};

		// Continuous Gaussian / Sine-Squared Harmonic Waves (LMS-Lappen)
		auto sampleHarmonicLMS = [](float u, float& r0, float& g0, float& b0) {
			float db = (u - 0.22f);
			b0 = std::exp(-(db * db) / 0.035f);

			float dg = (u - 0.50f);
			g0 = std::exp(-(dg * dg) / 0.040f);

			float dr1 = (u - 0.78f);
			float dr2 = (u - 0.02f);
			r0 = std::exp(-(dr1 * dr1) / 0.045f) + 0.25f * std::exp(-(dr2 * dr2) / 0.012f);

			r0 = std::clamp(r0, 0.0f, 1.0f);
			g0 = std::clamp(g0, 0.0f, 1.0f);
			b0 = std::clamp(b0, 0.0f, 1.0f);
		};

		ImVec2 prevPts[3];
		for (int step = 0; step <= kSteps; ++step) {
			float u = (float)step / (float)kSteps;
			float r0, g0, b0;
			sampleHarmonicLMS(u, r0, g0, b0);

			double cr = std::clamp(aM[0][0]*r0 + aM[0][1]*g0 + aM[0][2]*b0, 0.0, 1.0);
			double cg = std::clamp(aM[1][0]*r0 + aM[1][1]*g0 + aM[1][2]*b0, 0.0, 1.0);
			double cb = std::clamp(aM[2][0]*r0 + aM[2][1]*g0 + aM[2][2]*b0, 0.0, 1.0);

			float curX = plotX + u * plotW;
			ImVec2 curPts[3] = {
				ImVec2(curX, plotY + plotH - (float)cr * plotH),
				ImVec2(curX, plotY + plotH - (float)cg * plotH),
				ImVec2(curX, plotY + plotH - (float)cb * plotH)
			};

			if (step > 0) {
				for (int ch = 0; ch < 3; ++ch) {
					aDraw->AddLine(prevPts[ch], curPts[ch], kChanGlow[ch], 4.5f);
					aDraw->AddLine(prevPts[ch], curPts[ch], kChanCol[ch], 2.2f);
				}
			}

			for (int ch = 0; ch < 3; ++ch) {
				prevPts[ch] = curPts[ch];
			}
		}

		aDraw->PopClipRect();
	}

	// ── Mode 3: Diskrete Strahlen-Zerlegung (Lineare Strahlen / Ray Scope) ─────
	// Optisch-physikalisches Prinzip eines Beugungsgitter-Spektrometers.
	// Fächert das Spektrum in diskrete, leuchtende Strahlenvektoren mit Lichtspitzen auf.
	static void DrawRayCurvePanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                              const double aM[3][3], bool aIsDetached, float aOpacity)
	{
		const float pad = 8.0f;
		const float labelSpaceLeft = 28.0f;
		const float labelSpaceBottom = 20.0f;
		const float badgeSpaceRight = 6.0f;

		const float plotX = aOrigin.x + pad + labelSpaceLeft;
		const float plotY = aOrigin.y + pad + 4.0f;
		const float plotW = aW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		const float plotH = aH - pad * 2 - labelSpaceBottom - 4.0f;

		float panelA = aIsDetached ? std::clamp(aOpacity * 0.92f, 0.0f, 0.92f) : 0.92f;
		int bgAlpha = (int)(panelA * 255.0f);
		int borderAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f, 20.0f, 150.0f)) : 150;

		aDraw->AddRectFilled(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(12, 15, 22, bgAlpha), 6.0f);
		aDraw->AddRect(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(65, 85, 125, borderAlpha), 6.0f, 0, 1.2f);

		int gridAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 65.0f, 15.0f, 65.0f)) : 65;
		int textAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f + 60.0f, 60.0f, 210.0f)) : 210;

		// Horizontal grid lines
		for (int k = 0; k <= 2; ++k) {
			float frac = k / 2.0f;
			float yg = plotY + plotH * (1.0f - frac);
			if (k > 0 && k < 2) {
				aDraw->AddLine(ImVec2(plotX, yg), ImVec2(plotX + plotW, yg), IM_COL32(50, 65, 95, gridAlpha));
			}
			char buf[16];
			std::snprintf(buf, sizeof(buf), "%d%%", (int)(frac * 100.0f));
			aDraw->AddText(ImVec2(plotX - 26.0f, yg - 6.0f), IM_COL32(125, 140, 175, textAlpha), buf);
		}

		// Vertical landmarks
		struct SpecMark { float u; const char* name; ImU32 col; };
		static const SpecMark kMarks[] = {
			{ 0.000f, "R", IM_COL32(255, 80, 80, 255) },
			{ 0.167f, "Y", IM_COL32(255, 230, 70, 255) },
			{ 0.333f, "G", IM_COL32(70, 240, 110, 255) },
			{ 0.500f, "C", IM_COL32(60, 220, 240, 255) },
			{ 0.667f, "B", IM_COL32(80, 170, 255, 255) },
			{ 0.833f, "M", IM_COL32(240, 90, 230, 255) },
			{ 1.000f, "R", IM_COL32(255, 80, 80, 255) }
		};
		for (const auto& m : kMarks) {
			float xg = plotX + plotW * m.u;
			if (m.u > 0.01f && m.u < 0.99f) {
				aDraw->AddLine(ImVec2(xg, plotY), ImVec2(xg, plotY + plotH), IM_COL32(50, 65, 95, gridAlpha));
			}
			aDraw->AddText(ImVec2(xg - 4.0f, plotY + plotH + 4.0f), m.col, m.name);
		}

		aDraw->AddRect(ImVec2(plotX, plotY), ImVec2(plotX + plotW, plotY + plotH), IM_COL32(70, 90, 130, borderAlpha), 0.0f, 0, 1.0f);
		aDraw->PushClipRect(ImVec2(plotX - 0.5f, plotY - 0.5f), ImVec2(plotX + plotW + 0.5f, plotY + plotH + 0.5f), true);

		auto sampleSpectrumRGB = [](float u, float& r0, float& g0, float& b0) {
			float h = u * 6.0f;
			float x = 1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f);
			if (h < 1.0f)      { r0 = 1.0f; g0 = x;    b0 = 0.0f; }
			else if (h < 2.0f) { r0 = x;    g0 = 1.0f; b0 = 0.0f; }
			else if (h < 3.0f) { r0 = 0.0f; g0 = 1.0f; b0 = x;    }
			else if (h < 4.0f) { r0 = 0.0f; g0 = x;    b0 = 1.0f; }
			else if (h < 5.0f) { r0 = x;    g0 = 0.0f; b0 = 1.0f; }
			else               { r0 = 1.0f; g0 = 0.0f; b0 = x;    }
		};

		// 32 Discrete Spectral Rays across spectrum
		constexpr int kRays = 32;
		for (int i = 0; i <= kRays; ++i) {
			float u = (float)i / (float)kRays;
			float r0, g0, b0;
			sampleSpectrumRGB(u, r0, g0, b0);

			double cr = std::clamp(aM[0][0]*r0 + aM[0][1]*g0 + aM[0][2]*b0, 0.0, 1.0);
			double cg = std::clamp(aM[1][0]*r0 + aM[1][1]*g0 + aM[1][2]*b0, 0.0, 1.0);
			double cb = std::clamp(aM[2][0]*r0 + aM[2][1]*g0 + aM[2][2]*b0, 0.0, 1.0);

			float curX = plotX + u * plotW;
			float baselineY = plotY + plotH;

			float yr = baselineY - (float)cr * plotH;
			float yg = baselineY - (float)cg * plotH;
			float yb = baselineY - (float)cb * plotH;

			// Red Ray with glowing pin
			aDraw->AddLine(ImVec2(curX - 1.5f, baselineY), ImVec2(curX - 1.5f, yr), IM_COL32(255, 75, 75, 140), 1.2f);
			aDraw->AddCircleFilled(ImVec2(curX - 1.5f, yr), 2.2f, IM_COL32(255, 95, 95, 230));

			// Green Ray with glowing pin
			aDraw->AddLine(ImVec2(curX, baselineY), ImVec2(curX, yg), IM_COL32(65, 240, 110, 140), 1.2f);
			aDraw->AddCircleFilled(ImVec2(curX, yg), 2.2f, IM_COL32(85, 255, 130, 230));

			// Blue Ray with glowing pin
			aDraw->AddLine(ImVec2(curX + 1.5f, baselineY), ImVec2(curX + 1.5f, yb), IM_COL32(75, 170, 255, 140), 1.2f);
			aDraw->AddCircleFilled(ImVec2(curX + 1.5f, yb), 2.2f, IM_COL32(95, 190, 255, 230));
		}

		aDraw->PopClipRect();
	}

	// ── Spectral Graph Dispatcher ─────────────────────────────────────────────
	static void DrawSpectralGraphPanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                                   const double aM[3][3], bool aIsDetached, float aOpacity, int aMode)
	{
		if (aMode == 1) {
			DrawHarmonicCurvePanel(aDraw, aOrigin, aW, aH, aM, aIsDetached, aOpacity);
		} else if (aMode == 2) {
			DrawRayCurvePanel(aDraw, aOrigin, aW, aH, aM, aIsDetached, aOpacity);
		} else {
			DrawCurvePanel(aDraw, aOrigin, aW, aH, aM, aIsDetached, aOpacity);
		}
	}

	// ── Kontrast-Kombinationen (Überlappende Farbfelder Widget) ────────────────
	static void DrawContrastCombinationsWidget(bool isDe, bool& changed, bool& saveNeeded)
	{
		const char* pairNamesDe[] = {
			"Blau / Gruen (GW2 Standard)",
			"Rot / Gruen (Protan / Deutan Test)",
			"Gelb / Blau (Tritanopie Test)",
			"Cyan / Blau (Mittelwert Kontrast)",
			"Orange / Rot (Gefahrenzonen)"
		};
		const char* pairNamesEn[] = {
			"Blue / Green (GW2 Default)",
			"Red / Green (Protan / Deutan Test)",
			"Yellow / Blue (Tritanopia Test)",
			"Cyan / Blue (Midtone Contrast)",
			"Orange / Red (Hazard Zones)"
		};

		struct ColorPair { float r1, g1, b1; float r2, g2, b2; };
		static const ColorPair kPairs[5] = {
			{ 0.212f, 0.439f, 0.800f,   0.247f, 0.616f, 0.302f }, // Blau / Gruen
			{ 0.851f, 0.275f, 0.235f,   0.247f, 0.616f, 0.302f }, // Rot / Gruen
			{ 0.910f, 0.753f, 0.125f,   0.212f, 0.439f, 0.800f }, // Gelb / Blau
			{ 0.149f, 0.682f, 0.741f,   0.212f, 0.439f, 0.800f }, // Cyan / Blau
			{ 0.910f, 0.522f, 0.059f,   0.851f, 0.275f, 0.235f }  // Orange / Rot
		};

		int pIdx = std::clamp(CurrentSettings.ContrastPairIndex, 0, 4);

		ImGui::TextDisabled("%s:", isDe ? "Farben-Paarung auswaehlen" : "Select Color Pair");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::Combo("##contrast_pair_combo", &pIdx, isDe ? pairNamesDe : pairNamesEn, 5))
		{
			CurrentSettings.ContrastPairIndex = pIdx;
			saveNeeded = true;
		}

		ColorPair p = kPairs[pIdx];

		// Compute CVD Simulation for pair
		double sim1R = p.r1, sim1G = p.g1, sim1B = p.b1;
		double sim2R = p.r2, sim2G = p.g2, sim2B = p.b2;
		ColorMatrix::SimulatePixel(p.r1, p.g1, p.b1, CurrentSettings.Type, sim1R, sim1G, sim1B);
		ColorMatrix::SimulatePixel(p.r2, p.g2, p.b2, CurrentSettings.Type, sim2R, sim2G, sim2B);

		// Compute CBA Correction for pair (supports up to 1.25x boost)
		double corrMat[3][3];
		if (CurrentSettings.Mixed)
			ColorMatrix::MixedCorrectionMatrix(CurrentSettings.MixedRgSeverity01, CurrentSettings.MixedBySeverity01, corrMat);
		else
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, corrMat);

		float cor1R = (float)std::clamp(corrMat[0][0]*p.r1 + corrMat[0][1]*p.g1 + corrMat[0][2]*p.b1, 0.0, 1.0);
		float cor1G = (float)std::clamp(corrMat[1][0]*p.r1 + corrMat[1][0]*p.g1 + corrMat[1][2]*p.b1, 0.0, 1.0);
		float cor1B = (float)std::clamp(corrMat[2][0]*p.r1 + corrMat[2][1]*p.g1 + corrMat[2][2]*p.b1, 0.0, 1.0);

		float cor2R = (float)std::clamp(corrMat[0][0]*p.r2 + corrMat[0][1]*p.g2 + corrMat[0][2]*p.b2, 0.0, 1.0);
		float cor2G = (float)std::clamp(corrMat[1][0]*p.r2 + corrMat[1][1]*p.g2 + corrMat[1][2]*p.b2, 0.0, 1.0);
		float cor2B = (float)std::clamp(corrMat[2][0]*p.r2 + corrMat[2][1]*p.g2 + corrMat[2][2]*p.b2, 0.0, 1.0);

		ImGui::Spacing();

		float availW = ImGui::GetContentRegionAvail().x;
		float cardW = (availW - 12.0f) * 0.5f;
		if (cardW < 140.0f) cardW = availW;
		float cardH = 92.0f;

		// Card 1: Ohne Filter (CVD Simulation)
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.11f, 0.15f, 0.95f));
		ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.25f, 0.30f, 0.40f, 0.50f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));

		if (ImGui::BeginChild("##contrast_card_sim", ImVec2(cardW, cardH), true, ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::TextDisabled("%s", isDe ? "Ohne Filter (CVD)" : "Without Filter (CVD)");
			ImVec2 sp = ImGui::GetCursorScreenPos();
			ImDrawList* dl = ImGui::GetWindowDrawList();

			float r = 18.0f;
			float cx1 = sp.x + 36.0f;
			float cx2 = sp.x + 62.0f;
			float cy = sp.y + 24.0f;

			ImU32 cSim1 = IM_COL32((int)(sim1R*255), (int)(sim1G*255), (int)(sim1B*255), 220);
			ImU32 cSim2 = IM_COL32((int)(sim2R*255), (int)(sim2G*255), (int)(sim2B*255), 220);

			dl->AddCircleFilled(ImVec2(cx1, cy), r, cSim1);
			dl->AddCircleFilled(ImVec2(cx2, cy), r, cSim2);
			dl->AddCircle(ImVec2(cx1, cy), r, IM_COL32(200, 200, 200, 80), 0, 1.2f);
			dl->AddCircle(ImVec2(cx2, cy), r, IM_COL32(200, 200, 200, 80), 0, 1.2f);

			ImGui::SetCursorScreenPos(ImVec2(sp.x + 92.0f, sp.y + 14.0f));
			ImGui::TextColored(Theme::kTextGoldLabel, "%s", isDe ? "Identisch /" : "Identical /");
			ImGui::SetCursorScreenPos(ImVec2(sp.x + 92.0f, sp.y + 28.0f));
			ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "%s", isDe ? "Verwechselbar" : "Confusable");

			ImGui::EndChild();
		}

		if (cardW < availW) ImGui::SameLine(0, 12.0f);

		// Card 2: Mit CBA Filter (Kompensation)
		if (ImGui::BeginChild("##contrast_card_cba", ImVec2(cardW, cardH), true, ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::TextDisabled("%s", isDe ? "Mit CBA Filter (Boost)" : "With CBA Filter (Boost)");
			ImVec2 sp = ImGui::GetCursorScreenPos();
			ImDrawList* dl = ImGui::GetWindowDrawList();

			float r = 18.0f;
			float cx1 = sp.x + 36.0f;
			float cx2 = sp.x + 62.0f;
			float cy = sp.y + 24.0f;

			ImU32 cCor1 = IM_COL32((int)(cor1R*255), (int)(cor1G*255), (int)(cor1B*255), 255);
			ImU32 cCor2 = IM_COL32((int)(cor2R*255), (int)(cor2G*255), (int)(cor2B*255), 255);

			dl->AddCircleFilled(ImVec2(cx1, cy), r, cCor1);
			dl->AddCircleFilled(ImVec2(cx2, cy), r, cCor2);
			dl->AddCircle(ImVec2(cx1, cy), r, IM_COL32(80, 240, 160, 180), 0, 1.5f);
			dl->AddCircle(ImVec2(cx2, cy), r, IM_COL32(80, 240, 160, 180), 0, 1.5f);

			ImGui::SetCursorScreenPos(ImVec2(sp.x + 92.0f, sp.y + 14.0f));
			ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Absolut" : "Distinct /");
			ImGui::SetCursorScreenPos(ImVec2(sp.x + 92.0f, sp.y + 28.0f));
			ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.55f, 1.0f), "%s", isDe ? "verschieden!" : "Separated!");

			ImGui::EndChild();
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);

		ImGui::Spacing();
		ImGui::TextUnformatted(isDe ? "Intensitaets-Skala (inkl. +25% Boost fuer maximale Unterscheidung):" 
		                            : "Intensity Scale (incl. +25% Boost for maximum distinction):");
		float availSlider = ImGui::GetContentRegionAvail().x;
		ImGui::SetNextItemWidth(availSlider);
		float sevVal = (float)CurrentSettings.Severity01;
		if (ImGui::SliderFloat("##contrast_sev_slider", &sevVal, 0.0f, 1.25f, isDe ? "%.0f%% (Kompensation)" : "%.0f%% (Compensation)", ImGuiSliderFlags_None))
		{
			CurrentSettings.Severity01 = sevVal;
			changed = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
	}

	// ── Filter-Labor & Experimentierfeld Widget ───────────────────────────────
	static void DrawFilterLabWidget(bool isDe, bool& changed, bool& saveNeeded)
	{
		bool labActive = CurrentSettings.LabModeEnabled;
		if (ImGui::Checkbox(isDe ? "Filter-Labor aktiv (Mehrfach-Filter & Erfassung)##lab_master" 
		                         : "Filter Lab Active (Multi-Filter & Capture)##lab_master", &labActive))
		{
			CurrentSettings.LabModeEnabled = labActive;
			UpdateTagEnhancerConflicts();
			changed = true;
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Aktiviert das Filter-Labor: Alle aktiven Labor-Filter werden im DXGI-Render-Loop erfasst."
			                       : "Enables the Filter Lab: All active lab filters are captured in the DXGI render loop.");
		}

		ImGui::SameLine(0, 12.0f);
		// Quick detach/dock button
		bool labDetached = CurrentSettings.ShowLabWindow;
		if (ImGui::SmallButton(labDetached ? (isDe ? "[Eigenes Fenster aktiv]" : "[Detached Window Active]")
		                                   : (isDe ? "[^] Als eigenes Fenster oeffnen" : "[^] Open in Detached Window")))
		{
			CurrentSettings.ShowLabWindow = !CurrentSettings.ShowLabWindow;
			if (CurrentSettings.ShowLabWindow) s_focusLabWindow = true;
			saveNeeded = true;
		}

		if (CurrentSettings.LabFilters.empty())
		{
			Settings::LabFilter f1;
			f1.Enabled = true;
			f1.Name = "AoE Rot zu Signal-Cyan";
			f1.TargetRgb[0] = 0.851f; f1.TargetRgb[1] = 0.275f; f1.TargetRgb[2] = 0.235f;
			f1.ReplaceRgb[0] = 0.149f; f1.ReplaceRgb[1] = 0.682f; f1.ReplaceRgb[2] = 0.741f;
			f1.ToleranceTones = 4;
			f1.Diffusion = 0.30f;
			CurrentSettings.LabFilters.push_back(f1);
		}

		int count = (int)CurrentSettings.LabFilters.size();
		int selIdx = std::clamp(CurrentSettings.SelectedLabFilterIndex, 0, count - 1);
		CurrentSettings.SelectedLabFilterIndex = selIdx;

		ImGui::Spacing();

		// Tabs for Lab: Tab 1 = Ray Matrix & Filter Design, Tab 2 = Stack Actions & Automatics
		if (ImGui::BeginTabBar("##FilterLabTabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem(isDe ? "1. Strahl-Matrix & Filter-Design##tab1" : "1. Ray Matrix & Filter Design##tab1"))
			{
				ImGui::Spacing();
				ImGui::TextDisabled("%s (%d %s):", isDe ? "Filter-Instanzen" : "Filter Instances", count, isDe ? "Instanzen" : "instances");
				ImGui::Spacing();

				// Filter instances chip bar
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
				for (int i = 0; i < count; ++i)
				{
					if (i > 0) ImGui::SameLine(0, 4.0f);
					ImGui::PushID(i + 200);

					bool isSel = (i == selIdx);
					if (isSel) {
						ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
						ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
					} else {
						ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
						ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
					}

					char btnLbl[64];
					std::snprintf(btnLbl, sizeof(btnLbl), "%s %s", CurrentSettings.LabFilters[i].Enabled ? "[*]" : "[ ]", CurrentSettings.LabFilters[i].Name.c_str());
					if (ImGui::Button(btnLbl, ImVec2(0.0f, 22.0f)))
					{
						CurrentSettings.SelectedLabFilterIndex = i;
						selIdx = i;
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);
					ImGui::PopID();
				}

				ImGui::SameLine(0, 6.0f);
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
				if (ImGui::Button("+##add_lab_filter", ImVec2(26.0f, 22.0f)))
				{
					Settings::LabFilter newF;
					newF.Enabled = true;
					char nameBuf[32];
					std::snprintf(nameBuf, sizeof(nameBuf), "Filter %d", (int)CurrentSettings.LabFilters.size() + 1);
					newF.Name = nameBuf;
					newF.TargetRgb[0] = 0.20f; newF.TargetRgb[1] = 0.50f; newF.TargetRgb[2] = 0.85f;
					newF.ReplaceRgb[0] = 0.95f; newF.ReplaceRgb[1] = 0.85f; newF.ReplaceRgb[2] = 0.20f;
					newF.ToleranceTones = 3;
					newF.Diffusion = 0.35f;
					CurrentSettings.LabFilters.push_back(newF);
					CurrentSettings.SelectedLabFilterIndex = (int)CurrentSettings.LabFilters.size() - 1;
					selIdx = CurrentSettings.SelectedLabFilterIndex;
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip(isDe ? "Neuen Filter hinzufuegen" : "Add new filter instance");
				ImGui::PopStyleVar();

				// Detail config for selected filter
				ImGui::Spacing();
				Settings::LabFilter& curF = CurrentSettings.LabFilters[selIdx];

				// Name & Active row
				if (ImGui::Checkbox(isDe ? "Aktiv##cur_lab_en" : "Active##cur_lab_en", &curF.Enabled))
				{
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}
				ImGui::SameLine(0, 10.0f);
				char nameBuf[64];
				std::snprintf(nameBuf, sizeof(nameBuf), "%s", curF.Name.c_str());
				ImGui::SetNextItemWidth(140.0f);
				if (ImGui::InputText("##cur_lab_name", nameBuf, sizeof(nameBuf)))
				{
					curF.Name = nameBuf;
					saveNeeded = true;
				}

				ImGui::Spacing();

				// ── Side-by-Side: ColorPicker on Left, XY Ray Canvas on Right ───
				float totalAvail = ImGui::GetContentRegionAvail().x;
				float pickerW = (totalAvail > 420.0f) ? 210.0f : 180.0f;
				float rayCanvasW = (totalAvail > (pickerW + 20.0f)) ? (totalAvail - pickerW - 14.0f) : 180.0f;
				float canvasH = 210.0f;

				// Left column: Color Picker & Replacement Color
				ImGui::BeginGroup();
				ImGui::TextDisabled("%s:", isDe ? "1. Ziel-Farbe (HSV-Farbrad)" : "1. Target Color (HSV Color Wheel)");
				ImGuiColorEditFlags pickerFlags = ImGuiColorEditFlags_PickerHueWheel 
				                                | ImGuiColorEditFlags_NoSidePreview 
				                                | ImGuiColorEditFlags_NoSmallPreview 
				                                | ImGuiColorEditFlags_NoAlpha;
				ImGui::SetNextItemWidth(pickerW);
				if (ImGui::ColorPicker3("##lab_picker", curF.TargetRgb, pickerFlags))
				{
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}

				ImGui::Spacing();
				ImGui::TextDisabled("%s:", isDe ? "Signal- / Ersatzfarbe" : "Signal / Replacement Color");
				ImGui::SetNextItemWidth(pickerW);
				if (ImGui::ColorEdit3("##lab_rep_picker", curF.ReplaceRgb, ImGuiColorEditFlags_NoAlpha))
				{
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}
				ImGui::EndGroup();

				// Right column: XY Color Ray Matrix Diagram
				ImGui::SameLine(0, 14.0f);
				ImGui::BeginGroup();
				ImGui::TextColored(Theme::kTextCyanLicht, "%s:", isDe ? "XY Farbstrahl-Matrix" : "XY Color Ray Matrix");

				ImVec2 p0 = ImGui::GetCursorScreenPos();
				ImDrawList* dl = ImGui::GetWindowDrawList();

				// Canvas background & border
				dl->AddRectFilled(p0, ImVec2(p0.x + rayCanvasW, p0.y + canvasH), IM_COL32(10, 14, 22, 240), 6.0f);
				dl->AddRect(p0, ImVec2(p0.x + rayCanvasW, p0.y + canvasH), IM_COL32(30, 50, 75, 180), 6.0f);

				// Canvas coordinate bounds (origin at bottom-left)
				float ox = p0.x + 28.0f;
				float oy = p0.y + canvasH - 22.0f;
				float tx = p0.x + rayCanvasW - 12.0f;
				float ty = p0.y + 12.0f;
				float spanX = std::max(tx - ox, 10.0f);
				float spanY = std::max(oy - ty, 10.0f);

				// Grid lines: 25%, 50%, 75%, 100%
				for (int g = 1; g <= 4; ++g) {
					float fRatio = g / 4.0f;
					float gx = ox + fRatio * spanX;
					float gy = oy - fRatio * spanY;
					// Horizontal
					dl->AddLine(ImVec2(ox, gy), ImVec2(tx, gy), IM_COL32(25, 40, 58, 70), 1.0f);
					// Vertical
					dl->AddLine(ImVec2(gx, oy), ImVec2(gx, ty), IM_COL32(25, 40, 58, 70), 1.0f);
				}

				// Neutral 45° dashed identity reference line
				dl->AddLine(ImVec2(ox, oy), ImVec2(tx, ty), IM_COL32(65, 90, 120, 110), 1.0f);

				// Axis indicators & clear labels
				dl->AddText(ImVec2(ox - 24.0f, ty - 6.0f), IM_COL32(130, 160, 195, 220), "Lum");
				dl->AddText(ImVec2(ox - 25.0f, oy - 0.5f * spanY - 6.0f), IM_COL32(90, 115, 140, 170), "50%");
				dl->AddText(ImVec2(tx - 22.0f, oy + 4.0f), IM_COL32(130, 160, 195, 220), "Hue");
				dl->AddText(ImVec2(ox + 0.5f * spanX - 10.0f, oy + 4.0f), IM_COL32(90, 115, 140, 170), "180");
				dl->AddText(ImVec2(ox - 10.0f, oy + 4.0f), IM_COL32(90, 115, 140, 170), "0");

				// Canvas title
				dl->AddText(ImVec2(ox + 4.0f, ty - 8.0f), IM_COL32(120, 145, 175, 190), isDe ? "Farb-Vektorraum (Hue -> Luma)" : "Color Vector Space (Hue -> Luma)");

				// Draw each filter as a color ray originating from (ox, oy)
				for (size_t fIdx = 0; fIdx < CurrentSettings.LabFilters.size(); ++fIdx)
				{
					const auto& f = CurrentSettings.LabFilters[fIdx];
					if (!f.Enabled) continue;

					float th=0, ts=0, tv=0;
					RgbToHsv(f.TargetRgb[0], f.TargetRgb[1], f.TargetRgb[2], th, ts, tv);
					float rLum = RelativeLuma(f.ReplaceRgb[0], f.ReplaceRgb[1], f.ReplaceRgb[2]);
					float normY = std::clamp(rLum * 0.85f + 0.15f, 0.08f, 1.0f);

					float rx = ox + (th / 360.0f) * spanX;
					float ry = oy - normY * spanY;

					ImU32 colTarget = IM_COL32((int)(f.TargetRgb[0]*255), (int)(f.TargetRgb[1]*255), (int)(f.TargetRgb[2]*255), 240);
					ImU32 colReplace = IM_COL32((int)(f.ReplaceRgb[0]*255), (int)(f.ReplaceRgb[1]*255), (int)(f.ReplaceRgb[2]*255), 255);

					bool isSel = ((int)fIdx == selIdx);
					if (isSel)
					{
						// Tolerance aperture cone & diffusion halo
						float coneW = (f.ToleranceTones / 32.0f) * 20.0f;
						float diffW = coneW + f.Diffusion * 14.0f;

						// Translucent diffusion fan
						if (f.Diffusion > 0.01f) {
							dl->AddTriangleFilled(ImVec2(ox, oy), ImVec2(rx - diffW, ry), ImVec2(rx + diffW, ry),
								IM_COL32((int)(f.TargetRgb[0]*255), (int)(f.TargetRgb[1]*255), (int)(f.TargetRgb[2]*255), 35));
						}
						// Precision boundary fan
						dl->AddTriangleFilled(ImVec2(ox, oy), ImVec2(rx - coneW, ry), ImVec2(rx + coneW, ry),
							IM_COL32(255, 215, 60, 45));

						// Active radiant ray line
						dl->AddLine(ImVec2(ox, oy), ImVec2(rx, ry), colReplace, 2.5f);

						// Pulse ring on beacon
						float pulse = 6.0f + 3.0f * std::sin((float)ImGui::GetTime() * 4.5f);
						dl->AddCircle(ImVec2(rx, ry), pulse, IM_COL32(0, 230, 210, 180), 0, 1.2f);
						dl->AddCircleFilled(ImVec2(rx, ry), 5.0f, colReplace);
						dl->AddCircle(ImVec2(rx, ry), 5.0f, IM_COL32(10, 16, 26, 255), 0, 1.0f);
					}
					else
					{
						// Subtle non-selected ray
						dl->AddLine(ImVec2(ox, oy), ImVec2(rx, ry), colTarget, 1.5f);
						dl->AddCircleFilled(ImVec2(rx, ry), 3.5f, colReplace);
					}
				}

				// Distance delta-E score readout in top-right
				float dr = curF.TargetRgb[0] - curF.ReplaceRgb[0];
				float dg = curF.TargetRgb[1] - curF.ReplaceRgb[1];
				float db = curF.TargetRgb[2] - curF.ReplaceRgb[2];
				float dScore = std::sqrt(dr*dr + dg*dg + db*db);
				char dScoreBuf[64];
				std::snprintf(dScoreBuf, sizeof(dScoreBuf), "Delta-E: %.2f", dScore);
				ImVec2 dSize = ImGui::CalcTextSize(dScoreBuf);
				dl->AddText(ImVec2(tx - dSize.x - 2.0f, ty - 8.0f), IM_COL32(140, 235, 230, 240), dScoreBuf);

				ImGui::Dummy(ImVec2(rayCanvasW, canvasH));
				ImGui::EndGroup();

				// Sliders for Precision Radius & Diffusion
				ImGui::Spacing();
				float fullW = ImGui::GetContentRegionAvail().x;
				ImGui::TextUnformatted(isDe ? "2. Begrenzungsradius & Praezision (Schwellenwert):" 
				                            : "2. Boundary Radius & Precision (Threshold):");
				ImGui::SetNextItemWidth(fullW);
				if (ImGui::SliderInt("##lab_tol_slider", &curF.ToleranceTones, 1, 32, isDe ? "+/- %d Farbtoene (Hex-Toleranz)" : "+/- %d Color Tones (Hex-Tolerance)"))
				{
					UpdateTagEnhancerConflicts();
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::Spacing();
				ImGui::TextUnformatted(isDe ? "3. Leichte Diffusion / Sanfter Uebergang (Feathering):" 
				                            : "3. Soft Diffusion / Smooth Edge (Feathering):");
				ImGui::SetNextItemWidth(fullW);
				if (ImGui::SliderFloat("##lab_diff_slider", &curF.Diffusion, 0.0f, 1.0f, "%.0f%% (Diffusion)"))
				{
					UpdateTagEnhancerConflicts();
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(isDe ? "2. Aktionen & Automatiken##tab2" : "2. Stack & Automatics##tab2"))
			{
				ImGui::Spacing();
				Settings::LabFilter& curF = CurrentSettings.LabFilters[selIdx];

				// Duplicate & Delete
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
				if (ImGui::Button(isDe ? "Filter duplizieren" : "Duplicate Filter", ImVec2(150.0f, 24.0f)))
				{
					Settings::LabFilter dupF = curF;
					dupF.Name += isDe ? " (Kopie)" : " (Copy)";
					CurrentSettings.LabFilters.push_back(dupF);
					CurrentSettings.SelectedLabFilterIndex = (int)CurrentSettings.LabFilters.size() - 1;
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);

				if (CurrentSettings.LabFilters.size() > 1)
				{
					ImGui::SameLine(0, 10.0f);
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
					if (ImGui::Button(isDe ? "Filter loeschen" : "Delete Filter", ImVec2(125.0f, 24.0f)))
					{
						CurrentSettings.LabFilters.erase(CurrentSettings.LabFilters.begin() + selIdx);
						CurrentSettings.SelectedLabFilterIndex = std::max(0, selIdx - 1);
						UpdateTagEnhancerConflicts();
						changed = true;
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				// Automatics Buttons
				ImGui::TextDisabled("%s:", isDe ? "Automatiken fuer ausgewaehlten Filter" : "Automatics for Selected Filter");
				ImGui::Spacing();

				// Auto-Complementary (CVD Opt)
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
				if (ImGui::Button(isDe ? "Auto-Komplementaer (CVD Opt)##lab" : "Auto-Complementary (CVD Opt)##lab", ImVec2(225.0f, 26.0f)))
				{
					float h=0, s=0, v=0;
					RgbToHsv(curF.TargetRgb[0], curF.TargetRgb[1], curF.TargetRgb[2], h, s, v);
					float compH = std::fmod(h + 180.0f, 360.0f);
					float nr=0, ng=0, nb=0;
					HsvToRgb(compH, std::max(s, 0.75f), std::max(v, 0.85f), nr, ng, nb);
					curF.ReplaceRgb[0] = nr;
					curF.ReplaceRgb[1] = ng;
					curF.ReplaceRgb[2] = nb;
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe ? "Berechnet die mathematische Gegenfarbe fuer maximale CVD-Unterscheidbarkeit."
					                       : "Computes the complementary color for maximum CVD distinction.");
				}

				ImGui::SameLine(0, 10.0f);
				// Auto-Luminance
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
				if (ImGui::Button(isDe ? "Auto-Luminanz (WCAG)##lab" : "Auto-Luminance (WCAG)##lab", ImVec2(190.0f, 26.0f)))
				{
					float lum = RelativeLuma(curF.TargetRgb[0], curF.TargetRgb[1], curF.TargetRgb[2]);
					if (lum > 0.45f) {
						curF.ReplaceRgb[0] *= 0.35f;
						curF.ReplaceRgb[1] *= 0.35f;
						curF.ReplaceRgb[2] *= 0.35f;
					} else {
						curF.ReplaceRgb[0] = std::clamp(curF.ReplaceRgb[0] * 1.5f + 0.3f, 0.0f, 1.0f);
						curF.ReplaceRgb[1] = std::clamp(curF.ReplaceRgb[1] * 1.5f + 0.3f, 0.0f, 1.0f);
						curF.ReplaceRgb[2] = std::clamp(curF.ReplaceRgb[2] * 1.5f + 0.3f, 0.0f, 1.0f);
					}
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe ? "Passt die Helligkeit an, um mindestens WCAG 4.5:1 Kontrast zu gewaehrleisten."
					                       : "Adjusts brightness to ensure at least WCAG 4.5:1 contrast ratio.");
				}

				// Quick presets row
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::TextDisabled("%s:", isDe ? "GW2 Farb-Schnellauswahl" : "GW2 Color Presets");
				auto quickPick = [&](const char* lbl, float r, float g, float b) {
					if (ImGui::Button(lbl)) {
						curF.TargetRgb[0] = r; curF.TargetRgb[1] = g; curF.TargetRgb[2] = b;
						UpdateTagEnhancerConflicts();
						changed = true; saveNeeded = true;
					}
				};
				quickPick(isDe ? "AoE Rot##qp" : "AoE Red##qp", 0.851f, 0.275f, 0.235f);
				ImGui::SameLine(0, 6.0f);
				quickPick(isDe ? "Gift Gruen##qp" : "Poison Green##qp", 0.247f, 0.616f, 0.302f);
				ImGui::SameLine(0, 6.0f);
				quickPick(isDe ? "Wasser Cyan##qp" : "Water Cyan##qp", 0.149f, 0.682f, 0.741f);
				ImGui::SameLine(0, 6.0f);
				quickPick(isDe ? "Banner Gold##qp" : "Banner Gold##qp", 0.910f, 0.753f, 0.125f);

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	void RenderEmbeddedOptions()
	{
		if (!ImGui::GetCurrentContext()) return;
		const L10n& t = Strings();
		bool changed = false;
		bool saveNeeded = false;
		bool isDe = (t.Enabled[0] == 'A');

		ImGui::PushID("CBA_Embedded");

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

		// ── Row 1: Primary Window Toggles + Master ON/OFF + Live Status ───────
		// Button 1: Main Window (Toggle)
		bool mainOpen = CurrentSettings.ShowMainWindow;
		if (mainOpen) {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
		}
		if (ImGui::Button(t.OpenMainWindow, ImVec2(120.0f, 26.0f)))
		{
			EnsureDeferredInitialized();
			CurrentSettings.ShowMainWindow = !CurrentSettings.ShowMainWindow;
			if (CurrentSettings.ShowMainWindow)
			{
				s_focusMainWindow = true;
			}
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.OpenMainWindowTooltip);

		ImGui::SameLine(0, 6.0f);

		// Button 2: Sensor Graph HUD (Toggle)
		bool graphOpen = CurrentSettings.ShowGraphWindow;
		if (graphOpen) {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
		}
		if (ImGui::Button(t.OpenSensorGraph, ImVec2(120.0f, 26.0f)))
		{
			EnsureDeferredInitialized();
			CurrentSettings.ShowGraphWindow = !CurrentSettings.ShowGraphWindow;
			if (CurrentSettings.ShowGraphWindow)
			{
				s_focusGraphWindow = true;
			}
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.OpenSensorGraphTooltip);

		ImGui::SameLine(0, 8.0f);

		// Button 3: Master ON / OFF toggle
		{
			bool wasEnabled = CurrentSettings.Enabled;
			if (wasEnabled) {
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			}
			if (ImGui::Button(wasEnabled ? "ON##opt_master" : "OFF##opt_master", ImVec2(56.0f, 26.0f))) {
				EnsureDeferredInitialized();
				CurrentSettings.Enabled = !CurrentSettings.Enabled;
				CurrentSettings.Save(AddonDir);
				Recompute(/*aForce=*/true);
				changed    = true;
				saveNeeded = false;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(wasEnabled ? (isDe ? "Filter aktiv - Klicke zum Ausschalten" : "Filter active - click to disable")
				                             : (isDe ? "Filter inaktiv - Klicke zum Einschalten" : "Filter inactive - click to enable"));
		}

		ImGui::SameLine(0, 10.0f);
		DrawFilterStatusIndicator(true);

		// ── Row 2: Reset & Recovery Actions (Separate Line, never truncated) ───
		ImGui::Spacing();

		// Button 4: Reset Windows Position
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextPrimary);
		if (ImGui::Button(isDe ? "Reset UI" : "Reset UI", ImVec2(120.0f, 26.0f)))
		{
			s_resetMainWindowPos = true;
			s_resetGraphWindowPos = true;
			s_resetLabWindowPos = true;
			CurrentSettings.ShowMainWindow = true;
			s_focusMainWindow = true;
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt Position und Groesse aller CBA-Fenster (Hauptfenster, Sensor-Graph, Filter-Labor) auf Standardkoordinaten links oben zurueck."
			                       : "Resets position and size of all CBA windows (Main Window, Sensor Graph, Filter Lab) to default top-left coordinates.");
		}

		ImGui::SameLine(0, 8.0f);

		// Button 5: Reset Filter to Neutral
		if (ImGui::Button(isDe ? "Filter auf Neutral" : "Reset Filter to Neutral", ImVec2(190.0f, 26.0f)))
		{
			CurrentSettings.Enabled = false;
			CurrentSettings.Severity01 = 0.0;
			CurrentSettings.Mixed = false;
			CurrentSettings.MixedRgSeverity01 = 0.0;
			CurrentSettings.MixedBySeverity01 = 0.0;
			CurrentSettings.GammaGain = 1.0f;
			CurrentSettings.CommanderTagMode = 0;
			CurrentSettings.AutoBrightness = false;
			CurrentSettings.Save(AddonDir);
			{
				std::lock_guard<std::mutex> lock(s_recomputeMutex);
				GetColorEffectController().Clear();
				s_hasApplied = false;
			}
			Recompute(/*aForce=*/true);
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt Farbkorrektur, Helligkeit und Commander-Tags komplett auf neutral (Filter AUS, Staerke 0, Gamma 1.00x)."
			                       : "Resets all color correction, brightness and tag settings to neutral defaults (Filter OFF, Severity 0, Gamma 1.00x).");
		}
		ImGui::PopStyleColor(4);

		ImGui::PopStyleVar(2); // Pop FrameRounding & ItemSpacing

		// ── Subtle Divider & Subtitle ─────────────────────────────────────────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(Theme::kTextCyanLicht, "Color Logic Balancer & Enhancer");
		ImGui::SameLine();
		ImGui::TextDisabled("(cba4gw2)");
		ImGui::TextColored(Theme::kTextSecondary, isDe ? "Barrierefreie Farb- & Kontrastoptimierung fuer Guild Wars 2"
		                                               : "Accessible color & contrast balancer for Guild Wars 2");
		ImGui::Spacing();

		// ── Nexus Integrations & Startup Behavior ──────────────────────────────
		if (ImGui::Checkbox(t.ShowQuickAccess, &CurrentSettings.ShowQuickAccessIcon)) {
			UpdateQuickAccessIcon();
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.ShowQuickAccessTooltip);

		if (ImGui::Checkbox(t.LoadOnStartup, &CurrentSettings.LoadOnStartup)) {
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.LoadOnStartupTooltip);

		if (ImGui::Checkbox(t.KeepActiveBackground, &CurrentSettings.SystemWide)) {
			changed = true;
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.KeepActiveBackgroundTooltip);

		if (saveNeeded) {
			CurrentSettings.Save(AddonDir);
			Recompute(/*aForce=*/true);
		} else if (changed) {
			Recompute(/*aForce=*/false);
		}

		ImGui::PopID();
	}

	void RenderMainWindow()
	{
		if (!ImGui::GetCurrentContext()) return;
		const L10n& t = Strings();
		bool changed = false;
		bool saveNeeded = false;
		bool isDe = (t.Enabled[0] == 'A');

		ImGui::PushID("CBA_MainWindow");

		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 4.0f;
		style.WindowRounding = 6.0f;
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.24f, 0.44f, 0.68f, 0.85f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.54f, 0.82f, 0.95f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.36f, 0.58f, 1.00f);
		style.ItemSpacing = ImVec2(8, 5);

		// ── Fixed Top Header Bar (Always pinned on top, never scrolls) ──────
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(6.0f, 3.0f));

		ImGui::TextColored(Theme::kTextBlauPeak, "cba4gw2");
		ImGui::SameLine(0, 8.0f);

		// Master ON/OFF toggle button
		{
			bool wasEnabled = CurrentSettings.Enabled;
			if (wasEnabled) {
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
				ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.00f, 0.28f, 0.28f, 1.00f));
			}
			if (ImGui::Button(wasEnabled ? "ON##main_master" : "OFF##main_master", ImVec2(56.0f, 24.0f))) {
				EnsureDeferredInitialized();
				CurrentSettings.Enabled = !CurrentSettings.Enabled;
				CurrentSettings.Save(AddonDir);
				Recompute(/*aForce=*/true);
				changed    = true;
				saveNeeded = false;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(wasEnabled ? (isDe ? "Filter aktiv - Klicke zum Ausschalten" : "Filter active - click to disable")
				                             : (isDe ? "Filter inaktiv - Klicke zum Einschalten" : "Filter inactive - click to enable"));

			// Tiny orbiting mini-star along button perimeter when OFF
			if (!wasEnabled)
			{
				ImVec2 bMin = ImGui::GetItemRectMin();
				ImVec2 bMax = ImGui::GetItemRectMax();
				float bw = bMax.x - bMin.x;
				float bh = bMax.y - bMin.y;
				float peri = 2.0f * (bw + bh);
				float t = (float)ImGui::GetTime();
				float cyclePeriod = 3.6f;
				float cycleIdx = std::floor(t / cyclePeriod);
				float cycleFrac = (t - cycleIdx * cyclePeriod) / cyclePeriod;
				bool reverse = (((int)cycleIdx) & 1) != 0;
				float u = reverse ? (1.0f - cycleFrac) : cycleFrac;

				auto getPerimeterPoint = [&](float uNorm) -> ImVec2 {
					uNorm = uNorm - std::floor(uNorm);
					float dist = uNorm * peri;
					if (dist < bw) {
						return ImVec2(bMin.x + dist, bMin.y);
					} else if (dist < bw + bh) {
						return ImVec2(bMax.x, bMin.y + (dist - bw));
					} else if (dist < 2.0f * bw + bh) {
						return ImVec2(bMax.x - (dist - (bw + bh)), bMax.y);
					} else {
						return ImVec2(bMin.x, bMax.y - (dist - (2.0f * bw + bh)));
					}
				};

				ImVec2 pt = getPerimeterPoint(u);
				float trailOffset = reverse ? 0.04f : -0.04f;
				ImVec2 ptTrail = getPerimeterPoint(u + trailOffset);

				ImDrawList* dl = ImGui::GetWindowDrawList();
				// Faint soft red halo
				dl->AddCircleFilled(pt, 3.2f, IM_COL32(255, 60, 60, 65));
				// Soft trail dot
				dl->AddCircleFilled(ptTrail, 1.0f, IM_COL32(255, 110, 110, 110));
				// Bright white star core
				dl->AddCircleFilled(pt, 1.3f, IM_COL32(255, 255, 255, 255));
				// Micro 4-spoke star glint
				float glint = 2.4f;
				dl->AddLine(ImVec2(pt.x - glint, pt.y), ImVec2(pt.x + glint, pt.y), IM_COL32(255, 220, 220, 210), 1.0f);
				dl->AddLine(ImVec2(pt.x, pt.y - glint), ImVec2(pt.x, pt.y + glint), IM_COL32(255, 220, 220, 210), 1.0f);
			}
		}

		ImGui::SameLine(0, 6.0f);
		DrawFilterStatusIndicator(false);

		ImGui::SameLine(0, 6.0f);
		bool graphOpen = CurrentSettings.ShowGraphWindow;
		if (graphOpen) {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
		}
		if (ImGui::Button(isDe ? "Sensor-Graph##main_top" : "Sensor Graph##main_top", ImVec2(106.0f, 24.0f))) {
			CurrentSettings.ShowGraphWindow = !CurrentSettings.ShowGraphWindow;
			if (CurrentSettings.ShowGraphWindow) s_focusGraphWindow = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", t.OpenSensorGraphTooltip);
		}

		ImGui::SameLine(0, 5.0f);
		bool labOpen = CurrentSettings.ShowLabWindow;
		if (labOpen) {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
		}
		if (ImGui::Button(isDe ? "Filter-Labor##main_top" : "Filter Lab##main_top", ImVec2(96.0f, 24.0f))) {
			CurrentSettings.ShowLabWindow = !CurrentSettings.ShowLabWindow;
			if (CurrentSettings.ShowLabWindow) s_focusLabWindow = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Filter-Labor als eigenes Fenster oeffnen oder schliessen" : "Open or close Filter Lab detached window");
		}

		ImGui::SameLine(0, 5.0f);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextPrimary);
		if (ImGui::Button("Reset UI##main", ImVec2(84.0f, 24.0f))) {
			s_resetMainWindowPos = true;
			s_resetGraphWindowPos = true;
			s_resetLabWindowPos = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Setzt alle CBA-Fenster (Hauptfenster, Sensor-Graph, Filter-Labor) auf Standardposition links oben zurueck."
			                       : "Resets all CBA windows (Main Window, Sensor Graph, Filter Lab) to default top-left position.");
		}

		ImGui::PopStyleVar(2); // Pop FrameRounding & FramePadding

		ImGui::Spacing();
		ImGui::TextDisabled("%s", isDe ? "Farb- & Kontrastanpassung fuer Barrierefreiheit in Guild Wars 2 (DWM / Live-Filter)"
		                               : "Accessible Color & Contrast Enhancer for Guild Wars 2 (DWM / Live Filter)");
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ── Scrollable Body Content (Fixed Header stays pinned on top) ───────
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 4.0f));
		ImGui::BeginChild("##MainWindowScrollContent", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
		ImGui::PopStyleVar();

		// Helper lambda for dynamic slider width in narrow windows (~350px)
		auto calcSliderWidth = []() {
			float avail = ImGui::GetContentRegionAvail().x;
			return (avail > 140.0f) ? (avail - 65.0f) : 180.0f;
		};

		// State tracking for unfolding sections so newly opened sections have room and can auto-scroll into full view
		static bool s_secOpen[8] = { true, true, false, false, false, false, false, false };

		auto renderSectionHeader = [&](int secIdx, const char* label, ImGuiTreeNodeFlags extraFlags = 0) -> bool {
			bool wasOpen = s_secOpen[secIdx];
			ImGuiTreeNodeFlags flags = extraFlags;
			if (wasOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

			// Clear vertical spacing between collapsed or adjacent sections
			ImGui::Spacing();

			bool isOpen = ImGui::CollapsingHeader(label, flags);
			if (ImGui::IsItemClicked()) {
				if (!wasOpen) {
					// User just unfolded this section downwards!
					// Scroll so this header is positioned at the top of the viewport,
					// giving the whole section ample space to display its entire content!
					ImGui::SetScrollHereY(0.0f);
				}
			}
			s_secOpen[secIdx] = isOpen;
			return isOpen;
		};

		auto endSection = []() {
			ImGui::Spacing();
			ImGui::Dummy(ImVec2(0.0f, 12.0f)); // Generous breathing room so the rubrik is clearly separated from the next one
		};

		// ── Section 1: Farbprofil & Korrektur (Default Open) ─────────────────
		if (renderSectionHeader(0, t.HeaderSection1, ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Language selector row
			{
				ImGui::TextDisabled("%s:", t.Language);
				ImGui::SameLine(0, 8.0f);

				int langComboIdx = 0;
				if (CurrentSettings.Language == 0) langComboIdx = 1;      // System (Windows)
				else if (CurrentSettings.Language == 2) langComboIdx = 2; // Deutsch
				else langComboIdx = 0;                                     // English (1)

				const char* langComboItems[] = {
					"English",
					"System (Windows)",
					"Deutsch"
				};

				ImGui::SetNextItemWidth(140.0f);
				if (ImGui::Combo("##LangComboMain", &langComboIdx, langComboItems, IM_ARRAYSIZE(langComboItems)))
				{
					if (langComboIdx == 0) CurrentSettings.Language = 1;
					else if (langComboIdx == 1) CurrentSettings.Language = 0;
					else if (langComboIdx == 2) CurrentSettings.Language = 2;
					changed = true;
					saveNeeded = true;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.Language);
			}

			ImGui::Spacing();

			// Radio buttons for balance types
			auto typeBtn = [&](const char* aLabel, bool aActive, BalanceType aType) {
				if (ImGui::RadioButton(aLabel, aActive)) {
					if (CurrentSettings.Mixed || CurrentSettings.Type != aType) {
						CurrentSettings.Mixed = false;
						CurrentSettings.Type  = aType;
						CurrentSettings.Severity01 = 0.0;
						changed = true;
					}
				}
			};
			typeBtn(t.Protan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Protan, BalanceType::Protan);
			ImGui::SameLine();
			typeBtn(t.Deutan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Deutan, BalanceType::Deutan);
			ImGui::SameLine();
			typeBtn(t.Tritan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Tritan, BalanceType::Tritan);
			ImGui::SameLine();
			if (ImGui::RadioButton(t.Mixed, CurrentSettings.Mixed)) {
				if (!CurrentSettings.Mixed) {
					CurrentSettings.Mixed = true;
					CurrentSettings.MixedRgSeverity01 = 0.0;
					CurrentSettings.MixedBySeverity01 = 0.0;
					changed = true;
				}
			}

			ImGui::Spacing();

			if (CurrentSettings.Mixed) {
				float rg = (float)CurrentSettings.MixedRgSeverity01;
				float by = (float)CurrentSettings.MixedBySeverity01;

				ImGui::TextUnformatted(t.RgStrength);
				float avail = ImGui::GetContentRegionAvail().x;
				float btnW = 56.0f;
				float sp = 6.0f;
				float sW = (avail > (btnW + sp + 60.0f)) ? (avail - btnW - sp) : 180.0f;

				ImGui::SetNextItemWidth(sW);
				if (ImGui::SliderFloat("##rg_det", &rg, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
					CurrentSettings.MixedRgSeverity01 = std::clamp(rg, 0.0f, 1.25f);
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
				ImGui::SameLine(0, sp);
				if (ImGui::Button("Reset##rg_det", ImVec2(btnW, 0.0f))) { 
					CurrentSettings.MixedRgSeverity01 = 0.0f; 
					changed = true; 
					saveNeeded = true; 
				}
				
				ImGui::TextUnformatted(t.ByStrength);
				ImGui::SetNextItemWidth(sW);
				if (ImGui::SliderFloat("##by_det", &by, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
					CurrentSettings.MixedBySeverity01 = std::clamp(by, 0.0f, 1.25f);
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
				ImGui::SameLine(0, sp);
				if (ImGui::Button("Reset##by_det", ImVec2(btnW, 0.0f))) { 
					CurrentSettings.MixedBySeverity01 = 0.0f; 
					changed = true; 
					saveNeeded = true; 
				}
			} else {
				float sev = (float)CurrentSettings.Severity01;

				ImGui::TextUnformatted(t.Strength);
				float avail = ImGui::GetContentRegionAvail().x;
				float btnW = 56.0f;
				float sp = 6.0f;
				float sW = (avail > (btnW + sp + 60.0f)) ? (avail - btnW - sp) : 180.0f;

				ImGui::SetNextItemWidth(sW);
				if (ImGui::SliderFloat("##sev_det", &sev, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
					CurrentSettings.Severity01 = std::clamp(sev, 0.0f, 1.25f);
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
				ImGui::SameLine(0, sp);
				if (ImGui::Button("Reset##sev_det", ImVec2(btnW, 0.0f))) { 
					CurrentSettings.Severity01 = 0.0f; 
					changed = true; 
					saveNeeded = true; 
				}
			}

			// ── 3-Slot Profile Management System ──────────────────────────────
			ImGui::Spacing();
			int usedCount = 0;
			int firstEmptySlot = -1;
			for (int i = 0; i < 3; ++i) {
				if (CurrentSettings.Slots[i].Used) usedCount++;
				else if (firstEmptySlot == -1) firstEmptySlot = i;
			}

			static BalanceType s_baseType = CurrentSettings.Type;
			static double s_baseSev = CurrentSettings.Severity01;
			static bool s_baseMixed = CurrentSettings.Mixed;
			static double s_baseMixedRg = CurrentSettings.MixedRgSeverity01;
			static double s_baseMixedBy = CurrentSettings.MixedBySeverity01;
			static float s_baseGamma = CurrentSettings.GammaGain;
			static int s_activeSlotIdx = 0;
			static bool s_hasBaseline = false;
			if (!s_hasBaseline) {
				s_baseType = CurrentSettings.Type;
				s_baseSev = CurrentSettings.Severity01;
				s_baseMixed = CurrentSettings.Mixed;
				s_baseMixedRg = CurrentSettings.MixedRgSeverity01;
				s_baseMixedBy = CurrentSettings.MixedBySeverity01;
				s_baseGamma = CurrentSettings.GammaGain;
				s_hasBaseline = true;
			}

			bool isDirty = (CurrentSettings.Type != s_baseType ||
			                std::abs(CurrentSettings.Severity01 - s_baseSev) > 0.005 ||
			                CurrentSettings.Mixed != s_baseMixed ||
			                std::abs(CurrentSettings.MixedRgSeverity01 - s_baseMixedRg) > 0.005 ||
			                std::abs(CurrentSettings.MixedBySeverity01 - s_baseMixedBy) > 0.005 ||
			                std::abs(CurrentSettings.GammaGain - s_baseGamma) > 0.005f);

			static auto s_profileFeedbackTime = std::chrono::steady_clock::time_point{};
			static std::string s_profileFeedbackMsg = "";

			// Profile Quick Bar: "Profiles:" + Tiny Save Button (Grey -> Green -> Orange) + Slots [1] [2] [3]
			ImGui::TextDisabled("%s:", isDe ? "Profile" : "Profiles");
			ImGui::SameLine(0, 8.0f);

			// Dynamic Tiny Save Button (52px/72px width, 22px height)
			ImVec4 btnCol, btnHover, btnActive, textCol;
			const char* saveTip = "";

			if (!isDirty) {
				// State 1: Grey / Neutral (Profile up to date / saved)
				btnCol    = Theme::kBtnNeutralIdle;
				btnHover  = Theme::kBtnNeutralHover;
				btnActive = Theme::kBtnNeutralPress;
				textCol   = Theme::kTextSecondary;
				saveTip   = isDe ? "Profil unveraendert / aktuell" : "Profile up to date (no unsaved changes)";
			} else if (usedCount < 3) {
				// State 2: Green / Emerald (Settings changed -> ready to save)
				btnCol    = ImVec4(0.12f, 0.46f, 0.26f, 0.95f);
				btnHover  = ImVec4(0.16f, 0.58f, 0.34f, 1.00f);
				btnActive = ImVec4(0.09f, 0.36f, 0.20f, 1.00f);
				textCol   = Theme::GetContrastTextColor(btnCol);
				saveTip   = isDe ? "Einstellung geaendert! Klicke zum Speichern in freien Slot" : "Settings changed! Click to save to empty slot";
			} else {
				// State 3: Orange / Amber (Settings changed BUT all 3 slots are full -> warning: will overwrite!)
				btnCol    = ImVec4(0.72f, 0.36f, 0.08f, 0.95f);
				btnHover  = ImVec4(0.85f, 0.44f, 0.10f, 1.00f);
				btnActive = ImVec4(0.58f, 0.28f, 0.06f, 1.00f);
				textCol   = Theme::GetContrastTextColor(btnCol);
				saveTip   = isDe ? "Alle Slots voll! Klicke zum Ueberschreiben des aktiven Slots" : "All 3 slots full! Click to overwrite active slot";
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleColor(ImGuiCol_Button,        btnCol);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btnActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          textCol);

			if (ImGui::Button(isDe ? "Speichern##tiny_prof" : "Save##tiny_prof", ImVec2(isDe ? 72.0f : 52.0f, 22.0f)))
			{
				int targetSlot = (firstEmptySlot != -1) ? firstEmptySlot : std::clamp(s_activeSlotIdx, 0, 2);
				CurrentSettings.Slots[targetSlot].Used = true;
				char defaultName[64];
				if (CurrentSettings.Mixed) {
					std::snprintf(defaultName, sizeof(defaultName), "Slot %d (Mixed %d%%/%d%%)", targetSlot + 1,
						(int)(CurrentSettings.MixedRgSeverity01 * 100), (int)(CurrentSettings.MixedBySeverity01 * 100));
				} else {
					const char* tn = (CurrentSettings.Type == BalanceType::Protan) ? "Protan" :
					                 (CurrentSettings.Type == BalanceType::Deutan) ? "Deutan" : "Tritan";
					std::snprintf(defaultName, sizeof(defaultName), "Slot %d (%s %d%%)", targetSlot + 1, tn, (int)(CurrentSettings.Severity01 * 100));
				}
				if (CurrentSettings.Slots[targetSlot].Name.empty()) {
					CurrentSettings.Slots[targetSlot].Name = defaultName;
				}
				CurrentSettings.Slots[targetSlot].Type = CurrentSettings.Type;
				CurrentSettings.Slots[targetSlot].Severity01 = CurrentSettings.Severity01;
				CurrentSettings.Slots[targetSlot].Mixed = CurrentSettings.Mixed;
				CurrentSettings.Slots[targetSlot].MixedRg01 = CurrentSettings.MixedRgSeverity01;
				CurrentSettings.Slots[targetSlot].MixedBy01 = CurrentSettings.MixedBySeverity01;
				CurrentSettings.Slots[targetSlot].GammaGain = CurrentSettings.GammaGain;

				// Update baseline so button immediately turns back to grey!
				s_baseType = CurrentSettings.Type;
				s_baseSev = CurrentSettings.Severity01;
				s_baseMixed = CurrentSettings.Mixed;
				s_baseMixedRg = CurrentSettings.MixedRgSeverity01;
				s_baseMixedBy = CurrentSettings.MixedBySeverity01;
				s_baseGamma = CurrentSettings.GammaGain;
				s_activeSlotIdx = targetSlot;

				CurrentSettings.Save(AddonDir);
				s_profileFeedbackTime = std::chrono::steady_clock::now();
				s_profileFeedbackMsg = isDe ? "[OK] Gespeichert in Slot " + std::to_string(targetSlot + 1) : "[OK] Saved to Slot " + std::to_string(targetSlot + 1);
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", saveTip);

			// Slot quick selectors [1] [2] [3]
			for (int sIdx = 0; sIdx < 3; ++sIdx)
			{
				ImGui::SameLine(0, 4.0f);
				ImGui::PushID(sIdx + 450);
				bool used = CurrentSettings.Slots[sIdx].Used;
				bool isActive = (s_activeSlotIdx == sIdx);

				if (isActive) {
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
				} else if (used) {
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
				} else {
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
				}

				char slotChip[32];
				std::snprintf(slotChip, sizeof(slotChip), "[%d]", sIdx + 1);
				if (ImGui::Button(slotChip, ImVec2(32.0f, 22.0f)))
				{
					s_activeSlotIdx = sIdx;
					if (used)
					{
						CurrentSettings.Type = CurrentSettings.Slots[sIdx].Type;
						CurrentSettings.Severity01 = CurrentSettings.Slots[sIdx].Severity01;
						CurrentSettings.Mixed = CurrentSettings.Slots[sIdx].Mixed;
						CurrentSettings.MixedRgSeverity01 = CurrentSettings.Slots[sIdx].MixedRg01;
						CurrentSettings.MixedBySeverity01 = CurrentSettings.Slots[sIdx].MixedBy01;
						CurrentSettings.GammaGain = CurrentSettings.Slots[sIdx].GammaGain;

						s_baseType = CurrentSettings.Type;
						s_baseSev = CurrentSettings.Severity01;
						s_baseMixed = CurrentSettings.Mixed;
						s_baseMixedRg = CurrentSettings.MixedRgSeverity01;
						s_baseMixedBy = CurrentSettings.MixedBySeverity01;
						s_baseGamma = CurrentSettings.GammaGain;

						changed = true;
						saveNeeded = true;
					}
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered())
				{
					if (used)
						ImGui::SetTooltip("Slot %d: %s\n%s", sIdx + 1, CurrentSettings.Slots[sIdx].Name.c_str(), isDe ? "Klicken zum Laden" : "Click to load");
					else
						ImGui::SetTooltip("Slot %d: %s", sIdx + 1, isDe ? "Frei" : "Empty");
				}
				ImGui::PopID();
			}

			if (s_profileFeedbackTime.time_since_epoch().count() > 0) {
				auto now = std::chrono::steady_clock::now();
				float elapsed = std::chrono::duration<float>(now - s_profileFeedbackTime).count();
				if (elapsed >= 0.0f && elapsed < 4.5f) {
					float alpha = (elapsed > 3.0f) ? (4.5f - elapsed) / 1.5f : 1.0f;
					alpha = std::clamp(alpha, 0.0f, 1.0f);
					std::string savePath = AddonDir.empty() ? "settings.ini" : (AddonDir + "\\settings.ini");

					ImGui::SameLine(0, 10.0f);
					ImVec4 feedbackCol = Theme::kTextCyanLicht;
					feedbackCol.w = alpha;
					ImGui::TextColored(feedbackCol, "%s", s_profileFeedbackMsg.c_str());
					ImVec4 pathCol = Theme::kTextSecondary;
					pathCol.w = alpha;
					ImGui::TextColored(pathCol, isDe ? "  Pfad: %s" : "  Path: %s", savePath.c_str());
				}
			}

			// Render the 3 Slots with Load and Clear
			ImGui::Spacing();
			for (int sIdx = 0; sIdx < 3; ++sIdx) {
				ImGui::PushID(sIdx + 300);
				if (CurrentSettings.Slots[sIdx].Used) {
					char nameBuf[64];
					std::snprintf(nameBuf, sizeof(nameBuf), "%s", CurrentSettings.Slots[sIdx].Name.c_str());
					float cardAvail = ImGui::GetContentRegionAvail().x;
					float actionW = 90.0f;
					float nameW = (cardAvail > 214.0f) ? (cardAvail - actionW - 8.0f) : 120.0f;

					ImGui::SetNextItemWidth(nameW);
					if (ImGui::InputText("##slot_name", nameBuf, sizeof(nameBuf))) {
						CurrentSettings.Slots[sIdx].Name = nameBuf;
						saveNeeded = true;
					}
					ImGui::SameLine(0, 4.0f);
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
					if (ImGui::Button(isDe ? "Laden" : "Load", ImVec2(56.0f, 0.0f))) {
						s_activeSlotIdx = sIdx;
						CurrentSettings.Type = CurrentSettings.Slots[sIdx].Type;
						CurrentSettings.Severity01 = CurrentSettings.Slots[sIdx].Severity01;
						CurrentSettings.Mixed = CurrentSettings.Slots[sIdx].Mixed;
						CurrentSettings.MixedRgSeverity01 = CurrentSettings.Slots[sIdx].MixedRg01;
						CurrentSettings.MixedBySeverity01 = CurrentSettings.Slots[sIdx].MixedBy01;
						CurrentSettings.GammaGain = CurrentSettings.Slots[sIdx].GammaGain;

						s_baseType = CurrentSettings.Type;
						s_baseSev = CurrentSettings.Severity01;
						s_baseMixed = CurrentSettings.Mixed;
						s_baseMixedRg = CurrentSettings.MixedRgSeverity01;
						s_baseMixedBy = CurrentSettings.MixedBySeverity01;
						s_baseGamma = CurrentSettings.GammaGain;

						changed = true;
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);

					ImGui::SameLine(0, 4.0f);
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
					if (ImGui::Button("X##clr_slot", ImVec2(22.0f, 0.0f))) {
						CurrentSettings.Slots[sIdx].Used = false;
						CurrentSettings.Slots[sIdx].Name = "";
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);
					if (ImGui::IsItemHovered()) ImGui::SetTooltip(isDe ? "Slot leeren" : "Clear slot");
				} else {
					ImGui::TextDisabled("Slot %d: [%s]", sIdx + 1, isDe ? "Leer" : "Empty");
				}
				ImGui::PopID();
			}

			// Compact Live Feedback Badge
			ImGui::Spacing();
			{
				std::string profileName;
				std::string severityDesc;
				std::string clinicalGrade;
				bool isNeutral = false;

				if (CurrentSettings.Mixed) {
					profileName = isDe ? "Gemischt (Mixed)" : "Mixed Balance";
					char buf[96];
					std::snprintf(buf, sizeof(buf), "RG: %.0f%% | BY: %.0f%%", 
						CurrentSettings.MixedRgSeverity01 * 100.0, CurrentSettings.MixedBySeverity01 * 100.0);
					severityDesc = buf;
					double avgSev = (CurrentSettings.MixedRgSeverity01 + CurrentSettings.MixedBySeverity01) * 0.5;
					if (avgSev <= 0.005) {
						clinicalGrade = isDe ? "Neutral (Originalfarben)" : "Neutral (Original Colors)";
						isNeutral = true;
					} else if (avgSev <= 0.35) {
						clinicalGrade = isDe ? "Stufe 1 (Sanfte Balance)" : "Level 1 (Subtle Balance)";
					} else if (avgSev <= 0.70) {
						clinicalGrade = isDe ? "Stufe 2 (Ausgeglichen)" : "Level 2 (Balanced)";
					} else {
						clinicalGrade = isDe ? "Stufe 3 (Fokus-Kontrast)" : "Level 3 (Focus Contrast)";
					}
				} else {
					if (CurrentSettings.Type == BalanceType::Protan) {
						profileName = isDe ? "Protan (Rot-Fokus)" : "Protan (Red Focus)";
					} else if (CurrentSettings.Type == BalanceType::Deutan) {
						profileName = isDe ? "Deutan (Gruen-Fokus)" : "Deutan (Green Focus)";
					} else {
						profileName = isDe ? "Tritan (Blau-Fokus)" : "Tritan (Blue Focus)";
					}

					char buf[64];
					std::snprintf(buf, sizeof(buf), "%.1f%%", CurrentSettings.Severity01 * 100.0);
					severityDesc = buf;

					if (CurrentSettings.Severity01 <= 0.005) {
						clinicalGrade = isDe ? "Neutral (Originalfarben)" : "Neutral (Original Colors)";
						isNeutral = true;
					} else if (CurrentSettings.Severity01 <= 0.35) {
						clinicalGrade = isDe ? "Stufe 1 (Sanfte Balance)" : "Level 1 (Subtle Balance)";
					} else if (CurrentSettings.Severity01 <= 0.70) {
						clinicalGrade = isDe ? "Stufe 2 (Ausgeglichen)" : "Level 2 (Balanced)";
					} else {
						clinicalGrade = isDe ? "Stufe 3 (Fokus-Kontrast)" : "Level 3 (Focus Contrast)";
					}
				}

				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.18f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.38f, 0.58f, 0.65f));
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));

				if (ImGui::BeginChild("##status_feedback_card_main", ImVec2(0.0f, 58.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
					ImGui::TextColored(ImVec4(0.95f, 0.95f, 1.0f, 1.0f), "- %s - %s", profileName.c_str(), severityDesc.c_str());
					ImGui::Spacing();
					ImGui::TextColored(
						!isNeutral ? ImVec4(0.35f, 0.95f, 0.55f, 1.0f) : ImVec4(0.65f, 0.72f, 0.82f, 0.90f),
						"%s %s",
						t.ClassificationLabel,
						clinicalGrade.c_str()
					);
					ImGui::EndChild();
				}
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);

				// Eingabefeld fuer persoenliche Kalibrier- / Referenzwerte (AQ / HRR)
				ImGui::Spacing();
				const char* defaultHint = CurrentSettings.Mixed ? "RG: 50% | BY: 50%" :
					(CurrentSettings.Type == BalanceType::Protan ? "AQ: 0.35 | HRR: 8/10" :
					(CurrentSettings.Type == BalanceType::Deutan ? "AQ: 3.20 | HRR: 8/10" : "Moreland: 1.15 | HRR: 6/10"));

				ImGui::TextDisabled("%s:", isDe ? "Referenzwerte / Kalibrierung (AQ / HRR)" : "Reference Values / Calibration (AQ / HRR)");
				char diagBuf[128]{};
				std::snprintf(diagBuf, sizeof(diagBuf), "%s", CurrentSettings.DiagnosisHint.c_str());

				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::InputTextWithHint("##ref_values_input", defaultHint, diagBuf, sizeof(diagBuf)))
				{
					CurrentSettings.DiagnosisHint = diagBuf;
					changed = true;
					saveNeeded = true;
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe 
						? "Optionales Eingabefeld fuer persoenliche Kalibrier- oder Benchmarkwerte (z.B. Nagel-AQ, HRR-Plates).\nTypische Standardwerte fuer dieses Profil: %s"
						: "Optional input field for personal calibration or test benchmark scores (e.g. Nagel AQ, HRR plates).\nTypical default values for this profile: %s",
						defaultHint);
				}
			}
			endSection();
		}

		// ── Section 2: Commander-Tag Enhancer (Default Open) ─────────────────
		if (renderSectionHeader(1, t.HeaderSection2, ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool enhancerActive = (CurrentSettings.CommanderTagMode != 0);
			if (ImGui::Checkbox(isDe ? "Aktiv##enhancer_toggle" : "Active##enhancer_toggle", &enhancerActive)) {
				CurrentSettings.CommanderTagMode = enhancerActive ? 1 : 0;
				UpdateTagEnhancerConflicts();
				changed = true;
				saveNeeded = true;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.CmdrEnhancerDesc);

			int shiftedCount = 0;
			for (int i = 0; i < 9; ++i) {
				if (s_tagConflictStates[i].inConflict) shiftedCount++;
			}
			ImGui::SameLine(0, 14.0f);
			const char* curDefName = CurrentSettings.Type == BalanceType::Protan ? (isDe ? "Protan (Rot)" : "Protan (Red)") 
				: (CurrentSettings.Type == BalanceType::Deutan ? (isDe ? "Deutan (Gruen)" : "Deutan (Green)") : (isDe ? "Tritan (Blau)" : "Tritan (Blue)"));
			ImGui::TextColored(Theme::kTextGoldLabel, isDe ? "%s - %d von 9 Farben verschoben" : "%s - %d of 9 colors shifted", curDefName, shiftedCount);

			if (enhancerActive)
			{
				ImGui::Spacing();
				// 3 Preset Buttons from Mockup
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
				auto presetBtn = [&](const char* name, BalanceType dType) {
					bool act = (!CurrentSettings.Mixed && CurrentSettings.Type == dType);
					if (act) {
						ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
						ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
					} else {
						ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
						ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
					}
					if (ImGui::Button(name, ImVec2(100.0f, 24.0f))) {
						CurrentSettings.Type = dType;
						CurrentSettings.Mixed = false;
						CurrentSettings.CommanderTagMode = 1;
						CurrentSettings.SmartEnhancer = true;
						UpdateTagEnhancerConflicts();
						changed = true;
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);
				};
				presetBtn(isDe ? "Protan (Rot)" : "Protan (Red)", BalanceType::Protan);
				ImGui::SameLine(0, 6.0f);
				presetBtn(isDe ? "Deutan (Gruen)" : "Deutan (Green)", BalanceType::Deutan);
				ImGui::SameLine(0, 6.0f);
				presetBtn(isDe ? "Tritan (Blau)" : "Tritan (Blue)", BalanceType::Tritan);
				ImGui::PopStyleVar();

				ImGui::Spacing();
				if (ImGui::Checkbox(isDe ? "Smart-Auto##smart_toggle" : "Smart Auto##smart_toggle", &CurrentSettings.SmartEnhancer)) {
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.SmartEnhancerDesc);

				ImGui::Spacing();
				ImGui::TextUnformatted(isDe ? "Toleranz:" : "Tolerance:");
				float availTol = ImGui::GetContentRegionAvail().x;
				ImGui::SetNextItemWidth(availTol);
				if (ImGui::SliderFloat("##enhancer_tol_det",
				                       &CurrentSettings.EnhancerTolerance, 0.04f, 0.20f, "%.3f")) {
					UpdateTagEnhancerConflicts();
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::Spacing();
				ImGui::TextDisabled("%s", isDe ? "Tag-Farben: Betroffen (wird verschoben) vs. Sicher (unangetastet):"
				                               : "Tag Colors: Affected (shifted) vs. Safe (untouched):");
				ImGui::Spacing();

				// Clean horizontal row of circular tag swatches (Mockup Style)
				const float circleRadius = 11.0f;
				const float circleSpacing = 8.0f;
				ImDrawList* dlTags = ImGui::GetWindowDrawList();

				for (int i = 0; i < 9; ++i)
				{
					if (i > 0) ImGui::SameLine(0, circleSpacing);
					ImVec2 p = ImGui::GetCursorScreenPos();
					ImVec2 center(p.x + circleRadius, p.y + circleRadius);
					ImU32 col = IM_COL32((int)(kGw2TagRefs[i].r * 255), (int)(kGw2TagRefs[i].g * 255), (int)(kGw2TagRefs[i].b * 255), 255);

					bool conflict = s_tagConflictStates[i].inConflict;

					// Draw smooth circular swatch
					dlTags->AddCircleFilled(center, circleRadius, col);

					if (conflict) {
						// Glowing orange-red halo for shifted tag
						dlTags->AddCircle(center, circleRadius + 2.0f, IM_COL32(255, 80, 50, 240), 0, 2.0f);
						dlTags->AddCircleFilled(ImVec2(center.x + 8.0f, center.y - 7.0f), 3.5f, IM_COL32(255, 60, 50, 255));
					} else {
						// Subtle clean ring for untouched safe tag
						dlTags->AddCircle(center, circleRadius, IM_COL32(220, 230, 245, 140), 0, 1.2f);
						dlTags->AddCircleFilled(ImVec2(center.x + 8.0f, center.y - 7.0f), 3.0f, IM_COL32(70, 220, 110, 220));
					}

					ImGui::Dummy(ImVec2(circleRadius * 2.0f + 2.0f, circleRadius * 2.0f + 2.0f));
					if (ImGui::IsItemHovered())
					{
						const char* tagLabel = kGw2TagRefs[i].labelFunc(t);
						if (conflict)
							ImGui::SetTooltip(isDe ? "%s: Konflikt erkannt -> Auto-Verschiebung aktiv" 
							                       : "%s: Conflict detected -> Auto-shift active", tagLabel);
						else
							ImGui::SetTooltip(isDe ? "%s: Kein Konflikt -> Farbe bleibt unberuehrt" 
							                       : "%s: No conflict -> Color remains untouched", tagLabel);
					}
				}

				// ── Kurvenansicht (geladenes Preset / aktives Profil) im Hauptfenster ──
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::TextDisabled("%s", isDe ? "Kurvenansicht (geladenes Preset / Farbprofil):" 
				                               : "Curve View (Loaded Preset / Color Profile):");
				ImGui::Spacing();

				// 3-Way Mode selector for MAIN window (independent of HUD window!)
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));

				auto mainGraphModeBtn = [&](const char* aName, int aModeVal, const char* aTip) {
					bool active = (CurrentSettings.MainGraphMode == aModeVal);
					if (active) {
						ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
						ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
					} else {
						ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
						ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
					}
					if (ImGui::Button(aName, ImVec2(92.0f, 22.0f))) {
						CurrentSettings.MainGraphMode = aModeVal;
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", aTip);
				};

				ImGui::TextDisabled("%s:", isDe ? "Ansicht" : "View");
				ImGui::SameLine(0, 8.0f);
				mainGraphModeBtn(isDe ? "Polygonal##main" : "Polygonal##main", 0, isDe ? "1. Spektrale Transferfunktion (Polygonal / PWL)\nStueckweise lineare Farbvektor-Projektion ueber die Hue-Winkel."
				                                            : "1. Spectral Transfer Function (Piecewise-Linear / PWL)\nPiecewise linear color vector projection across hue angles.");
				ImGui::SameLine();
				mainGraphModeBtn(isDe ? "Harmonisch##main" : "Harmonic##main", 1, isDe ? "2. Harmonische Resonanz (Gauss / Sinusoidale LMS-Kurven)\nFliessende, stetige Wellenkurven nach dem LMS-Zapfenmodell des menschlichen Auges."
				                                             : "2. Harmonic Spectral Response (Gaussian / Smooth Spline)\nFlowing, continuous wave curves based on the human LMS cone model.");
				ImGui::SameLine();
				mainGraphModeBtn(isDe ? "Strahlen##main" : "Rays##main", 2, isDe ? "3. Diskrete Strahlen-Zerlegung (Lineare Strahlen / Ray Scope)\nPhysikalische Strahlenzerlegung der Farbkanaele wie bei einem Gitterspektrometer."
				                                           : "3. Linear Spectral Rays (Ray Scope / Dispersion Bars)\nPhysical ray-optics decomposition of channels like a diffraction spectrometer.");

				ImGui::PopStyleVar(2);
				ImGui::Spacing();

				double mainCorrMat[3][3];
				if (CurrentSettings.Mixed)
					ColorMatrix::MixedCorrectionMatrix(CurrentSettings.MixedRgSeverity01, CurrentSettings.MixedBySeverity01, mainCorrMat);
				else
					ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, mainCorrMat);

				float availW = ImGui::GetContentRegionAvail().x;
				float graphW = (availW > 260.0f) ? availW : 260.0f;
				float graphH = 112.0f;

				ImVec2 cpMain = ImGui::GetCursorScreenPos();
				DrawSpectralGraphPanel(ImGui::GetWindowDrawList(), cpMain, graphW, graphH, mainCorrMat, /*isDetached=*/false, CurrentSettings.UiOpacity, CurrentSettings.MainGraphMode);
				ImGui::InvisibleButton("##curve_panel_main", ImVec2(graphW, graphH));

				// Color beam under curves
				const float pad = 8.0f;
				const float labelSpaceLeft = 28.0f;
				const float badgeSpaceRight = 6.0f;
				float plotX = cpMain.x + pad + labelSpaceLeft;
				float plotW = graphW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
				float beamH = 10.0f;

				ImVec2 beamPos = ImGui::GetCursorScreenPos();
				beamPos.x = plotX;
				ImDrawList* dlMain = ImGui::GetWindowDrawList();

				constexpr int kBeamSteps = 48;
				for (int b = 0; b < kBeamSteps; ++b) {
					float u0 = (float)b / kBeamSteps;
					float u1 = (float)(b + 1) / kBeamSteps;
					float uMid = (u0 + u1) * 0.5f;
					float h = uMid * 6.0f;
					float x = 1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f);
					float r0 = 0.0f, g0 = 0.0f, b0 = 0.0f;
					if (h < 1.0f)      { r0 = 1.0f; g0 = x;    b0 = 0.0f; }
					else if (h < 2.0f) { r0 = x;    g0 = 1.0f; b0 = 0.0f; }
					else if (h < 3.0f) { r0 = 0.0f; g0 = 1.0f; b0 = x;    }
					else if (h < 4.0f) { r0 = 0.0f; g0 = x;    b0 = 1.0f; }
					else if (h < 5.0f) { r0 = x;    g0 = 0.0f; b0 = 1.0f; }
					else               { r0 = 1.0f; g0 = 0.0f; b0 = x;    }

					double cr = std::clamp(mainCorrMat[0][0]*r0 + mainCorrMat[0][1]*g0 + mainCorrMat[0][2]*b0, 0.0, 1.0);
					double cg = std::clamp(mainCorrMat[1][0]*r0 + mainCorrMat[1][1]*g0 + mainCorrMat[1][2]*b0, 0.0, 1.0);
					double cb = std::clamp(mainCorrMat[2][0]*r0 + mainCorrMat[2][1]*g0 + mainCorrMat[2][2]*b0, 0.0, 1.0);

					int beamAlpha = (int)(std::clamp(CurrentSettings.UiOpacity * 210.0f, 40.0f, 255.0f));
					ImU32 col = IM_COL32((int)(cr*255), (int)(cg*255), (int)(cb*255), beamAlpha);
					dlMain->AddRectFilled(ImVec2(plotX + u0 * plotW, beamPos.y), ImVec2(plotX + u1 * plotW, beamPos.y + beamH), col, (b == 0 || b == kBeamSteps - 1) ? 2.0f : 0.0f);
				}
				int borderAlpha = (int)(CurrentSettings.UiOpacity * 130.0f);
				dlMain->AddRect(ImVec2(plotX, beamPos.y), ImVec2(plotX + plotW, beamPos.y + beamH), IM_COL32(80, 100, 140, borderAlpha), 2.0f);
				ImGui::Dummy(ImVec2(graphW, beamH));

				// Gespeicherte Profile (Mockup Chips)
				ImGui::Spacing();
				ImGui::TextDisabled("%s:", isDe ? "Gespeicherte Profile (Schnellauswahl)" : "Saved Profiles (Quick Select)");
				ImGui::Spacing();

				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
				if (ImGui::Button(isDe ? "Tag-Kontrast Protan" : "Tag Contrast Protan", ImVec2(140.0f, 22.0f)))
				{
					CurrentSettings.Type = BalanceType::Protan;
					CurrentSettings.Mixed = false;
					CurrentSettings.Severity01 = 1.0f;
					CurrentSettings.CommanderTagMode = 1;
					CurrentSettings.SmartEnhancer = true;
					UpdateTagEnhancerConflicts();
					changed = true; saveNeeded = true;
				}
				ImGui::SameLine(0, 6.0f);
				if (ImGui::Button(isDe ? "Mein WvW Setup" : "My WvW Setup", ImVec2(120.0f, 22.0f)))
				{
					CurrentSettings.Type = BalanceType::Protan;
					CurrentSettings.Mixed = false;
					CurrentSettings.Severity01 = 1.25f; // +25% Boost
					CurrentSettings.CommanderTagMode = 1;
					CurrentSettings.SmartEnhancer = true;
					CurrentSettings.EnhancerTolerance = 0.14f;
					UpdateTagEnhancerConflicts();
					changed = true; saveNeeded = true;
				}
				ImGui::PopStyleColor(4);

				// Presets
				ImGui::Spacing();
				ImGui::TextDisabled("%s:", t.PresetsTitle);
				ImGui::Spacing();

				for (int pIdx = 0; pIdx < 3; ++pIdx)
				{
					ImGui::PushID(pIdx + 100);
					char nameBuf[64];
					std::snprintf(nameBuf, sizeof(nameBuf), "%s", CurrentSettings.Presets[pIdx].Name.c_str());
					ImGui::SetNextItemWidth(90.0f);
					if (ImGui::InputText("##preset_name_det", nameBuf, sizeof(nameBuf)))
					{
						CurrentSettings.Presets[pIdx].Name = nameBuf;
						saveNeeded = true;
					}

					ImGui::SameLine(0, 4.0f);
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
					if (ImGui::Button(isDe ? "Laden##det" : "Load##det", ImVec2(52.0f, 0.0f)))
					{
						CurrentSettings.Type = static_cast<BalanceType>(CurrentSettings.Presets[pIdx].Type);
						CurrentSettings.Severity01 = CurrentSettings.Presets[pIdx].Severity;
						CurrentSettings.EnhancerTolerance = CurrentSettings.Presets[pIdx].Tolerance;
						CurrentSettings.CommanderTagMode = 1;
						UpdateTagEnhancerConflicts();
						changed = true;
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip(isDe ? "Preset '%s' laden" : "Load preset '%s'", CurrentSettings.Presets[pIdx].Name.c_str());
					}

					ImGui::SameLine(0, 4.0f);
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
					if (ImGui::Button(isDe ? "Speichern##det" : "Save##det", ImVec2(72.0f, 0.0f)))
					{
						CurrentSettings.Presets[pIdx].Type = static_cast<int>(CurrentSettings.Type);
						CurrentSettings.Presets[pIdx].Severity = CurrentSettings.Severity01;
						CurrentSettings.Presets[pIdx].Tolerance = CurrentSettings.EnhancerTolerance;
						CurrentSettings.Save(AddonDir);
					}
					ImGui::PopStyleColor(4);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip(isDe ? "Aktuelle Einstellungen in Slot %d speichern" : "Save current settings to slot %d", pIdx + 1);
					}

					ImGui::PopID();
				}

				// Reset on Neutral
				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
				if (ImGui::Button(isDe ? "Reset auf Neutral##det" : "Reset to Neutral##det", ImVec2(180.0f, 24.0f)))
				{
					CurrentSettings.CommanderTagMode = 0;
					CurrentSettings.EnhancerTolerance = 0.12f;
					CurrentSettings.SmartEnhancer = true;
					UpdateTagEnhancerConflicts();
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe ? "Setzt Commander Tag Enhancer auf Inaktiv / Neutral zurueck" : "Resets Commander Tag Enhancer to Off / Neutral");
				}
			}
			endSection();
		}

		// ── Section 3: Kontrast-Kombinationen (Überlappende Farbfelder) ──────
		if (renderSectionHeader(2, t.HeaderSection3))
		{
			DrawContrastCombinationsWidget(isDe, changed, saveNeeded);
			endSection();
		}

		// ── Section 4: Eye Comfort (Helligkeit) ──────────────────────────────
		if (renderSectionHeader(3, t.HeaderSection4))
		{
			static int s_lastHdrCheckFrameDet = -1;
			static bool s_cachedHdrDetectedDet = false;
			int curFrame = ImGui::GetFrameCount();
			if (curFrame != s_lastHdrCheckFrameDet + 1)
			{
				IDXGISwapChain* sc = APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr;
				s_cachedHdrDetectedDet = DetectHdrColorSpace(sc);
			}
			s_lastHdrCheckFrameDet = curFrame;

			ImVec2 dotPos = ImGui::GetCursorScreenPos();
			float dotRadius = 4.0f;
			ImU32 dotColor = s_cachedHdrDetectedDet ? Theme::kDotReadyCol : Theme::kDotOffCol;
			ImVec2 dotCenter(dotPos.x + dotRadius + 2.0f, dotPos.y + ImGui::GetTextLineHeight() * 0.5f);
			ImGui::GetWindowDrawList()->AddCircleFilled(dotCenter, dotRadius, dotColor);
			ImGui::Dummy(ImVec2(dotRadius * 2.0f + 4.0f, ImGui::GetTextLineHeight()));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("HDR: %s\n(%s)", s_cachedHdrDetectedDet ? (isDe ? "Aktiv" : "Active") : (isDe ? "Inaktiv (SDR)" : "Inactive (SDR)"), t.EyeComfortHdrTooltip);
			}
			ImGui::SameLine(0, 6.0f);
			ImGui::TextDisabled("HDR: %s", s_cachedHdrDetectedDet ? (isDe ? "Erkannt" : "Detected") : (isDe ? "Aus (SDR)" : "Off (SDR)"));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("HDR: %s\n(%s)", s_cachedHdrDetectedDet ? (isDe ? "Aktiv" : "Active") : (isDe ? "Inaktiv (SDR)" : "Inactive (SDR)"), t.EyeComfortHdrTooltip);
			}

			ImGui::Spacing();
			ImGui::TextUnformatted(isDe ? "Helligkeit (Eye Comfort Gamma):" : "Brightness (Eye Comfort Gamma):");
			float availGamma = ImGui::GetContentRegionAvail().x;
			float btnWGamma = 56.0f;
			float spGamma = 6.0f;
			float sWGamma = (availGamma > (btnWGamma + spGamma + 60.0f)) ? (availGamma - btnWGamma - spGamma) : 180.0f;

			ImGui::SetNextItemWidth(sWGamma);
			if (ImGui::SliderFloat("##EyeComfortGammaSlider", &CurrentSettings.GammaGain, 0.70f, 1.30f, "%.2fx"))
			{
				CurrentSettings.AutoBrightness = false; // User manually set
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				saveNeeded = true;
			}
			ImGui::SameLine(0, spGamma);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			if (ImGui::Button("Reset##gamma_main", ImVec2(btnWGamma, 0.0f)))
			{
				CurrentSettings.GammaGain = 1.0f;
				CurrentSettings.AutoBrightness = false;
				changed = true;
				saveNeeded = true;
			}
			ImGui::PopStyleColor(4);

			BrightnessRetentionResult retention = GetBrightnessRetention();
			if (CurrentSettings.AutoBrightness)
			{
				if (std::abs(CurrentSettings.GammaGain - retention.recommendedGain) > 0.005f)
				{
					CurrentSettings.GammaGain = retention.recommendedGain;
					changed = true;
					saveNeeded = true;
				}
			}

			ImGui::Spacing();
			ImGui::Text(t.EyeComfortRetention, retention.retentionRatio * 100.0f, retention.recommendedGain);
			ImGui::SameLine(0, 8.0f);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
			if (ImGui::Button(t.EyeComfortApply, ImVec2(isDe ? 175.0f : 165.0f, 24.0f)))
			{
				CurrentSettings.GammaGain = retention.recommendedGain;
				changed = true;
				saveNeeded = true;
			}
			ImGui::PopStyleColor(4);
			ImGui::SameLine(0, 8.0f);
			if (ImGui::Checkbox(isDe ? "Auto-Helligkeit##main_auto" : "Auto-Brightness##main_auto", &CurrentSettings.AutoBrightness))
			{
				if (CurrentSettings.AutoBrightness)
				{
					CurrentSettings.GammaGain = retention.recommendedGain;
					changed = true;
				}
				saveNeeded = true;
			}
			endSection();
		}

		// ── Section 5: Spiel- & Fenstermodus ──────────────────────────────────
		if (renderSectionHeader(4, t.HeaderSection5))
		{
			WindowMode mode = DetectWindowMode(
				APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);
			if (mode == WindowMode::ExclusiveFullscreen) {
				ImGui::TextColored({1.0f,0.55f,0.2f,1.0f}, "%s: %s", t.WindowMode, ToDisplayString(mode));
				ImGui::TextWrapped(
					"Switch GW2 to Windowed or Windowed Fullscreen (Borderless) "
					"in Graphics Options to enable the filter.");
			} else {
				ImGui::TextColored({0.4f,0.85f,0.4f,1.0f}, "%s: %s", t.WindowMode, ToDisplayString(mode));
			}
			ImGui::Spacing();
			if (ImGui::Checkbox(t.KeepActiveBackground, &CurrentSettings.SystemWide)) {
				changed = true;
				saveNeeded = true;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", t.KeepActiveBackgroundTooltip);
			}

			if (!CurrentSettings.SystemWide) {
				ImGui::TextDisabled("%s", t.FocusWatchdogExclusive);
			} else {
				ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.25f, 1.0f), "%s", t.FocusWatchdogBackground);
			}
			endSection();
		}

		// ── Section 6: Hybrid Modus (Beta) ────────────────────────────────────
		if (renderSectionHeader(5, t.HeaderSection6))
		{
			if (ImGui::Checkbox(t.HybridMode, &CurrentSettings.EnableHybridMode)) {
				GetHybridScanner().SetEnabled(CurrentSettings.EnableHybridMode);
				changed = true;
				saveNeeded = true;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", t.HybridModeHelp);
			}
			ImGui::TextDisabled("%s", isDe ? "Kinematic Fader: Automatische Weichzeichnung bei schnellen Kameraschwenks."
			                               : "Kinematic Fader: Automatic smoothing during rapid camera pans.");
			endSection();
		}

		// ── Section 7: Filter-Labor & Experimentierfeld ──────────────────────
		if (renderSectionHeader(6, t.HeaderSection7))
		{
			DrawFilterLabWidget(isDe, changed, saveNeeded);
			endSection();
		}

		// ── Section 8: Über, Diagnose & Credits (Everything clean in one collapsible section) ──
		if (renderSectionHeader(7, t.HeaderSection8))
		{
			if (ImGui::Checkbox(t.DebugModeCheckbox, &CurrentSettings.DebugMode)) {
				changed = true;
				saveNeeded = true;
			}
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.52f, 0.58f, 0.80f));
			ImGui::TextUnformatted(t.MethodologyTitle);
			ImGui::TextWrapped("%s", t.MethodologyDesc);
			ImGui::PopStyleColor();
			ImGui::Spacing();

			ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.24f, 0.20f, 0.35f, 0.75f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.28f, 0.50f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.18f, 0.14f, 0.26f, 1.00f));
			if (ImGui::Button(t.CreditsBtn, ImVec2(120.0f, 24.0f)))
			{
				s_showC64Credits.store(true);
				StartC64Audio();
			}
			ImGui::PopStyleColor(3);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.CreditsTooltip);
			ImGui::Spacing();
		}

		ImGui::EndChild(); // ##MainWindowScrollContent

		if (saveNeeded) {
			CurrentSettings.Save(AddonDir);
			Recompute(/*aForce=*/true);
		} else if (changed) {
			Recompute(/*aForce=*/false);
		}

		ImGui::PopID();
	}

	// ── Dedicated Sensor & Spectral Graph HUD Window ─────────────────────────
	void RenderGraphWindow()
	{
		if (!ImGui::GetCurrentContext()) return;
		const L10n& t = Strings();
		bool changed = false;
		bool saveNeeded = false;
		bool isDe = (t.Enabled[0] == 'A');

		ImGui::PushID("CBA_GraphHUD");

		ImGuiStyle& style = ImGui::GetStyle();
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);

		// ── Header Bar: Live Status Dot, Live Profile & Brightness Info, Reset Button, Mischpult Opacity Button ──
		DrawFilterStatusIndicator(false);
		ImGui::SameLine(0, 6.0f);

		// Status text (e.g. "Mixed (0%/0%) | 0.92x" or "Protan (80%) | 0.95x")
		{
			std::string profStr;
			if (CurrentSettings.Mixed) {
				char b[64];
				std::snprintf(b, sizeof(b), "%s (%d%%/%d%%)", isDe ? "Gemischt" : "Mixed",
					(int)(CurrentSettings.MixedRgSeverity01 * 100.0), (int)(CurrentSettings.MixedBySeverity01 * 100.0));
				profStr = b;
			} else {
				const char* name = (CurrentSettings.Type == BalanceType::Protan) ? "Protan" :
				                   (CurrentSettings.Type == BalanceType::Deutan) ? "Deutan" : "Tritan";
				char b[64];
				std::snprintf(b, sizeof(b), "%s (%d%%)", name, (int)(CurrentSettings.Severity01 * 100.0));
				profStr = b;
			}
			ImGui::TextColored(ImVec4(0.70f, 0.78f, 0.90f, 0.95f), "%s: %s | %s: %.2fx", 
				isDe ? "Profil" : "Profile", profStr.c_str(),
				isDe ? "Helligkeit" : "Brightness", CurrentSettings.GammaGain);
		}

		ImGui::SameLine(0, 8.0f);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextPrimary);
		if (ImGui::Button("Reset##graph", ImVec2(56.0f, 22.0f)))
		{
			CurrentSettings.Enabled = false;
			CurrentSettings.Severity01 = 0.0;
			CurrentSettings.Mixed = false;
			CurrentSettings.MixedRgSeverity01 = 0.0;
			CurrentSettings.MixedBySeverity01 = 0.0;
			CurrentSettings.GammaGain = 1.0f;
			CurrentSettings.AutoBrightness = false;
			CurrentSettings.CommanderTagMode = 0;
			CurrentSettings.EnableHybridMode = false;
			GetHybridScanner().SetEnabled(false);
			CurrentSettings.FreeFilterEnabled = false;
			CurrentSettings.LabModeEnabled = false;
			CurrentSettings.Save(AddonDir);
			{
				std::lock_guard<std::mutex> lock(s_recomputeMutex);
				GetColorEffectController().Clear();
				s_hasApplied = false;
			}
			Recompute(/*aForce=*/true);
			changed = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt alle aktiven Kurven, Filter und Effekte (Commander-Tags, Hybrid, Helligkeit) komplett auf neutral zurueck (Filter AUS)."
			                       : "Resets all active curves, filters, and effect functions (Commander Tags, Hybrid, Brightness) to neutral (Filter OFF).");
		}

		ImGui::SameLine(0, 6.0f);
		static bool s_showGraphOpacityDrawer = false;
		char opacLabel[32];
		std::snprintf(opacLabel, sizeof(opacLabel), "%.2f", CurrentSettings.UiOpacity);
		if (s_showGraphOpacityDrawer) {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
		}
		if (ImGui::Button(opacLabel, ImVec2(48.0f, 0.0f)))
		{
			s_showGraphOpacityDrawer = !s_showGraphOpacityDrawer;
		}
		ImGui::PopStyleColor(3);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Deckkraft-Fader (Mischpult-Regler oeffnen)" : "Window Opacity (Toggle mixer fader)");
		}

		// Mischpult-Regler drawer
		if (s_showGraphOpacityDrawer)
		{
			ImGui::Spacing();
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 3.0f);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.16f, 0.22f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.35f, 0.75f, 0.95f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.45f, 0.85f, 1.00f, 1.00f));

			float fullW = ImGui::GetContentRegionAvail().x;
			ImGui::SetNextItemWidth(fullW);
			if (ImGui::SliderFloat("##HUDOpacityFader", &CurrentSettings.UiOpacity, 0.00f, 1.00f, isDe ? "Fader / Deckkraft: %.2f" : "Fader / Opacity: %.2f"))
			{
				CurrentSettings.UiOpacity = std::clamp(CurrentSettings.UiOpacity, 0.00f, 1.00f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);
		}

		ImGui::Spacing();

		// ── 3-Way Graph Visualization Mode Selector ─────────────────────────
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));

		auto graphModeBtn = [&](const char* aName, int aModeVal, const char* aTip) {
			bool active = (CurrentSettings.GraphMode == aModeVal);
			if (active) {
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
			}
			if (ImGui::Button(aName, ImVec2(92.0f, 22.0f))) {
				CurrentSettings.GraphMode = aModeVal;
				saveNeeded = true;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", aTip);
		};

		ImGui::TextDisabled("%s:", isDe ? "Ansicht" : "View");
		ImGui::SameLine(0, 8.0f);
		graphModeBtn("Polygonal", 0, isDe ? "1. Spektrale Transferfunktion (Polygonal / PWL)\nStueckweise lineare Farbvektor-Projektion ueber die Hue-Winkel."
		                                  : "1. Spectral Transfer Function (Piecewise-Linear / PWL)\nPiecewise linear color vector projection across hue angles.");
		ImGui::SameLine();
		graphModeBtn(isDe ? "Harmonisch" : "Harmonic", 1, isDe ? "2. Harmonische Resonanz (Gauss / Sinusoidale LMS-Kurven)\nFliessende, stetige Wellenkurven nach dem LMS-Zapfenmodell des menschlichen Auges."
		                                   : "2. Harmonic Spectral Response (Gaussian / Smooth Spline)\nFlowing, continuous wave curves based on the human LMS cone model.");
		ImGui::SameLine();
		graphModeBtn(isDe ? "Strahlen" : "Rays", 2, isDe ? "3. Diskrete Strahlen-Zerlegung (Lineare Strahlen / Ray Scope)\nPhysikalische Strahlenzerlegung der Farbkanaele wie bei einem Gitterspektrometer."
		                                 : "3. Linear Spectral Rays (Ray Scope / Dispersion Bars)\nPhysical ray-optics decomposition of channels like a diffraction spectrometer.");

		ImGui::PopStyleVar(2);
		ImGui::Spacing();

		// Correction curves
		double corrMat[3][3];
		if (CurrentSettings.Mixed)
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01, corrMat);
		else
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, corrMat);

		float availW = ImGui::GetContentRegionAvail().x;
		float graphW = (availW > 260.0f) ? availW : 260.0f;
		float graphH = 120.0f;

		ImVec2 cp = ImGui::GetCursorScreenPos();
		DrawSpectralGraphPanel(ImGui::GetWindowDrawList(), cp, graphW, graphH, corrMat, /*isDetached=*/true, CurrentSettings.UiOpacity, CurrentSettings.GraphMode);
		ImGui::InvisibleButton("##curve_panel_hud", ImVec2(graphW, graphH));

		// Live filtered color beam preview (Strahl-Anzeiger)
		const float pad = 8.0f;
		const float labelSpaceLeft = 28.0f;
		const float badgeSpaceRight = 6.0f;
		float plotX = cp.x + pad + labelSpaceLeft;
		float plotW = graphW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		float beamH = 12.0f;

		ImVec2 beamPos = ImGui::GetCursorScreenPos();
		beamPos.x = plotX;
		ImDrawList* dl = ImGui::GetWindowDrawList();

		constexpr int kBeamSteps = 48;
		for (int b = 0; b < kBeamSteps; ++b) {
			float u0 = (float)b / kBeamSteps;
			float u1 = (float)(b + 1) / kBeamSteps;
			float uMid = (u0 + u1) * 0.5f;
			float h = uMid * 6.0f;
			float x = 1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f);
			float r0 = 0.0f, g0 = 0.0f, b0 = 0.0f;
			if (h < 1.0f)      { r0 = 1.0f; g0 = x;    b0 = 0.0f; }
			else if (h < 2.0f) { r0 = x;    g0 = 1.0f; b0 = 0.0f; }
			else if (h < 3.0f) { r0 = 0.0f; g0 = 1.0f; b0 = x;    }
			else if (h < 4.0f) { r0 = 0.0f; g0 = x;    b0 = 1.0f; }
			else if (h < 5.0f) { r0 = x;    g0 = 0.0f; b0 = 1.0f; }
			else               { r0 = 1.0f; g0 = 0.0f; b0 = x;    }

			double cr = std::clamp(corrMat[0][0]*r0 + corrMat[0][1]*g0 + corrMat[0][2]*b0, 0.0, 1.0);
			double cg = std::clamp(corrMat[1][0]*r0 + corrMat[1][1]*g0 + corrMat[1][2]*b0, 0.0, 1.0);
			double cb = std::clamp(corrMat[2][0]*r0 + corrMat[2][1]*g0 + corrMat[2][2]*b0, 0.0, 1.0);

			int beamAlpha = (int)(std::clamp(CurrentSettings.UiOpacity * 210.0f + 45.0f, 45.0f, 255.0f));
			ImU32 col = IM_COL32((int)(cr*255), (int)(cg*255), (int)(cb*255), beamAlpha);
			dl->AddRectFilled(ImVec2(plotX + u0 * plotW, beamPos.y), ImVec2(plotX + u1 * plotW, beamPos.y + beamH), col, (b == 0 || b == kBeamSteps - 1) ? 2.0f : 0.0f);
		}
		int borderAlpha = (int)(std::clamp(CurrentSettings.UiOpacity * 140.0f, 20.0f, 140.0f));
		dl->AddRect(ImVec2(plotX, beamPos.y), ImVec2(plotX + plotW, beamPos.y + beamH), IM_COL32(80, 100, 140, borderAlpha), 2.0f);
		ImGui::Dummy(ImVec2(graphW, beamH));
		ImGui::TextDisabled("%s", isDe ? "Echtzeit-Spektrum (gefiltert)" : "Real-time spectrum (filtered)");

		// ── Lower Controls (Cloned from Section 1, up until Save Profile) ───────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Radio buttons for balance types
		auto typeBtnHUD = [&](const char* aLabel, bool aActive, BalanceType aType) {
			if (ImGui::RadioButton(aLabel, aActive)) {
				if (CurrentSettings.Mixed || CurrentSettings.Type != aType) {
					CurrentSettings.Mixed = false;
					CurrentSettings.Type  = aType;
					CurrentSettings.Severity01 = 0.0;
					changed = true;
					saveNeeded = true;
				}
			}
		};
		typeBtnHUD(t.Protan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Protan, BalanceType::Protan);
		ImGui::SameLine();
		typeBtnHUD(t.Deutan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Deutan, BalanceType::Deutan);
		ImGui::SameLine();
		typeBtnHUD(t.Tritan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Tritan, BalanceType::Tritan);
		ImGui::SameLine();
		if (ImGui::RadioButton(t.Mixed, CurrentSettings.Mixed)) {
			if (!CurrentSettings.Mixed) {
				CurrentSettings.Mixed = true;
				CurrentSettings.MixedRgSeverity01 = 0.0;
				CurrentSettings.MixedBySeverity01 = 0.0;
				changed = true;
				saveNeeded = true;
			}
		}

		ImGui::Spacing();

		if (CurrentSettings.Mixed) {
			float rg = (float)CurrentSettings.MixedRgSeverity01;
			float by = (float)CurrentSettings.MixedBySeverity01;

			ImGui::TextUnformatted(t.RgStrength);
			float avail = ImGui::GetContentRegionAvail().x;
			float btnW = 56.0f;
			float sp = 6.0f;
			float sW = (avail > (btnW + sp + 60.0f)) ? (avail - btnW - sp) : 180.0f;

			ImGui::SetNextItemWidth(sW);
			if (ImGui::SliderFloat("##rg_hud", &rg, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.MixedRgSeverity01 = std::clamp(rg, 0.0f, 1.25f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine(0, sp);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			if (ImGui::Button("Reset##rg_hud", ImVec2(btnW, 0.0f))) { 
				CurrentSettings.MixedRgSeverity01 = 0.0f; 
				changed = true; 
				saveNeeded = true; 
			}
			ImGui::PopStyleColor(4);
			
			ImGui::TextUnformatted(t.ByStrength);
			ImGui::SetNextItemWidth(sW);
			if (ImGui::SliderFloat("##by_hud", &by, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.MixedBySeverity01 = std::clamp(by, 0.0f, 1.25f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine(0, sp);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			if (ImGui::Button("Reset##by_hud", ImVec2(btnW, 0.0f))) { 
				CurrentSettings.MixedBySeverity01 = 0.0f; 
				changed = true; 
				saveNeeded = true; 
			}
			ImGui::PopStyleColor(4);
		} else {
			float sev = (float)CurrentSettings.Severity01;

			ImGui::TextUnformatted(t.Strength);
			float avail = ImGui::GetContentRegionAvail().x;
			float btnW = 56.0f;
			float sp = 6.0f;
			float sW = (avail > (btnW + sp + 60.0f)) ? (avail - btnW - sp) : 180.0f;

			ImGui::SetNextItemWidth(sW);
			if (ImGui::SliderFloat("##sev_hud", &sev, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.Severity01 = std::clamp(sev, 0.0f, 1.25f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine(0, sp);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			if (ImGui::Button("Reset##sev_hud", ImVec2(btnW, 0.0f))) { 
				CurrentSettings.Severity01 = 0.0f; 
				changed = true; 
				saveNeeded = true; 
			}
			ImGui::PopStyleColor(4);
		}

		// ── Section: Eye Comfort / Helligkeits-Logik ────────────────────────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		BrightnessRetentionResult retention = GetBrightnessRetention();

		// Auto-Brightness real-time sync:
		if (CurrentSettings.AutoBrightness)
		{
			if (std::abs(CurrentSettings.GammaGain - retention.recommendedGain) > 0.005f)
			{
				CurrentSettings.GammaGain = retention.recommendedGain;
				changed = true;
				saveNeeded = true;
			}
		}

		// Highlighted calculation card (TAC-inspired)
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.08f, 0.12f, 0.95f));
		ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.12f, 0.28f, 0.40f, 0.75f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 7.0f));

		if (ImGui::BeginChild("##eye_comfort_hud_card", ImVec2(0.0f, 54.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Helligkeits-Kompensation (Eye Comfort)" : "Brightness Compensation (Eye Comfort)");
			ImGui::Spacing();

			ImGui::TextUnformatted(isDe ? "Luminanz-Retention:" : "Luminance Retention:");
			ImGui::SameLine(0, 6.0f);
			ImGui::TextColored(Theme::kTextCyanLicht, "%.1f%%", retention.retentionRatio * 100.0f);

			ImGui::SameLine(0, 14.0f);
			ImGui::TextUnformatted(isDe ? "Empfehlung:" : "Target:");
			ImGui::SameLine(0, 6.0f);
			ImGui::TextColored(Theme::kTextGoldLabel, "%.2fx", retention.recommendedGain);

			ImGui::EndChild();
		}
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);

		ImGui::Spacing();

		float impactPct = (retention.retentionRatio - 1.0f) * 100.0f;

		// Action buttons & Continuous Auto toggle
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);

		char applyBtnLabel[64];
		std::snprintf(applyBtnLabel, sizeof(applyBtnLabel), isDe ? "Optimalwert (%.2fx)##hud_apply" : "Apply Target (%.2fx)##hud_apply", retention.recommendedGain);
		float btnWHUDApply = isDe ? 155.0f : 145.0f;

		if (ImGui::Button(applyBtnLabel, ImVec2(btnWHUDApply, 24.0f)))
		{
			CurrentSettings.GammaGain = retention.recommendedGain;
			changed = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt den GammaGain einmalig auf den berechneten Optimalwert (%.2fx).\nProfil-Impact auf Helligkeit: %+.1f%% (Retention: %.1f%%)"
			                       : "Applies the calculated optimal gain (%.2fx) once.\nProfile impact on brightness: %+.1f%% (Retention: %.1f%%)",
			                       retention.recommendedGain, impactPct, retention.retentionRatio * 100.0f);
		}

		// Errechneter Gegenwert / Impact in dezentem Grau rechts neben dem Button
		ImGui::SameLine(0, 8.0f);
		ImGui::TextDisabled(isDe ? "Impact: %+.1f%%" : "Impact: %+.1f%%", impactPct);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Errechnete Luminanz-Verschiebung durch das aktive Farbprofil (%+.1f%%).\nDer Optimalwert gleicht diesen Helligkeitsverlust praezise aus."
			                       : "Calculated luminance shift caused by active color profile (%+.1f%%).\nThe target value accurately compensates this difference.", impactPct);
		}

		ImGui::SameLine(0, 10.0f);
		if (ImGui::Checkbox(isDe ? "Auto-Sync##hud_auto" : "Auto-Sync##hud_auto", &CurrentSettings.AutoBrightness))
		{
			if (CurrentSettings.AutoBrightness)
			{
				CurrentSettings.GammaGain = retention.recommendedGain;
				changed = true;
			}
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Wenn aktiv: Passt die Helligkeit bei jeder Profil-Aenderung dynamisch und vollautomatisch an."
			                       : "When active: Dynamically synchronizes brightness compensation in real time.");
		}

		// Sleek manual slider for GammaGain
		ImGui::Spacing();
		float availHUD = ImGui::GetContentRegionAvail().x;
		float btnWHUD = 56.0f;
		float spHUD = 6.0f;
		float sliderWHUD = (availHUD > (btnWHUD + spHUD + 60.0f)) ? (availHUD - btnWHUD - spHUD) : 180.0f;

		ImGui::TextDisabled("%s (%.2fx):", isDe ? "Manuelle Helligkeit" : "Manual Brightness", CurrentSettings.GammaGain);
		ImGui::SetNextItemWidth(sliderWHUD);
		if (ImGui::SliderFloat("##GammaSliderHUD", &CurrentSettings.GammaGain, 0.70f, 1.30f, "%.2fx"))
		{
			CurrentSettings.AutoBrightness = false; // User manually intervened
			changed = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
		ImGui::SameLine(0, spHUD);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
		if (ImGui::Button("Reset##gamma_hud", ImVec2(btnWHUD, 0.0f)))
		{
			CurrentSettings.GammaGain = 1.0f;
			CurrentSettings.AutoBrightness = false;
			changed = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip(isDe ? "Setzt Helligkeit auf 1.00x zurueck" : "Resets brightness to 1.00x");

		// ── Bottom Status Bar: Active Features & Modules ─────────────────────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		struct ActiveModule {
			std::string label;
			ImVec4 col;
		};
		std::vector<ActiveModule> activeModules;

		if (CurrentSettings.Enabled) {
			if (CurrentSettings.Mixed) {
				char b[64];
				std::snprintf(b, sizeof(b), "%s (%d%%/%d%%)", isDe ? "Gemischt" : "Mixed",
					(int)(CurrentSettings.MixedRgSeverity01 * 100.0), (int)(CurrentSettings.MixedBySeverity01 * 100.0));
				activeModules.push_back({ std::string("Filter: ") + b, Theme::kTextCyanLicht });
			} else if (CurrentSettings.Severity01 > 0.001) {
				const char* name = (CurrentSettings.Type == BalanceType::Protan) ? "Protan" :
				                   (CurrentSettings.Type == BalanceType::Deutan) ? "Deutan" : "Tritan";
				char b[64];
				std::snprintf(b, sizeof(b), "%s (%d%%)", name, (int)(CurrentSettings.Severity01 * 100.0));
				activeModules.push_back({ std::string("Filter: ") + b, Theme::kTextCyanLicht });
			} else {
				activeModules.push_back({ isDe ? "Filter: Bereit (0%)" : "Filter: Ready (0%)", Theme::kTextBlauPeak });
			}
		}

		if (CurrentSettings.CommanderTagMode != 0) {
			activeModules.push_back({ CurrentSettings.SmartEnhancer ? (isDe ? "Com-Tag: Smart-Auto" : "Com-Tag: Smart-Auto")
			                                                        : (isDe ? "Com-Tag: Preset" : "Com-Tag: Preset"),
			                          Theme::kTextGoldLabel });
		}

		if (CurrentSettings.EnableHybridMode) {
			activeModules.push_back({ isDe ? "Hybrid-Modus" : "Hybrid Mode", ImVec4(0.40f, 0.90f, 0.70f, 1.0f) });
		}

		if (CurrentSettings.AutoBrightness) {
			char b[64];
			std::snprintf(b, sizeof(b), "%s (%.2fx)", isDe ? "Auto-Helligkeit" : "Auto Brightness", CurrentSettings.GammaGain);
			activeModules.push_back({ b, Theme::kTextCyanLicht });
		} else if (std::abs(CurrentSettings.GammaGain - 1.0f) > 0.01f) {
			char b[64];
			std::snprintf(b, sizeof(b), "Gamma: %.2fx", CurrentSettings.GammaGain);
			activeModules.push_back({ b, Theme::kTextBlauPeak });
		}

		if (CurrentSettings.FreeFilterEnabled) {
			activeModules.push_back({ isDe ? "Farb-Tausch" : "Color Swap", ImVec4(0.95f, 0.65f, 0.35f, 1.0f) });
		}

		if (CurrentSettings.LabModeEnabled) {
			activeModules.push_back({ isDe ? "Filter-Labor" : "Filter Lab", ImVec4(0.85f, 0.50f, 0.95f, 1.0f) });
		}

		// Render the active features row with clean styled chips
		ImGui::TextDisabled("%s:", isDe ? "Aktiv" : "Active");
		ImGui::SameLine(0, 8.0f);

		if (activeModules.empty()) {
			ImGui::TextColored(Theme::kTextSecondary, "%s", isDe ? "Keine Effekte aktiv (Neutral)" : "No effects active (Neutral)");
		} else {
			for (size_t i = 0; i < activeModules.size(); ++i) {
				if (i > 0) ImGui::SameLine(0, 6.0f);

				ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.08f, 0.14f, 0.20f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.20f, 0.28f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.06f, 0.10f, 0.16f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_Text,          activeModules[i].col);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 1.0f));

				char chipId[32];
				std::snprintf(chipId, sizeof(chipId), "##mod_chip_%zu", i);
				std::string chipText = std::string("[+] ") + activeModules[i].label + chipId;
				ImGui::Button(chipText.c_str());

				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(4);
			}
		}

		if (saveNeeded) {
			CurrentSettings.Save(AddonDir);
			Recompute(/*aForce=*/true);
		} else if (changed) {
			Recompute(/*aForce=*/false);
		}

		ImGui::PopID();
	}

	void RenderFullDetachedWindow()
	{
		RenderMainWindow();
	}

	void EnsureDeferredInitialized()
	{
		if (s_deferredInitDone.load()) return;

		if (GetColorEffectController().Initialize())
		{
			s_deferredInitDone.store(true);
			if (CurrentSettings.Enabled)
			{
				Recompute(/*aForce=*/true);
			}
		}
		else
		{
			s_deferredInitDone.store(true);
			if (APIDefs && APIDefs->Log)
			{
				APIDefs->Log(ELogLevel_WARNING, "cba4gw2", "MagInitialize deferred init failed.");
			}
		}
	}

	void AddonOptions()
	{
		if (!ImGui::GetCurrentContext()) return;
		EnsureDeferredInitialized();
		RenderEmbeddedOptions();
	}

	void RenderSafeStartDialog()
	{
		if (!s_safeStartPending.load() || !ImGui::GetCurrentContext()) return;

		bool isDe = (CurrentSettings.Language == 2 || CurrentSettings.Language == 0);
		ImGui::SetNextWindowSize(ImVec2(480.0f, 0.0f), ImGuiCond_Always);
		ImVec2 disp = ImGui::GetIO().DisplaySize;
		ImVec2 center(disp.x * 0.5f, disp.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
		bool open = true;
		if (ImGui::Begin(isDe ? "cba4gw2 - Sicherheitsstart & Profilauswahl###CBA_SafeStart" : "cba4gw2 - Safe-Start & Profile Gate###CBA_SafeStart", &open, flags))
		{
			if (CurrentSettings.SafeModeTriggered)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Theme::kTextGoldLabel);
				ImGui::TextUnformatted(isDe ? "[!] CBA Safe-Mode aktiv!" : "[!] CBA Safe-Mode Active!");
				ImGui::PopStyleColor();
				ImGui::Spacing();
				ImGui::TextWrapped(isDe 
					? "Das Spiel wurde beim letzten Mal unplanmaessig beendet oder es gab einen Absturz. Um Blendungen zu vermeiden, bleibt der Filter vorerst neutral (AUS)."
					: "The game exited unexpectedly or crashed last session. To prevent blinding visual effects, the filter starts disarmed (OFF).");
				ImGui::Spacing();
			}
			else
			{
				ImGui::TextColored(Theme::kTextBlauPeak, "%s", isDe ? "Willkommen bei cba4gw2!" : "Welcome to cba4gw2!");
				ImGui::Spacing();
				ImGui::TextWrapped(isDe
					? "Waehle, wie das Addon fuer diese Sitzung gestartet werden soll:"
					: "Choose how the addon should be initialized for this session:");
				ImGui::Spacing();
			}

			ImGui::Separator();
			ImGui::Spacing();

			// Option 1: Start with saved settings
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
			if (ImGui::Button(isDe ? "  Gespeicherte Settings aktivieren  " : "  Activate Saved Settings  ", ImVec2(-FLT_MIN, 34.0f)))
			{
				CurrentSettings.Enabled = true;
				s_safeStartPending.store(false);
				CurrentSettings.Save(AddonDir);
				Recompute(/*aForce=*/true);
			}
			ImGui::PopStyleColor(4);

			ImGui::Spacing();

			// Option 2: Open setup with filter OFF
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
			if (ImGui::Button(isDe ? "  Setup oeffnen (Filter bleibt AUS)  " : "  Open Setup (Filter remains OFF)  ", ImVec2(-FLT_MIN, 32.0f)))
			{
				CurrentSettings.Enabled = false;
				CurrentSettings.ShowMainWindow = true;
				s_focusMainWindow = true;
				s_safeStartPending.store(false);
				Recompute(/*aForce=*/true);
			}
			ImGui::PopStyleColor(4);

			ImGui::Spacing();

			// Option 3: Reset to factory defaults
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
			if (ImGui::Button(isDe ? "  Auf Werkseinstellung (Neutral) zuruecksetzen  " : "  Reset to Factory Neutral Defaults  ", ImVec2(-FLT_MIN, 28.0f)))
			{
				CurrentSettings.Enabled = false;
				CurrentSettings.Severity01 = 0.0;
				CurrentSettings.Mixed = false;
				CurrentSettings.MixedRgSeverity01 = 0.0;
				CurrentSettings.MixedBySeverity01 = 0.0;
				CurrentSettings.GammaGain = 1.0f;
				CurrentSettings.Save(AddonDir);
				s_safeStartPending.store(false);
				Recompute(/*aForce=*/true);
			}
			ImGui::PopStyleColor(3);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Checkbox(isDe ? "Zukuenftig direkt starten (ohne Gate)" : "Always start directly (skip this gate)", &CurrentSettings.AlwaysDirectStart))
			{
				CurrentSettings.Save(AddonDir);
			}

			ImGui::End();
		}
		if (!open)
		{
			s_safeStartPending.store(false);
		}
	}

	void AddonRenderWindow()
	{
		RenderSafeStartDialog();

		// ── Deferred Warmup Gate ────────────────────────────────────────────────
		// Give GW2, D3D11 swapchains, ArcDPS, FastLoad and NVIDIA Overlay
		// 30 frames of stable rendering before touching DWM magnification.
		if (!s_deferredInitDone.load())
		{
			static int s_renderWarmupFrames = 0;
			if (++s_renderWarmupFrames >= 30)
			{
				EnsureDeferredInitialized();
			}
		}

		// ── Hybrid Scanner Frame Capture & Overlay ──────────────────────────────
		if ((CurrentSettings.EnableHybridMode || CurrentSettings.CommanderTagMode != 0) && s_deferredInitDone.load())
		{
			IDXGISwapChain* swapChain = APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr;
			if (swapChain)
			{
				GetHybridScanner().ScanFrame(swapChain);
			}

			int texW = 0, texH = 0;
			ID3D11ShaderResourceView* srv = GetHybridScanner().GetOverlaySRV(texW, texH);
			if (srv)
			{
				ImVec2 disp = ImGui::GetIO().DisplaySize;
				ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)srv, ImVec2(0, 0), disp);
			}
		}

		if (s_showC64Credits.load())
		{
			RenderC64CreditsOverlay();
		}

		// ── Window 1: CBA Main Window ─────────────────────────────────────────
		if (CurrentSettings.ShowMainWindow && ImGui::GetCurrentContext())
		{
			if (s_focusMainWindow)
			{
				ImGui::SetNextWindowFocus();
				s_focusMainWindow = false;
			}

			ImVec2 disp = ImGui::GetIO().DisplaySize;
			float screenH = (disp.y > 400.0f) ? disp.y : 1080.0f;
			float defaultW = 530.0f;
			float defaultH = std::clamp(screenH * 0.76f, 620.0f, 860.0f);

			ImGui::SetNextWindowBgAlpha(0.96f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg,             ImVec4(0.06f, 0.08f, 0.12f, 0.96f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          ImVec4(0.04f, 0.06f, 0.09f, 0.65f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ImVec4(0.24f, 0.42f, 0.65f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.34f, 0.56f, 0.85f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ImVec4(0.42f, 0.72f, 1.00f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Border,               ImVec4(0.22f, 0.35f, 0.52f, 0.55f));

			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,     14.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,     ImVec2(12.0f, 10.0f));

			// Sleek, variable sidebar sizing: min 480x420, max 1600 x screenH
			ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f, 420.0f), ImVec2(1600.0f, screenH - 40.0f));

			if (s_resetMainWindowPos)
			{
				float posX = 40.0f;
				float posY = 60.0f;
				ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(defaultW, defaultH), ImGuiCond_Always);
				s_resetMainWindowPos = false;
			}
			else
			{
				ImGui::SetNextWindowSize(ImVec2(defaultW, defaultH), ImGuiCond_FirstUseEver);
			}

			bool isDe = (Strings().Enabled[0] == 'A');
			ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
			if (ImGui::Begin(isDe ? "cba4gw2 - Hauptfenster###CBA_MainWindow" : "cba4gw2 - Main Window###CBA_MainWindow", &CurrentSettings.ShowMainWindow, winFlags))
			{
				RenderMainWindow();
			}
			ImGui::End();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor(6);
		}

		// ── Window 2: Sensor & Spectral Graph HUD Window ─────────────────────
		if (CurrentSettings.ShowGraphWindow && ImGui::GetCurrentContext())
		{
			if (s_focusGraphWindow)
			{
				ImGui::SetNextWindowFocus();
				s_focusGraphWindow = false;
			}

			float clampedOpacity = std::clamp(CurrentSettings.UiOpacity, 0.0f, 1.0f);
			float winBgAlpha = std::clamp(clampedOpacity * 0.96f, 0.05f, 0.96f);
			ImGui::SetNextWindowBgAlpha(winBgAlpha);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.06f, 0.10f, winBgAlpha));

			// Sleek, variable HUD sizing: min 340x280, max 1600x1200
			ImGui::SetNextWindowSizeConstraints(ImVec2(340.0f, 280.0f), ImVec2(1600.0f, 1200.0f));

			if (s_resetGraphWindowPos)
			{
				float w = 440.0f;
				float h = 405.0f;
				float posX = 40.0f;
				float posY = 60.0f;
				ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
				s_resetGraphWindowPos = false;
			}
			else
			{
				ImGui::SetNextWindowSize(ImVec2(440.0f, 405.0f), ImGuiCond_FirstUseEver);
			}

			ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse;
			if (ImGui::Begin("cba graph###CBA_GraphWindow", &CurrentSettings.ShowGraphWindow, winFlags))
			{
				RenderGraphWindow();
			}
			ImGui::End();
			ImGui::PopStyleColor();
		}

		// ── Window 3: Filter Lab Detached Floating Window ────────────────────
		if (CurrentSettings.ShowLabWindow && ImGui::GetCurrentContext())
		{
			if (s_focusLabWindow)
			{
				ImGui::SetNextWindowFocus();
				s_focusLabWindow = false;
			}

			ImGui::SetNextWindowBgAlpha(0.96f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg,             ImVec4(0.06f, 0.08f, 0.12f, 0.96f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          ImVec4(0.04f, 0.06f, 0.09f, 0.65f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ImVec4(0.24f, 0.42f, 0.65f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.34f, 0.56f, 0.85f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ImVec4(0.42f, 0.72f, 1.00f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Border,               ImVec4(0.22f, 0.35f, 0.52f, 0.55f));

			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,     14.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,     ImVec2(12.0f, 10.0f));

			ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f, 400.0f), ImVec2(1600.0f, 1200.0f));

			if (s_resetLabWindowPos)
			{
				float posX = 40.0f;
				float posY = 60.0f;
				ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(620.0f, 520.0f), ImGuiCond_Always);
				s_resetLabWindowPos = false;
			}
			else
			{
				ImGui::SetNextWindowSize(ImVec2(620.0f, 520.0f), ImGuiCond_FirstUseEver);
			}

			bool isDe = (Strings().Enabled[0] == 'A');
			ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse;
			if (ImGui::Begin(isDe ? "cba4gw2 - Filter-Labor###CBA_LabWindow" : "cba4gw2 - Filter Lab###CBA_LabWindow", &CurrentSettings.ShowLabWindow, winFlags))
			{
				bool labChanged = false;
				bool labSaveNeeded = false;
				DrawFilterLabWidget(isDe, labChanged, labSaveNeeded);
				if (labSaveNeeded)
				{
					CurrentSettings.Save(AddonDir);
					Recompute(/*aForce=*/true);
				}
				else if (labChanged)
				{
					Recompute(/*aForce=*/false);
				}
			}
			ImGui::End();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor(6);
		}
	}

	void AddonLoad(AddonAPI* aApi)
	{
		try
		{
			APIDefs = aApi;
			if (!APIDefs) return;

			if (APIDefs->DataLink.Get)
			{
				NexusLink = (NexusLinkData*)APIDefs->DataLink.Get("DL_NEXUS_LINK");
				MumbleLinkData = (GW2MumbleLink*)APIDefs->DataLink.Get("GW2_MUMBLE_LINK");
			}

			if (APIDefs->ImguiContext)
				ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
			if (APIDefs->ImguiMalloc && APIDefs->ImguiFree)
				ImGui::SetAllocatorFunctions(
					(void* (*)(size_t, void*))APIDefs->ImguiMalloc,
					(void(*)(void*, void*))APIDefs->ImguiFree);

			const char* dir = (APIDefs->Paths.GetAddonDirectory) ? APIDefs->Paths.GetAddonDirectory("cba") : nullptr;
			AddonDir = dir ? dir : "";
			if (!AddonDir.empty())
			{
				std::error_code ec;
				std::filesystem::create_directories(AddonDir, ec);
			}
			CurrentSettings = Settings::Load(AddonDir);

			// Backward compatibility migration: If DetachedWindow was previously set, open Main Window
			if (CurrentSettings.DetachedWindow)
			{
				CurrentSettings.ShowMainWindow = true;
				CurrentSettings.DetachedWindow = false;
			}

			// Initialize hybrid background scanner
			GetHybridScanner().Initialize();
			GetHybridScanner().SetEnabled(CurrentSettings.EnableHybridMode);
			UpdateTagEnhancerConflicts();

			// Session Breadcrumb crash guard: mark session running
			Settings::MarkRunning(AddonDir);

			// Safe-Start Gate: If previous crash happened or AlwaysDirectStart is false, hold at safe gate
			if (CurrentSettings.SafeModeTriggered || !CurrentSettings.AlwaysDirectStart)
			{
				s_safeStartPending.store(true);
				CurrentSettings.Enabled = false; // Filter starts disarmed
			}
			else if (!CurrentSettings.LoadOnStartup)
			{
				CurrentSettings.Enabled = false;
			}

			// Escape closes windows
			if (APIDefs->UI.RegisterCloseOnEscape)
			{
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Hauptfenster###CBA_MainWindow", &CurrentSettings.ShowMainWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Main Window###CBA_MainWindow", &CurrentSettings.ShowMainWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba graph###CBA_GraphWindow", &CurrentSettings.ShowGraphWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Sensor Graph###CBA_GraphWindow", &CurrentSettings.ShowGraphWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Filter-Labor###CBA_LabWindow", &CurrentSettings.ShowLabWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Filter Lab###CBA_LabWindow", &CurrentSettings.ShowLabWindow);
			}

			// Renderers
			if (APIDefs->Renderer.Register)
			{
				APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);
				APIDefs->Renderer.Register(ERenderType_Render, AddonRenderWindow);
			}

			// WndProc
			if (APIDefs->WndProc.Register)
			{
				APIDefs->WndProc.Register(AddonWndProc);
			}

			// QuickAccess toolbar icon & window toggle keybinds (Clean, left-aligned names in Nexus!)
			if (APIDefs->InputBinds.RegisterWithString)
			{
				APIDefs->InputBinds.RegisterWithString("CBA - Main Window", ProcessKeybind, "CTRL+SHIFT+C");
				APIDefs->InputBinds.RegisterWithString("CBA - Filter Off", ProcessKeybind, "CTRL+SHIFT+O");
			}
			if (APIDefs->Textures.GetOrCreateFromMemory)
			{
				APIDefs->Textures.GetOrCreateFromMemory("CBA_ICON", (void*)kCbaIconPng, kCbaIconPngSize);
			}
			if (APIDefs->QuickAccess.Add && CurrentSettings.ShowQuickAccessIcon)
			{
				APIDefs->QuickAccess.Add("QA_CBA", "CBA_ICON", "CBA_ICON", "CBA - Main Window", "cba4gw2 (Strg+Shift+C / Filter Off: Strg+Shift+O)");
			}

			// Start state watchdog thread (monitors focus transitions every 50ms)
			s_watchdogRunning = true;
			s_watchdogThread = std::thread(WatchdogLoop);
		}
		catch (...)
		{
			// Never allow an unhandled exception to escape AddonLoad
		}
	}

	void AddonUnload()
	{
		try
		{
			// Session Breadcrumb: mark graceful exit
			Settings::MarkCleanExit(AddonDir);

			// Always close windows on game exit so they start closed on next launch
			CurrentSettings.ShowMainWindow = false;
			CurrentSettings.ShowGraphWindow = false;
			CurrentSettings.ShowLabWindow = false;
			CurrentSettings.DetachedWindow = false;
			CurrentSettings.Save(AddonDir);

			s_showC64Credits.store(false);
			StopC64Audio();

			// Stop state watchdog thread
			s_watchdogRunning = false;
			if (s_watchdogThread.joinable())
			{
				s_watchdogThread.join();
			}

			if (APIDefs)
			{
				if (APIDefs->QuickAccess.Remove)
				{
					APIDefs->QuickAccess.Remove("QA_CBA");
				}
				if (APIDefs->InputBinds.Deregister)
				{
					APIDefs->InputBinds.Deregister("CBA - Main Window");
					APIDefs->InputBinds.Deregister("CBA - Filter Off");
					APIDefs->InputBinds.Deregister("CBA - Sensor Graph");
					APIDefs->InputBinds.Deregister("CBA - Not-Aus");
					APIDefs->InputBinds.Deregister("KB_CBA_WINDOW");
				}
				if (APIDefs->UI.DeregisterCloseOnEscape)
				{
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Hauptfenster###CBA_MainWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Main Window###CBA_MainWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba graph###CBA_GraphWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Sensor Graph###CBA_GraphWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Filter-Labor###CBA_LabWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Filter Lab###CBA_LabWindow");
				}
				if (APIDefs->WndProc.Deregister)
					APIDefs->WndProc.Deregister(AddonWndProc);
				if (APIDefs->Renderer.Deregister)
				{
					APIDefs->Renderer.Deregister(AddonRenderWindow);
					APIDefs->Renderer.Deregister(AddonOptions);
				}
				if (APIDefs->UI.DeregisterCloseOnEscape)
				{
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Hauptfenster###CBA_MainWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Sensor Graph###CBA_GraphWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2###CBA_FloatingWindow");
				}
			}

			GetHybridScanner().Shutdown();
			GetColorEffectController().Shutdown(); // clears the effect before unload
		}
		catch (...)
		{
		}
	}


BOOL APIENTRY DllMain(HMODULE aModule, DWORD aReasonForCall, LPVOID)
{
	if (aReasonForCall == DLL_PROCESS_ATTACH)
		AddonModuleHandle = aModule;

	return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef.Signature = -78341; // arbitrary negative ID - not on Raidcore (yet)
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "cba4gw2";
	AddonDef.Version.Major = 1;
	AddonDef.Version.Minor = 0;
	AddonDef.Version.Build = 2;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "Emisan01";
	AddonDef.Description =
		"Color balance, contrast enhancement and visual assist for Guild Wars 2. "
		"Applied via the Windows Magnification API.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	AddonDef.Provider = EUpdateProvider_GitHub;
	AddonDef.UpdateLink = "https://github.com/Emisan01/cba4gw2";

	return &AddonDef;
}
