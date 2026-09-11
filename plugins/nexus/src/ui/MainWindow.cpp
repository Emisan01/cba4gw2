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
#include "ParameterRegistry.h"
#include "FeatureModule.h"
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

namespace cba
{
	void DrawContrastTestSwatches(bool isDe, const double aCorrMat[3][3], bool& saveNeeded)
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
			{ 0.212f, 0.439f, 0.800f,   0.247f, 0.616f, 0.302f },
			{ 0.851f, 0.275f, 0.235f,   0.247f, 0.616f, 0.302f },
			{ 0.910f, 0.753f, 0.125f,   0.212f, 0.439f, 0.800f },
			{ 0.149f, 0.682f, 0.741f,   0.212f, 0.439f, 0.800f },
			{ 0.910f, 0.522f, 0.059f,   0.851f, 0.275f, 0.235f }
		};

		int pIdx = std::clamp(CurrentSettings.ContrastPairIndex, 0, 4);

		ImGui::TextDisabled("%s:", isDe ? "Kontrast-Test Farbfelder" : "Contrast Test Swatches");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::Combo("##contrast_pair_combo", &pIdx, isDe ? pairNamesDe : pairNamesEn, 5))
		{
			CurrentSettings.ContrastPairIndex = pIdx;
			saveNeeded = true;
		}

		ColorPair p = kPairs[pIdx];

		double sim1R = p.r1, sim1G = p.g1, sim1B = p.b1;
		double sim2R = p.r2, sim2G = p.g2, sim2B = p.b2;
		ColorMatrix::SimulatePixel(p.r1, p.g1, p.b1, CurrentSettings.Type, sim1R, sim1G, sim1B);
		ColorMatrix::SimulatePixel(p.r2, p.g2, p.b2, CurrentSettings.Type, sim2R, sim2G, sim2B);

		float cor1R = (float)std::clamp(aCorrMat[0][0]*p.r1 + aCorrMat[0][1]*p.g1 + aCorrMat[0][2]*p.b1, 0.0, 1.0);
		float cor1G = (float)std::clamp(aCorrMat[1][0]*p.r1 + aCorrMat[1][1]*p.g1 + aCorrMat[1][2]*p.b1, 0.0, 1.0);
		float cor1B = (float)std::clamp(aCorrMat[2][0]*p.r1 + aCorrMat[2][1]*p.g1 + aCorrMat[2][2]*p.b1, 0.0, 1.0);

		float cor2R = (float)std::clamp(aCorrMat[0][0]*p.r2 + aCorrMat[0][1]*p.g2 + aCorrMat[0][2]*p.b2, 0.0, 1.0);
		float cor2G = (float)std::clamp(aCorrMat[1][0]*p.r2 + aCorrMat[1][1]*p.g2 + aCorrMat[1][2]*p.b2, 0.0, 1.0);
		float cor2B = (float)std::clamp(aCorrMat[2][0]*p.r2 + aCorrMat[2][1]*p.g2 + aCorrMat[2][2]*p.b2, 0.0, 1.0);

		ImGui::Spacing();
		float availW = ImGui::GetContentRegionAvail().x;
		float cardW = (availW - 12.0f) * 0.5f;
		if (cardW < 140.0f) cardW = availW;
		// Redesigned 2026-09-11 (Emi: "man sieht den Effekt gar nicht
		// wirklich") - two separate circles with a gap between them always
		// read as "two different colored things" regardless of how close
		// the colors actually are, since the GAP itself already visually
		// separates them - a sighted viewer's own normal color perception
		// does the rest of the work, defeating the point of simulating a
		// different one. Replaced with one continuous swatch split exactly
		// down the middle, no gap, no per-half border - only the OUTER
		// edge is framed. Whether the seam down the middle is visible now
		// depends entirely on the color difference, which is the actual
		// experience the "Without Filter" card is supposed to convey.
		float cardH = 162.0f;

		// Flat, no-card look (2026-09-11, superseding the same-day "Glass"
		// toggle below the same afternoon - Emi's live-testing diagnosis:
		// a near-transparent child still sits on top of THIS WINDOW's own
		// opaque background, so removing the card's own tint just revealed
		// a flatter black underneath, not the live game behind it - true
		// see-through would need the swatches drawn on the background draw
		// list instead of inside a window, a bigger architectural change.
		// The fix that actually works today, and Emi's own suggestion:
		// drop the card fill entirely and match Vision Lab's Anomaloscope
		// circle - plain shapes directly on the panel's own background,
		// exactly as transparent as the rest of this window already is, no
		// separate dark layer to fight with. Removed the now-pointless
		// toggle along with Settings.GlassContrastCards (a keyed field,
		// safe to drop outright - see CLAUDE.md on positional vs. keyed
		// serialization risk).
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));

		// Round 2 (Emi: "muss hübscher sein, sieht aus wie eine Creditcard") -
		// a flat rectangle split down the middle was too card-like. Two
		// large, generously overlapping circles (Venn-diagram style) read
		// as a proper "compare these" UI, not a swatch chip, and the
		// overlap lens itself becomes the test: circle 2 is painted on top
		// of circle 1, so the arc where it cuts across circle 1 is the only
		// visible seam - similar colors make that boundary nearly vanish,
		// distinct colors make it obvious. Bigger overlap = more of the
		// comparison happens in that one telling boundary, per Emi's ask.
		auto drawOverlapSwatch = [&](ImVec2 aCenter, float aRadius, float aOverlapFrac, ImU32 aColL, ImU32 aColR, ImU32 aFrameCol, float aFrameThick) {
			ImDrawList* dl = ImGui::GetWindowDrawList();
			float offset = aRadius * (1.0f - aOverlapFrac);
			ImVec2 c1 = ImVec2(aCenter.x - offset, aCenter.y);
			ImVec2 c2 = ImVec2(aCenter.x + offset, aCenter.y);
			dl->AddCircleFilled(c1, aRadius, aColL, 48);
			dl->AddCircleFilled(c2, aRadius, aColR, 48);
			dl->AddCircle(c1, aRadius, aFrameCol, 48, aFrameThick);
			dl->AddCircle(c2, aRadius, aFrameCol, 48, aFrameThick);
		};

		if (ImGui::BeginChild("##contrast_card_sim", ImVec2(cardW, cardH), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground))
		{
			ImGui::SetWindowFontScale(1.05f);
			ImGui::TextDisabled("%s", isDe ? "Ohne Filter (CVD)" : "Without Filter (CVD)");
			ImVec2 sp = ImGui::GetCursorScreenPos();

			float swW = ImGui::GetContentRegionAvail().x;
			float swH = 88.0f;
			float radius = std::min(44.0f, swW * 0.32f);
			ImU32 cSim1 = IM_COL32((int)(sim1R*255), (int)(sim1G*255), (int)(sim1B*255), 255);
			ImU32 cSim2 = IM_COL32((int)(sim2R*255), (int)(sim2G*255), (int)(sim2B*255), 255);
			drawOverlapSwatch(ImVec2(sp.x + swW * 0.5f, sp.y + swH * 0.5f), radius, 0.55f, cSim1, cSim2, IM_COL32(190, 190, 190, 150), 1.5f);

			ImGui::SetCursorScreenPos(ImVec2(sp.x, sp.y + swH + 10.0f));
			ImGui::SetNextItemWidth(swW);
			float textW = ImGui::CalcTextSize(isDe ? "Identisch / Verwechselbar" : "Identical / Confusable").x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (swW - textW) * 0.5f));
			ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "%s", isDe ? "Identisch / Verwechselbar" : "Identical / Confusable");
			ImGui::SetWindowFontScale(1.0f);
		}
		ImGui::EndChild();

		if (cardW < availW) ImGui::SameLine(0, 12.0f);
		else ImGui::Spacing();

		if (ImGui::BeginChild("##contrast_card_cba", ImVec2(cardW, cardH), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground))
		{
			ImGui::SetWindowFontScale(1.05f);
			ImGui::TextDisabled("%s", isDe ? "Mit CBA Filter (Boost)" : "With CBA Filter (Boost)");
			ImVec2 sp = ImGui::GetCursorScreenPos();

			float swW = ImGui::GetContentRegionAvail().x;
			float swH = 88.0f;
			float radius = std::min(44.0f, swW * 0.32f);
			ImU32 cCor1 = IM_COL32((int)(cor1R*255), (int)(cor1G*255), (int)(cor1B*255), 255);
			ImU32 cCor2 = IM_COL32((int)(cor2R*255), (int)(cor2G*255), (int)(cor2B*255), 255);
			drawOverlapSwatch(ImVec2(sp.x + swW * 0.5f, sp.y + swH * 0.5f), radius, 0.55f, cCor1, cCor2, IM_COL32(80, 240, 160, 220), 2.0f);

			ImGui::SetCursorScreenPos(ImVec2(sp.x, sp.y + swH + 10.0f));
			float textW = ImGui::CalcTextSize(isDe ? "Klar getrennt / verschieden!" : "Clearly separated!").x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (swW - textW) * 0.5f));
			ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.55f, 1.0f), "%s", isDe ? "Klar getrennt / verschieden!" : "Clearly separated!");
			ImGui::SetWindowFontScale(1.0f);
		}
		ImGui::EndChild();

		ImGui::PopStyleVar(1);
	}

	void RenderEmbeddedOptions()
	{
		if (!ImGui::GetCurrentContext()) return;
		const L10n& t = Strings();
		bool changed = false;
		bool saveNeeded = false;
		bool isDe = cba::IsGerman(); // was a fragile first-letter check - see CLAUDE.md 2026-09-09

		ImGui::PushID("CBA_Embedded");

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

		// One-line orientation for a brand-new user (2026-09-10, UI-weighting
		// pass) - the panel used to jump straight into controls (OFF/Inactive/
		// Advanced Mode) with no hint what the tool even does until the full
		// branding block at the very bottom. Kept to one muted line here on
		// purpose - the fuller branding text stays the footer, this is just
		// enough context to not be confusing on first sight.
		ImGui::TextColored(Theme::kTextSecondary, "%s", isDe
			? "Automatischer Farbkontrast-Ausgleich fuer Guild Wars 2"
			: "Automatic color & contrast correction for Guild Wars 2");
		ImGui::Spacing();

		// ── Row 1: Master ON/OFF + live status. Pushed to the very top
		// (2026-09-09, Emi's reorder request) - this is "the basic button,"
		// the single most fundamental control, so it's the first thing on
		// the page instead of being buried below the Commander Tag section.
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
			const char* masterOptLbl = isDe ? (wasEnabled ? "EIN##opt_master" : "AUS##opt_master")
			                                : (wasEnabled ? "ON##opt_master" : "OFF##opt_master");
			if (ImGui::Button(masterOptLbl, ImVec2(0.0f, 26.0f))) {
				ToggleMasterEnabled();
				changed    = true;
				saveNeeded = false;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(wasEnabled ? (isDe ? "Filter aktiv - Klicke zum Ausschalten" : "Filter active - click to disable")
				                             : (isDe ? "Filter inaktiv - Klicke zum Einschalten" : "Filter inactive - click to enable"));
		}

		ImGui::SameLine(0, 10.0f);
		// Deliberately kept as its own live indicator, not merged into the
		// OFF/ON button above (2026-09-09 UI review flagged this as
		// redundant-looking; Emi's call: keep it - this doubles as a
		// Nexus/Mumble handshake/connectivity check, not just a restatement
		// of Enabled, so it stays a separate signal).
		DrawFilterStatusIndicator(true);

		// Extend the "it's alive" handshake indicator with what's actually
		// active (2026-09-10, Emi: same idea as the toolbar icon only
		// lighting up when on - the status dot already says "alive," this
		// adds "and here's what's running"). Reuses the same
		// FeatureModuleRegistry loop the Sensor Graph HUD's "Aktiv:" list
		// already runs - Commander Tag/Hybrid Mode/Filter Lab/Eye-Sensitive
		// otherwise have no visibility at all on this compact panel.
		if (CurrentSettings.Enabled)
		{
			std::string activeList;
			for (const auto& module : FeatureModuleRegistry::Get().GetAll())
			{
				if (!module.isActive || !module.isActive()) continue;
				if (!activeList.empty()) activeList += ", ";
				activeList += isDe ? module.labelDe : module.labelEn;
			}
			if (!activeList.empty())
			{
				ImGui::SameLine(0, 6.0f);
				ImGui::TextDisabled("(%s)", activeList.c_str());
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe ? "Aktive Zusatzmodule (siehe auch Sensor-Graph-HUD fuer Details)."
					                       : "Active add-on modules (see the Sensor Graph HUD for details).");
				}
			}
		}

		// Advanced Mode gate + Studio entry, moved onto its own row
		// (2026-09-10, UI-weighting pass) - used to sit crammed onto the same
		// line as the master ON/OFF button, reading like a sub-option of it
		// even though it's a completely different concept ("unlock the
		// Studio" vs. "filter on/off"). Muted/secondary styling on purpose -
		// this is a setup choice, not the panel's primary action.
		ImGui::Spacing();
		{
			ImGui::PushStyleColor(ImGuiCol_Text, Theme::kTextSecondary);
			if (ImGui::Checkbox(isDe ? "Advanced Mode##adv_gate" : "Advanced Mode##adv_gate", &CurrentSettings.AdvancedModeUnlocked))
			{
				saveNeeded = true;
			}
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe ? "Schaltet das Studio frei (Hauptfenster, Sensor-Graph, Filter-Labor, Vision-Lab). Ohne Advanced Mode ist dieses kompakte Panel die gesamte Oberflaeche."
				                       : "Unlocks the Studio (Main Window, Sensor Graph, Filter Lab, Vision Lab). Without Advanced Mode, this compact panel is the entire interface.");
			}

			if (CurrentSettings.AdvancedModeUnlocked)
			{
				ImGui::SameLine(0, 10.0f);
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
				// Bidirectional (2026-09-10, Emi: "sollte auch das Fenster
				// wieder schliessen koennen") - used to only ever open the
				// Main Window; now toggles, same as the keybind/toolbar-icon
				// click already do.
				bool studioOpen = CurrentSettings.ShowMainWindow;
				if (ImGui::SmallButton(studioOpen ? (isDe ? "Studio schliessen" : "Close Studio")
				                                   : (isDe ? "Studio oeffnen ->" : "Open Studio ->")))
				{
					if (!studioOpen) EnsureDeferredInitialized();
					CurrentSettings.ShowMainWindow = !studioOpen;
					if (CurrentSettings.ShowMainWindow) s_focusMainWindow = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(studioOpen
						? (isDe ? "Schliesst das Hauptfenster (Studio)." : "Closes the Main Window (Studio).")
						: (isDe ? "Oeffnet die vollstaendige CBA-Oberflaeche (Hauptfenster, Sensor-Graph, Filter-Labor, Vision-Lab) fuer Feineinstellungen."
						        : "Opens the full CBA interface (Main Window, Sensor Graph, Filter Lab, Vision Lab) for fine-tuning."));
				}
			}
		}

		// Profile Slots - quick switch between saved profiles without opening
		// the Studio. Deliberately independent of Main Window's slot-chip loop
		// (which tracks "changed since load" via its own static baseline
		// variables local to that function) - this is just Save + Load, no
		// dirty-tracking, kept simple since the embedded panel has no "Profile
		// Summary" display to keep in sync with. The Auto-Start-on-launch
		// checkbox lives only in the full Main Window (needs a saved slot to
		// attach to and is a one-time setup action, not a per-session one).
		ImGui::Spacing();
		{
			ImGui::TextDisabled("%s:", isDe ? "Profile" : "Profiles");
			ImGui::SameLine(0, 8.0f);
			if (ImGui::SmallButton(isDe ? "Speichern##emb_save" : "Save##emb_save")) {
				int firstEmpty = -1;
				for (int i = 0; i < 3; ++i) if (!CurrentSettings.Slots[i].Used) { firstEmpty = i; break; }
				int targetSlot = (firstEmpty != -1) ? firstEmpty : ParameterRegistry::Get().GetInt(ParamId::ActiveSlotIdx);
				CurrentSettings.Slots[targetSlot].Used = true;
				if (CurrentSettings.Slots[targetSlot].Name.empty()) {
					const char* tn = CurrentSettings.Mixed ? (isDe ? "Gemischt" : "Mixed") :
						(CurrentSettings.Type == BalanceType::Protan) ? "Protan" :
						(CurrentSettings.Type == BalanceType::Deutan) ? "Deutan" : "Tritan";
					char defaultName[64];
					std::snprintf(defaultName, sizeof(defaultName), "Slot %d (%s)", targetSlot + 1, tn);
					CurrentSettings.Slots[targetSlot].Name = defaultName;
				}
				CurrentSettings.Slots[targetSlot].Type = CurrentSettings.Type;
				CurrentSettings.Slots[targetSlot].Severity01 = CurrentSettings.Severity01;
				CurrentSettings.Slots[targetSlot].Mixed = CurrentSettings.Mixed;
				CurrentSettings.Slots[targetSlot].MixedRg01 = CurrentSettings.MixedRgSeverity01;
				CurrentSettings.Slots[targetSlot].MixedBy01 = CurrentSettings.MixedBySeverity01;
				CurrentSettings.Slots[targetSlot].GammaGain = CurrentSettings.GammaGain;
				ParameterRegistry::Get().SetInt(ParamId::ActiveSlotIdx, targetSlot);
				saveNeeded = true;
			}
			for (int sIdx = 0; sIdx < 3; ++sIdx) {
				ImGui::SameLine(0, 4.0f);
				ImGui::PushID(sIdx + 700);
				bool used = CurrentSettings.Slots[sIdx].Used;
				if (used) {
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
				char chip[16];
				std::snprintf(chip, sizeof(chip), "[%d]", sIdx + 1);
				bool chipClicked = ImGui::SmallButton(chip);
				// Right-click toggles this slot as the Auto-Start profile -
				// deliberately not a checkbox per slot (Emi: "ohne 3 Checkboxen"),
				// this keeps the compact panel clutter-free while still reachable
				// without opening the Studio.
				bool autoStartToggled = used && ImGui::IsItemClicked(ImGuiMouseButton_Right);
				if (chipClicked && used) {
					ParameterRegistry::Get().SetInt(ParamId::ActiveSlotIdx, sIdx);
					CurrentSettings.Type = CurrentSettings.Slots[sIdx].Type;
					CurrentSettings.Severity01 = CurrentSettings.Slots[sIdx].Severity01;
					CurrentSettings.Mixed = CurrentSettings.Slots[sIdx].Mixed;
					CurrentSettings.MixedRgSeverity01 = CurrentSettings.Slots[sIdx].MixedRg01;
					CurrentSettings.MixedBySeverity01 = CurrentSettings.Slots[sIdx].MixedBy01;
					CurrentSettings.GammaGain = CurrentSettings.Slots[sIdx].GammaGain;
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (autoStartToggled) {
					CurrentSettings.AutoStartSlot = (CurrentSettings.AutoStartSlot == sIdx) ? -1 : sIdx;
					saveNeeded = true;
				}
				if (ImGui::IsItemHovered()) {
					if (used) ImGui::SetTooltip("Slot %d: %s\n%s\n%s", sIdx + 1, CurrentSettings.Slots[sIdx].Name.c_str(),
						isDe ? "Klicken zum Laden" : "Click to load",
						isDe ? "Rechtsklick: Auto-Start an/aus" : "Right-click: toggle Auto-Start");
					else ImGui::SetTooltip("Slot %d: %s", sIdx + 1, isDe ? "Frei" : "Empty");
				}
				if (sIdx == CurrentSettings.AutoStartSlot) {
					ImGui::SameLine(0, 2.0f);
					ImGui::TextColored(Theme::kTextGoldLabel, "*");
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", isDe ? "Auto-Start-Profil (Rechtsklick zum Entfernen)" : "Auto-Start profile (right-click to remove)");
				}
				ImGui::PopID();
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ── Row 2: Commander Tag Contrast - the 1-click core feature. Emi's
		// spec (2026-09-09): put the simple base logic for automatic commander
		// tag contrast directly on this page; the full multi-window UI becomes
		// an opt-in "Studio" button instead of the default view.
		ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Commander-Tag-Kontrast" : "Commander Tag Contrast");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "1-Klick: Waehlt dein Farbprofil und schaltet den automatischen Kontrast-Verstaerker fuer Commander-Tags ein."
			                       : "One click: picks your color profile and turns on the automatic contrast enhancer for commander tags.");
		}

		{
			float avail = ImGui::GetContentRegionAvail().x;
			float btnW = (avail - 8.0f) / 3.0f;

			auto quickProfileBtn = [&](const char* aName, BalanceType aType) {
				bool active = (CurrentSettings.CommanderTagMode != 0 && !CurrentSettings.Mixed && CurrentSettings.Type == aType);
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
				if (ImGui::Button(aName, ImVec2(btnW, 28.0f))) {
					ActivateCommanderTagProfile(aType);
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
			};

			quickProfileBtn(isDe ? "Protan (Rot)" : "Protan (Red)", BalanceType::Protan);
			ImGui::SameLine(0, 4.0f);
			quickProfileBtn(isDe ? "Deutan (Gruen)" : "Deutan (Green)", BalanceType::Deutan);
			ImGui::SameLine(0, 4.0f);
			quickProfileBtn(isDe ? "Tritan (Blau)" : "Tritan (Blue)", BalanceType::Tritan);
		}

		{
			bool enhancerActive = (CurrentSettings.CommanderTagMode != 0);
			int shiftedCount = 0;
			for (int i = 0; i < 9; ++i) if (s_tagConflictStates[i].inConflict) shiftedCount++;
			if (enhancerActive)
				ImGui::TextColored(Theme::kTextGoldLabel, isDe ? "Aktiv - %d von 9 Farben verschoben" : "Active - %d of 9 colors shifted", shiftedCount);
			else
				// Was "waehle oben ein Profil"/"pick a profile above" - "Profil"
				// already means something else on this panel (the Save/[1][2][3]
				// slots above), a naming collision found in the 2026-09-09 UI
				// review. These buttons pick a CVD type, not a profile.
				ImGui::TextDisabled("%s", isDe ? "Inaktiv - waehle oben einen Typ" : "Inactive - pick a type above");
			if (enhancerActive) {
				ImGui::SameLine(0, 10.0f);
				if (ImGui::SmallButton(isDe ? "Aus##cmdr_quick_off" : "Off##cmdr_quick_off")) {
					CurrentSettings.CommanderTagMode = 0;
					UpdateTagEnhancerConflicts();
					Recompute(/*aForce=*/true);
					changed = true;
					saveNeeded = true;
				}
			}
		}

		if (CurrentSettings.CommanderTagMode != 0)
		{
			// Compact "which color becomes which" row (2026-09-11, Emi's ask -
			// inspired by Vision Lab's flat circle style, no card/box): all 9
			// Commander Tag reference colors at a glance, each a tiny
			// two-circle overlap swatch (original vs. the color the enhancer
			// actually emits). Reuses s_tagConflictStates[i].rep* - the exact
			// color the highlighter overlay draws, already computed above by
			// UpdateTagEnhancerConflicts(); for tags with no conflict, rep*
			// equals the original, so the two circles fully coincide and the
			// pair just reads as one plain dot - no separate branch needed
			// for "safe" vs. "shifted" tags, the visualization does the work.
			ImGui::Spacing();
			ImGui::TextDisabled("%s", isDe ? "Original -> Kontrastfarbe (alle 9 Tags):" : "Original -> contrast color (all 9 tags):");
			float availMini = ImGui::GetContentRegionAvail().x;
			float miniR = std::clamp(availMini / 9.0f * 0.28f, 6.0f, 9.0f);
			float offset = miniR * 0.5f;
			float cellW = 2.0f * (miniR + offset);
			ImDrawList* dlMini = ImGui::GetWindowDrawList();
			for (int i = 0; i < 9; ++i)
			{
				if (i > 0) ImGui::SameLine(0, 4.0f);
				ImVec2 mp = ImGui::GetCursorScreenPos();
				ImVec2 center(mp.x + cellW * 0.5f, mp.y + miniR);
				ImU32 colOrig = IM_COL32((int)(kGw2TagRefs[i].r * 255), (int)(kGw2TagRefs[i].g * 255), (int)(kGw2TagRefs[i].b * 255), 255);
				float repR = std::clamp(s_tagConflictStates[i].repR, 0.0f, 1.0f);
				float repG = std::clamp(s_tagConflictStates[i].repG, 0.0f, 1.0f);
				float repB = std::clamp(s_tagConflictStates[i].repB, 0.0f, 1.0f);
				ImU32 colRep = IM_COL32((int)(repR * 255), (int)(repG * 255), (int)(repB * 255), 255);
				dlMini->AddCircleFilled(ImVec2(center.x - offset, center.y), miniR, colOrig, 20);
				dlMini->AddCircleFilled(ImVec2(center.x + offset, center.y), miniR, colRep, 20);
				bool miniConflict = s_tagConflictStates[i].inConflict;
				if (miniConflict)
					dlMini->AddCircle(ImVec2(center.x + offset, center.y), miniR, IM_COL32(80, 240, 160, 220), 20, 1.5f);
				ImGui::Dummy(ImVec2(cellW, miniR * 2.0f));
				if (ImGui::IsItemHovered())
				{
					const char* tagLabel = kGw2TagRefs[i].labelFunc(t);
					if (miniConflict)
						ImGui::SetTooltip(isDe ? "%s: Original -> Kontrastfarbe (verschoben)" : "%s: Original -> contrast color (shifted)", tagLabel);
					else
						ImGui::SetTooltip(isDe ? "%s: Bereits klar erkennbar (unveraendert)" : "%s: Already clearly distinct (unchanged)", tagLabel);
				}
			}
		}

		// Contrast Test Swatches moved into the "Advanced" section below
		// (2026-09-10, UI-weighting pass) - at full size (enlarged
		// 2026-09-09) this comparison competed with Commander Tag Contrast,
		// the panel's actual headline feature, for visual weight in the
		// default view. The "Active - N of 9 colors shifted" status line
		// above already gives a returning user a live functioning-proof;
		// the fuller before/after comparison is still one click away for
		// anyone who wants to see it, just not fighting for space by default.

		// ── Row 3: Reset & Recovery Actions ───
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextPrimary);
		if (ImGui::Button(isDe ? "UI zuruecksetzen" : "Reset UI", ImVec2(0.0f, 26.0f)))
		{
			ResetUiLayout();
			// Only force-open the Main Window if Advanced Mode actually
			// allows it (2026-09-09) - otherwise this button would silently
			// bypass the Advanced Mode gate right next to it.
			if (CurrentSettings.AdvancedModeUnlocked)
			{
				CurrentSettings.ShowMainWindow = true;
				s_focusMainWindow = true;
			}
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt Position und Groesse aller CBA-Fenster (Hauptfenster, Sensor-Graph, Filter-Labor, Vision-Lab) und des Taskleisten-Symbols auf Standardkoordinaten links oben zurueck."
			                       : "Resets position and size of all CBA windows (Main Window, Sensor Graph, Filter Lab, Vision Lab) and the toolbar icon to default top-left coordinates.");
		}

		ImGui::PopStyleColor(4);

		ImGui::SameLine(0, 8.0f);

		// Visually distinct from "Reset UI" above (2026-09-09, Emi's UI
		// walkthrough - the two used to look identical, easy to misclick).
		// Same danger-subtle tint the Main Window's own Reset Filter button
		// already used - this one just never got it.
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
		if (ImGui::Button(isDe ? "Filter zuruecksetzen" : "Reset Filter", ImVec2(0.0f, 26.0f)))
		{
			// Was a third, incomplete hand-rolled reset (missed
			// LabModeEnabled/EnableHybridMode - same bug class as the "CBA -
			// Filter Off" keybind before it used this shared function too).
			ResetFilterSettingsAndDisable();
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt Farbkorrektur, Helligkeit und Commander-Tags komplett auf neutral (Filter AUS, Staerke 0, Gamma 1.00x)."
			                       : "Resets all color correction, brightness and tag settings to neutral defaults (Filter OFF, Severity 0, Gamma 1.00x).");
		}
		ImGui::PopStyleColor(4);



		// Export/Import as one visible text field (2026-09-10, Emi's
		// rethink) - used to be two buttons that silently talked to the OS
		// clipboard with nothing shown on screen, only interesting once
		// profile slots existed to share. Now: "Generate" fills this field
		// (and still copies to clipboard, for anyone who prefers that flow)
		// so the code is actually visible and selectable for manual
		// copy-paste (e.g. into Discord), and the SAME field accepts a
		// pasted-in code for "Import" - one field, both directions, instead
		// of two opaque one-way buttons.
		ImGui::Spacing();
		ImGui::TextDisabled("%s:", isDe ? "Profil-Code (Export/Import)" : "Profile Code (Export/Import)");
		{
			static char s_presetIoBuf[256] = "";
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::InputText("##emb_preset_io", s_presetIoBuf, sizeof(s_presetIoBuf));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe ? "Zeigt den generierten Profil-Code zum Kopieren, oder fuege hier einen erhaltenen Code ein und klicke Import."
				                       : "Shows the generated profile code for copying, or paste in a received code and click Import.");
			}

			float ioAvail = ImGui::GetContentRegionAvail().x;
			float ioBtnW = (ioAvail - 8.0f) * 0.5f;
			if (ImGui::Button(isDe ? "Generieren" : "Generate", ImVec2(ioBtnW, 0.0f)))
			{
				std::string presetStr = CurrentSettings.ExportPresetString();
				std::snprintf(s_presetIoBuf, sizeof(s_presetIoBuf), "%s", presetStr.c_str());
				ImGui::SetClipboardText(presetStr.c_str());
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe ? "Erzeugt den Code fuer dein aktuelles Profil (auch in die Zwischenablage kopiert)."
				                       : "Generates the code for your current profile (also copied to clipboard).");
			}
			ImGui::SameLine(0, 8.0f);
			if (ImGui::Button(isDe ? "Import" : "Import", ImVec2(ioBtnW, 0.0f)))
			{
				std::string err;
				if (CurrentSettings.ImportPresetString(s_presetIoBuf, &err))
				{
					CurrentSettings.Save(AddonDir);
					GetColorEffectController().Clear();
					Recompute(/*aForce=*/true);
					saveNeeded = true;
				}
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe ? "Uebernimmt den Code oben ins aktuelle Profil (ueberschreibt aktuelle Einstellungen)."
				                       : "Applies the code above into the current profile (overwrites current settings).");
			}
		}

		ImGui::PopStyleVar(2);

		// Advanced (2026-09-09, Emi's UI walkthrough): these are setup-once
		// settings, not quick-access content - a first-time user doesn't
		// need toolbar X-position tuning in their first 10 seconds. Used to
		// sit inline in the main flow, right where the branding block used
		// to interrupt it too (see the footer below).
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		if (ImGui::TreeNodeEx(isDe ? "Erweitert##emb_advanced" : "Advanced##emb_advanced", ImGuiTreeNodeFlags_None))
		{
			// Contrast Test Swatches (2026-09-10, moved here from the main
			// flow - see the comment further up) - the practical "does this
			// actually help" before/after comparison, without the spectral
			// Curve View graph (Emi's original spec: swatches belong on this
			// panel, the technical curve chart stays in the Studio).
			{
				double embCorrMat[3][3];
				if (CurrentSettings.Mixed)
					ColorMatrix::MixedCorrectionMatrix(CurrentSettings.MixedRgSeverity01, CurrentSettings.MixedBySeverity01, embCorrMat);
				else
					ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, embCorrMat);
				DrawContrastTestSwatches(isDe, embCorrMat, saveNeeded);
			}
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Checkbox(t.ShowQuickAccess, &CurrentSettings.ShowQuickAccessIcon)) {
				UpdateQuickAccessIcon();
				saveNeeded = true;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.ShowQuickAccessTooltip);

			if (CurrentSettings.ShowQuickAccessIcon) {
				ImGui::Indent(16.0f);
				if (ImGui::Checkbox(isDe ? "Eigenes Toolbar-Icon erzwingen##movable_opt"
				                         : "Force custom toolbar icon##movable_opt", &CurrentSettings.MovableToolbarIcon)) {
					UpdateQuickAccessIcon();
					saveNeeded = true;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip(isDe ? "Zeigt ein unabhaengiges Icon anstelle des statischen Nexus-Icons."
					                       : "Shows an independent icon instead of the static Nexus icon.");
				}

				if (CurrentSettings.MovableToolbarIcon) {
					// Own row, not fused onto the checkbox's line via SameLine
					// (2026-09-10, Emi noticed the merge while looking for
					// this setting) - the checkbox is "force custom icon on/
					// off," the slider is a separate "where" concern once
					// that's on.
					ImGui::SetNextItemWidth(120.0f);
					if (ImGui::SliderFloat("X-Position", &CurrentSettings.ToolbarIconPosX, 0.0f, 2500.0f, "%.0f")) {
						saveNeeded = true;
					}
					ImGui::SameLine(0, 10.0f);
					if (ImGui::Button(isDe ? "Reset##rst_pos_opt" : "Reset##rst_pos_opt", ImVec2(0.0f, 0.0f))) {
						CurrentSettings.ToolbarIconPosX = 405.0f;
						saveNeeded = true;
					}
				}
				ImGui::Unindent(16.0f);
			}

			if (ImGui::Checkbox(t.KeepActiveBackground, &CurrentSettings.SystemWide)) {
				changed = true;
				saveNeeded = true;
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.KeepActiveBackgroundTooltip);
			ImGui::TreePop();
		}

		// Branding footer - used to sit mid-flow, between Export/Import and
		// the Advanced toggles above, interrupting the task flow like an ad.
		// Moved to the very end 2026-09-09 (Emi's UI walkthrough).
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextColored(Theme::kTextCyanLicht, "Color Logic Balancer & Enhancer");
		ImGui::SameLine();
		ImGui::TextDisabled("(cba4gw2)");
		ImGui::TextColored(Theme::kTextSecondary, isDe ? "Barrierefreie Farb- & Kontrastoptimierung fuer Guild Wars 2"
		                                               : "Accessible color & contrast balancer for Guild Wars 2");

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
		bool isDe = cba::IsGerman(); // was a fragile first-letter check - see CLAUDE.md 2026-09-09

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
			const char* masterBtnLabel = isDe ? (wasEnabled ? "EIN##main_master" : "AUS##main_master")
			                                  : (wasEnabled ? "ON##main_master"  : "OFF##main_master");
			if (ImGui::Button(masterBtnLabel, ImVec2(0.0f, 24.0f))) {
				ToggleMasterEnabled();
				changed    = true;
				saveNeeded = false;
			}
			ImGui::PopStyleColor(4);

			// ── OS Block Warning Banner ─────────────────────────────────────────
			if (CurrentSettings.Enabled && !g_DwmLastCallSuccessful) {
				ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.0f, 0.0f, 1.0f));
				ImGui::BeginChild("ErrorBanner", ImVec2(0, 40), true);
				// Was a raw German-only string literal, no isDe ternary at
				// all - the one string in this file that skipped translation,
				// found right when an English user most needs to understand
				// an error (2026-09-09 codebase review).
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", isDe
					? " OS BLOCKIERT FILTER! Bitte GW2 auf 'Windowed Fullscreen' stellen\n"
					  " oder HDR/Windows-Farbfilter in den OS-Einstellungen deaktivieren."
					: " OS IS BLOCKING THE FILTER! Please switch GW2 to 'Windowed Fullscreen'\n"
					  " or disable HDR/Windows color filters in your OS settings.");
				ImGui::EndChild();
				ImGui::PopStyleColor();
			}
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
		if (ImGui::Button(isDe ? "Sensor-Graph##main_top" : "Sensor Graph##main_top", ImVec2(0.0f, 24.0f))) {
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
		bool visionOpen = CurrentSettings.ShowVisionLabWindow;
		if (visionOpen) {
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
		if (ImGui::Button(isDe ? "Vision-Lab##main_top" : "Vision Lab##main_top", ImVec2(0.0f, 24.0f))) {
			CurrentSettings.ShowVisionLabWindow = !CurrentSettings.ShowVisionLabWindow;
			if (CurrentSettings.ShowVisionLabWindow) s_focusVisionLabWindow = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Vision-Lab (Klinischer Farbtest, Anomaloskop & GW2-Praxistest) oeffnen" 
			                       : "Open Vision Lab (Clinical color tests, Anomaloscope & GW2 usability test bench)");
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
			ImGui::SetTooltip(isDe ? "Setzt alle CBA-Fenster (Hauptfenster, Sensor-Graph, Filter-Labor, Vision-Lab) auf Standardposition links oben zurueck."
			                       : "Resets all CBA windows (Main Window, Sensor Graph, Filter Lab, Vision Lab) to default top-left position.");
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
		if (ImGui::Button(isDe ? "Export" : "Export", ImVec2(0.0f, 24.0f))) {
			std::string presetStr = CurrentSettings.ExportPresetString();
			ImGui::SetClipboardText(presetStr.c_str());
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Aktuelles Profil in die Zwischenablage kopieren" : "Copy current profile to clipboard");
		}

		ImGui::SameLine(0, 5.0f);
		if (ImGui::Button(isDe ? "Import" : "Import", ImVec2(0.0f, 24.0f))) {
			const char* clip = ImGui::GetClipboardText();
			if (clip) {
				std::string err;
				if (CurrentSettings.ImportPresetString(clip, &err)) {
					CurrentSettings.Save(AddonDir);
					GetColorEffectController().Clear();
					Recompute(/*aForce=*/true);
					saveNeeded = true;
				}
			}
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(isDe ? "Profil aus der Zwischenablage laden" : "Load profile from clipboard");
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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 4.0f));
		ImGui::BeginChild("##MainWindowScrollContent", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
		ImGui::PopStyleVar();

		static bool s_secOpen[8] = { true, false, false, false, false, false, false, false };

		auto renderSectionHeader = [&](int secIdx, const char* label, ImGuiTreeNodeFlags extraFlags = 0) -> bool {
			bool wasOpen = s_secOpen[secIdx];
			ImGuiTreeNodeFlags flags = extraFlags;
			if (wasOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

			ImGui::Spacing();

			bool isOpen = ImGui::CollapsingHeader(label, flags);
			s_secOpen[secIdx] = isOpen;
			return isOpen;
		};

		auto endSection = []() {
			ImGui::Spacing();
			ImGui::Dummy(ImVec2(0.0f, 12.0f));
		};

		// ── Exclusive Fullscreen Warning Banner ──────────────────────────────
		WindowMode curWinMode = DetectWindowMode(APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);
		if (curWinMode == WindowMode::ExclusiveFullscreen)
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.35f, 0.20f, 0.05f, 0.85f));
			ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.95f, 0.65f, 0.20f, 0.90f));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
			if (ImGui::BeginChild("##excl_fullscreen_warning", ImVec2(0.0f, 46.0f), true, ImGuiWindowFlags_NoScrollbar))
			{
				ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.35f, 1.0f), "%s", isDe ? "[WARNUNG] GW2 laeuft im exklusiven Vollbildmodus!" : "[WARNING] GW2 is running in exclusive fullscreen!");
				ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe 
					? "DWM-Filter pausiert. Bitte in den GW2-Optionen auf 'Fenster' oder 'Rahmenlos' stellen."
					: "DWM filter paused. Please set GW2 Graphics Options to 'Windowed' or 'Borderless'.");
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(2);
			ImGui::Spacing();
		}

		// ── Live Game Mode Context Indicator ──────────────────────────────────
		MumbleGameContext gameCtx = GetCurrentGameContext();
		if (gameCtx.mapId != 0)
		{
			ImGui::TextColored(Theme::kTextCyanLicht, "%s: %s (Map %u)%s",
				isDe ? "Aktiver Spielmodus" : "Active Game Mode",
				isDe ? gameCtx.modeNameDe : gameCtx.modeNameEn,
				gameCtx.mapId,
				gameCtx.isInCombat ? (isDe ? " [Im Kampf]" : " [In Combat]") : "");
			if (gameCtx.isWvW && CurrentSettings.CommanderTagMode == 0)
			{
				ImGui::TextColored(Theme::kTextGoldLabel, "%s", isDe 
					? "[Tipp] WvW erkannt! Der Commander-Tag Enhancer in Sektion 1 wird empfohlen."
					: "[Tip] WvW detected! Commander-Tag Enhancer in Section 1 is recommended.");
			}
			ImGui::Spacing();
		}

		// ── Section 1: Farbprofil & Korrektur ────────────────────────────────
		if (renderSectionHeader(0, t.HeaderSection1, ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Base profile (Type/Mixed/Severity/RG/BY) is now read-only here
			// (2026-09-11, "ein Zuhause pro Einstellung" - see CLAUDE.md's
			// UI-restructure entry). It used to be a full duplicate editor
			// of the exact same radios+sliders the Nexus-embedded panel
			// already owns (a third copy also lives in Sensor Graph HUD) -
			// three different widgets for the same value, easy to lose
			// track of which one you last touched. Studio is the diagnostic/
			// power-user surface now; the embedded panel is the only editor.
			const char* activeTypeLabel = CurrentSettings.Mixed
				? (isDe ? "Gemischt" : "Mixed")
				: (CurrentSettings.Type == BalanceType::Protan ? t.Protan
					: CurrentSettings.Type == BalanceType::Deutan ? t.Deutan : t.Tritan);
			if (CurrentSettings.Mixed) {
				ImGui::Text("%s: %s  (%s %.0f%% / %s %.0f%%)",
					isDe ? "Aktives Profil" : "Active Profile", activeTypeLabel,
					t.RgStrength, CurrentSettings.MixedRgSeverity01 * 100.0f,
					t.ByStrength, CurrentSettings.MixedBySeverity01 * 100.0f);
			} else {
				ImGui::Text("%s: %s  (%s %.0f%%)",
					isDe ? "Aktives Profil" : "Active Profile", activeTypeLabel,
					t.Strength, CurrentSettings.Severity01 * 100.0f);
			}
			ImGui::TextDisabled("%s", isDe ? "Bearbeiten: Nexus-Panel (Optionen -> cba4gw2)"
			                               : "Edit in: Nexus Panel (Options -> cba4gw2)");

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// ── Commander-Tag & Contrast Enhancer Block ─────────────────────
			// The on/off toggle + 3 profile-select buttons removed here
			// 2026-09-11 ("ein Zuhause pro Einstellung") were an exact
			// duplicate of the Nexus-embedded panel's "Commander Tag
			// Contrast" quick-select buttons (both ultimately call
			// ActivateCommanderTagProfile()) - kept only what's genuinely
			// unique to Studio: saving the current profile into the named
			// slot bank. The unused "Load on startup" width calculation
			// that used to sit alongside this (computed, never actually
			// rendered as a checkbox) was dead code, removed with it.
			bool enhancerActive = (CurrentSettings.CommanderTagMode != 0);
			int shiftedCount = 0;
			for (int i = 0; i < 9; ++i) {
				if (s_tagConflictStates[i].inConflict) shiftedCount++;
			}
			const char* curDefName = CurrentSettings.Type == BalanceType::Protan ? (isDe ? "Protan (Rot)" : "Protan (Red)")
				: (CurrentSettings.Type == BalanceType::Deutan ? (isDe ? "Deutan (Gruen)" : "Deutan (Green)") : (isDe ? "Tritan (Blau)" : "Tritan (Blue)"));
			if (enhancerActive)
				ImGui::TextColored(Theme::kTextGoldLabel, isDe ? "Com-Tag-Kontrast: %s - %d von 9 Farben verschoben" : "Com-Tag Contrast: %s - %d of 9 colors shifted", curDefName, shiftedCount);
			else
				ImGui::TextDisabled("%s", isDe ? "Com-Tag-Kontrast: Inaktiv (im Nexus-Panel einschalten)" : "Com-Tag Contrast: Inactive (enable in Nexus Panel)");

			if (enhancerActive)
			{
				ImGui::Spacing();
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
				if (ImGui::Button(isDe ? "Aktuelles Profil in Profilbank speichern##save_com_bank" : "Save current profile to profile bank##save_com_bank"))
				{
					EnsureDeferredInitialized();
					int targetSlot = (CurrentSettings.Type == BalanceType::Protan) ? 0 :
					                 (CurrentSettings.Type == BalanceType::Deutan) ? 1 : 2;
					CurrentSettings.Slots[targetSlot].Used = true;
					// Only auto-generate a name if the slot doesn't
					// already have one - previously overwrote
					// unconditionally, so a user's manually-renamed slot
					// ("WvW Raid Preset") could silently lose its name
					// the next time this quick-save button was clicked
					// (found in the 2026-09-09 codebase review), matching
					// the .Name.empty() check every other slot-save path
					// in this file already uses.
					if (CurrentSettings.Slots[targetSlot].Name.empty())
					{
						char buf[64];
						std::snprintf(buf, sizeof(buf), "Com-Tag %s", (CurrentSettings.Type == BalanceType::Protan) ? "Protan" :
						                                              (CurrentSettings.Type == BalanceType::Deutan) ? "Deutan" : "Tritan");
						CurrentSettings.Slots[targetSlot].Name = buf;
					}
					CurrentSettings.Slots[targetSlot].Type = CurrentSettings.Type;
					CurrentSettings.Slots[targetSlot].Severity01 = CurrentSettings.Severity01;
					CurrentSettings.Slots[targetSlot].Mixed = false;
					CurrentSettings.Slots[targetSlot].GammaGain = CurrentSettings.GammaGain;

					CurrentSettings.Save(AddonDir);
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe ? "Speichert das aktuell gesetzte Com-Tag Profil dauerhaft in der Profilspeicherbank"
					                       : "Saves the currently configured Com-Tag profile to the profile bank");
				}

				ImGui::PopStyleVar();
			}

			// Always visible from here on (not gated behind Auto Com-Tag anymore).
			// The Curve View further below used to be accidentally nested
			// inside `if (enhancerActive)` too - a coupling bug from an
			// earlier reorg, not intentional (Emi flagged this).
			ImGui::Spacing();
			float availSliders = ImGui::GetContentRegionAvail().x;

			// Brightness/Eye Comfort Gamma used to be duplicated here AND in
			// Section 2 - two sliders bound to the same value, in two
			// different places. Removed from here entirely 2026-09-09 (Emi's
			// UI walkthrough); Section 2 "Eye Comfort" is its one home now,
			// it has the fuller picture anyway (Retention/HDR/Apply-Target).
			//
			// Tolerance and the AQ/HRR reference field also moved, into the
			// collapsed "Advanced" group below - Section 1 was doing too much
			// for a first impression (everything at the same visual weight,
			// no core/advanced distinction). Not the full Core/Advanced UI
			// split from CLAUDE.md step 2 (that's a bigger, separate pass) -
			// just decluttering this one section's obvious overflow.
			if (ImGui::TreeNodeEx(isDe ? "Erweitert##sec1_advanced" : "Advanced##sec1_advanced", ImGuiTreeNodeFlags_None))
			{
				ImGui::TextUnformatted(isDe ? "Toleranz (Erkennungsradius):" : "Tolerance (Detection Radius):");
				ImGui::SetNextItemWidth(availSliders);
				// Registry-backed pilot (CLAUDE.md, Registry/Control Layer step 1).
				// The clamp now lives in ParamMeta (ParameterRegistry::SetFloat),
				// not just in this widget's min/max args - fixes the slider being
				// overridable via ImGui's CTRL-click-to-type text entry, which the
				// previous rescale attempts did not address.
				{
					float tol = ParameterRegistry::Get().GetFloat(ParamId::EnhancerTolerance);
					if (ImGui::SliderFloat("##enhancer_tol_det", &tol, 0.04f, 0.20f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
						ParameterRegistry::Get().SetFloat(ParamId::EnhancerTolerance, tol);
						UpdateTagEnhancerConflicts();
						changed = true;
					}
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::Spacing();
				const char* defaultHintSec1 = CurrentSettings.Mixed ? "RG: 50% | BY: 50%" :
					(CurrentSettings.Type == BalanceType::Protan ? "AQ: 0.35 | HRR: 8/10" :
					(CurrentSettings.Type == BalanceType::Deutan ? "AQ: 3.20 | HRR: 8/10" : "Moreland: 1.15 | HRR: 6/10"));

				ImGui::TextDisabled("%s:", isDe ? "Referenzwerte / Kalibrierung (AQ / HRR)" : "Reference Values / Calibration (AQ / HRR)");
				char diagBufSec1[128]{};
				std::snprintf(diagBufSec1, sizeof(diagBufSec1), "%s", CurrentSettings.DiagnosisHint.c_str());

				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::InputTextWithHint("##ref_values_input", defaultHintSec1, diagBufSec1, sizeof(diagBufSec1)))
				{
					CurrentSettings.DiagnosisHint = diagBufSec1;
					changed = true;
					saveNeeded = true;
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe
						? "Optionales Eingabefeld fuer persoenliche Kalibrier- oder Benchmarkwerte (z.B. Nagel-AQ, HRR-Plates).\nTypische Standardwerte fuer dieses Profil: %s"
						: "Optional input field for personal calibration or test benchmark scores (e.g. Nagel AQ, HRR plates).\nTypical default values for this profile: %s",
						defaultHintSec1);
				}
				ImGui::TreePop();
			}

			// "Intensity Scale (Compensation)" used to live here as a second
			// slider bound to the exact same CurrentSettings.Severity01 as the
			// "Strength" slider above (just a different range/display format) -
			// removed as a duplicate rather than relocated, per Emi.

			if (enhancerActive)
			{
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
			}

			// Curve View is unconditional from here (see note above) - it shows
			// the currently active Type/Severity/Mixed correction regardless of
			// whether Auto Com-Tag happens to be on.
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
					if (ImGui::Button(aName, ImVec2(0.0f, 22.0f))) {
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
				ImGui::Separator();
				ImGui::Spacing();

				DrawContrastTestSwatches(isDe, mainCorrMat, saveNeeded);

				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnDangerSubtleIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnDangerSubtleHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnDangerSubtlePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextDangerSubtle);
				if (ImGui::Button(isDe ? "Reset auf Neutral##det" : "Reset to Neutral##det", ImVec2(0.0f, 24.0f)))
				{
					EnsureDeferredInitialized();
					CurrentSettings.CommanderTagMode = 0;
					CurrentSettings.EnhancerTolerance = 0.12f;
					UpdateTagEnhancerConflicts();
					Recompute(/*aForce=*/true);
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe ? "Setzt Commander Tag Enhancer auf Inaktiv / Neutral zurueck" : "Resets Commander Tag Enhancer to Off / Neutral");
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

			if (ImGui::Button(isDe ? "Speichern##tiny_prof" : "Save##tiny_prof", ImVec2(0.0f, 22.0f)))
			{
				int targetSlot = (firstEmptySlot != -1) ? firstEmptySlot : ParameterRegistry::Get().GetInt(ParamId::ActiveSlotIdx);
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
				ParameterRegistry::Get().SetInt(ParamId::ActiveSlotIdx, targetSlot);

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
				bool isActive = (ParameterRegistry::Get().GetInt(ParamId::ActiveSlotIdx) == sIdx);

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
					ParameterRegistry::Get().SetInt(ParamId::ActiveSlotIdx, sIdx);
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
					if (ImGui::Button(isDe ? "Laden" : "Load", ImVec2(0.0f, 0.0f))) {
						ParameterRegistry::Get().SetInt(ParamId::ActiveSlotIdx, sIdx);
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
					float xBtnW = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.0f + 6.0f;
					if (ImGui::Button("X##clr_slot", ImVec2(xBtnW, 0.0f))) {
						CurrentSettings.Slots[sIdx].Used = false;
						CurrentSettings.Slots[sIdx].Name = "";
						if (CurrentSettings.AutoStartSlot == sIdx) CurrentSettings.AutoStartSlot = -1;
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);
					if (ImGui::IsItemHovered()) ImGui::SetTooltip(isDe ? "Slot leeren" : "Clear slot");

					ImGui::SameLine(0, 8.0f);
					bool isAutoStart = (CurrentSettings.AutoStartSlot == sIdx);
					if (ImGui::Checkbox(isDe ? "Auto-Start##autostart_slot" : "Auto-Start##autostart_slot", &isAutoStart)) {
						CurrentSettings.AutoStartSlot = isAutoStart ? sIdx : -1;
						saveNeeded = true;
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip(isDe ? "Laedt dieses Profil automatisch und schaltet den Filter ein, sobald GW2 startet (nicht nach einem Absturz)."
						                       : "Automatically loads this profile and turns the filter on whenever GW2 starts (skipped after a crash).");
					}
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

				float feedbackCardH = ImGui::GetTextLineHeightWithSpacing() * 2.0f + 24.0f;
				if (ImGui::BeginChild("##status_feedback_card_main", ImVec2(0.0f, feedbackCardH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
					ImGui::TextColored(ImVec4(0.95f, 0.95f, 1.0f, 1.0f), "- %s - %s", profileName.c_str(), severityDesc.c_str());
					ImGui::Spacing();
					ImGui::TextColored(
						!isNeutral ? ImVec4(0.35f, 0.95f, 0.55f, 1.0f) : ImVec4(0.65f, 0.72f, 0.82f, 0.90f),
						"%s %s",
						t.ClassificationLabel,
						clinicalGrade.c_str()
					);
				}
				ImGui::EndChild();
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);

				// Reference Values / Calibration (AQ/HRR) used to live here -
				// moved into the "Advanced" collapsed group near the top of
				// this section 2026-09-09 (Emi's UI walkthrough), it's a
				// personal-notes field, not a core everyday control.

				// Preset Exchange (Clipboard)
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Profil-Austausch (Zwischenablage):" : "Profile Exchange (Clipboard):");
				ImGui::Spacing();

				if (ImGui::Button(isDe ? "Profil in Zwischenablage kopieren##exp" : "Copy profile to clipboard##exp", ImVec2(0.0f, 24.0f)))
				{
					std::string expStr = CurrentSettings.ExportPresetString();
					ImGui::SetClipboardText(expStr.c_str());
					s_profileFeedbackTime = std::chrono::steady_clock::now();
					s_profileFeedbackMsg = isDe ? "[OK] Profil in Zwischenablage kopiert!" : "[OK] Profile copied to clipboard!";
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe 
						? "Kopiert dein aktuelles Farbprofil als kompakten String zum Teilen in Discord oder Chat."
						: "Copies your current color profile as a compact string to share in Discord or chat.");
				}

				ImGui::SameLine(0, 8.0f);

				if (ImGui::Button(isDe ? "Aus Zwischenablage importieren##imp" : "Import from clipboard##imp", ImVec2(0.0f, 24.0f)))
				{
					const char* clip = ImGui::GetClipboardText();
					if (clip && clip[0] != '\0')
					{
						std::string err;
						if (CurrentSettings.ImportPresetString(clip, &err))
						{
							EnsureDeferredInitialized();
							CurrentSettings.Save(AddonDir);
							Recompute(/*aForce=*/true);
							s_profileFeedbackTime = std::chrono::steady_clock::now();
							s_profileFeedbackMsg = isDe ? "[OK] Profil erfolgreich importiert!" : "[OK] Profile imported successfully!";
							changed = true;
							saveNeeded = true;
						}
						else
						{
							s_profileFeedbackTime = std::chrono::steady_clock::now();
							s_profileFeedbackMsg = isDe ? "[FEHLER] Ungueltiger Profil-String!" : "[ERROR] Invalid profile string!";
						}
					}
					else
					{
						s_profileFeedbackTime = std::chrono::steady_clock::now();
						s_profileFeedbackMsg = isDe ? "[FEHLER] Zwischenablage ist leer!" : "[ERROR] Clipboard is empty!";
					}
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe 
						? "Liest ein vorher kopiertes CBA-Profil (CBA1:...) aus der Zwischenablage ein und wendet es an."
						: "Reads a previously copied CBA profile (CBA1:...) from the clipboard and applies it.");
				}
			}
			endSection();
		}

		// ── Section 2: Eye Comfort (Helligkeit) ──────────────────────────────
		if (renderSectionHeader(1, t.HeaderSection2))
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
			float padXGamma = ImGui::GetStyle().FramePadding.x * 2.0f;
			float btnWGamma = ImGui::CalcTextSize("Reset").x + padXGamma + 8.0f;
			float spGamma = 6.0f;
			float sWGamma = (availGamma > (btnWGamma + spGamma + 60.0f)) ? (availGamma - btnWGamma - spGamma) : 180.0f;

			ImGui::SetNextItemWidth(sWGamma);
			// Registry-backed (CLAUDE.md, Registry/Control Layer step 1) - same
			// pattern as the Tolerance slider pilot: clamp lives in ParamMeta.
			{
				float gain = ParameterRegistry::Get().GetFloat(ParamId::GammaGain);
				if (ImGui::SliderFloat("##EyeComfortGammaSlider", &gain, 0.70f, 1.30f, "%.2fx", ImGuiSliderFlags_AlwaysClamp))
				{
					SetGammaGainManual(gain);
					changed = true;
				}
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
				SetGammaGainManual(1.0f);
				changed = true;
				saveNeeded = true;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip(isDe ? "Helligkeit auf 1.00x zuruecksetzen" : "Reset brightness to 1.00x");

			SyncAutoBrightnessGain(changed, saveNeeded);
			BrightnessRetentionResult retention = GetBrightnessRetention();

			ImGui::Spacing();
			ImGui::Text(t.EyeComfortRetention, retention.retentionRatio * 100.0f, retention.recommendedGain);
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
			if (ImGui::Button(t.EyeComfortApply, ImVec2(0.0f, 24.0f)))
			{
				ApplyAutoBrightnessGain();
				changed = true;
				saveNeeded = true;
			}
			ImGui::PopStyleColor(4);
			ImGui::SameLine(0, 12.0f);
			if (ImGui::Checkbox(isDe ? "Auto-Helligkeit##main_auto" : "Auto-Brightness##main_auto", &CurrentSettings.AutoBrightness))
			{
				if (CurrentSettings.AutoBrightness)
				{
					ApplyAutoBrightnessGain();
					changed = true;
				}
				saveNeeded = true;
			}

			// Eye-Sensitive Mode (2026-09-09) - its own module within Eye
			// Comfort, independent of the Gamma/Auto-Brightness pair above
			// (which stays untouched, Emi: "funktioniert einwandfrei").
			// Composes with CVD correction in Recompute(), never replaces it.
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			if (ImGui::Checkbox(isDe ? "Eye-Sensitive Mode aktivieren" : "Activate Eye-Sensitive Mode", &CurrentSettings.EyeComfortModeEnabled))
			{
				changed = true;
				saveNeeded = true;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe
					? "Eigenstaendiges Layer fuer Blaufilter, Warmton und Saettigungsreduktion - unabhaengig von der Farbkorrektur, wird zusaetzlich angewendet."
					: "Independent layer for blue-light filter, warm tint and saturation reduction - separate from color correction, applied on top of it.");
			}

			if (CurrentSettings.EyeComfortModeEnabled)
			{
				ImGui::Indent(16.0f);

				// ImGui's SliderFloat format string does not auto-scale - a
				// 0.0-1.0 range with "%.0f%%" just printed the raw fraction
				// with a % sign glued on, so the label only ever read "0%"
				// or "1%" across the whole range (found in the 2026-09-09
				// codebase review). Fix: widget operates in 0-100 display
				// units, converted to/from the registry's 0.0-1.0 storage
				// range right at the boundary - same trick the "Strength"
				// display text a few lines up already uses (manual *100.0).
				ImGui::TextUnformatted(isDe ? "Blaufilter:" : "Blue Light Filter:");
				ImGui::SetNextItemWidth(-FLT_MIN);
				{
					float v = ParameterRegistry::Get().GetFloat(ParamId::BlueFilter01) * 100.0f;
					if (ImGui::SliderFloat("##blue_filter_slider", &v, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
					{
						ParameterRegistry::Get().SetFloat(ParamId::BlueFilter01, v / 100.0f);
						changed = true;
					}
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::TextUnformatted(isDe ? "Warmton:" : "Warm Tint:");
				ImGui::SetNextItemWidth(-FLT_MIN);
				{
					float v = ParameterRegistry::Get().GetFloat(ParamId::WarmTint01) * 100.0f;
					if (ImGui::SliderFloat("##warm_tint_slider", &v, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
					{
						ParameterRegistry::Get().SetFloat(ParamId::WarmTint01, v / 100.0f);
						changed = true;
					}
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::TextUnformatted(isDe ? "Saettigungsreduktion:" : "Saturation Reduction:");
				ImGui::SetNextItemWidth(-FLT_MIN);
				{
					float v = ParameterRegistry::Get().GetFloat(ParamId::SaturationReduction01) * 100.0f;
					if (ImGui::SliderFloat("##sat_reduction_slider", &v, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
					{
						ParameterRegistry::Get().SetFloat(ParamId::SaturationReduction01, v / 100.0f);
						changed = true;
					}
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::Unindent(16.0f);
			}

			endSection();
		}

		// ── Section 3: Spiel- & Fenstermodus ──────────────────────────────────
		if (renderSectionHeader(2, t.HeaderSection3))
		{
			WindowMode mode = DetectWindowMode(
				APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);
			if (mode == WindowMode::ExclusiveFullscreen) {
				ImGui::TextColored({1.0f,0.55f,0.2f,1.0f}, "%s: %s", t.WindowMode, ToDisplayString(mode, isDe));
				ImGui::TextWrapped(isDe 
					? "Stelle GW2 in den Grafik-Optionen auf 'Fenster' oder 'Vollbild im Fenster' (Rahmenlos), damit der Filter aktiv werden kann."
					: "Switch GW2 to Windowed or Windowed Fullscreen (Borderless) in Graphics Options to enable the filter.");
			} else {
				ImGui::TextColored({0.4f,0.85f,0.4f,1.0f}, "%s: %s", t.WindowMode, ToDisplayString(mode, isDe));
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

		// ── Section 4: Hybrid Modus (Beta) ────────────────────────────────────
		if (renderSectionHeader(3, t.HeaderSection4))
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

		// ── Section 5: Filter-Labor & Experimentierfeld ──────────────────────
		if (renderSectionHeader(4, t.HeaderSection5))
		{
			DrawFilterLabWidget(isDe, changed, saveNeeded);
			endSection();
		}

		// ── Section 6: Über, Diagnose & Credits ──────────────────────────────
		if (renderSectionHeader(5, t.HeaderSection6))
		{
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
				}
				ImGui::EndChild();
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);

				// ── Self-Test (2026-09-09) - automated, read-only runtime
				// checks (registry integrity, live color-math invariants,
				// Settings export/import roundtrip, cross-field consistency
				// invariants). Complements, doesn't replace, manual testing -
				// see core/SelfTest.h for exactly what is and isn't covered.
				ImGui::Spacing();
				static std::vector<SelfTestCheck> s_selfTestResults;
				static bool s_selfTestRan = false;
				if (ImGui::Button(isDe ? "Selbst-Test ausfuehren##selftest_run" : "Run Self-Test##selftest_run", ImVec2(0.0f, 24.0f)))
				{
					s_selfTestResults = RunSelfTest();
					s_selfTestRan = true;
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe
						? "Prueft automatisch Registry-Konsistenz, Farbmathe-Invarianten, Settings-Export/Import und Wertebereiche.\nKann keine visuellen/optischen Probleme erkennen - das braucht weiterhin manuelles Testen."
						: "Automatically checks registry consistency, color-math invariants, Settings export/import, and value ranges.\nCannot detect visual/perceptual issues - manual testing is still needed for those.");
				}
				if (s_selfTestRan)
				{
					int passed = 0, failed = 0, info = 0;
					for (const auto& c : s_selfTestResults)
					{
						if (c.isInfo) ++info;
						else if (c.passed) ++passed;
						else ++failed;
					}
					ImGui::SameLine(0, 10.0f);
					ImGui::TextColored(failed == 0 ? ImVec4(0.35f, 0.95f, 0.55f, 1.0f) : ImVec4(0.95f, 0.35f, 0.35f, 1.0f),
						isDe ? "%d/%d bestanden (%d Info)" : "%d/%d passed (%d info)", passed, passed + failed, info);

					ImGui::Spacing();
					ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.08f, 0.12f, 0.90f));
					ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.18f, 0.32f, 0.45f, 0.70f));
					ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
					float listH = ImGui::GetTextLineHeightWithSpacing() * std::min<float>(10.0f, (float)s_selfTestResults.size()) + 12.0f;
					if (ImGui::BeginChild("##selftest_results", ImVec2(0.0f, listH), true))
					{
						const char* lastCategory = "";
						for (const auto& c : s_selfTestResults)
						{
							if (std::strcmp(lastCategory, c.category) != 0)
							{
								lastCategory = c.category;
								ImGui::TextColored(Theme::kTextGoldLabel, "%s", lastCategory);
							}
							ImVec4 col = c.isInfo ? ImVec4(0.55f, 0.60f, 0.68f, 1.0f)
								: (c.passed ? ImVec4(0.35f, 0.95f, 0.55f, 1.0f) : ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
							const char* mark = c.isInfo ? "[i]" : (c.passed ? "[OK]" : "[FAIL]");
							ImGui::TextColored(col, "  %s %s", mark, c.name);
							if (!c.detail.empty())
							{
								ImGui::SameLine();
								ImGui::TextDisabled("- %s", c.detail.c_str());
							}
						}
					}
					ImGui::EndChild();
					ImGui::PopStyleVar(2);
					ImGui::PopStyleColor(2);
				}

				ImGui::Spacing();
				static std::chrono::steady_clock::time_point s_diagFeedbackTime{};
				if (ImGui::Button(isDe ? "System-Diagnose in Zwischenablage kopieren##diag_copy" : "Copy System Diagnostics to Clipboard##diag_copy", ImVec2(0.0f, 24.0f)))
				{
					MumbleGameContext gctx = GetCurrentGameContext();
					WindowMode wMode = DetectWindowMode(APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);
					bool hdr = DetectHdrColorSpace(APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);

					// Was a hardcoded "1.0.2.0 (Build 2)" literal, drifted from
					// reality basically immediately - the real Version fields
					// carry the hour/minute-second build stamp added this
					// session specifically so a report can prove which exact
					// compile is loaded (found in the 2026-09-09 codebase
					// review, the very feature this stamp exists for wasn't
					// using it).
					AddonVersion ver = GetAddonVersion();
					char report[1024];
					std::snprintf(report, sizeof(report),
						"============================================================\n"
						" CBA4GW2 SYSTEM & DIAGNOSTIC REPORT\n"
						"============================================================\n"
						"- Addon Version: %d.%d.%d.%d | Nexus API: %d\n"
						"- Profile: %s | Severity: %.1f%% | Enabled: %s\n"
						"- Magnification API: Initialized: %s | Filter Applied: %s\n"
						"- Window Mode: %s | HDR Detected: %s\n"
						"- MumbleLink: Map ID %u (%s) | In Combat: %s\n"
						"- Performance Timings: Main: %.2f ms | HUD: %.2f ms | Curves: %.2f ms | Lab: %.2f ms | Total: %.2f ms\n"
						"============================================================",
						ver.Major, ver.Minor, ver.Build, ver.Revision,
						NEXUS_API_VERSION,
						(CurrentSettings.Mixed ? "Mixed" : (CurrentSettings.Type == BalanceType::Protan ? "Protan" : (CurrentSettings.Type == BalanceType::Deutan ? "Deutan" : "Tritan"))),
						CurrentSettings.Severity01 * 100.0,
						CurrentSettings.Enabled ? "YES" : "NO",
						s_deferredInitDone.load() ? "YES" : "NO",
						CurrentSettings.Enabled ? "ACTIVE" : "INACTIVE",
						ToDisplayString(wMode, false),
						hdr ? "YES" : "NO",
						gctx.mapId, gctx.modeNameEn,
						gctx.isInCombat ? "YES" : "NO",
						g_perfMainWindowMs, g_perfSensorGraphMs, g_perfCurvesMs, g_perfFilterLabMs, g_perfTotalImGuiMs);

					std::string fullReport = report;
					if (s_selfTestRan)
					{
						fullReport += "\n- Self-Test:\n";
						for (const auto& c : s_selfTestResults)
						{
							const char* mark = c.isInfo ? "[i]" : (c.passed ? "[OK]" : "[FAIL]");
							fullReport += "  ";
							fullReport += mark;
							fullReport += " ";
							fullReport += c.category;
							fullReport += ": ";
							fullReport += c.name;
							if (!c.detail.empty()) { fullReport += " - "; fullReport += c.detail; }
							fullReport += "\n";
						}
						fullReport += "============================================================";
					}
					ImGui::SetClipboardText(fullReport.c_str());
					s_diagFeedbackTime = std::chrono::steady_clock::now();
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(isDe 
						? "Kopiert einen detaillierten, anonymisierten Diagnose-Report in die Zwischenablage (ideal fuer Bug-Reports auf GitHub oder Discord)."
						: "Copies a detailed, anonymized diagnostic report to the clipboard (ideal for bug reports on GitHub or Discord).");
				}

				auto nowDiag = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::seconds>(nowDiag - s_diagFeedbackTime).count() < 3)
				{
					ImGui::SameLine(0, 8.0f);
					ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.55f, 1.0f), "%s", isDe ? "[OK] Diagnose kopiert!" : "[OK] Diagnostics copied!");
				}
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
			if (ImGui::Button(t.CreditsBtn, ImVec2(0.0f, 24.0f)))
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
			bool isClickedLeft = ImGui::IsItemClicked(ImGuiMouseButton_Left);
			bool isClickedRight = ImGui::IsItemClicked(ImGuiMouseButton_Right);

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
