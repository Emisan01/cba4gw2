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

	void ResetMainWindowState()
	{
		s_setupStep = 0;
		s_setupAxisRedGreen = true;
		s_setupPendingType = BalanceType::Deutan;
		s_setupPendingMixed = false;
		s_setupStrength = 0.6f;
		s_setupDismissed = false;
	}

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

	void RenderDashboardTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded)
	{
		// Tile: Curve View (moved to the top 2026-09-13, Emi: the graph
		// is what people look at first)
		{
		cba::ScopedChild tileCurveView("Tile_CurveView", ImVec2(0, 260), true, ImGuiWindowFlags_MenuBar);
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
		
		}

		ImGui::Spacing();

		// Tile: Commander Tag
		{
		cba::ScopedChild tileComTag("Tile_ComTag", ImVec2(0, 320), true, ImGuiWindowFlags_MenuBar);
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

		
		}

		ImGui::Spacing();

		// Tile: Vision Assessment (Sehtest) & Profil - moved to sit
		// directly above the profile-management tile (2026-09-13)
		{
		cba::ScopedChild tileProfile("Tile_Profile", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);
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

		}

		ImGui::Spacing();

		// Tile: Profile Management - save/load slots, auto-start and
		// profile export/import all live here now, nowhere else
		// (2026-09-13, Emi: "die ganze Profil-Logik auf dieses Tile").
		{
		cba::ScopedChild tileProfileMgr("Tile_ProfileManager", ImVec2(0, 360), true, ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar()) { ImGui::TextColored(Theme::kTextCyanLicht, isDe ? "Profil-Verwaltung" : "Profile Management"); ImGui::EndMenuBar(); }
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

			// Preset Exchange - a visible field instead of two blind clipboard
			// buttons (2026-09-13, Emi: "ein Ein- und Ausgabefeld... damit
			// man eine visuelle Unterstuetzung hat"). "Anwenden" reads
			// whatever is actually in the field, not the OS clipboard
			// directly, so pasting in a foreign profile is a two-step,
			// see-before-you-apply action rather than a blind one.
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Profil-Code (Kopieren / Einfuegen):" : "Profile Code (Copy / Paste):");
			ImGui::Spacing();

			static char s_profileCodeBuf[512] = "";
			static bool s_profileCodeInit = false;
			if (!s_profileCodeInit)
			{
				std::string cur = CurrentSettings.ExportPresetString();
				std::snprintf(s_profileCodeBuf, sizeof(s_profileCodeBuf), "%s", cur.c_str());
				s_profileCodeInit = true;
			}

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 180.0f);
			ImGui::InputText("##profile_code_field", s_profileCodeBuf, sizeof(s_profileCodeBuf));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe
					? "Dein Farbprofil als Text. Zum Teilen kopieren, oder ein fremdes Profil hier einfuegen und anwenden."
					: "Your color profile as text. Copy it to share, or paste someone else's profile here and apply it.");
			}

			ImGui::SameLine(0, 6.0f);
			if (ImGui::Button(isDe ? "Kopieren##exp" : "Copy##exp", ImVec2(78.0f, 0.0f)))
			{
				std::string expStr = CurrentSettings.ExportPresetString();
				std::snprintf(s_profileCodeBuf, sizeof(s_profileCodeBuf), "%s", expStr.c_str());
				ImGui::SetClipboardText(expStr.c_str());
				s_profileFeedbackTime = std::chrono::steady_clock::now();
				s_profileFeedbackMsg = isDe ? "[OK] Profil in Zwischenablage kopiert!" : "[OK] Profile copied to clipboard!";
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe
					? "Aktualisiert das Feld mit deinem aktuellen Profil und kopiert es in die Zwischenablage."
					: "Refreshes the field with your current profile and copies it to the clipboard.");
			}

			ImGui::SameLine(0, 4.0f);
			if (ImGui::Button(isDe ? "Anwenden##imp" : "Apply##imp", ImVec2(82.0f, 0.0f)))
			{
				std::string err;
				if (CurrentSettings.ImportPresetString(s_profileCodeBuf, &err))
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
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe
					? "Wendet den Text im Feld links als CBA-Profil (CBA1:...) an."
					: "Applies the text in the field on the left as a CBA profile (CBA1:...).");
			}
		}
		}

		ImGui::Spacing();

		// Tile: Active Functions - mirrors the Sensor Graph HUD's own
		// status chip row (2026-09-13, shared via RenderActiveModulesChips).
		{
		cba::ScopedChild tileActiveFns("Tile_ActiveFunctions", ImVec2(0, 70), true, ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar()) { ImGui::TextColored(Theme::kTextCyanLicht, isDe ? "Aktive Funktionen" : "Active Functions"); ImGui::EndMenuBar(); }
		cba::RenderActiveModulesChips(isDe);
		}
	}
}
