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
	void RenderEmbeddedOptions()
	{
		bool isDe = (CurrentSettings.Language == 2);
		bool changed = false;

		ImGui::TextDisabled("%s", isDe ? "CBA: Color Balance Assist" : "CBA: Color Balance Assist");
		ImGui::Spacing();

		if (ImGui::Checkbox(isDe ? "Aktiviert##emb_main_toggle" : "Enabled##emb_main_toggle", &CurrentSettings.Enabled))
		{
			changed = true;
			if (!CurrentSettings.Enabled) {
				CurrentSettings.Mixed = false;
				CurrentSettings.ShowQuickAccessIcon = false;
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button, Theme::kBtnStateActiveIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::kBtnStateActivePress);
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::kTextCyanLicht);
		if (ImGui::Button(isDe ? "CBA Studio Oeffnen" : "Open CBA Studio", ImVec2(180.0f, 28.0f)))
		{
			CurrentSettings.ShowMainWindow = true;
		}
		ImGui::PopStyleColor(4);

		ImGui::Spacing();
		ImGui::TextDisabled("%s", isDe ? "Tipp: Studio kann auch ueber Keybind (Strg+O) geoeffnet werden." 
		                               : "Tip: Studio can also be opened via keybind (Ctrl+O).");

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
			const char* masterBtnLabel = isDe ? (wasEnabled ? "EIN##main_master" : "AUS##main_master")
			                                  : (wasEnabled ? "ON##main_master"  : "OFF##main_master");
			if (ImGui::Button(masterBtnLabel, ImVec2(0.0f, 24.0f))) {
				ToggleMasterEnabled();
				changed    = true;
				saveNeeded = false;
			}
			ImGui::PopStyleColor(4);

			// The "OS BLOCKIERT FILTER!" banner used to sit right here, and
			// it was wrong twice over (removed 2026-09-12, Emi's report).
			//
			// Wrong about the world: g_DwmLastCallSuccessful goes false
			// whenever GW2 is not the foreground window, because a background
			// process's MagSetFullscreenColorEffect call does not go through.
			// Alt-tab to a browser or open the snipping tool and the banner
			// appeared - while the already-installed colour effect kept
			// working perfectly. It accused the OS of blocking a filter that
			// was visibly running.
			//
			// Wrong about this window: it was a full-width BeginChild dropped
			// between the master button and the toolbar buttons that follow it
			// on the same row via SameLine(). The child ends the row, so every
			// one of those buttons - Sensor Graph, Filter Lab, Vision Lab,
			// Reset UI, Reset Filter, Export, Import, language - was laid out
			// past the right edge and vanished. The whole tab bar disappeared
			// exactly when someone alt-tabbed away to screenshot it. It also
			// stole the two ImGui "last item" queries below: the master
			// button's own tooltip, and the GetItemRectMin/Max that the
			// OFF-state glint animation traces, both read the banner's rect
			// instead of the button's whenever it showed. Removing it repairs
			// all three at once.
			//
			// The signal is not lost: SelfTest reports it as INFO, the right
			// surface for a fact that legitimately varies (CLAUDE.md, "Where a
			// fact belongs").
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(wasEnabled ? (isDe ? "Filter aktiv - Klicke zum Ausschalten" : "Filter active - click to disable")
				                             : (isDe ? "Filter inaktiv - Klicke zum Einschalten" : "Filter inactive - click to enable"));

			if (!wasEnabled)
			{
				// Two glints, starting top-center and bottom-center (exactly
				// half a perimeter apart on a rectangle) and rotating
				// continuously counter-clockwise (2026-09-10, Emi's redesign -
				// was a single dot ping-ponging back and forth; slower, calmer,
				// and easier to notice at a glance that the filter is OFF).
				ImVec2 bMin = ImGui::GetItemRectMin();
				ImVec2 bMax = ImGui::GetItemRectMax();
				float bw = bMax.x - bMin.x;
				float bh = bMax.y - bMin.y;
				float peri = 2.0f * (bw + bh);

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

				float timeVal = (float)ImGui::GetTime();
				float revolutionPeriod = 5.5f; // seconds per full lap - "langsam wandern"
				float u0Top = (bw * 0.5f) / peri; // top-center's position along the walk
				// The walk (top edge left->right, then down, then bottom
				// right->left, then up) traces clockwise on screen as u
				// increases - so decreasing u is counter-clockwise.
				float rotation = -(timeVal / revolutionPeriod);

				ImDrawList* dl = ImGui::GetWindowDrawList();
				// Elongated streak instead of a dot+crosshair sparkle
				// (2026-09-10, Emi's ask: "laengliche Glanzpunkte...wie eine
				// Lichtreflektion auf glaenzender Oberflaeche") - several
				// samples trailing behind the head, tapering in size and
				// alpha, bending naturally around the button's corners since
				// they're all sampled along the same perimeter-walk function
				// as the head. Classic "comet trail" specular-sweep look.
				auto drawGlint = [&](float uBase) {
					float u = uBase + rotation;
					const int kTrailSamples = 7;
					const float kTrailSpan = 0.028f; // how far back along the perimeter the streak reaches
					for (int i = kTrailSamples - 1; i >= 0; --i) {
						float t = (float)i / (float)(kTrailSamples - 1); // 0 = head, 1 = tail tip
						ImVec2 p = getPerimeterPoint(u + t * kTrailSpan);
						float taper = 1.0f - t;
						float radius = 1.0f + taper * 2.6f;
						int alpha = (int)(taper * taper * 220.0f);
						dl->AddCircleFilled(p, radius, IM_COL32(255, 225, 225, alpha));
					}
					ImVec2 head = getPerimeterPoint(u);
					dl->AddCircleFilled(head, 4.2f, IM_COL32(255, 90, 90, 45)); // soft glow
					dl->AddCircleFilled(head, 1.6f, IM_COL32(255, 255, 255, 255)); // hot core
				};
				drawGlint(u0Top);
				drawGlint(u0Top + 0.5f);
			}
		}

		ImGui::SameLine(0, 6.0f);
		DrawFilterStatusIndicator(false);

		ImGui::SameLine(0, 6.0f);
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
		if (ImGui::Button(isDe ? "Filter-Labor##main_top" : "Filter Lab##main_top", ImVec2(0.0f, 24.0f))) {
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
		if (ImGui::Button(isDe ? "UI zuruecksetzen##main" : "Reset UI##main", ImVec2(0.0f, 24.0f))) {
			ResetUiLayout();
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Setzt alle CBA-Fenster (Hauptfenster, Sensor-Graph, Filter-Labor) auf Standardposition links oben zurueck."
			                       : "Resets all CBA windows (Main Window, Sensor Graph, Filter Lab) to default top-left position.");
		}

		// The other of the two reset functions (window-layout reset above,
		// filter-state reset here) - replaces the old "Factory Reset" button,
		// which called a Settings::FactoryReset() that both preserved some
		// fields and reset others in ways nobody could fully account for.
		// This one is exactly ResetFilterSettingsAndDisable() - same function
		// the "CBA - Filter Off" keybind and the Sensor Graph HUD's Reset
		// button use, so there's one reset behavior, not three.
		ImGui::SameLine(0, 5.0f);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
		if (ImGui::Button(isDe ? "Filter zuruecksetzen##main" : "Reset Filter##main", ImVec2(0.0f, 24.0f))) {
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
		const char* curLangBtnText = (CurrentSettings.Language == 2) ? "Deutsch##main_top_lang" :
		                             (CurrentSettings.Language == 0) ? "System##main_top_lang" : "English##main_top_lang";
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

		ImGui::PopStyleVar(2);

		ImGui::Spacing();
		ImGui::TextDisabled("%s", isDe ? "Farb- & Kontrastanpassung fuer Barrierefreiheit in Guild Wars 2 (DWM / Live-Filter)"
		                               : "Accessible Color & Contrast Enhancer for Guild Wars 2 (DWM / Live Filter)");
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
		
		tabBtn(isDe ? "Dashboard" : "Dashboard", 0);
		ImGui::SameLine();
		tabBtn(isDe ? "Eye Comfort" : "Eye Comfort", 1);
		ImGui::SameLine();
		tabBtn(isDe ? "Vision Lab" : "Vision Lab", 2);
		ImGui::SameLine();
		tabBtn(isDe ? "System" : "System", 3);
		
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 285.0f);
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
		if (ImGui::Button(isDe ? "Sensor##tabbar_graph" : "Sensor##tabbar_graph", ImVec2(95.0f, 32.0f))) {
			CurrentSettings.ShowGraphWindow = !CurrentSettings.ShowGraphWindow;
			if (CurrentSettings.ShowGraphWindow) s_focusGraphWindow = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", t.OpenSensorGraphTooltip);
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", isDe ? "Backend:" : "Backend:");
		ImGui::SameLine();
		if (CurrentSettings.RenderBackend == 1 && GetShaderColorPipeline().IsReady()) {
			ImGui::TextColored(Theme::kTextCyanLicht, "Shader");
		} else if (CurrentSettings.RenderBackend == 0) {
			ImGui::TextColored(Theme::kTextGoldLabel, "DWM");
		} else {
			ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Inaktiv");
		}

		ImGui::EndGroup();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
		
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

		Texture* tex = nullptr;
		if (APIDefs && APIDefs->Textures.Get)
		{
			bool isActive = CurrentSettings.Enabled;
			tex = APIDefs->Textures.Get(isActive ? "CBA_ICON" : "CBA_ICON_INACTIVE");
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

			// Left Click: Toggle Main Window - respects the Advanced Mode
			// gate the same way the keybind does (2026-09-09): can always
			// close, can only open once unlocked.
			if (isClickedLeft && (CurrentSettings.AdvancedModeUnlocked || CurrentSettings.ShowMainWindow))
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
				Texture* hovTex = APIDefs ? APIDefs->Textures.Get("CBA_ICON_HOVER") : nullptr;
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
