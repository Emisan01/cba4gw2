#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MainWindow.h"
#include "VisionLab.h"
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

namespace cba
{
	// Guided-entry flow state (2026-09-11, see PRODUCT_CONCEPT.md 3.1 and the
	// block in RenderEmbeddedOptions). Session-only by design: the derived
	// profile itself is persisted in Settings like any other, but where the
	// user is inside the questionnaire is transient UI state with no meaning
	// across launches. File-scope statics rather than UIState globals - a
	// single window's wizard step is not shared state, and UIState.h's own
	// comment block rules out exactly this kind of value.
	static int s_setupStep = 0;              // 0 = done/not in flow, 1..3 = question
	static bool s_setupAxisRedGreen = true;  // which axis the user picked in step 1
	static BalanceType s_setupPendingType = BalanceType::Deutan;
	static bool s_setupPendingMixed = false;
	static float s_setupStrength = 0.6f;     // proposed severity, raised by the user in step 3
	static bool s_setupDismissed = false;    // user said "I can tell them all apart" this session

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

		// Round 2 (Emi: "muss huebscher sein, sieht aus wie eine Creditcard") -
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

	// ── Auto-Start profile: one value, one control, two homes ─────────────
	//
	// Settings.AutoStartSlot is a single int (-1 = off, 0..2 = the slot that
	// loads and arms the filter at launch), so it is one choice, not three
	// independent ones. It used to be edited two incompatible ways: a
	// right-click on a slot chip in the Nexus panel (invisible - nothing on
	// screen said it existed) and a per-slot "Auto-Start" checkbox in the
	// Studio (three checkboxes standing in for one radio group). Emi asked for
	// the logic to be understandable at a glance; this is that control, and
	// both surfaces now draw the same one.
	//
	// Two steps on purpose: "do I want this at all" and "which profile" are
	// genuinely different questions, and separating them is what makes the
	// second one legible. They cannot desync - switching on always picks a
	// real slot, and clearing the chosen slot switches it off.
	void DrawAutoStartControl(bool& aSaveNeeded, bool aIsDe, bool aCompact)
	{
		int firstUsed = -1;
		for (int i = 0; i < 3; ++i)
		{
			if (CurrentSettings.Slots[i].Used) { firstUsed = i; break; }
		}

		if (firstUsed < 0)
		{
			// No slot saved yet. This vendored ImGui has no BeginDisabled, and
			// a checkbox that silently refuses the click is worse than a
			// sentence explaining what is missing.
			ImGui::TextDisabled("%s", aIsDe ? "Automatisch mit GW2 starten: erst ein Profil speichern."
			                                : "Start automatically with GW2: save a profile first.");
			return;
		}

		bool on = (CurrentSettings.AutoStartSlot >= 0);
		if (ImGui::Checkbox(aIsDe ? "Automatisch mit GW2 starten##autostart_on" : "Start automatically with GW2##autostart_on", &on))
		{
			if (on)
			{
				// Never "on" without a target: prefer the slot the user is
				// currently working in, fall back to the first saved one.
				const int active = ParameterRegistry::Get().GetInt(ParamId::ActiveSlotIdx);
				const bool activeUsable = (active >= 0 && active < 3 && CurrentSettings.Slots[active].Used);
				CurrentSettings.AutoStartSlot = activeUsable ? active : firstUsed;
			}
			else
			{
				CurrentSettings.AutoStartSlot = -1;
			}
			aSaveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", aIsDe
				? "Laedt beim Start von GW2 ein gespeichertes Profil und schaltet den Filter ein.\nOhne das startet CBA immer neutral - Filter aus, alle Fenster zu."
				: "Loads a saved profile and turns the filter on when GW2 starts.\nWithout it CBA always starts neutral - filter off, all windows closed.");
		}

		if (CurrentSettings.AutoStartSlot < 0) return;

		ImGui::Indent(16.0f);
		if (!aCompact)
		{
			ImGui::TextDisabled("%s", aIsDe ? "Profil beim Start:" : "Profile at launch:");
		}
		for (int i = 0; i < 3; ++i)
		{
			if (i > 0) ImGui::SameLine(0, 10.0f);
			if (!CurrentSettings.Slots[i].Used)
			{
				// Drawn, not hidden: three positions that stay in the same
				// place read faster than a list that changes length.
				ImGui::TextDisabled("%d", i + 1);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", aIsDe ? "Slot ist leer" : "Slot is empty");
				continue;
			}
			char radioId[48];
			std::snprintf(radioId, sizeof(radioId), "%d##autostart_pick_%d", i + 1, i);
			if (ImGui::RadioButton(radioId, CurrentSettings.AutoStartSlot == i))
			{
				CurrentSettings.AutoStartSlot = i;
				aSaveNeeded = true;
			}
			if (ImGui::IsItemHovered())
			{
				const std::string& n = CurrentSettings.Slots[i].Name;
				ImGui::SetTooltip("%s", n.empty() ? (aIsDe ? "Gespeichertes Profil" : "Saved profile") : n.c_str());
			}
		}

		const int pick = CurrentSettings.AutoStartSlot;
		if (pick >= 0 && pick < 3)
		{
			ImGui::SameLine(0, 12.0f);
			const std::string& pickName = CurrentSettings.Slots[pick].Name;
			ImGui::TextColored(Theme::kTextGoldLabel, "%s", pickName.empty() ? (aIsDe ? "Gespeichertes Profil" : "Saved profile") : pickName.c_str());
		}

		ImGui::TextDisabled("%s", aIsDe ? "Nach einem Absturz wird das uebersprungen."
		                                : "Skipped after a crash.");
		ImGui::Unindent(16.0f);
	}

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
		if (ImGui::Button(isDe ? "CBA Studio Oeffnen" : "Open CBA Studio", ImVec2(ImGui::GetContentRegionAvail().x, 32.0f)))
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
		
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 180.0f);
		ImGui::TextDisabled("%s", isDe ? "Backend:" : "Backend:");
		ImGui::SameLine();
		if (CurrentSettings.RenderBackend == 1 && GetShaderColorPipeline().IsReady()) {
			ImGui::TextColored(Theme::kTextCyanLicht, "Shader");
		} else if (CurrentSettings.RenderBackend == 0) {
			ImGui::TextColored(Theme::kTextGoldLabel, "DWM");
		} else {
			ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Inaktiv");
				ImGui::EndChild();
	}
		
		ImGui::EndGroup();
		
		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(2);
		
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		
		// ── Content Area ──────────────────────────────────────────
		ImGui::BeginChild("##MainWindowScrollContent"
, ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);


		if (s_ActiveTab == 2)
		{
			cba::RenderVisionLabContent(isDe);
		}

// ── Section 1: Farbprofil & Korrektur ────────────────────────────────
		if (s_ActiveTab == 0)
		
				{
			// Tile: Vision Assessment (Sehtest) & Profil
			ImGui::BeginChild("Tile_Profile", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
			if (ImGui::BeginMenuBar()) { ImGui::TextColored(Theme::kTextCyanLicht, "Farbprofil & Korrektur"); ImGui::EndMenuBar(); }

auto applyDerivedProfile = [&](BalanceType aType, bool aMixed, float aSeverity) {
				// Routed through the shared activation function rather than
				// setting the fields by hand (it owns EnsureDeferredInitialized,
				// Enabled, CommanderTagMode and Recompute) - then severity is
				// overridden, because that function deliberately forces 100%
				// for its own one-click button semantics.
				ActivateCommanderTagProfile(aType);
				CurrentSettings.Mixed = aMixed;
				ParameterRegistry::Get().SetFloat(ParamId::Severity01, aSeverity);
				if (aMixed)
				{
					ParameterRegistry::Get().SetFloat(ParamId::MixedRgSeverity01, aSeverity);
					ParameterRegistry::Get().SetFloat(ParamId::MixedBySeverity01, aSeverity);
				}
				Recompute(/*aForce=*/true);
				changed = true;
				saveNeeded = true;
			};



auto pairOption = [&](const char* aId, int aTagA, int aTagB, const char* aLabel) -> bool {
				ImGui::PushID(aId);
				// Floor the width: this panel lives inside Nexus's own window,
				// whose width the user controls, and GetContentRegionAvail can
				// come back tiny or negative there. A non-positive InvisibleButton
				// size is an ImGui assert, i.e. someone else's narrow panel would
				// take the addon down.
				float w = ImGui::GetContentRegionAvail().x;
				if (w < 60.0f) w = 60.0f;
				float h = 46.0f;
				ImVec2 p = ImGui::GetCursorScreenPos();
				bool clicked = ImGui::InvisibleButton("##opt", ImVec2(w, h));
				bool hovered = ImGui::IsItemHovered();
				ImDrawList* dl = ImGui::GetWindowDrawList();
				dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h),
					hovered ? IM_COL32(60, 78, 100, 130) : IM_COL32(40, 52, 68, 90), 5.0f);
				if (hovered)
					dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(120, 190, 230, 200), 5.0f, 0, 1.5f);
				float r = 15.0f;
				float cy = p.y + h * 0.5f;
				float cx = p.x + 14.0f + r;
				ImU32 cA = IM_COL32((int)(kGw2TagRefs[aTagA].r * 255), (int)(kGw2TagRefs[aTagA].g * 255), (int)(kGw2TagRefs[aTagA].b * 255), 255);
				ImU32 cB = IM_COL32((int)(kGw2TagRefs[aTagB].r * 255), (int)(kGw2TagRefs[aTagB].g * 255), (int)(kGw2TagRefs[aTagB].b * 255), 255);
				dl->AddCircleFilled(ImVec2(cx, cy), r, cA, 32);
				dl->AddCircleFilled(ImVec2(cx + r * 0.9f, cy), r, cB, 32);
				dl->AddText(ImVec2(cx + r * 2.4f, cy - ImGui::GetTextLineHeight() * 0.5f),
					IM_COL32(226, 232, 240, 255), aLabel);
				ImGui::PopID();
				return clicked;
			};

// s_setupDismissed is what makes "I can tell them all apart" stick.
			// Without it that button is a no-op: it leaves both CommanderTagMode
			// and Severity01 at zero, so `configured` stays false and the next
			// frame drops the user straight back into question 1 - an
			// inescapable questionnaire. Session-only on purpose: someone who
			// dismisses it today should still be met by the offer next launch,
			// since a new player may simply not have realised yet that it helps.
			bool configured = (CurrentSettings.CommanderTagMode != 0) || (CurrentSettings.Severity01 > 0.01f);
			int step = s_setupStep;
			if (!configured && !s_setupDismissed && step == 0) step = 1; // fresh install lands straight in the flow

			if (step == 1)
			{
				ImGui::TextWrapped("%s", isDe
					? "Welches Farbpaar faellt dir am schwersten zu unterscheiden?"
					: "Which colour pair is hardest for you to tell apart?");
				ImGui::Spacing();
				// Indices into kGw2TagRefs: 0 Red, 2 Yellow, 3 Green, 5 Blue.
				if (pairOption("rg", 0, 3, isDe ? "Rot und Gruen" : "Red and green"))
				{
					s_setupAxisRedGreen = true;
					s_setupStep = 2;
				}
				if (pairOption("by", 2, 5, isDe ? "Gelb und Blau" : "Yellow and blue"))
				{
					s_setupAxisRedGreen = false;
					s_setupPendingType = BalanceType::Tritan;
					s_setupPendingMixed = false;
					s_setupStep = 3;
				}
				if (pairOption("both", 0, 5, isDe ? "Beide etwa gleich schwer" : "Both about equally hard"))
				{
					s_setupAxisRedGreen = false;
					s_setupPendingType = BalanceType::Deutan;
					s_setupPendingMixed = true;
					s_setupStep = 3;
				}
				ImGui::Spacing();
				if (ImGui::SmallButton(isDe ? "Ich kann alle gut unterscheiden##skip" : "I can tell them all apart##skip"))
				{
					s_setupStep = 0;
					s_setupDismissed = true;
					CurrentSettings.CommanderTagMode = 0;
					saveNeeded = true;
				}
			}
			else if (step == 2)
			{
				// The one discriminator between Protan and Deutan that a user
				// can actually answer: protans have markedly reduced luminance
				// response to long wavelengths, so saturated red reads as much
				// darker to them than it does to a deutan. Asking about
				// BRIGHTNESS is answerable; asking "protan or deutan?" is not.
				ImGui::TextWrapped("%s", isDe
					? "Wie wirkt das Rot im Vergleich zum Gruen?"
					: "How does the red look compared to the green?");
				ImGui::Spacing();
				if (pairOption("dark", 0, 3, isDe ? "Das Rot wirkt deutlich dunkler" : "The red looks much darker"))
				{
					s_setupPendingType = BalanceType::Protan;
					s_setupPendingMixed = false;
					s_setupStep = 3;
				}
				if (pairOption("same", 0, 3, isDe ? "Beide etwa gleich hell" : "Both about equally bright"))
				{
					s_setupPendingType = BalanceType::Deutan;
					s_setupPendingMixed = false;
					s_setupStep = 3;
				}
				ImGui::Spacing();
				if (ImGui::SmallButton(isDe ? "Zurueck##back2" : "Back##back2")) s_setupStep = 1;
			}
			else if (step == 3)
			{
				ImGui::TextWrapped("%s", isDe
					? "Und jetzt - kannst du die beiden Farben unterscheiden?"
					: "And now - can you tell the two colours apart?");
				ImGui::Spacing();

				// Preview the pair exactly as the correction will render it, at
				// the strength currently being proposed. Built from explicit
				// parameters rather than via ActiveCorrectionMatrix(), which
				// reads CurrentSettings: briefly swapping those fields in and
				// out to borrow it would race the Watchdog thread, which calls
				// Recompute() on the same fields every 50ms and would then push
				// a not-yet-chosen matrix to the whole screen.
				double previewMat[3][3];
				if (s_setupPendingMixed)
					ColorMatrix::MixedCorrectionMatrix(s_setupStrength, s_setupStrength, previewMat);
				else
					ColorMatrix::CorrectionMatrix(s_setupPendingType, s_setupStrength, previewMat);

				// Mixed is built on a Deutan base, so the red/green pair is what
				// actually demonstrates it - red/blue would show the axis this
				// profile affects least.
				int tagA = s_setupAxisRedGreen ? 0 : 2;
				int tagB = s_setupAxisRedGreen ? 3 : 5;
				if (s_setupPendingMixed) { tagA = 0; tagB = 3; }

				double oa[3], ob[3];
				ColorMatrix::ApplyPixel(kGw2TagRefs[tagA].r, kGw2TagRefs[tagA].g, kGw2TagRefs[tagA].b, previewMat, oa[0], oa[1], oa[2]);
				ColorMatrix::ApplyPixel(kGw2TagRefs[tagB].r, kGw2TagRefs[tagB].g, kGw2TagRefs[tagB].b, previewMat, ob[0], ob[1], ob[2]);

				{
					float w = ImGui::GetContentRegionAvail().x;
					if (w < 60.0f) w = 60.0f; // same narrow-panel floor as pairOption
					float h = 56.0f;
					ImVec2 p = ImGui::GetCursorScreenPos();
					ImDrawList* dl = ImGui::GetWindowDrawList();
					float r = 20.0f;
					float cy = p.y + h * 0.5f;
					float cx = p.x + w * 0.5f - r * 0.45f;
					dl->AddCircleFilled(ImVec2(cx, cy), r, IM_COL32((int)(oa[0]*255), (int)(oa[1]*255), (int)(oa[2]*255), 255), 40);
					dl->AddCircleFilled(ImVec2(cx + r * 0.9f, cy), r, IM_COL32((int)(ob[0]*255), (int)(ob[1]*255), (int)(ob[2]*255), 255), 40);
					ImGui::Dummy(ImVec2(w, h));
				}

				ImGui::TextDisabled(isDe ? "Staerke: %.0f%%" : "Strength: %.0f%%", s_setupStrength * 100.0f);
				ImGui::Spacing();

				float availS = ImGui::GetContentRegionAvail().x;
				float halfW = (availS - 6.0f) * 0.5f;
				if (ImGui::Button(isDe ? "Nein, staerker##more" : "No, stronger##more", ImVec2(halfW, 30.0f)))
				{
					s_setupStrength = (s_setupStrength >= 1.0f) ? 1.0f : (s_setupStrength + 0.2f);
				}
				ImGui::SameLine(0, 6.0f);
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
				if (ImGui::Button(isDe ? "Ja, passt##done" : "Yes, that works##done", ImVec2(halfW, 30.0f)))
				{
					applyDerivedProfile(s_setupPendingType, s_setupPendingMixed, s_setupStrength);
					s_setupStep = 0;
				}
				ImGui::PopStyleColor(4);
				ImGui::Spacing();
				if (ImGui::SmallButton(isDe ? "Zurueck##back3" : "Back##back3"))
					s_setupStep = s_setupAxisRedGreen ? 2 : 1;
			}
			else
			{
				// Configured: no questions, just the state and a way back in.
				const char* typeName = CurrentSettings.Mixed
					? (isDe ? "Gemischt" : "Mixed")
					: (CurrentSettings.Type == BalanceType::Protan ? "Protan"
					 : CurrentSettings.Type == BalanceType::Deutan ? "Deutan" : "Tritan");
				ImGui::TextDisabled(isDe ? "Dein Profil: %s (%.0f%%)" : "Your profile: %s (%.0f%%)",
					typeName, CurrentSettings.Severity01 * 100.0f);
				if (ImGui::SmallButton(isDe ? "Sehtest wiederholen##retest" : "Redo the test##retest"))
				{
					s_setupStrength = 0.6f;
					s_setupStep = 1;
				}

				}


			ImGui::EndChild();
			ImGui::Spacing();
			
			// Tile: Commander Tag
			ImGui::BeginChild("Tile_ComTag", ImVec2(0, 310), true, ImGuiWindowFlags_MenuBar);
			if (ImGui::BeginMenuBar()) { ImGui::TextColored(Theme::kTextCyanLicht, "Commander-Tag Enhancer"); ImGui::EndMenuBar(); }
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
					SaveSettingsToSlot(targetSlot);

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

			// Brightness/Eye Comfort Gamma used to be duplicated here AND in
			// Section 2 - two sliders bound to the same value, in two
			// different places. Removed from here entirely 2026-09-09 (Emi's
			// UI walkthrough); Section 2 "Eye Comfort" is its one home now,
			// it has the fuller picture anyway (Retention/HDR/Apply-Target).
			//
			// The "Advanced" group that held Tolerance and the AQ/HRR
			// reference field moved to the Sensor Graph window 2026-09-12,
			// into the manual-control module. Decluttering this section in
			// 2026-09-09 had collapsed those two into a fold nobody opens;
			// the real problem was that they had been separated from the
			// contrast logic they drive in the first place. They are editable
			// in exactly one place now, next to the sliders they interact
			// with - see SensorGraphHUD.cpp's "Manual Filter Controls".

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

			
			ImGui::EndChild();
			ImGui::Spacing();
			
			// Tile: Curve View
			ImGui::BeginChild("Tile_CurveView", ImVec2(0, 160), true, ImGuiWindowFlags_MenuBar);
			if (ImGui::BeginMenuBar()) { ImGui::TextColored(Theme::kTextCyanLicht, "Sensor Matrix Visualisierung"); ImGui::EndMenuBar(); }
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
				ActiveCorrectionMatrix(mainCorrMat);

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
				SaveSettingsToSlot(targetSlot);

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
					if (used)
					{
						if (LoadSettingsFromSlot(sIdx)) {
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
					else
					{
						ParameterRegistry::Get().SetInt(ParamId::ActiveSlotIdx, sIdx);
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
						if (LoadSettingsFromSlot(sIdx)) {
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

					// The per-slot "Auto-Start" checkbox that used to sit here
					// was three checkboxes standing in for one radio group -
					// AutoStartSlot holds a single value, so two of them were
					// always the wrong shape for the data. Replaced by the
					// shared control below the list (2026-09-12).
				} else {
					ImGui::TextDisabled("Slot %d: [%s]", sIdx + 1, isDe ? "Leer" : "Empty");
				}
				ImGui::PopID();
			}

			ImGui::Spacing();
			DrawAutoStartControl(saveNeeded, isDe, /*aCompact=*/false);

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
			
	
			ImGui::EndChild();
	}

		// ── Section 2: Eye Comfort (Helligkeit) ──────────────────────────────
		if (s_ActiveTab == 1)
		
		{
			ImGui::BeginChild("Tile_Eye", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
			if (ImGui::BeginMenuBar()) { ImGui::TextColored(Theme::kTextCyanLicht, "Eye Comfort Settings"); ImGui::EndMenuBar(); }

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
				// Three near-identical hand-copied blocks until 2026-09-12,
				// which is how they came to differ from the Nexus panel's own
				// copy of the same three sliders (that one had no Reset at
				// all). One lambda now, Reset included - same shape as the
				// panel's embEyeSlider and the Sensor Graph window's
				// severitySlider.
				auto eyeSlider = [&](const char* aLabel, const char* aId, ParamId aParam) {
					ImGui::TextUnformatted(aLabel);

					float avail = ImGui::GetContentRegionAvail().x;
					float padX  = ImGui::GetStyle().FramePadding.x * 2.0f;
					float btnW  = ImGui::CalcTextSize("Reset").x + padX + 8.0f;
					const float sp = 6.0f;
					float sW = (avail > (btnW + sp + 60.0f)) ? (avail - btnW - sp) : 180.0f;

					ImGui::SetNextItemWidth(sW);
					float v = ParameterRegistry::Get().GetFloat(aParam) * 100.0f;
					if (ImGui::SliderFloat(aId, &v, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
					{
						ParameterRegistry::Get().SetFloat(aParam, v / 100.0f);
						changed = true;
					}
					if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

					ImGui::SameLine(0, sp);
					char eyeResetId[64];
					std::snprintf(eyeResetId, sizeof(eyeResetId), "Reset%s", aId);
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
					if (ImGui::Button(eyeResetId, ImVec2(btnW, 0.0f)))
					{
						ParameterRegistry::Get().SetFloat(aParam, 0.0f);
						changed = true;
						saveNeeded = true;
					}
					ImGui::PopStyleColor(4);
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", isDe ? "Wert auf 0% zuruecksetzen" : "Reset value to 0%");
				};

				eyeSlider(isDe ? "Blaufilter:" : "Blue Light Filter:", "##blue_filter_slider", ParamId::BlueFilter01);
				eyeSlider(isDe ? "Warmton:" : "Warm Tint:", "##warm_tint_slider", ParamId::WarmTint01);
				eyeSlider(isDe ? "Saettigungsreduktion:" : "Saturation Reduction:", "##sat_reduction_slider", ParamId::SaturationReduction01);

				ImGui::Unindent(16.0f);
			}

			
		}

		// ── Section 3: Spiel- & Fenstermodus ──────────────────────────────────
		if (s_ActiveTab == 3)
		
		{
			ImGui::BeginChild("Tile_System", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
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
			
		}

		// ── Section 4: Hybrid Modus (Beta) ────────────────────────────────────
		if (s_ActiveTab == 3)
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
		if (s_ActiveTab == 3)
		{
			DrawFilterLabWidget(isDe, changed, saveNeeded);
			
		}

		// ── Section 6: Über, Diagnose & Credits ──────────────────────────────
		if (s_ActiveTab == 3)
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
						ImGui::EndChild();
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
						"- Colour path: %s | Painting now: %s | Magnification session: %s\n"
						"- Window Mode: %s | HDR Detected: %s\n"
						"- MumbleLink: Map ID %u (%s) | In Combat: %s\n"
						"- Performance Timings: Main: %.2f ms | HUD: %.2f ms | Curves: %.2f ms | Lab: %.2f ms | Total ImGui: %.2f ms | Colour pass: %.3f ms\n"
						"============================================================",
						ver.Major, ver.Minor, ver.Build, ver.Revision,
						NEXUS_API_VERSION,
						(CurrentSettings.Mixed ? "Mixed" : (CurrentSettings.Type == BalanceType::Protan ? "Protan" : (CurrentSettings.Type == BalanceType::Deutan ? "Deutan" : "Tritan"))),
						CurrentSettings.Severity01 * 100.0,
						CurrentSettings.Enabled ? "YES" : "NO",
						// This line used to read "Filter Applied: ACTIVE" straight
						// off CurrentSettings.Enabled, on a line about the
						// Magnification API - so a shader-backend report claimed
						// the DWM effect was applied while SelfTest, a few lines
						// down, correctly reported it was not. Two lines in one
						// report disagreeing about the same fact is the
						// false-evidence problem in its purest form. Each half
						// now states what it actually is.
						(CurrentSettings.RenderBackend == 1) ? "Shader (GW2 frame only)" : "DWM (screen-wide)",
						((CurrentSettings.RenderBackend == 1)
							? (ShouldShaderPassRun() && GetShaderColorPipeline().IsReady())
							: IsScreenEffectApplied()) ? "YES" : "NO",
						s_deferredInitDone.load() ? "initialized" : "not initialized",
						ToDisplayString(wMode, false),
						hdr ? "YES" : "NO",
						gctx.mapId, gctx.modeNameEn,
						gctx.isInCombat ? "YES" : "NO",
						g_perfMainWindowMs, g_perfSensorGraphMs, g_perfCurvesMs, g_perfFilterLabMs, g_perfTotalImGuiMs,
						g_perfShaderPassMs);

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
			ImGui::PopStyleColor(6);
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
