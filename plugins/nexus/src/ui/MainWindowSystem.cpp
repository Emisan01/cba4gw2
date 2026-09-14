#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ui/MainWindow.h"
#include "ui/SensorGraphHUD.h"
#include "ui/UIState.h"
#include "ui/Theme.h"
#include "ui/L10n.h"
#include "ui/ImGuiSafe.h"

#include "core/NexusEcosystem.h"
#include <imgui.h>
#include "ColorMatrix.h"
#include "ColorMath.h"
#include "FilterLab.h"
#include "CreditsDialog.h"
#include "HybridScanner.h"
#include "VisionLab.h"
#include "WindowMode.h"
#include "ColorEffectController.h"
#include "ParameterRegistry.h"
#include "FeatureModule.h"
#include "FilterLayers.h"
#include "ShaderColorPipeline.h"
#include "SelfTest.h"
#include "Shared.h"

#include <imgui.h>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cfloat>

#include "MainWindowTabs.h"

namespace cba
{
	// ── One look for every block on the panel ─────────────────────────────
	// The heading + one-line "what this does for you" pair was written out by
	// hand per section, which is how two blocks doing the same job end up
	// looking like two different kinds of thing. Defined once as of
	// 2026-09-12 (Emi: "eine einheitliche UI-Darstellung insgesamt"), so a new
	// section cannot quietly adopt its own spacing or its own shade of cyan.
	//
	// The subtitle is not decoration: PRODUCT_CONCEPT.md's rule is that a
	// heading naming a mechanism ("Commander-Tag-Kontrast") tells a player
	// nothing about whether they want it, so every section owes one line
	// saying what it does FOR them. Making it a parameter makes that rule
	// hard to skip.
	void PanelSection(const char* aTitle, const char* aSubtitle)
	{
		ImGui::TextColored(Theme::kTextCyanLicht, "%s", aTitle);
		if (aSubtitle && *aSubtitle)
		{
			ImGui::TextDisabled("%s", aSubtitle);
		}
	}

	void RenderSystemTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded)
	{
		// ── Section 3: Spiel- & Fenstermodus ──────────────────────────────────
		{
			cba::ScopedChild tileSystem("Tile_System", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
			if (ImGui::BeginMenuBar()) { ImGui::TextColored(Theme::kTextCyanLicht, "System & Backend"); ImGui::EndMenuBar(); }

			// Reuses curWinMode from the fullscreen banner above rather than
			// querying again (2026-09-11): DetectWindowMode is an
			// IDXGISwapChain::GetFullscreenState() COM round-trip, and the
			// value cannot change within a single frame - so a second call
			// here was pure duplicated cost every frame this section stayed
			// expanded. The third call site (the diagnostics button) keeps its
			// own fresh query on purpose: it runs on click, not per frame, and
			// a snapshot report should read current truth.
			// Reported, not judged (2026-09-12). The window mode is worth
			// showing in a diagnostics section; telling the user to change it
			// is not, because the shader backend works in every mode.
			WindowMode curWinMode = DetectWindowMode(APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);
		WindowMode mode = curWinMode;
			ImGui::TextColored({0.4f,0.85f,0.4f,1.0f}, "%s: %s", t.WindowMode, ToDisplayString(mode, isDe));
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


			ImGui::Spacing();
			PanelSection(isDe ? "Oekosystem & Integration" : "Ecosystem & Integration",
				         isDe ? "Erkennung und Kompatibilitaet mit anderen Addons" : "Detection and compatibility with other addons");

			bool hasArc = cba::NexusEcosystem::Get().IsArcDPSLoaded();
			bool hasFastLoad = cba::NexusEcosystem::Get().IsFastLoadLoaded();

			// The "Synchronize ArcDPS Theme" checkbox this used to gate lived
			// here until 2026-09-15 - its own NexusEcosystem sync functions
			// were never called from anywhere (a leftover from an ArcDPS
			// ini-writer attempt disabled before it was ever finished), so
			// the checkbox toggled a setting nothing read. Detection stays,
			// the false promise doesn't.
			ImGui::TextColored(hasArc ? Theme::kTextCyanLicht : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
				hasArc ? (isDe ? "[ArcDPS] Aktiv" : "[ArcDPS] Active")
				       : (isDe ? "[ArcDPS] Nicht erkannt" : "[ArcDPS] Not detected"));

			ImGui::TextColored(hasFastLoad ? Theme::kTextCyanLicht : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
				hasFastLoad ? (isDe ? "[FastLoad] Aktiv: Lade-Safe-Start aktiviert" : "[FastLoad] Active: Loading Safe-Start activated")
				            : (isDe ? "[FastLoad] Nicht erkannt" : "[FastLoad] Not detected"));

			ImGui::Spacing();
			if (ImGui::Checkbox(isDe ? "Mini-HUD aktivieren (ArcDPS Style)" : "Enable Mini-HUD (ArcDPS Style)", &CurrentSettings.ShowMiniHUD))
			{
				changed = true;
				saveNeeded = true;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip(isDe ? "Zeigt ein minimalistisches schwebendes Info-HUD an, das sich am visuellen Stil von ArcDPS orientiert."
				                       : "Shows a minimalist floating info HUD styled to match the ArcDPS design language.");
			}
			
			if (CurrentSettings.ShowMiniHUD)
			{
				ImGui::Indent();
				if (ImGui::SliderFloat(isDe ? "Hintergrund-Transparenz" : "Background Opacity", &CurrentSettings.MiniHudBgAlpha, 0.0f, 1.0f, "%.2f")) { changed = true; saveNeeded = true; }
				if (ImGui::Checkbox(isDe ? "Titelleiste anzeigen" : "Show Title Bar", &CurrentSettings.MiniHudTitleBar)) { changed = true; saveNeeded = true; }
				if (ImGui::Checkbox(isDe ? "Rahmen anzeigen" : "Show Borders", &CurrentSettings.MiniHudBorders)) { changed = true; saveNeeded = true; }
				ImGui::Unindent();
			}

		}

		// ── Section 4: Hybrid Modus (Beta) ────────────────────────────────────
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
			
		}

		// ── Section 5: Filter-Labor & Experimentierfeld ──────────────────────
		{
			DrawFilterLabWidget(isDe, changed, saveNeeded);
			
		}

		// ── Section 6: About ──────────────────────────────────────────────────
		// Used to be "Ueber, Diagnose & Credits" - Self-Test and the
		// diagnostics-clipboard button moved to the Dashboard's own
		// Diagnose tile 2026-09-14 (Emi: those are checks a player runs to
		// see if something's wrong, not a dev-only feature - they belong
		// somewhere reachable without opening System > Debug Mode). What's
		// left here is genuinely "about the addon": identity, dev toggles,
		// methodology, credits - Emi's own framing, "wie macht Nexus selbst
		// das, wie macht arcdps das" (pick the best of both conventions).
		{
			PanelSection(isDe ? "Ueber CBA" : "About", nullptr);
			ImGui::Spacing();
			ImGui::TextColored(Theme::kTextSecondary, "%s",
				isDe ? "Emisan01 sagt: \"nothing will be done if you dont start\""
				     : "Emisan01 says: \"nothing will be done if you dont start\"");
			ImGui::Spacing();

			{
				AddonVersion ver = GetAddonVersion();
				ImGui::Text("cba4gw2  v%d.%d.%d.%d", ver.Major, ver.Minor, ver.Build, ver.Revision);
				ImGui::SameLine(0, 12.0f);
				ImGui::TextDisabled("Nexus API %d", NEXUS_API_VERSION);
				ImGui::TextDisabled("%s", isDe ? "von Emisan01" : "by Emisan01");
				ImGui::TextDisabled("%s", "Color and Contrast Balance Enhancer and Filter Lab for Guild Wars 2");

				ImGui::SetNextItemWidth(-1.0f);
				char repoBuf[64] = "https://github.com/Emisan01/cba4gw2";
				ImGui::InputText("##about_repo_link", repoBuf, sizeof(repoBuf), ImGuiInputTextFlags_ReadOnly);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", isDe ? "GitHub-Repository (zum Kopieren markieren)" : "GitHub repository (select to copy)");
				}

				// AGENTS.md Rule 4 requires this notice in the About dialog
				// (and the README, which already has it) - missing here
				// since the About section was first built, found during the
				// 2026-09-14 docs cleanup pass.
				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.60f, 0.68f, 0.90f));
				ImGui::TextWrapped("%s", isDe
					? "Drittanbieter-Addon, Nutzung auf eigenes Risiko gemaess ArenaNets Third-Party-Programs-Policy - keine Automatisierung, kein Zugriff auf Spielspeicher, rein visuell."
					: "Third-party addon, used at your own risk per ArenaNet's Third-Party Programs Policy - no automation, no game-memory access, visual-only.");
				ImGui::PopStyleColor();
			}
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Checkbox(t.DebugModeCheckbox, &CurrentSettings.DebugMode)) {
				changed = true;
				saveNeeded = true;
			}

			// Advanced UI Theme (2026-09-09, experimental) - palette-only,
			// opt-in, defaults to Classic (index 0) so nothing changes for
			// anyone who doesn't touch this. See Theme.cpp for what each
			// index actually looks like right now.
			{
				const char* themeNames[] = {
					isDe ? "Klassisch" : "Classic",
					isDe ? "Symbiont (experimentell)" : "Symbiont (experimental)"
				};
				ImGui::SetNextItemWidth(220.0f);
				int themeIdx = CurrentSettings.UiTheme;
				if (ImGui::Combo(isDe ? "UI-Thema (Advanced)##ui_theme" : "UI Theme (Advanced)##ui_theme", &themeIdx, themeNames, 2))
				{
					CurrentSettings.UiTheme = themeIdx;
					Theme::ApplyTheme(CurrentSettings.UiTheme);
					saveNeeded = true;
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe
						? "Experimentelles alternatives Farbschema fuer die CBA-Oberflaeche. Rein optisch, keine Funktionsaenderung - jederzeit reversibel."
						: "Experimental alternate color palette for the CBA UI. Purely visual, no functional change - fully reversible at any time.");
				}
			}

			if (CurrentSettings.DebugMode)
			{
				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.08f, 0.12f, 0.90f));
				ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.18f, 0.32f, 0.45f, 0.70f));
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
				float perfPanelH = ImGui::GetTextLineHeightWithSpacing() * 3.0f + 26.0f;
				if (ImGui::BeginChild("##debug_perf_panel", ImVec2(0.0f, perfPanelH), true, ImGuiWindowFlags_NoScrollbar))
				{
					ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "[*] Performance Watchdog (Fenster-Messwerte):" 
					                                                     : "[*] Performance Watchdog (Per-Window Metrics):");
					ImGui::Text(isDe ? "  Hauptfenster (Main):  %.2f ms" : "  Main Window (Main):   %.2f ms", g_perfMainWindowMs);
					ImGui::SameLine(0, 16.0f);
					ImGui::Text(isDe ? "  Sensor-Graph (HUD):   %.2f ms" : "  Sensor Graph (HUD):   %.2f ms", g_perfSensorGraphMs);
					ImGui::SameLine(0, 8.0f);
					ImGui::TextDisabled(isDe ? "(Kurven: %.2f ms)" : "(Curves: %.2f ms)", g_perfCurvesMs);
					ImGui::Text(isDe ? "  Filter-Labor (Lab):   %.2f ms" : "  Filter Lab (Lab):     %.2f ms", g_perfFilterLabMs);
					ImGui::SameLine(0, 16.0f);
					ImGui::Text(isDe ? "  Total ImGui CBA:      %.2f ms" : "  Total ImGui CBA:      %.2f ms", g_perfTotalImGuiMs);
					if (CurrentSettings.RenderBackend == 1)
					{
						ImGui::Text(isDe ? "  Farb-Pass (CPU):      %.3f ms" : "  Colour pass (CPU):    %.3f ms", g_perfShaderPassMs);
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("%s", isDe
								? "Zeit auf dem Render-Thread fuer Kopie und Draw des Farb-Passes.\nDie GPU-Zeit ist von hier aus nicht messbar - das hier ist, was der Pass das Spiel an CPU kostet."
								: "Render-thread time for the colour pass's copy and draw.\nGPU time is not visible from here - this is what the pass costs the game on the CPU.");
						}
					}
				}
				ImGui::EndChild();
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);
			}
			// Self-Test and the diagnostics-clipboard report moved to the
			// Dashboard's own Diagnose tile (2026-09-14) - see
			// MainWindowDashboard.cpp, RenderDashboardTab. No longer gated
			// behind Debug Mode there.

			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.52f, 0.58f, 0.80f));
			ImGui::TextUnformatted(t.MethodologyTitle);
			ImGui::TextWrapped("%s", t.MethodologyDesc);
			ImGui::PopStyleColor();
			ImGui::Spacing();

			ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.24f, 0.20f, 0.35f, 0.75f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.28f, 0.50f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.18f, 0.14f, 0.26f, 1.00f));
			if (ImGui::Button(t.CreditsBtn, ImVec2(0.0f, 24.0f)))
			{
				s_showC64Credits.store(true);
				StartC64Audio();
			}
			ImGui::PopStyleColor(3);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.CreditsTooltip);
			ImGui::Spacing();
		}

	}
}
