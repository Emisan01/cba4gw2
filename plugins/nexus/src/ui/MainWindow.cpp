#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MainWindow.h"
#include "UIState.h"
#include "Theme.h"
#include "L10n.h"
#include "ColorMatrix.h"
#include "ColorMath.h"
#include "SensorGraphHUD.h"
#include "FilterLab.h"
#include "CreditsDialog.h"
#include "HybridScanner.h"
#include "WindowMode.h"
#include "ColorEffectController.h"

#include <imgui.h>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include <cfloat>

namespace cba
{
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

		// ── Row 2: Reset & Recovery Actions ───
		ImGui::Spacing();

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
			GetColorEffectController().Clear();
			Recompute(/*aForce=*/true);
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt Farbkorrektur, Helligkeit und Commander-Tags komplett auf neutral (Filter AUS, Staerke 0, Gamma 1.00x)."
			                       : "Resets all color correction, brightness and tag settings to neutral defaults (Filter OFF, Severity 0, Gamma 1.00x).");
		}
		ImGui::PopStyleColor(4);

		ImGui::PopStyleVar(2);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(Theme::kTextCyanLicht, "Color Logic Balancer & Enhancer");
		ImGui::SameLine();
		ImGui::TextDisabled("(cba4gw2)");
		ImGui::TextColored(Theme::kTextSecondary, isDe ? "Barrierefreie Farb- & Kontrastoptimierung fuer Guild Wars 2"
		                                               : "Accessible color & contrast balancer for Guild Wars 2");
		ImGui::Spacing();

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

		// ── Fixed Top Header Bar ──────────────────────────────────────────────
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

			if (!wasEnabled)
			{
				ImVec2 bMin = ImGui::GetItemRectMin();
				ImVec2 bMax = ImGui::GetItemRectMax();
				float bw = bMax.x - bMin.x;
				float bh = bMax.y - bMin.y;
				float peri = 2.0f * (bw + bh);
				float timeVal = (float)ImGui::GetTime();
				float cyclePeriod = 3.6f;
				float cycleIdx = std::floor(timeVal / cyclePeriod);
				float cycleFrac = (timeVal - cycleIdx * cyclePeriod) / cyclePeriod;
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
				dl->AddCircleFilled(pt, 3.2f, IM_COL32(255, 60, 60, 65));
				dl->AddCircleFilled(ptTrail, 1.0f, IM_COL32(255, 110, 110, 110));
				dl->AddCircleFilled(pt, 1.3f, IM_COL32(255, 255, 255, 255));
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

		ImGui::PopStyleVar(2);

		ImGui::Spacing();
		ImGui::TextDisabled("%s", isDe ? "Farb- & Kontrastanpassung fuer Barrierefreiheit in Guild Wars 2 (DWM / Live-Filter)"
		                               : "Accessible Color & Contrast Enhancer for Guild Wars 2 (DWM / Live Filter)");
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ── Scrollable Body Content ──────────────────────────────────────────
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 4.0f));
		ImGui::BeginChild("##MainWindowScrollContent", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
		ImGui::PopStyleVar();

		static bool s_secOpen[8] = { true, true, false, false, false, false, false, false };

		auto renderSectionHeader = [&](int secIdx, const char* label, ImGuiTreeNodeFlags extraFlags = 0) -> bool {
			bool wasOpen = s_secOpen[secIdx];
			ImGuiTreeNodeFlags flags = extraFlags;
			if (wasOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

			ImGui::Spacing();

			bool isOpen = ImGui::CollapsingHeader(label, flags);
			if (ImGui::IsItemClicked()) {
				if (!wasOpen) {
					ImGui::SetScrollHereY(0.0f);
				}
			}
			s_secOpen[secIdx] = isOpen;
			return isOpen;
		};

		auto endSection = []() {
			ImGui::Spacing();
			ImGui::Dummy(ImVec2(0.0f, 12.0f));
		};

		// ── Section 1: Farbprofil & Korrektur ────────────────────────────────
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
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
				if (ImGui::Button("Reset##rg_det", ImVec2(btnW, 0.0f))) { 
					CurrentSettings.MixedRgSeverity01 = 0.0f; 
					changed = true; 
					saveNeeded = true; 
				}
				ImGui::PopStyleColor(4);
				
				ImGui::TextUnformatted(t.ByStrength);
				ImGui::SetNextItemWidth(sW);
				if (ImGui::SliderFloat("##by_det", &by, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
					CurrentSettings.MixedBySeverity01 = std::clamp(by, 0.0f, 1.25f);
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
				ImGui::SameLine(0, sp);
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
				if (ImGui::Button("Reset##by_det", ImVec2(btnW, 0.0f))) { 
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
				if (ImGui::SliderFloat("##sev_det", &sev, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
					CurrentSettings.Severity01 = std::clamp(sev, 0.0f, 1.25f);
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
				ImGui::SameLine(0, sp);
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
				if (ImGui::Button("Reset##sev_det", ImVec2(btnW, 0.0f))) { 
					CurrentSettings.Severity01 = 0.0f; 
					changed = true; 
					saveNeeded = true; 
				}
				ImGui::PopStyleColor(4);
			}

			ImGui::Spacing();
			ImGui::Separator();
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

			ImGui::TextDisabled("%s:", isDe ? "Profile" : "Profiles");
			ImGui::SameLine(0, 8.0f);

			ImVec4 btnCol, btnHover, btnActive, textCol;
			const char* saveTip = "";

			if (!isDirty) {
				btnCol    = Theme::kBtnNeutralIdle;
				btnHover  = Theme::kBtnNeutralHover;
				btnActive = Theme::kBtnNeutralPress;
				textCol   = Theme::kTextSecondary;
				saveTip   = isDe ? "Profil unveraendert / aktuell" : "Profile up to date (no unsaved changes)";
			} else if (usedCount < 3) {
				btnCol    = ImVec4(0.12f, 0.46f, 0.26f, 0.95f);
				btnHover  = ImVec4(0.16f, 0.58f, 0.34f, 1.00f);
				btnActive = ImVec4(0.09f, 0.36f, 0.20f, 1.00f);
				textCol   = Theme::GetContrastTextColor(btnCol);
				saveTip   = isDe ? "Einstellung geaendert! Klicke zum Speichern in freien Slot" : "Settings changed! Click to save to empty slot";
			} else {
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

		// ── Section 2: Commander-Tag Enhancer ────────────────────────────────
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

					dlTags->AddCircleFilled(center, circleRadius, col);

					if (conflict) {
						dlTags->AddCircle(center, circleRadius + 2.0f, IM_COL32(255, 80, 50, 240), 0, 2.0f);
						dlTags->AddCircleFilled(ImVec2(center.x + 8.0f, center.y - 7.0f), 3.5f, IM_COL32(255, 60, 50, 255));
					} else {
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

				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::TextDisabled("%s", isDe ? "Kurvenansicht (geladenes Preset / Farbprofil):" 
				                               : "Curve View (Loaded Preset / Color Profile):");
				ImGui::Spacing();

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
					CurrentSettings.Severity01 = 1.25f;
					CurrentSettings.CommanderTagMode = 1;
					CurrentSettings.SmartEnhancer = true;
					CurrentSettings.EnhancerTolerance = 0.14f;
					UpdateTagEnhancerConflicts();
					changed = true; saveNeeded = true;
				}
				ImGui::PopStyleColor(4);

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

		// ── Section 3: Kontrast-Kombinationen ─────────────────────────────────
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
				CurrentSettings.AutoBrightness = false;
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

		// ── Section 8: Über, Diagnose & Credits ──────────────────────────────
		if (renderSectionHeader(7, t.HeaderSection8))
		{
			if (ImGui::Checkbox(t.DebugModeCheckbox, &CurrentSettings.DebugMode)) {
				changed = true;
				saveNeeded = true;
			}

			if (CurrentSettings.DebugMode)
			{
				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.08f, 0.12f, 0.90f));
				ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.18f, 0.32f, 0.45f, 0.70f));
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
				if (ImGui::BeginChild("##debug_perf_panel", ImVec2(0.0f, 74.0f), true, ImGuiWindowFlags_NoScrollbar))
				{
					ImGui::TextColored(Theme::kTextCyanLicht, "[*] Performance Watchdog (Per-Window Metrics):");
					ImGui::Text("  Hauptfenster (Main):  %.2f ms", g_perfMainWindowMs);
					ImGui::SameLine(0, 16.0f);
					ImGui::Text("  Sensor-Graph (HUD):  %.2f ms", g_perfSensorGraphMs);
					ImGui::SameLine(0, 8.0f);
					ImGui::TextDisabled("(Kurven: %.2f ms)", g_perfCurvesMs);
					ImGui::Text("  Filter-Labor (Lab):   %.2f ms", g_perfFilterLabMs);
					ImGui::SameLine(0, 16.0f);
					ImGui::Text("  Total ImGui CBA:     %.2f ms", g_perfTotalImGuiMs);
					ImGui::EndChild();
				}
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);
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

		ImGui::EndChild();

		if (saveNeeded) {
			CurrentSettings.Save(AddonDir);
			Recompute(/*aForce=*/true);
		} else if (changed) {
			Recompute(/*aForce=*/false);
		}

		ImGui::PopID();
	}
}
