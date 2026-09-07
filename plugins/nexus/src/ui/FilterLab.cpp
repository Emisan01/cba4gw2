#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FilterLab.h"
#include "UIState.h"
#include "Theme.h"
#include "L10n.h"
#include "ColorMatrix.h"
#include "ColorMath.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace cba
{
	// ── Kontrast-Kombinationen (Überlappende Farbfelder Widget) ────────────────
	void DrawContrastCombinationsWidget(bool isDe, bool& changed, bool& saveNeeded)
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

		// Compute CBA Correction for pair
		double corrMat[3][3];
		if (CurrentSettings.Mixed)
			ColorMatrix::MixedCorrectionMatrix(CurrentSettings.MixedRgSeverity01, CurrentSettings.MixedBySeverity01, corrMat);
		else
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, corrMat);

		float cor1R = (float)std::clamp(corrMat[0][0]*p.r1 + corrMat[0][1]*p.g1 + corrMat[0][2]*p.b1, 0.0, 1.0);
		float cor1G = (float)std::clamp(corrMat[1][0]*p.r1 + corrMat[1][0]*p.g1 + corrMat[1][2]*p.b1, 0.0, 1.0);
		float cor1B = (float)std::clamp(corrMat[2][0]*p.r1 + corrMat[2][1]*p.g1 + corrMat[2][2]*p.b1, 0.0, 1.0);

		float cor2R = (float)std::clamp(corrMat[0][0]*p.r2 + corrMat[0][1]*p.g2 + corrMat[0][2]*p.b2, 0.0, 1.0);
		float cor2G = (float)std::clamp(corrMat[1][0]*p.r2 + corrMat[1][0]*p.g2 + corrMat[1][2]*p.b2, 0.0, 1.0);
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
		}
		ImGui::EndChild();

		if (cardW < availW) ImGui::SameLine(0, 12.0f);
		else ImGui::Spacing();

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
		}
		ImGui::EndChild();

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
	void DrawFilterLabWidget(bool isDe, bool& changed, bool& saveNeeded)
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
					if (i > 0) ImGui::SameLine(0, 5.0f);
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

					char btnLbl[80];
					std::snprintf(btnLbl, sizeof(btnLbl), "    %s %s", CurrentSettings.LabFilters[i].Enabled ? "[x]" : "[ ]", CurrentSettings.LabFilters[i].Name.c_str());
					if (ImGui::Button(btnLbl, ImVec2(0.0f, 23.0f)))
					{
						CurrentSettings.SelectedLabFilterIndex = i;
						selIdx = i;
						saveNeeded = true;
					}

					// Draw dual-color indicator dot inside chip: center = target color, border = replacement color
					ImVec2 bMin = ImGui::GetItemRectMin();
					ImVec2 bMax = ImGui::GetItemRectMax();
					float swatchY = (bMin.y + bMax.y) * 0.5f;
					float swatchX = bMin.x + 9.0f;
					ImU32 tCol = IM_COL32((int)(CurrentSettings.LabFilters[i].TargetRgb[0] * 255),
					                      (int)(CurrentSettings.LabFilters[i].TargetRgb[1] * 255),
					                      (int)(CurrentSettings.LabFilters[i].TargetRgb[2] * 255), 255);
					ImU32 rCol = IM_COL32((int)(CurrentSettings.LabFilters[i].ReplaceRgb[0] * 255),
					                      (int)(CurrentSettings.LabFilters[i].ReplaceRgb[1] * 255),
					                      (int)(CurrentSettings.LabFilters[i].ReplaceRgb[2] * 255), 255);
					ImDrawList* dlChips = ImGui::GetWindowDrawList();
					dlChips->AddCircleFilled(ImVec2(swatchX, swatchY), 4.5f, tCol);
					dlChips->AddCircle(ImVec2(swatchX, swatchY), 4.5f, rCol, 0, 1.6f);

					ImGui::PopStyleColor(4);
					ImGui::PopID();
				}

				ImGui::SameLine(0, 6.0f);
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
				if (ImGui::Button("+##add_lab_filter", ImVec2(26.0f, 23.0f)))
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
				ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "XY Farbstrahl-Matrix (Hue -> Luma):" : "XY Color Ray Matrix (Hue -> Luma):");

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
					dl->AddLine(ImVec2(ox, gy), ImVec2(tx, gy), IM_COL32(25, 40, 58, 70), 1.0f);
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

				// Draw each filter as a color ray originating from (ox, oy)
				for (size_t fIdx = 0; fIdx < CurrentSettings.LabFilters.size(); ++fIdx)
				{
					const auto& f = CurrentSettings.LabFilters[fIdx];
					if (!f.Enabled && (int)fIdx != selIdx) continue;

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
						float coneW = (f.ToleranceTones / 32.0f) * 20.0f;
						float diffW = coneW + f.Diffusion * 14.0f;

						if (f.Diffusion > 0.01f) {
							dl->AddTriangleFilled(ImVec2(ox, oy), ImVec2(rx - diffW, ry), ImVec2(rx + diffW, ry),
								IM_COL32((int)(f.TargetRgb[0]*255), (int)(f.TargetRgb[1]*255), (int)(f.TargetRgb[2]*255), 35));
						}
						dl->AddTriangleFilled(ImVec2(ox, oy), ImVec2(rx - coneW, ry), ImVec2(rx + coneW, ry),
							IM_COL32(255, 215, 60, 45));

						dl->AddLine(ImVec2(ox, oy), ImVec2(rx, ry), colReplace, 2.5f);

						float pulse = 6.0f + 3.0f * std::sin((float)ImGui::GetTime() * 4.5f);
						dl->AddCircle(ImVec2(rx, ry), pulse, IM_COL32(0, 230, 210, 180), 0, 1.2f);
						dl->AddCircleFilled(ImVec2(rx, ry), 5.0f, colReplace);
						dl->AddCircle(ImVec2(rx, ry), 5.0f, IM_COL32(10, 16, 26, 255), 0, 1.0f);
					}
					else
					{
						dl->AddLine(ImVec2(ox, oy), ImVec2(rx, ry), colTarget, 1.5f);
						dl->AddCircleFilled(ImVec2(rx, ry), 3.5f, colReplace);
					}
				}

				float dr = curF.TargetRgb[0] - curF.ReplaceRgb[0];
				float dg = curF.TargetRgb[1] - curF.ReplaceRgb[1];
				float db = curF.TargetRgb[2] - curF.ReplaceRgb[2];
				float dScore = std::sqrt(dr*dr + dg*dg + db*db);
				char dScoreBuf[64];
				std::snprintf(dScoreBuf, sizeof(dScoreBuf), "Delta-E: %.2f", dScore);
				ImVec2 dSize = ImGui::CalcTextSize(dScoreBuf);

				// Sleek Delta-E badge in top-right corner of canvas
				float badgeX = tx - dSize.x - 10.0f;
				float badgeY = p0.y + 6.0f;
				dl->AddRectFilled(ImVec2(badgeX - 5.0f, badgeY - 2.0f), ImVec2(badgeX + dSize.x + 5.0f, badgeY + dSize.y + 2.0f), IM_COL32(12, 20, 32, 220), 4.0f);
				dl->AddRect(ImVec2(badgeX - 5.0f, badgeY - 2.0f), ImVec2(badgeX + dSize.x + 5.0f, badgeY + dSize.y + 2.0f), IM_COL32(26, 75, 105, 200), 4.0f);
				dl->AddText(ImVec2(badgeX, badgeY), IM_COL32(140, 235, 230, 240), dScoreBuf);

				ImGui::Dummy(ImVec2(rayCanvasW, canvasH));
				ImGui::EndGroup();

				// Sliders for Precision Radius & Diffusion
				ImGui::Spacing();
				float fullW = ImGui::GetContentRegionAvail().x;
				ImGui::TextUnformatted(isDe ? "2. Begrenzungsradius & Praezision (Schwellenwert):" 
				                            : "2. Boundary Radius & Precision (Threshold):");
				ImGui::SetNextItemWidth(fullW);
				bool hasTol = (curF.ToleranceTones > 0);
				if (hasTol) {
					ImGui::PushStyleColor(ImGuiCol_SliderGrab,       Theme::kBtnStateActiveIdle);
					ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Theme::kBtnStateActiveHover);
				}
				if (ImGui::SliderInt("##lab_tol_slider", &curF.ToleranceTones, 0, 32, isDe ? "+/- %d Farbtoene (Hex-Toleranz)" : "+/- %d Color Tones (Hex-Tolerance)"))
				{
					UpdateTagEnhancerConflicts();
					changed = true;
				}
				if (hasTol) ImGui::PopStyleColor(2);
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::Spacing();
				ImGui::TextUnformatted(isDe ? "3. Weiche Kanten / Diffusion (Feathering):" 
				                            : "3. Soft Diffusion / Smooth Edge (Feathering):");
				ImGui::SetNextItemWidth(fullW);
				bool hasDiff = (curF.Diffusion > 0.01f);
				if (hasDiff) {
					ImGui::PushStyleColor(ImGuiCol_SliderGrab,       Theme::kBtnStateActiveIdle);
					ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Theme::kBtnStateActiveHover);
				}
				if (ImGui::SliderFloat("##lab_diff_slider", &curF.Diffusion, 0.0f, 1.0f, "%.0f%% (Diffusion)"))
				{
					UpdateTagEnhancerConflicts();
					changed = true;
				}
				if (hasDiff) ImGui::PopStyleColor(2);
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
				if (ImGui::Button(isDe ? "Filter duplizieren" : "Duplicate Filter", ImVec2(0.0f, 24.0f)))
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
					if (ImGui::Button(isDe ? "Filter loeschen" : "Delete Filter", ImVec2(0.0f, 24.0f)))
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

				float availAct = ImGui::GetContentRegionAvail().x;
				bool fitSideBySide = (availAct >= 430.0f);

				// Auto-Complementary (CVD Opt)
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
				if (ImGui::Button(isDe ? "Auto-Komplementaer (CVD Opt)##lab" : "Auto-Complementary (CVD Opt)##lab", ImVec2(fitSideBySide ? 0.0f : availAct, 26.0f)))
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

				if (fitSideBySide) ImGui::SameLine(0, 10.0f);
				else ImGui::Spacing();

				// Auto-Luminance
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
				if (ImGui::Button(isDe ? "Auto-Luminanz (WCAG)##lab" : "Auto-Luminance (WCAG)##lab", ImVec2(fitSideBySide ? 0.0f : availAct, 26.0f)))
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
}
