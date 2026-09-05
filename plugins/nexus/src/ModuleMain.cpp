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

using namespace cba;

namespace
{
	AddonDefinition AddonDef{}; // empty definition
	// UI helper constants and functions
	static const float kPanelItemWidth = 240.0f;
	static void PanelHeader(const char* title) {
	    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f,0.6f,0.9f,1.0f));
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
		const char* TagGreen;
		const char* TagPurple;
		const char* TagYellow;
		const char* TagBlue;
		const char* TagPink;
		const char* TagOrange;
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
			"Korrekturprofil",
			"Protan",
			"Deutan",
			"Tritan",
			"Gemischt",
			"St\xc3\xa4rke",
			"Rot-Gr\xc3\xbc\x6e St\xc3\xa4rke",
			"Blau-Gelb St\xc3\xa4rke",
			"Fenstermodus",
			"Screenshots: GW2-intern wirkt vor dem Filter. PrintScreen / Win+PrintScreen und die meisten Display-Captures sehen den Filter.",
			"Sprache",
			"AQ/HRR Diagnose",
			"Freitext f\xc3\xbcr Diagnose-Presets oder eine genauere Zuordnung.",
			"Hybrid Modus",
			"Kinematic Fader: Blendet das Overlay bei schnellen Kamerabewegungen automatisch sanft aus.\nLiest den GW2 Render-Buffer im Hintergrund, um WCAG-Fehler zu erkennen.",
			"Color Profile Graph: Verlauf der R/G/B-Farbkan\xc3\xa4le",
			"Diagnose Profil-Referenz (AQ/HRR):",
			"Hinweis: Trage hier z.B. Farnsworth-Munsell Scores, HRR-Ergebnisse\n(wie 'Deutan Mild') oder andere Referenzen ein, um dieses\nFarb-Profil eindeutig zuzuordnen.",
			"Commander Tag Enhancer (Symbol-Unterscheidung)",
			"Ersetzt bestimmte GW2-Symbole durch extrem kontrastreiche Signalfarben.",
			"Enhancer Aktivieren",
			"Voreinstellungen (Metabattle)",
			"Tag Rot",
			"Tag Gr\xc3\xbc\x6e",
			"Tag Lila",
			"Tag Gelb",
			"Tag Blau",
			"Tag Pink",
			"Tag Orange",
			"Tag Wei\xc3\x9f",

			"Auto",
			"Smart-Enhancer: Symbole automatisch anpassen",
			"Passt Commander-Tags und Wegmarker automatisch an die oben gew\xc3\xa4hlte Farbsehschw\xc3\xa4\x63he an.",

			// Bottom Section
			" Beim Spielstart laden (Load on Startup)",
			"Aktiviert: Der gespeicherte Filterzustand wird beim Starten von GW2 geladen.\nDeaktiviert: Der Filter startet bei Spielstart immer inaktiv/neutral (kein ungewollter Farbstich).",
			"Profil-Zusammenfassung (Live-Feedback):",
			"Werte:",
			"Einstufung:",
			" Filter auch im Hintergrund aktiv lassen (z. B. bei Klick in Browser / 2. Monitor)",
			"Standard (Deaktiviert): Sobald GW2 den Fokus verliert (z. B. Klick in den Browser auf Monitor 2 oder Alt-Tab), pausiert der Filter sofort, damit andere Programme nicht beeinflusst werden.\n\nAktiviert: L\xc3\xa4sst den Filter auch weiterlaufen, wenn ein anderes Fenster aktiv ist.\nHinweis: Bei Minimieren von GW2 pausiert der Filter in jedem Fall sofort.",
			"Fokus-W\xc3\xa4\x63hter: Filter ist exklusiv an GW2 gebunden und pausiert bei Alt-Tab/Klick auf 2. Monitor.",
			"Hintergrund-Modus: Filter bleibt auch bei Fokusverlust aktiv (pausiert nur bei Minimieren).",
			"Entwickler- & Debug-Modus (Performance Watchdog)",
			"Methodik & Referenzen:",
			"  \xe2\x80\xa2 Daltonisierung: Fidaner et al. (2005)   \xe2\x80\xa2 LMS-Dichromasie: Vi\xc3\xa9not, Brettel & Mollon (1999)\n  \xe2\x80\xa2 Hunt-Pointer-Est\xc3\xa9vez (HPE) Farbraum   \xe2\x80\xa2 W3C WCAG 2.1 Farbkontrast"
		};

		static const L10n en{
			"Enabled",
			"Correction profile",
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
			"AQ/HRR diagnosis",
			"Free text for diagnosis presets or more precise mapping.",
			"Hybrid Mode",
			"Kinematic Fader: Automatically fades out the overlay during fast camera movements.\nReads the GW2 render buffer in the background to show WCAG errors.",
			"Color Profile Graph: Transfer function of R/G/B channels",
			"Diagnostic Profile Reference (AQ/HRR):",
			"Hint: Enter Farnsworth-Munsell Scores, HRR results\n(like 'Deutan Mild') or other references here to uniquely\nassign this color profile.",
			"Commander Tag Enhancer",
			"Replaces specific GW2 symbols with high-contrast signal colors.",
			"Enable Enhancer",
			"Presets",
			"Red Tag",
			"Green Tag",
			"Purple Tag",
			"Yellow Tag",
			"Blue Tag",
			"Pink Tag",
			"Orange Tag",
			"White Tag",

			"Auto",
			"Smart-Enhancer: Adjust symbols automatically",
			"Automatically adjusts Commander Tags and waymarkers based on the selected color blindness type above.",

			// Bottom Section
			" Load on Startup",
			"Enabled: Saved filter profile is restored when Guild Wars 2 launches.\nDisabled: Filter starts inactive/neutral at launch to prevent unintended color shifts.",
			"Profile Summary (Live Feedback):",
			"Values:",
			"Classification:",
			" Keep filter active in background (e.g. browser / 2nd monitor)",
			"Default (Disabled): As soon as GW2 loses focus (e.g. clicking browser on 2nd monitor or Alt-Tab), the filter pauses immediately to avoid tinting other applications.\n\nEnabled: Keeps the filter active even when another window has focus.\nNote: Minimizing GW2 always pauses the filter immediately.",
			"Focus Watchdog: Filter is bound exclusively to GW2 and pauses on Alt-Tab / 2nd monitor focus.",
			"Background Mode: Filter remains active when focus is lost (only pauses when minimized).",
			"Developer & Debug Mode (Performance Watchdog)",
			"Methodology & References:",
			"  \xe2\x80\xa2 Daltonization: Fidaner et al. (2005)   \xe2\x80\xa2 LMS Dichromacy: Vi\xc3\xa9not, Brettel & Mollon (1999)\n  \xe2\x80\xa2 Hunt-Pointer-Est\xc3\xa9vez (HPE) Color Space   \xe2\x80\xa2 W3C WCAG 2.1 Color Contrast"
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
		std::atomic<bool> s_resetDetachedWindowPos{false};
		std::atomic<bool> s_deferredInitDone{false};

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

		if (!CurrentSettings.Enabled)
		{
			if (s_hasApplied)
			{
				controller.Clear();
				s_hasApplied = false;
			}
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
			if (s_hasApplied)
			{
				controller.Clear();
				s_hasApplied = false;
			}
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
				if (aWParam == 'C')
				{
					bool altDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
					bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
					bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

					if (altDown && !shiftDown)
					{
						if (ctrlDown)
						{
							CurrentSettings.Enabled = !CurrentSettings.Enabled;
							CurrentSettings.Save(AddonDir);
							Recompute(/*aForce=*/true);
						}
						else
						{
							CurrentSettings.DetachedWindow = !CurrentSettings.DetachedWindow;
						}
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

		if (strcmp(aIdentifier, "KB_CBA_WINDOW") == 0)
		{
			EnsureDeferredInitialized();
			CurrentSettings.DetachedWindow = !CurrentSettings.DetachedWindow;
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
		const float badgeSpaceRight = 60.0f;

		const float plotX = aOrigin.x + pad + labelSpaceLeft;
		const float plotY = aOrigin.y + pad + 4.0f;
		const float plotW = aW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		const float plotH = aH - pad * 2 - labelSpaceBottom - 4.0f;

		// Panel background: transparent glass when detached (showing GW2), deep slate when docked
		float panelA = aIsDetached ? std::clamp(aOpacity * 0.18f, 0.02f, 0.70f) : 0.92f;
		int bgAlpha = (int)(panelA * 255.0f);
		int borderAlpha = aIsDetached ? (int)(aOpacity * 130.0f) : 150;

		aDraw->AddRectFilled(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(12, 15, 22, bgAlpha), 6.0f);
		aDraw->AddRect(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(65, 85, 125, borderAlpha), 6.0f, 0, 1.2f);

		// Grid with axis tick labels
		int gridAlpha = aIsDetached ? (int)(aOpacity * 45.0f) : 65;
		int textAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 1.4f, 0.45f, 1.0f) * 210.0f) : 210;

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

		// Right side legend badges for Red, Green, Blue
		static const char* kChanLabels[3] = { "R-Kurve", "G-Kurve", "B-Kurve" };
		float legendY = plotY + 12.0f;
		for (int ch = 0; ch < 3; ++ch) {
			float itemY = legendY + ch * 20.0f;
			aDraw->AddCircleFilled(ImVec2(plotX + plotW + 10.0f, itemY + 5.0f), 4.0f, kChanCol[ch]);
			aDraw->AddText(ImVec2(plotX + plotW + 18.0f, itemY - 1.0f), kChanCol[ch], kChanLabels[ch]);
		}
	}

	void RenderCbaControls(bool isDetached)
	{
		if (!ImGui::GetCurrentContext()) return;
		const L10n& t = Strings();
		bool changed = false;
		bool saveNeeded = false;

		float clampedOpacity = std::clamp(CurrentSettings.UiOpacity, 0.2f, 1.0f);

		ImGui::PushID(isDetached ? "CBA_Detached" : "CBA_Embedded");

		// UI style settings for a premium look with live background transparency
		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 4.0f;
		style.WindowRounding = 6.0f;
		style.Colors[ImGuiCol_Header] = ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
		style.ItemSpacing = ImVec2(8, 5);

		if (!isDetached) {
			// Dynamically modulate the already-rendered background vertices of the Nexus window
			// so opacity adjustments take effect immediately without requiring a window re-open.
			ImGuiWindow* curWin = ImGui::GetCurrentWindow();
			ImGuiWindow* rootWin = curWin ? curWin->RootWindow : nullptr;
			ImU32 alphaByte = (ImU32)(clampedOpacity * 255.0f) << IM_COL32_A_SHIFT;
			if (rootWin && rootWin->DrawList && rootWin->DrawList->VtxBuffer.Size >= 4) {
				for (int i = 0; i < 4; ++i) {
					rootWin->DrawList->VtxBuffer[i].col = (rootWin->DrawList->VtxBuffer[i].col & ~IM_COL32_A_MASK) | alphaByte;
				}
			}
			if (curWin && curWin != rootWin && curWin->DrawList && curWin->DrawList->VtxBuffer.Size >= 4) {
				for (int i = 0; i < 4; ++i) {
					curWin->DrawList->VtxBuffer[i].col = (curWin->DrawList->VtxBuffer[i].col & ~IM_COL32_A_MASK) | alphaByte;
				}
			}
		}

		// Keep widget content crisp and readable while window background becomes see-through
		float widgetAlpha = std::clamp(0.75f + 0.25f * clampedOpacity, 0.75f, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, widgetAlpha);

		// ── Header Bar & Dock/Detach Controls ─────────────────────────────────
		ImGui::Text("cba4gw2");
		ImGui::SameLine();
		
		// Status indicators
		bool hasNexus = (APIDefs && APIDefs->DataLink.Get("DL_NEXUS_LINK") != nullptr);
		bool hasMumble = (APIDefs && APIDefs->DataLink.Get("GW2_MUMBLE_LINK") != nullptr);
		bool handshake = hasNexus && hasMumble;
		
		ImGui::ColorButton("##handshakeStatus", handshake ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoAlpha, ImVec2(16, 16));
		if (ImGui::IsItemHovered()) ImGui::SetTooltip(handshake ? "Handshake OK (Nexus & Mumble)" : "Handshake fehlgeschlagen (Nexus/Mumble fehlt)");

		ImGui::SameLine();
		if (ImGui::Checkbox("Detach Graph", &CurrentSettings.DetachedWindow)) {
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Aktiviert: Schwebendes, transparentes Graph-Fenster direkt im Spiel einblenden.\nDeaktiviert: Fenster andocken / schlie\xc3\x9f" "en.\n(Schnelltaste: ALT+C)");
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset GUI")) {
			s_resetDetachedWindowPos = true;
			CurrentSettings.DetachedWindow = true;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Setzt Position und Gr\xc3\xb6\xc3\x9f" "e des schwebenden Fensters wieder mittig auf den Bildschirm zur\xc3\xbc" "ck.");
		}

		// ── Primary Controls (Enable, Language, Transparency) ────────────────
		// Styled ON / OFF toggle — the button that starts the magic
		{
			bool wasEnabled = CurrentSettings.Enabled;
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(9.0f, 3.0f));
			if (wasEnabled) {
				ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.11f, 0.50f, 0.21f, 0.92f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.64f, 0.28f, 0.97f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.08f, 0.38f, 0.16f, 1.00f));
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.26f, 0.26f, 0.28f, 0.78f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.36f, 0.36f, 0.38f, 0.88f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.18f, 0.18f, 0.20f, 1.00f));
			}
			if (ImGui::Button(wasEnabled ? "ON " : "OFF")) {
				CurrentSettings.Enabled = !CurrentSettings.Enabled;
				changed    = true;
				saveNeeded = true;
			}
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(wasEnabled ? "Filter active — click to disable" : "Filter inactive — click to enable");
		}
		
		ImGui::SameLine();
		ImGui::TextDisabled(" | ");
		ImGui::SameLine();

		int langComboIdx = 0;
		if (CurrentSettings.Language == 0) langComboIdx = 1;      // System (Windows)
		else if (CurrentSettings.Language == 2) langComboIdx = 2; // Deutsch
		else langComboIdx = 0;                                     // English (1)

		const char* langComboItems[] = {
			"English",
			"System (Windows)",
			"Deutsch"
		};

		ImGui::SetNextItemWidth(130.0f);
		if (ImGui::Combo("##LangCombo", &langComboIdx, langComboItems, IM_ARRAYSIZE(langComboItems)))
		{
			if (langComboIdx == 0) CurrentSettings.Language = 1;      // English
			else if (langComboIdx == 1) CurrentSettings.Language = 0; // System (Windows)
			else if (langComboIdx == 2) CurrentSettings.Language = 2; // Deutsch
			changed = true;
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Language:\n- English: Standard (Default)\n- System (Windows): Matches active Windows OS language\n- Deutsch: Explizit Deutsch");
		}

		ImGui::SameLine();
		ImGui::TextDisabled(" | ");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(110.0f);
		if (ImGui::SliderFloat("Opacity", &CurrentSettings.UiOpacity, 0.20f, 1.00f, "%.2f")) {
			CurrentSettings.UiOpacity = std::clamp(CurrentSettings.UiOpacity, 0.20f, 1.00f);
			changed = true;
		}



		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ── Correction Profile Selection ──────────────────────────────────────
		ImGui::TextUnformatted(t.CorrectionProfile);
		ImGui::Spacing();

		auto typeBtn = [&](const char* aLabel, bool aActive, DeficiencyType aType) {
			if (ImGui::RadioButton(aLabel, aActive)) {
				if (CurrentSettings.Mixed || CurrentSettings.Type != aType) {
					CurrentSettings.Mixed = false;
					CurrentSettings.Type  = aType;
					CurrentSettings.Severity01 = 0.0; // Start neutral on mode switch unless saved
					changed = true;
				}
			}
		};
		typeBtn(t.Protan, !CurrentSettings.Mixed && CurrentSettings.Type == DeficiencyType::Protan, DeficiencyType::Protan);
		ImGui::SameLine();
		typeBtn(t.Deutan, !CurrentSettings.Mixed && CurrentSettings.Type == DeficiencyType::Deutan, DeficiencyType::Deutan);
		ImGui::SameLine();
		typeBtn(t.Tritan, !CurrentSettings.Mixed && CurrentSettings.Type == DeficiencyType::Tritan, DeficiencyType::Tritan);
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
			ImGui::SetNextItemWidth(240.0f);
			if (ImGui::SliderFloat(t.RgStrength, &rg, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.MixedRgSeverity01 = std::clamp(rg, 0.0f, 1.0f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine();
			if (ImGui::Button((std::string("Reset##rg")).c_str())) { 
				CurrentSettings.MixedRgSeverity01 = 0.0f; 
				changed = true; 
				saveNeeded = true; 
			}
			
			ImGui::SetNextItemWidth(240.0f);
			if (ImGui::SliderFloat(t.ByStrength, &by, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.MixedBySeverity01 = std::clamp(by, 0.0f, 1.0f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine();
			if (ImGui::Button((std::string("Reset##by")).c_str())) { 
				CurrentSettings.MixedBySeverity01 = 0.0f; 
				changed = true; 
				saveNeeded = true; 
			}
		} else {
			float sev = (float)CurrentSettings.Severity01;
			ImGui::SetNextItemWidth(240.0f);
			if (ImGui::SliderFloat(t.Strength, &sev, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.Severity01 = std::clamp(sev, 0.0f, 1.0f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine();
			if (ImGui::Button((std::string("Reset##sev")).c_str())) { 
				CurrentSettings.Severity01 = 0.0f; 
				changed = true; 
				saveNeeded = true; 
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ── Correction curves ─────────────────────────────────────────────────
		double corrMat[3][3];
		if (CurrentSettings.Mixed)
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01, corrMat);
		else
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, corrMat);

		float availW = ImGui::GetContentRegionAvail().x;
		float graphW = std::clamp(availW, 280.0f, 380.0f);
		float graphH = 175.0f;

		{
			ImVec2 cp = ImGui::GetCursorScreenPos();
			DrawCurvePanel(ImGui::GetWindowDrawList(), cp, graphW, graphH, corrMat, isDetached, CurrentSettings.UiOpacity);
			ImGui::InvisibleButton("##curve_panel", ImVec2(graphW, graphH));
			ImGui::TextDisabled("%s", t.ColorProfileGraph);

			// Live filtered color beam preview (Strahl-Anzeiger)
			// Aligned horizontally with the coordinate system above
			const float pad = 8.0f;
			const float labelSpaceLeft = 28.0f;
			const float badgeSpaceRight = 60.0f;
			float plotX = cp.x + pad + labelSpaceLeft;
			float plotW = graphW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
			float beamH = 16.0f;

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

				int beamAlpha = isDetached ? (int)(std::clamp(CurrentSettings.UiOpacity * 210.0f, 40.0f, 255.0f)) : 255;
				ImU32 col = IM_COL32((int)(cr*255), (int)(cg*255), (int)(cb*255), beamAlpha);
				dl->AddRectFilled(ImVec2(plotX + u0 * plotW, beamPos.y), ImVec2(plotX + u1 * plotW, beamPos.y + beamH), col, (b == 0 || b == kBeamSteps - 1) ? 2.0f : 0.0f);
			}
			int borderAlpha = isDetached ? (int)(CurrentSettings.UiOpacity * 130.0f) : 180;
			dl->AddRect(ImVec2(plotX, beamPos.y), ImVec2(plotX + plotW, beamPos.y + beamH), IM_COL32(80, 100, 140, borderAlpha), 3.0f);
			ImGui::Dummy(ImVec2(graphW, beamH));
			ImGui::TextDisabled("Echtzeit-Spektrum (gefiltert)");
		}

		// ── Alles ab hier (Speichern, Reset, Status, Referenzen) NUR im Hauptfenster (Options) ──
		if (!isDetached)
		{
			ImGui::Spacing();
			ImGui::Spacing();

			// ── Save Profile with Compact Styling & Inline Animated Feedback ────
			static auto s_saveFeedbackTime = std::chrono::steady_clock::time_point{};

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
			ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.13f, 0.54f, 0.36f, 0.90f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.66f, 0.44f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.09f, 0.42f, 0.28f, 1.00f));
			if (ImGui::Button("Save Profile", ImVec2(120.0f, 26.0f))) {
				CurrentSettings.Save(AddonDir);
				s_saveFeedbackTime = std::chrono::steady_clock::now();
				changed = false;
			}
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Save current profile and settings to disk");
			}

			// Animated feedback badge (clearly visible right next to the Save button)
			if (s_saveFeedbackTime.time_since_epoch().count() > 0) {
				auto now = std::chrono::steady_clock::now();
				float elapsed = std::chrono::duration<float>(now - s_saveFeedbackTime).count();
				if (elapsed >= 0.0f && elapsed < 3.0f) {
					// Smooth fade out over the last 1.2s (from 1.8s to 3.0s)
					float alpha = (elapsed > 1.8f) ? (3.0f - elapsed) / 1.2f : 1.0f;
					alpha = std::clamp(alpha, 0.0f, 1.0f);

					// Dynamic pulse during the first 0.35s
					float pulse = (elapsed < 0.35f) ? (1.0f + 0.20f * sinf(elapsed * 3.14159265f / 0.35f)) : 1.0f;
					float green = std::clamp(0.95f * pulse, 0.0f, 1.0f);

					ImGui::SameLine(0, 12.0f);

					// Vertically center text relative to the 26px button
					float textOffset = (26.0f - ImGui::GetTextLineHeight()) * 0.5f;
					if (textOffset > 0.0f) {
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset);
					}

					bool isDe = (t.Enabled[0] == 'A');
					const char* msg = isDe ? "[OK] Einstellungen gespeichert!" : "[OK] Profile saved successfully!";
					ImGui::TextColored(ImVec4(0.20f, green, 0.45f, alpha), "%s", msg);
				}
			}

			// ── Startup Checkbox ─────────────────────────────────────────────────
			ImGui::Spacing();
			if (ImGui::Checkbox(t.LoadOnStartup, &CurrentSettings.LoadOnStartup)) {
				CurrentSettings.Save(AddonDir);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", t.LoadOnStartupTooltip);
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// ── Profil-Status & Live-Feedback ────────────────────────────────────
			ImGui::TextColored(ImVec4(0.40f, 0.80f, 1.0f, 1.0f), "%s", t.ProfileSummaryTitle);
			ImGui::Spacing();

			{
				std::string profileName;
				std::string severityDesc;
				std::string clinicalGrade;
				bool isNeutral = false;
				bool isDe = (t.Enabled[0] == 'A');

				if (CurrentSettings.Mixed) {
					profileName = isDe ? "Gemischte Farbsehschw\xc3\xa4\x63he (Mixed)" : "Mixed Color Deficiency (Dual-Axis)";
					char buf[96];
					std::snprintf(buf, sizeof(buf), isDe ? "Rot-Gr\xc3\xbc\x6e: %.1f%%   |   Blau-Gelb: %.1f%%" : "Red-Green: %.1f%%   |   Blue-Yellow: %.1f%%", 
						CurrentSettings.MixedRgSeverity01 * 100.0, CurrentSettings.MixedBySeverity01 * 100.0);
					severityDesc = buf;
					double avgSev = (CurrentSettings.MixedRgSeverity01 + CurrentSettings.MixedBySeverity01) * 0.5;
					if (avgSev <= 0.005) {
						clinicalGrade = isDe ? "Neutral (Filter inaktiv / 100% Originalfarben)" : "Neutral (Filter inactive / 100% original colors)";
						isNeutral = true;
					} else if (avgSev <= 0.35) {
						clinicalGrade = isDe ? "Milde Kompensation" : "Mild Compensation";
					} else if (avgSev <= 0.70) {
						clinicalGrade = isDe ? "Mittlere Kompensation" : "Moderate Compensation";
					} else {
						clinicalGrade = isDe ? "Starke Kompensation (hohe Farbtrennung)" : "Strong Compensation (High separation)";
					}
				} else {
					if (CurrentSettings.Type == DeficiencyType::Protan) {
						profileName = isDe ? "Protanopie (Rotsehschw\xc3\xa4\x63he)" : "Protanopia (Red Deficiency)";
					} else if (CurrentSettings.Type == DeficiencyType::Deutan) {
						profileName = isDe ? "Deuteranopie (Gr\xc3\xbcnsehschw\xc3\xa4\x63he)" : "Deuteranopia (Green Deficiency)";
					} else {
						profileName = isDe ? "Tritanopie (Blau-Gelb-Schw\xc3\xa4\x63he)" : "Tritanopia (Blue-Yellow Deficiency)";
					}

					char buf[64];
					std::snprintf(buf, sizeof(buf), isDe ? "Korrekturst\xc3\xa4rke: %.1f%%" : "Correction strength: %.1f%%", CurrentSettings.Severity01 * 100.0);
					severityDesc = buf;

					if (CurrentSettings.Severity01 <= 0.005) {
						clinicalGrade = isDe ? "Neutral (Filter inaktiv / 100% Originalfarben)" : "Neutral (Filter inactive / 100% original colors)";
						isNeutral = true;
					} else if (CurrentSettings.Severity01 <= 0.35) {
						clinicalGrade = isDe ? "Leichte Auspr\xc3\xa4gung (HRR / Farnsworth Mild)" : "Mild Expression (HRR / Farnsworth Mild)";
					} else if (CurrentSettings.Severity01 <= 0.70) {
						clinicalGrade = isDe ? "M\xc3\xa4\xc3\x9fige Auspr\xc3\xa4gung (HRR Moderate)" : "Moderate Expression (HRR Moderate)";
					} else {
						clinicalGrade = isDe ? "Starke Auspr\xc3\xa4gung / Anopie (HRR Severe)" : "Severe Expression / Anopia (HRR Severe)";
					}
				}

				// Render a clean, spacious, responsive feedback badge
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.18f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.38f, 0.58f, 0.65f));
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));

				float cardW = (availW < 480.0f) ? availW : 480.0f;
				if (ImGui::BeginChild("##status_feedback_card", ImVec2(cardW, 88.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
					// Line 1: Active Profile (clean bullet without broken unicode '?')
					ImGui::TextColored(ImVec4(0.95f, 0.95f, 1.0f, 1.0f), "- %s", profileName.c_str());
					
					// Line 2: Values & Numbers with plenty of space
					ImGui::Spacing();
					ImGui::TextColored(ImVec4(0.40f, 0.80f, 1.0f, 1.0f), "%s   %s", t.ValuesLabel, severityDesc.c_str());
					
					// Line 3: Clinical Classification
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
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// ── Window-mode status ────────────────────────────────────────────────
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

			// ── Hybrid Modus ──────────────────────────────────────────────────────
			ImGui::Spacing();
			if (ImGui::Checkbox(t.HybridMode, &CurrentSettings.EnableHybridMode)) {
				GetHybridScanner().SetEnabled(CurrentSettings.EnableHybridMode);
				changed = true;
				saveNeeded = true;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", t.HybridModeHelp);
			}

			// ── Entwickler- & Debug-Modus ─────────────────────────────────────────
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Checkbox(t.DebugModeCheckbox, &CurrentSettings.DebugMode)) {
				changed = true;
				saveNeeded = true;
			}

			// ── Unauffällige Keynotes / Referenzen (ganz unten, dezent gräulich) ──
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.52f, 0.58f, 0.70f));
			ImGui::TextUnformatted(t.MethodologyTitle);
			ImGui::TextWrapped("%s", t.MethodologyDesc);
			ImGui::PopStyleColor();

		}

		if (saveNeeded) {
			CurrentSettings.Save(AddonDir);
			Recompute(/*aForce=*/true);
		} else if (changed) {
			Recompute(/*aForce=*/false);
		}

		ImGui::PopStyleVar();
		ImGui::PopID();
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
		RenderCbaControls(/*isDetached=*/false);
	}

	void AddonRenderWindow()
	{
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
		if (CurrentSettings.EnableHybridMode && s_deferredInitDone.load())
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

		if (!CurrentSettings.DetachedWindow || !ImGui::GetCurrentContext()) return;

		float clampedOpacity = std::clamp(CurrentSettings.UiOpacity, 0.2f, 1.0f);
		ImGui::SetNextWindowBgAlpha(clampedOpacity);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.08f, 0.12f, clampedOpacity * 0.75f));

		if (s_resetDetachedWindowPos)
		{
			ImVec2 disp = ImGui::GetIO().DisplaySize;
			float w = 430.0f;
			float h = 390.0f;
			float posX = (disp.x > w) ? (disp.x - w) * 0.5f : 50.0f;
			float posY = (disp.y > h) ? (disp.y - h) * 0.5f : 50.0f;
			ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
			s_resetDetachedWindowPos = false;
		}
		else
		{
			ImGui::SetNextWindowSize(ImVec2(430.0f, 390.0f), ImGuiCond_FirstUseEver);
		}

		ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse;
		if (ImGui::Begin("cba4gw2###CBA_FloatingWindow", &CurrentSettings.DetachedWindow, winFlags))
		{
			RenderCbaControls(/*isDetached=*/true);
		}
		ImGui::End();
		ImGui::PopStyleColor();
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
			CurrentSettings = Settings::Load(AddonDir);

			// Initialize hybrid background scanner
			GetHybridScanner().Initialize();
			GetHybridScanner().SetEnabled(CurrentSettings.EnableHybridMode);

			// Startup policy: Filter only active on startup if LoadOnStartup is explicitly enabled.
			if (!CurrentSettings.LoadOnStartup)
			{
				CurrentSettings.Enabled = false;
			}

			// Language defaults to 0 (Auto/Sys) via Settings struct — no override needed.

			// Escape closes floating window
			if (APIDefs->UI.RegisterCloseOnEscape)
			{
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2###CBA_FloatingWindow", &CurrentSettings.DetachedWindow);
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

			// QuickAccess toolbar icon & window toggle keybind
			if (APIDefs->InputBinds.RegisterWithString)
			{
				APIDefs->InputBinds.RegisterWithString("KB_CBA_WINDOW", ProcessKeybind, "ALT+C");
			}
			if (APIDefs->Textures.GetOrCreateFromMemory)
			{
				APIDefs->Textures.GetOrCreateFromMemory("CBA_ICON", (void*)kCbaIconPng, kCbaIconPngSize);
			}
			if (APIDefs->QuickAccess.Add)
			{
				APIDefs->QuickAccess.Add("QA_CBA", "CBA_ICON", "CBA_ICON", "KB_CBA_WINDOW", "cba4gw2 (ALT+C)");
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
					APIDefs->InputBinds.Deregister("KB_CBA_WINDOW");
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
	AddonDef.Signature = -78341; // arbitrary negative ID — not on Raidcore (yet)
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "cba4gw2";
	AddonDef.Version.Major = 1;
	AddonDef.Version.Minor = 0;
	AddonDef.Version.Build = 1;
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
