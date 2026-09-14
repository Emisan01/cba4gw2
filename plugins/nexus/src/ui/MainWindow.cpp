#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ui/MainWindow.h"
#include "ui/SensorGraphHUD.h"
#include "ui/UIState.h"
#include "ui/Theme.h"
#include "ui/L10n.h"
#include "ui/ImGuiSafe.h"
#include "ui/CbaIcon.h"

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
	void RenderEmbeddedOptions()
	{
		bool isDe = (CurrentSettings.Language == 2);
		bool changed = false;

		ImGui::TextDisabled("%s", isDe ? "CBA: Color Balance Assist" : "CBA: Color Balance Assist");
		ImGui::Spacing();

		// Reordered 2026-09-14 (Emi: panel felt buried, wanted the primary
		// action first) - Open CBA Studio at the very top, the panel's
		// three checkmarks (Enabled/Icon/Start-with-GW2) grouped right
		// below it, Reset UI/Reset Filter last since they are the
		// destructive pair, not the everyday ones.
		ImGui::PushStyleColor(ImGuiCol_Button, Theme::kBtnStateActiveIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::kBtnStateActivePress);
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::kTextCyanLicht);
		// Toggle, not open-only (2026-09-14, Emi: "der Button im Nexus
		// Main Window auch [bidirektional]" - matches the toolbar icon's
		// own click, which got the same change today).
		if (ImGui::Button(isDe ? "CBA Studio Oeffnen" : "Open CBA Studio", ImVec2(180.0f, 28.0f)))
		{
			CurrentSettings.ShowMainWindow = !CurrentSettings.ShowMainWindow;
		}
		ImGui::PopStyleColor(4);

		ImGui::Spacing();
		ImGui::TextDisabled("%s", isDe ? "Tipp: Studio kann auch ueber einen Keybind geoeffnet werden (einstellbar unter Nexus > Keybinds)."
		                               : "Tip: Studio can also be opened via a keybind (configurable under Nexus > Keybinds).");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Checkbox(isDe ? "Aktiviert##emb_main_toggle" : "Enabled##emb_main_toggle", &CurrentSettings.Enabled))
		{
			changed = true;
			if (!CurrentSettings.Enabled) {
				CurrentSettings.Mixed = false;
				CurrentSettings.ShowQuickAccessIcon = false;
			}
		}

		ImGui::SameLine(0, 16.0f);
		if (ImGui::Checkbox(isDe ? "Icon##emb_icon_toggle" : "Icon##emb_icon_toggle", &CurrentSettings.ShowQuickAccessIcon))
		{
			UpdateQuickAccessIcon();
			changed = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Zeigt ein Icon in Nexus' Quick-Access-Leiste zum Oeffnen des Studios."
			                       : "Shows an icon in Nexus's Quick Access bar to open the Studio.");
		}

		// Third checkmark alongside Enabled/Icon (2026-09-14, Emi: "die
		// 'active on startup' bitte noch dazu"). Was its own paragraph
		// with a full sentence before - DrawAutoStartControl now has a
		// short aCompact label specifically so it reads as one more
		// checkbox in this group, not a separate feature. Own line since
		// a third checkbox would not fit this panel's width on the same
		// row as the first two, but still visually grouped with them
		// (no separator between this and the pair above).
		DrawAutoStartControl(changed, isDe, /*aCompact=*/true);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Same two reset actions as the Studio's own header row - one
		// implementation each (ResetUiLayout / ResetFilterSettingsAndDisable),
		// so this embedded panel and the Studio can never drift into two
		// different ideas of what "reset" means (2026-09-13, Emi: "was noch
		// fehlt ist die Reset-Logik fuer UI und Filter").
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextPrimary);
		if (ImGui::Button(isDe ? "UI zuruecksetzen##emb" : "Reset UI##emb", ImVec2(0.0f, 24.0f))) {
			ResetUiLayout();
			changed = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Setzt alle CBA-Fenster (Hauptfenster, Sensor-Graph, Filter-Labor) auf Standardposition links oben zurueck."
			                       : "Resets all CBA windows (Main Window, Sensor Graph, Filter Lab) to default top-left position.");
		}

		ImGui::SameLine(0, 5.0f);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
		if (ImGui::Button(isDe ? "Filter zuruecksetzen##emb" : "Reset Filter##emb", ImVec2(0.0f, 24.0f))) {
			ResetFilterSettingsAndDisable();
			changed = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Setzt Farbprofil, Commander-Tag-Enhancer, Hybrid-Modus, Free Filter und Filter-Labor zurueck und schaltet den Filter aus."
			                       : "Resets color profile, Commander Tag Enhancer, Hybrid Mode, Free Filter and Filter Lab, and turns the filter off.");
		}

		if (changed) {
			Recompute(true);
			CurrentSettings.Save(AddonDir);
		}
	}

	void RenderMainWindow()
	{
		if (!ImGui::GetCurrentContext()) return;
		const L10n& t = Strings();
		bool changed = false;
		bool saveNeeded = false;
		bool isDe = cba::IsGerman(); // was a fragile first-letter check - see CLAUDE.md 2026-09-09

		ImGui::PushID("CBA_MainWindow");

		// Scoped, not global (2026-09-12). This block used to write straight
		// into ImGui::GetStyle(), which returns a reference to the ONE style
		// struct shared by everything drawing in Nexus's ImGui context - Nexus
		// itself, arcdps, every other addon. Those writes were never restored,
		// so from the first frame CBA's window rendered, our frame rounding,
		// our item spacing, our button text alignment and our blue on
		// CollapsingHeaders applied to every other addon for the rest of the
		// session.
		//
		// Nobody reported it because it looks like a theme rather than a bug.
		// It is still us redecorating someone else's house: we are a guest in
		// this context, and 179 other Push/Pop pairs in this file already get
		// that right. These eight lines were the leftovers.
		//
		// Popped at the end of the function, next to PopID. There is no early
		// return after this point - the only one is the context guard above -
		// so the pairing cannot be skipped.
		// ── Ocellus / TAC Theme Overrides ──────────────────────────────────────
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.06f, 0.09f, 0.95f)); // Darker, slightly blue-tinted background
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.15f, 0.20f, 0.28f, 1.00f));   // Subtle cyan-grey borders
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 0.60f));  // Slightly lighter tiles
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.12f, 0.18f, 0.26f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.28f, 0.38f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, Theme::kBtnStateActiveIdle);
		
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

		// ── Fixed Top Header Bar ──────────────────────────────────────────────
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(6.0f, 3.0f));

		// Master ON/OFF toggle button, round 3 (2026-09-14, Emi's reference
		// screenshots: a pill-shaped badge, dark background, coloured
		// border and text, small leading status dot - not a solid colour
		// fill like round 2. Wording stays "ON/OFF" ("EIN/AUS") - the
		// reference's own "Error"/"Connected" text was style-only
		// inspiration, not a literal label Emi asked for.
		{
			bool wasEnabled = CurrentSettings.Enabled;
			ImVec4 stateCol = wasEnabled ? ImVec4(0.35f, 0.85f, 0.45f, 1.00f) : ImVec4(0.92f, 0.32f, 0.32f, 1.00f);
			ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.06f, 0.08f, 0.10f, 0.92f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.13f, 0.16f, 0.95f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.03f, 0.04f, 0.05f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Text,          stateCol);
			ImGui::PushStyleColor(ImGuiCol_Border,        stateCol);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
			const char* masterBtnLabel = isDe ? (wasEnabled ? "EIN##main_master" : "AUS##main_master")
			                                  : (wasEnabled ? "ON##main_master"  : "OFF##main_master");
			bool masterClicked = ImGui::Button(masterBtnLabel, ImVec2(140.0f, 32.0f));
			// Status dot, drawn as a filled circle (Rule 7: ASCII-only
			// string literals rules out a unicode bullet glyph like the
			// reference's own "*") - same technique DrawFilterStatusIndicator
			// already uses, overlaid near the button's left edge.
			{
				ImVec2 bMin = ImGui::GetItemRectMin();
				ImVec2 bMax = ImGui::GetItemRectMax();
				ImVec2 dotCenter(bMin.x + 14.0f, (bMin.y + bMax.y) * 0.5f);
				ImGui::GetWindowDrawList()->AddCircleFilled(dotCenter, 3.5f, ImGui::ColorConvertFloat4ToU32(stateCol));
			}
			if (masterClicked) {
				ToggleMasterEnabled();
				changed    = true;
				saveNeeded = false;
			}
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(5);

			// The "OS BLOCKIERT FILTER!" banner used to sit right here, and
			// it was wrong twice over (removed 2026-09-12, Emi's report):
			// wrong about the world (g_DwmLastCallSuccessful goes false
			// whenever GW2 is not the foreground window, so alt-tabbing away
			// made it accuse the OS of blocking a filter that was visibly
			// still running) and wrong about this window (a full-width
			// BeginChild between the master button and the row of buttons
			// that follow it via SameLine() ended the row, pushing every one
			// of them past the right edge and out of view). The signal is
			// not lost: SelfTest reports it as INFO, the right surface for a
			// fact that legitimately varies (CLAUDE.md, "Where a fact
			// belongs").
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(wasEnabled ? (isDe ? "Filter aktiv - Klicke zum Ausschalten" : "Filter active - click to disable")
				                             : (isDe ? "Filter inaktiv - Klicke zum Einschalten" : "Filter inactive - click to enable"));
		}

		ImGui::SameLine(0, 6.0f);
		DrawFilterStatusIndicator(false);

		ImGui::PopStyleVar(2);

		// Row 2 + 3 (2026-09-14, second pass - Emi caught the first version
		// overlapping at narrow widths: "die reset buttons und english
		// dropdown... dann kommt der text, damit es sich einfach nicht
		// ueberlappt"). Two separate rows instead of one shared line with
		// right-align math: nothing computed against a shrinking width
		// means nothing left to overlap. The trio's total width stays
		// comfortably under the window's 480px minimum
		// (SetNextWindowSizeConstraints, ModuleMain.cpp) so it never needs
		// its own scrollbar; the tagline below wraps instead of clipping
		// if the window is narrower than its text.
		ImGui::Spacing();
		{
			const char* resetUiLabel  = isDe ? "UI zuruecksetzen##main" : "Reset UI##main";
			const char* resetFiltLabel = isDe ? "Filter zuruecksetzen##main" : "Reset Filter##main";
			const char* curLangBtnText = (CurrentSettings.Language == 2) ? "Deutsch##main_top_lang" :
			                             (CurrentSettings.Language == 0) ? "System##main_top_lang" : "English##main_top_lang";

			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextPrimary);
			if (ImGui::Button(resetUiLabel, ImVec2(0.0f, 24.0f))) {
				ResetUiLayout();
				saveNeeded = true;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip(isDe ? "Setzt alle CBA-Fenster (Hauptfenster, Sensor-Graph, Filter-Labor) auf Standardposition links oben zurueck."
				                       : "Resets all CBA windows (Main Window, Sensor Graph, Filter Lab) to default top-left position.");
			}

			// Same two reset actions as the embedded panel's own row - one
			// implementation each (ResetUiLayout / ResetFilterSettingsAndDisable).
			ImGui::SameLine(0, 5.0f);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
			if (ImGui::Button(resetFiltLabel, ImVec2(0.0f, 24.0f))) {
				ResetFilterSettingsAndDisable();
				changed = true;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip(isDe ? "Setzt Farbprofil, Commander-Tag-Enhancer, Hybrid-Modus, Free Filter und Filter-Labor zurueck und schaltet den Filter aus."
				                       : "Resets color profile, Commander Tag Enhancer, Hybrid Mode, Free Filter and Filter Lab, and turns the filter off.");
			}

			ImGui::SameLine(0, 5.0f);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextPrimary);
			if (ImGui::Button(curLangBtnText, ImVec2(0.0f, 24.0f))) {
				ImGui::OpenPopup("##LangSelectPopupTop");
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip(isDe ? "Sprache waehlen (English / Deutsch / System)"
				                       : "Select Language (English / Deutsch / System)");
			}

			if (ImGui::BeginPopup("##LangSelectPopupTop")) {
				int currentLang = CurrentSettings.Language;
				if (ImGui::Selectable("English", currentLang == 1)) {
					CurrentSettings.Language = 1;
					UpdateQuickAccessIcon();
					changed = true;
					saveNeeded = true;
				}
				if (ImGui::Selectable(isDe ? "System (Windows)" : "System (Windows)", currentLang == 0)) {
					CurrentSettings.Language = 0;
					UpdateQuickAccessIcon();
					changed = true;
					saveNeeded = true;
				}
				if (ImGui::Selectable("Deutsch", currentLang == 2)) {
					CurrentSettings.Language = 2;
					UpdateQuickAccessIcon();
					changed = true;
					saveNeeded = true;
				}
				ImGui::EndPopup();
			}
		}

		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
		ImGui::TextWrapped("%s", isDe ? "Automatischer visueller Enhancer, anpassbares Live-Filter-Labor & mehr!"
		                               : "Automatic Visual Enhancer, Adjustable Live Filter Lab & More!");
		ImGui::PopStyleColor();

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ── Scrollable Body Content ──────────────────────────────────────────
		
		
		// ── TAC Top-Tabs Layout ──────────────────────────────────────────
		static int s_ActiveTab = 0;
        
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.18f, 0.24f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::kBtnStateActiveIdle);

		// Six 140x32 buttons run 860px wide (6*140 + 5*4 spacing) - wider
		// than the window's 480px minimum, so dragging the window narrow
		// used to clip the trailing buttons clean off with nothing to
		// reach them by (Emi: "Buttons in der Breite verschwinden... eine
		// Scrollbar entwerfen"). The outer window can't grow its own
		// scrollbar for this (NoScrollbar by design - the one real
		// scrollbar is ##MainWindowScrollContent below, kept singular so
		// tiles never nest a second one), so this row gets its own
		// horizontal one instead: invisible at comfortable widths, a thin
		// scrollable strip instead of a cliff once it does not fit.
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		{
			cba::ScopedChild tabRowScroll("##HeaderTabRowScroll", ImVec2(0, 40.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::BeginGroup();
		auto tabBtn = [&](const char* label, int idx) {
			if (s_ActiveTab == idx) {
				ImGui::PushStyleColor(ImGuiCol_Button, Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_Text, Theme::kTextCyanLicht);
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.16f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			}
			if (ImGui::Button(label, ImVec2(140.0f, 32.0f))) s_ActiveTab = idx;
			ImGui::PopStyleColor(2);
		};
		
		// Filter Lab and Sensor Graph are window toggles, not tabs, but they
		// read as one family with the tabs around them - same two-color
		// scheme and 140x32 size as tabBtn above (2026-09-13, Emi: "genau so
		// designen wie links").
		auto toolWindowToggleBtn = [&](const char* aLabel, bool aOpen, const char* aTooltip) -> bool {
			if (aOpen) {
				ImGui::PushStyleColor(ImGuiCol_Button, Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_Text,   Theme::kTextCyanLicht);
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.16f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Text,   ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			}
			bool clicked = ImGui::Button(aLabel, ImVec2(140.0f, 32.0f));
			ImGui::PopStyleColor(2);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", aTooltip);
			return clicked;
		};

		// Row order (2026-09-14, Emi): Dashboard, Eye Comfort, Sensor Graph,
		// Vision Lab, Filter Lab, System - System moved all the way to the
		// right end on purpose. Tab indices (0/1/2/3) are unchanged, only
		// the draw order/position in the row moved.
		tabBtn(isDe ? "Dashboard" : "Dashboard", 0);
		ImGui::SameLine();
		tabBtn(isDe ? "Eye Comfort" : "Eye Comfort", 1);

		ImGui::SameLine();
		if (toolWindowToggleBtn(isDe ? "Sensor Graph##tabbar_graph" : "Sensor Graph##tabbar_graph", CurrentSettings.ShowGraphWindow,
			t.OpenSensorGraphTooltip))
		{
			CurrentSettings.ShowGraphWindow = !CurrentSettings.ShowGraphWindow;
			if (CurrentSettings.ShowGraphWindow) s_focusGraphWindow = true;
			saveNeeded = true;
		}

		ImGui::SameLine();
		tabBtn(isDe ? "Vision Lab" : "Vision Lab", 2);

		ImGui::SameLine();
		if (toolWindowToggleBtn(isDe ? "Filter-Labor##tabbar_lab" : "Filter Lab##tabbar_lab", CurrentSettings.ShowLabWindow,
			isDe ? "Filter-Labor als eigenes Fenster oeffnen oder schliessen" : "Open or close Filter Lab detached window"))
		{
			CurrentSettings.ShowLabWindow = !CurrentSettings.ShowLabWindow;
			if (CurrentSettings.ShowLabWindow) s_focusLabWindow = true;
			saveNeeded = true;
		}

		ImGui::SameLine();
		tabBtn(isDe ? "System" : "System", 3);

		ImGui::EndGroup();
		}
		ImGui::PopStyleColor();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);

		// "What's active" overview, its own row below the tabs/toggles with
		// a bit of breathing room (2026-09-14, Emi's screenshot feedback -
		// this used to live squeezed inline at the end of the button row
		// and wrapped awkwardly at normal window widths). Backend is folded
		// in as the tile's first line instead of a separate text run beside
		// it. Same chip row already shared between Sensor Graph HUD and the
		// Dashboard tab (RenderActiveModulesChips) - a third home for it,
		// not a new implementation. Sized for up to five lines: Backend
		// plus several modules active at once (Filter + Commander Tag +
		// Hybrid + Eye-Sensitive + Gamma) can wrap past two.
		ImGui::Spacing();
		{
			float tileW = ImGui::GetContentRegionAvail().x;
			if (tileW > 40.0f)
			{
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.13f, 0.19f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.22f, 0.34f, 0.50f, 0.55f));
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 5.0f));
				float tileH = ImGui::GetTextLineHeightWithSpacing() * 5.0f + 10.0f;
				if (ImGui::BeginChild("##HeaderActiveModulesTile", ImVec2(tileW, tileH), true))
				{
					ImGui::TextDisabled("%s", isDe ? "Backend:" : "Backend:");
					ImGui::SameLine();
					if (CurrentSettings.RenderBackend == 1 && GetShaderColorPipeline().IsReady()) {
						ImGui::TextColored(Theme::kTextCyanLicht, "Shader");
					} else if (CurrentSettings.RenderBackend == 0) {
						ImGui::TextColored(Theme::kTextGoldLabel, "DWM");
					} else {
						ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Inaktiv");
					}
					RenderActiveModulesChips(isDe);
				}
				ImGui::EndChild();
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);
			}
		}
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		// ── Content Area ──────────────────────────────────────────
		cba::ScopedChild scrollContent("##MainWindowScrollContent", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);


		if (s_ActiveTab == 2)
		{
			cba::RenderVisionLabContent(isDe);
		}

// ── Section 1: Farbprofil & Korrektur ────────────────────────────────
		if (s_ActiveTab == 0)
		{
			RenderDashboardTab(isDe, t, changed, saveNeeded);
		}


		// ── Section 2: Eye Comfort (Helligkeit) ──────────────────────────────
		if (s_ActiveTab == 1)
		{
			RenderEyeComfortTab(isDe, t, changed, saveNeeded);
		}


		// ── Section 3: Spiel- & Fenstermodus ──────────────────────────────────
		if (s_ActiveTab == 3)
		{
			RenderSystemTab(isDe, t, changed, saveNeeded);
		}


		if (saveNeeded) {
			CurrentSettings.Save(AddonDir);
			Recompute(/*aForce=*/true);
		} else if (changed) {
			Recompute(/*aForce=*/false);
		}

		// Pairs with the four PushStyleVar / three PushStyleColor at the top of
		// this function. Order does not matter to ImGui, but keeping them next
		// to PopID keeps the whole scope visible in one place.
		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(7);
		ImGui::PopID();
	}

	void RenderMovableToolbarIcon()
	{
		if (!ImGui::GetCurrentContext()) return;
		if (!CurrentSettings.ShowQuickAccessIcon) return;
		if (!CurrentSettings.MovableToolbarIcon) return;
		bool isDe = cba::IsGerman();

		Texture* tex = nullptr;
		if (APIDefs && APIDefs->Textures.Get)
		{
			bool isActive = CurrentSettings.Enabled;
			tex = APIDefs->Textures.Get(isActive ? CBA_ICON_NAME : CBA_ICON_INACTIVE_NAME);
		}
		if (!tex || !tex->Resource) return;

		const float iconDim = 32.0f;
		const float fixedRowY = 2.0f; // Adjusted to align better with Nexus QA bar

		ImGuiIO& io = ImGui::GetIO();
		// Clamp X to screen width so the icon can never get lost or pushed offscreen
		float maxW = (io.DisplaySize.x > 100.0f) ? (io.DisplaySize.x - iconDim - 4.0f) : 1920.0f;
		CurrentSettings.ToolbarIconPosX = std::clamp(CurrentSettings.ToolbarIconPosX, 0.0f, maxW);
		CurrentSettings.ToolbarIconPosY = fixedRowY;

		ImGui::SetNextWindowPos(ImVec2(CurrentSettings.ToolbarIconPosX, fixedRowY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(iconDim + 4.0f, iconDim + 4.0f));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
		                         ImGuiWindowFlags_NoBackground |
		                         ImGuiWindowFlags_NoScrollWithMouse |
		                         ImGuiWindowFlags_NoSavedSettings |
		                         ImGuiWindowFlags_AlwaysAutoResize |
		                         ImGuiWindowFlags_NoFocusOnAppearing;

		if (ImGui::Begin("##CBA_MovableToolbarIcon", nullptr, flags))
		{
			ImGui::InvisibleButton("##cba_tb_hit", ImVec2(iconDim, iconDim));
			bool isHovered = ImGui::IsItemHovered();
			bool isClickedRight = ImGui::IsItemClicked(ImGuiMouseButton_Right);

			// Left-drag: reposition along the row (2026-09-13, Emi: "einfach
			// das icon in der Zeile verschieben"). Y stays pinned to
			// fixedRowY above - this is horizontal repositioning only.
			// A static flag (not a local) because it has to survive across
			// the frames of one held-down drag, not just this single frame.
			static bool s_wasDragged = false;
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f))
			{
				CurrentSettings.ToolbarIconPosX += ImGui::GetIO().MouseDelta.x;
				s_wasDragged = true;
			}

			// Click fires on release, not press, so a click-then-drag never
			// also toggles the window on the way to becoming a drag.
			bool isClickedLeft = ImGui::IsItemDeactivated() && !s_wasDragged;
			if (ImGui::IsItemDeactivated())
			{
				if (s_wasDragged) CurrentSettings.Save(AddonDir); // deferred - once, on release
				s_wasDragged = false;
			}

			// Left Click: toggles the Main Window (2026-09-14, Emi: "das
			// Icon sollte bidirektional sein" - reverses the 2026-09-13
			// open-only decision quoted above; preferences change, this is
			// the current one). Was previously also gated behind
			// CurrentSettings.AdvancedModeUnlocked - a 2026-09-09 gate
			// nothing in the codebase ever set true; removed 2026-09-14
			// once found dead.
			if (isClickedLeft)
			{
				EnsureDeferredInitialized();
				CurrentSettings.ShowMainWindow = !CurrentSettings.ShowMainWindow;
				if (CurrentSettings.ShowMainWindow) s_focusMainWindow = true;
			}

			// Right Click: Master Filter Toggle - was a 4th inline copy of
			// the exact sequence already shared as ToggleMasterEnabled()
			// (found in the 2026-09-09 codebase review; two other call
			// sites already used the shared function).
			if (isClickedRight)
			{
				ToggleMasterEnabled();
			}

			if (isHovered)
			{
				// Hardcoded to the requested-at-registration combo (2026-09-13,
				// Emi: "das tooltip ueber dem icon soll nur sein ALT+STRG+C").
				// Nexus lets the user rebind "CBA - Main Window" freely in its
				// own Keybinds settings, so this can drift from whatever is
				// actually bound - same tradeoff already accepted for the
				// Studio's own tip text, just spelled out here instead of
				// left generic because Emi asked for the literal combo.
				ImGui::SetTooltip("%s", isDe ? "ALT+STRG+C" : "ALT+CTRL+C");
			}

			// Rendering the Icon Image
			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImVec2 pMin = ImGui::GetItemRectMin();
			ImVec2 pMax = ImGui::GetItemRectMax();

			void* drawSrv = tex->Resource;
			// Brightens on hover regardless of Active/Inactive now
			// (2026-09-10, "volle Integration in die Nexus Icon Familie") -
			// matches every native Nexus icon, which all brighten to the
			// same near-white tone on hover no matter their own state.
			if (isHovered)
			{
				Texture* hovTex = APIDefs ? APIDefs->Textures.Get(CBA_ICON_HOVER_NAME) : nullptr;
				if (hovTex && hovTex->Resource) drawSrv = hovTex->Resource;
			}

			dl->AddImage((ImTextureID)drawSrv, pMin, pMax);

			if (isHovered)
			{
				dl->AddRect(pMin, pMax, IM_COL32(230, 210, 120, 200), 4.0f, 0, 1.5f);
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}
}
