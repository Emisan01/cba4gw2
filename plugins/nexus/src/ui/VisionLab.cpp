#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "VisionLab.h"
#include "UIState.h"
#include "Settings.h"
#include "Theme.h"
#include "L10n.h"
#include "ColorMatrix.h"
#include "ColorMath.h"
#include "FilterLayers.h"
#include "Shared.h"

#include <imgui.h>
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdio>
#include <string>

namespace cba
{
	namespace
	{
		// ── Procedural Plate Dot Table ──────────────────────────────────────
		struct PlateDot
		{
			float nx, ny;  // Normalized -1.0 to 1.0 within plate radius
			float r;       // Dot radius
			float lumVar;  // Luminance variation (-0.15 to +0.15)
		};

		static std::vector<PlateDot> s_plateDots;
		static bool s_dotsInitialized = false;

		void InitPlateDots()
		{
			if (s_dotsInitialized) return;
			s_plateDots.reserve(150);

			// Deterministic LCG pseudo-random generator
			uint32_t seed = 0x5EED1234;
			auto nextRand = [&]() -> float {
				seed = seed * 1664525u + 1013904223u;
				return (float)(seed & 0x00FFFFFF) / (float)0x01000000;
			};

			int attempts = 0;
			while (s_plateDots.size() < 140 && attempts < 2000)
			{
				attempts++;
				float angle = nextRand() * 6.2831853f;
				float dist = std::sqrt(nextRand()) * 0.90f; // Sqrt for uniform circular distribution
				float x = std::cos(angle) * dist;
				float y = std::sin(angle) * dist;
				float radius = 3.2f + nextRand() * 3.6f;

				// Check minimum distance to existing dots
				bool collision = false;
				for (const auto& d : s_plateDots)
				{
					float dx = (x - d.nx) * 80.0f;
					float dy = (y - d.ny) * 80.0f;
					float minDist = radius + d.r + 1.2f;
					if (dx * dx + dy * dy < minDist * minDist)
					{
						collision = true;
						break;
					}
				}
				if (!collision)
				{
					PlateDot dot;
					dot.nx = x;
					dot.ny = y;
					dot.r = radius;
					dot.lumVar = (nextRand() - 0.5f) * 0.28f;
					s_plateDots.push_back(dot);
				}
			}
			s_dotsInitialized = true;
		}

		// Helper to check if (nx, ny) is inside test shape
		bool IsInsideShape(float nx, float ny, int shapeType)
		{
			// shapeType: 0=Circle, 1=Triangle, 2=Cross, 3=Square
			switch (shapeType)
			{
			case 0: // Circle
				return (nx * nx + ny * ny < 0.22f && nx * nx + ny * ny > 0.04f);
			case 1: // Triangle
			{
				float px = nx;
				float py = ny + 0.08f;
				if (py > 0.35f || py < -0.38f) return false;
				float halfW = (0.35f - py) * 0.65f;
				return (std::abs(px) < halfW && (std::abs(px) > halfW - 0.14f || py > 0.24f));
			}
			case 2: // Cross
			{
				bool inVert = (std::abs(nx) < 0.12f && std::abs(ny) < 0.42f);
				bool inHoriz = (std::abs(ny) < 0.12f && std::abs(nx) < 0.42f);
				return inVert || inHoriz;
			}
			case 3: // Square
			{
				float ax = std::abs(nx);
				float ay = std::abs(ny);
				return (ax < 0.38f && ay < 0.38f && (ax > 0.24f || ay > 0.24f));
			}
			default:
				return false;
			}
		}
	}

	void RenderVisionLabWindow()
	{
		if (!ImGui::GetCurrentContext()) return;
		const L10n& t = Strings();
		bool isDe = cba::IsGerman(); // was a fragile first-letter check - see CLAUDE.md 2026-09-09

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

		ImVec2 disp = ImGui::GetIO().DisplaySize;
		float screenH = (disp.y > 400.0f) ? disp.y : 1080.0f;
		float defaultW = 600.0f;
		float defaultH = std::clamp(screenH * 0.72f, 540.0f, 800.0f);

		ImGui::SetNextWindowSizeConstraints(ImVec2(520.0f, 440.0f), ImVec2(1600.0f, screenH - 40.0f));

		if (s_resetVisionLabWindowPos)
		{
			ImGui::SetNextWindowPos(ImVec2(60.0f, 70.0f), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(defaultW, defaultH), ImGuiCond_Always);
			s_resetVisionLabWindowPos = false;
		}
		else
		{
			ImGui::SetNextWindowSize(ImVec2(defaultW, defaultH), ImGuiCond_FirstUseEver);
		}

		if (s_focusVisionLabWindow)
		{
			ImGui::SetNextWindowFocus();
			s_focusVisionLabWindow = false;
		}

		bool pOpen = CurrentSettings.ShowVisionLabWindow;
		const char* winTitle = isDe ? "cba4gw2 - Vision Lab###CBA_VisionLabWindow" : "cba4gw2 - Vision Lab###CBA_VisionLabWindow";

		if (ImGui::Begin(winTitle, &pOpen, ImGuiWindowFlags_NoCollapse))
		{
			CurrentSettings.ShowVisionLabWindow = pOpen;

			// Header Description
			ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Klinische Farbseh-Pruefung, Anomaloskop & GW2-Praxistest"
			                                                     : "Clinical Color Vision Testing, Anomaloscope & GW2 Usability Lab");
			ImGui::TextDisabled("%s", isDe ? "Optische Verifikation und Abstimmung der CBA-Kompensation nach ophthalmologischen Standards."
			                               : "Optical verification and tuning of CBA color compensation according to clinical standards.");
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Precompute main correction matrix for filter preview in all tabs
			double corrMat[3][3];
			ActiveCorrectionMatrix(corrMat);

			if (ImGui::BeginTabBar("##VisionLabTabs", ImGuiTabBarFlags_None))
			{
				// ══════════════════════════════════════════════════════════════════
				// TAB 1: NAGEL- & MORELAND-ANOMALOSKOP
				// ══════════════════════════════════════════════════════════════════
				if (ImGui::BeginTabItem(isDe ? "1. Anomaloskop (Rayleigh / Moreland)##tab_anom" 
				                             : "1. Anomaloscope (Rayleigh / Moreland)##tab_anom"))
				{
					ImGui::Spacing();
					static int s_anomMode = 0; // 0 = Rayleigh (Rot/Gruen), 1 = Moreland (Blau/Cyan)
					static float s_rayleighMix = 0.50f;  // 0.0 = Rein Gruen (546nm), 1.0 = Rein Rot (671nm)
					static float s_rayleighLuma = 0.55f; // Referenz-Gelb (589nm) Luminanz
					static float s_morelandMix = 0.50f;  // 0.0 = Cyan-Gruen (490nm), 1.0 = Blau (436nm)
					static float s_morelandLuma = 0.55f; // Referenz-Cyan (480nm) Luminanz
					static bool s_anomPreviewFilter = false;

					// Mode selector
					ImGui::TextDisabled("%s:", isDe ? "Test-Gleichung" : "Equation / Mode");
					ImGui::SameLine(0, 8.0f);
					if (ImGui::RadioButton(isDe ? "Rayleigh (Rot/Gruen - Protan/Deutan)" : "Rayleigh (Red/Green - Protan/Deutan)", s_anomMode == 0))
						s_anomMode = 0;
					ImGui::SameLine(0, 12.0f);
					if (ImGui::RadioButton(isDe ? "Moreland (Blau/Cyan - Tritan)" : "Moreland (Blue/Cyan - Tritan)", s_anomMode == 1))
						s_anomMode = 1;

					ImGui::Spacing();

					// Calculate Colors
					float topR = 0, topG = 0, topB = 0;
					float botR = 0, botG = 0, botB = 0;

					if (s_anomMode == 0)
					{
						// Rayleigh: Upper is mixture of Red and Green
						topR = s_rayleighMix;
						topG = (1.0f - s_rayleighMix);
						topB = 0.0f;
						// Lower is reference yellow (589nm)
						botR = s_rayleighLuma;
						botG = s_rayleighLuma * 0.88f;
						botB = 0.0f;
					}
					else
					{
						// Moreland: Upper is mixture of Blue and Green/Cyan
						topR = 0.0f;
						topG = (1.0f - s_morelandMix) * 0.90f;
						topB = s_morelandMix;
						// Lower is reference cyan (480nm)
						botR = 0.0f;
						botG = s_morelandLuma * 0.75f;
						botB = s_morelandLuma * 0.95f;
					}

					// Optional CBA Filter preview on eyepiece
					if (s_anomPreviewFilter && CurrentSettings.Enabled)
					{
						// Snapshot originals first - the old code reassigned
						// topR/botR then used the ALREADY-TRANSFORMED value as
						// input for topG/botG, a corrupted sequential mangle
						// rather than a real matrix-vector multiply (found in
						// the 2026-09-09 codebase review).
						float srcTopR = topR, srcTopG = topG, srcTopB = topB;
						float srcBotR = botR, srcBotG = botG, srcBotB = botB;
						topR = (float)std::clamp(corrMat[0][0]*srcTopR + corrMat[0][1]*srcTopG + corrMat[0][2]*srcTopB, 0.0, 1.0);
						topG = (float)std::clamp(corrMat[1][0]*srcTopR + corrMat[1][1]*srcTopG + corrMat[1][2]*srcTopB, 0.0, 1.0);
						topB = (float)std::clamp(corrMat[2][0]*srcTopR + corrMat[2][1]*srcTopG + corrMat[2][2]*srcTopB, 0.0, 1.0);

						botR = (float)std::clamp(corrMat[0][0]*srcBotR + corrMat[0][1]*srcBotG + corrMat[0][2]*srcBotB, 0.0, 1.0);
						botG = (float)std::clamp(corrMat[1][0]*srcBotR + corrMat[1][1]*srcBotG + corrMat[1][2]*srcBotB, 0.0, 1.0);
						botB = (float)std::clamp(corrMat[2][0]*srcBotR + corrMat[2][1]*srcBotG + corrMat[2][2]*srcBotB, 0.0, 1.0);
					}

					// Draw Eyepiece Canvas
					ImGui::BeginGroup();
					ImVec2 p0 = ImGui::GetCursorScreenPos();
					float eyeRadius = 56.0f;
					float eyeBoxW = eyeRadius * 2.0f + 20.0f;
					float eyeBoxH = eyeRadius * 2.0f + 16.0f;

					ImDrawList* dl = ImGui::GetWindowDrawList();
					ImVec2 center(p0.x + eyeRadius + 10.0f, p0.y + eyeRadius + 8.0f);

					// Outer dark housing
					dl->AddCircleFilled(center, eyeRadius + 4.0f, IM_COL32(16, 20, 28, 255));
					dl->AddCircle(center, eyeRadius + 4.0f, IM_COL32(60, 85, 120, 220), 0, 2.0f);

					// Upper half disc (Mixture)
					ImU32 colTop = IM_COL32((int)(topR * 255), (int)(topG * 255), (int)(topB * 255), 255);
					dl->PathClear();
					dl->PathArcTo(center, eyeRadius, 3.14159265f, 6.2831853f, 32);
					dl->PathFillConvex(colTop);

					// Lower half disc (Reference)
					ImU32 colBot = IM_COL32((int)(botR * 255), (int)(botG * 255), (int)(botB * 255), 255);
					dl->PathClear();
					dl->PathArcTo(center, eyeRadius, 0.0f, 3.14159265f, 32);
					dl->PathFillConvex(colBot);

					// Sharp dividing line
					dl->AddLine(ImVec2(center.x - eyeRadius, center.y), ImVec2(center.x + eyeRadius, center.y), IM_COL32(10, 12, 18, 255), 2.0f);

					// Eyepiece lens reflection glare
					dl->AddCircle(center, eyeRadius, IM_COL32(180, 210, 255, 60), 0, 1.5f);

					ImGui::Dummy(ImVec2(eyeBoxW, eyeBoxH));
					ImGui::EndGroup();

					// Sliders & Readout Column
					ImGui::SameLine(0, 16.0f);
					ImGui::BeginGroup();

					float fullW = ImGui::GetContentRegionAvail().x;
					if (s_anomMode == 0)
					{
						ImGui::TextUnformatted(isDe ? "Obere Haelfte: Rot / Gruen Mischung (546nm - 671nm):" 
						                            : "Upper Half: Red / Green Mixture (546nm - 671nm):");
						ImGui::SetNextItemWidth(fullW);
						ImGui::SliderFloat("##rayleigh_mix", &s_rayleighMix, 0.0f, 1.0f, isDe ? "%.0f%% Rot-Anteil" : "%.0f%% Red Proportion");

						ImGui::Spacing();
						ImGui::TextUnformatted(isDe ? "Untere Haelfte: Referenz-Gelb Helligkeit (589nm):" 
						                            : "Lower Half: Reference Yellow Brightness (589nm):");
						ImGui::SetNextItemWidth(fullW);
						ImGui::SliderFloat("##rayleigh_luma", &s_rayleighLuma, 0.05f, 1.0f, isDe ? "%.0f%% Helligkeit" : "%.0f%% Brightness");
					}
					else
					{
						ImGui::TextUnformatted(isDe ? "Obere Haelfte: Blau / Cyan Mischung (436nm - 490nm):" 
						                            : "Upper Half: Blue / Cyan Mixture (436nm - 490nm):");
						ImGui::SetNextItemWidth(fullW);
						ImGui::SliderFloat("##moreland_mix", &s_morelandMix, 0.0f, 1.0f, isDe ? "%.0f%% Blau-Anteil" : "%.0f%% Blue Proportion");

						ImGui::Spacing();
						ImGui::TextUnformatted(isDe ? "Untere Haelfte: Referenz-Cyan Helligkeit (480nm):" 
						                            : "Lower Half: Reference Cyan Brightness (480nm):");
						ImGui::SetNextItemWidth(fullW);
						ImGui::SliderFloat("##moreland_luma", &s_morelandLuma, 0.05f, 1.0f, isDe ? "%.0f%% Helligkeit" : "%.0f%% Brightness");
					}

					ImGui::Spacing();

					// Empirical Anomalous Quotient (AQ) calculation
					float rawMix = (s_anomMode == 0) ? s_rayleighMix : s_morelandMix;
					float aqVal = 1.0f;
					if (rawMix > 0.01f && rawMix < 0.99f)
					{
						// Inverted ratio: higher red fraction -> lower AQ (< 0.70)
						aqVal = ((1.0f - rawMix) / rawMix);
					}
					else if (rawMix <= 0.01f) aqVal = 12.0f;
					else aqVal = 0.05f;

					const char* diagText = "";
					ImVec4 diagCol = Theme::kTextCyanLicht;

					if (s_anomMode == 0)
					{
						if (aqVal < 0.70f) {
							diagText = isDe ? "Protanomalie (Rotschwaeche - braucht mehr Rot)" : "Protanomaly (Red-weak - requires more red)";
							diagCol = ImVec4(0.95f, 0.45f, 0.35f, 1.0f);
						} else if (aqVal > 1.40f) {
							diagText = isDe ? "Deuteranomalie (Gruenschwaeche - braucht mehr Gruen)" : "Deuteranomaly (Green-weak - requires more green)";
							diagCol = ImVec4(0.40f, 0.90f, 0.50f, 1.0f);
						} else {
							diagText = isDe ? "Normalsichtig (Normaler Farbsinn / Trichromat)" : "Normal Trichromat (Within normal variance)";
							diagCol = Theme::kTextCyanLicht;
						}
					}
					else
					{
						if (s_morelandMix < 0.38f || s_morelandMix > 0.62f) {
							diagText = isDe ? "Tritan-Verschiebung (Blauschwaeche erkannt)" : "Tritan Shift (Blue-axis deviation detected)";
							diagCol = ImVec4(0.45f, 0.75f, 0.95f, 1.0f);
						} else {
							diagText = isDe ? "Normaler Blaurezeptor (Trichromat)" : "Normal Blue Receptor (Trichromat)";
							diagCol = Theme::kTextCyanLicht;
						}
					}

					// Styled HUD Result Box
					ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.15f, 0.90f));
					ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.25f, 0.35f, 0.50f, 0.45f));
					ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));

					float anomBoxH = ImGui::GetTextLineHeightWithSpacing() * 2.0f + 18.0f;
					if (ImGui::BeginChild("##anom_result_box", ImVec2(fullW, anomBoxH), true, ImGuiWindowFlags_NoScrollbar))
					{
						if (s_anomMode == 0)
							ImGui::Text(isDe ? "Errechneter Anomalie-Quotient (AQ): %.2f (Normal: 0.70 - 1.40)" 
							                 : "Calculated Anomalous Quotient (AQ): %.2f (Normal: 0.70 - 1.40)", aqVal);
						else
							ImGui::Text(isDe ? "Moreland-Index: %.2f (Normalbereich: ~0.45 - 0.55)" 
							                 : "Moreland Index: %.2f (Normal range: ~0.45 - 0.55)", s_morelandMix);

						ImGui::TextColored(diagCol, "%s: %s", isDe ? "Befund" : "Diagnostic", diagText);
					}
					ImGui::EndChild();
					ImGui::PopStyleVar(2);
					ImGui::PopStyleColor(2);

					ImGui::EndGroup();

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					// Action Controls Row
					ImGui::Checkbox(isDe ? "CBA-Filter im Okular testen##anom_filter" : "Preview CBA Filter in Eyepiece##anom_filter", &s_anomPreviewFilter);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(isDe ? "Zeigt den Effekt des CBA-Filters direkt im Okular: Bei passendem Filter driften die angleichten Farben auseinander!"
						                       : "Previews the CBA filter effect in the eyepiece: Separates matched colors when filter is active!");

					ImGui::SameLine(0, 14.0f);
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);

					if (ImGui::Button(isDe ? "Als CBA-Profil uebernehmen##apply_anom" : "Apply to CBA Profile##apply_anom", ImVec2(0.0f, 24.0f)))
					{
						EnsureDeferredInitialized();
						CurrentSettings.Enabled = true;
						CurrentSettings.Mixed = false;

						if (s_anomMode == 0)
						{
							if (aqVal < 0.70f)
							{
								CurrentSettings.Type = BalanceType::Protan;
								CurrentSettings.Severity01 = std::clamp((0.70f - aqVal) / 0.55f * 0.70f + 0.35f, 0.35f, 1.25f);
							}
							else if (aqVal > 1.40f)
							{
								CurrentSettings.Type = BalanceType::Deutan;
								CurrentSettings.Severity01 = std::clamp((aqVal - 1.40f) / 2.20f * 0.70f + 0.35f, 0.35f, 1.25f);
							}
							else
							{
								CurrentSettings.Severity01 = 0.20f;
							}
						}
						else
						{
							// Mirrors the Rayleigh branch above: the on-screen
							// diagnostic (line ~355) calls anything within
							// [0.38, 0.62] "Normal Blue Receptor" - this used
							// to have no matching normal-range case here, so
							// even a dead-center 0.50 reading force-applied
							// Tritan at a 40% severity floor (found in the
							// 2026-09-09 codebase review).
							if (s_morelandMix < 0.38f || s_morelandMix > 0.62f)
							{
								CurrentSettings.Type = BalanceType::Tritan;
								float diff = std::abs(s_morelandMix - 0.50f);
								CurrentSettings.Severity01 = std::clamp(diff / 0.35f * 0.80f + 0.40f, 0.40f, 1.25f);
							}
							else
							{
								CurrentSettings.Severity01 = 0.20f;
							}
						}

						CurrentSettings.Save(AddonDir);
						Recompute(/*aForce=*/true);
					}
					ImGui::PopStyleColor(4);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(isDe ? "Uebernimmt die ermittelte Farbbalance und Schweregrad direkt in das aktive CBA-Filterprofil"
						                       : "Transfers the measured color balance and severity directly into the active CBA filter profile");

					ImGui::EndTabItem();
				}

				// ══════════════════════════════════════════════════════════════════
				// TAB 2: BEFUND-UEBERSETZER (CLINICAL DIAGNOSIS TRANSLATOR)
				// ══════════════════════════════════════════════════════════════════
				if (ImGui::BeginTabItem(isDe ? "2. Befund-Uebersetzer##tab_trans" : "2. Clinical Report Translator##tab_trans"))
				{
					ImGui::Spacing();
					ImGui::TextDisabled("%s", isDe ? "Uebersetzt augenaerztliche Diagnosen oder Atteste direkt in optimale Filterwerte."
					                               : "Translates ophthalmological findings or test reports directly into optimal filter values.");
					ImGui::Spacing();

					static int s_repType = 1;      // 0=Protan, 1=Deutan, 2=Tritan
					static int s_repSeverity = 1;  // 0=Leicht, 1=Mittelgradig, 2=Ausgepraegt, 3=Anopie
					static float s_repAqInput = 2.80f;
					static char s_repHrrInput[32] = "6/10";

					float availW = ImGui::GetContentRegionAvail().x;
					bool twoCols = (availW >= 460.0f);
					float colW = twoCols ? (availW - 14.0f) * 0.5f : availW;

					// Left Box: Verbal Classification
					ImGui::BeginGroup();
					ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "A. Verbale Klassifikation (Standard):" : "A. Verbal Classification (Standard):");
					ImGui::Spacing();

					const char* typesDe[] = {
						"Protanomalie (Rotschwaeche)",
						"Deuteranomalie (Gruenschwaeche - haeufigste Form)",
						"Tritanomalie (Blauschwaeche)"
					};
					const char* typesEn[] = {
						"Protanomaly (Red-Weakness)",
						"Deuteranomaly (Green-Weakness - Most Common)",
						"Tritanomaly (Blue-Weakness)"
					};

					ImGui::TextDisabled("%s:", isDe ? "Farbsinnstoerung" : "Type of Deficiency");
					ImGui::SetNextItemWidth(colW);
					ImGui::Combo("##rep_type_combo", &s_repType, isDe ? typesDe : typesEn, 3);

					ImGui::Spacing();
					const char* sevsDe[] = {
						"Leicht / Mild (~35% Kompensation)",
						"Mittelgradig / Moderate (~65% Kompensation)",
						"Ausgepraegt / Severe (~95% Kompensation)",
						"Vollstaendig / Anopie (100% + Kontrast-Boost)"
					};
					const char* sevsEn[] = {
						"Mild (~35% Compensation)",
						"Moderate (~65% Compensation)",
						"Severe (~95% Compensation)",
						"Complete / Anopia (100% + Contrast Boost)"
					};

					ImGui::TextDisabled("%s:", isDe ? "Schweregrad" : "Severity Grade");
					ImGui::SetNextItemWidth(colW);
					ImGui::Combo("##rep_sev_combo", &s_repSeverity, isDe ? sevsDe : sevsEn, 4);

					ImGui::EndGroup();

					// Right Box: Clinical Benchmark Scores
					if (twoCols) ImGui::SameLine(0, 14.0f);
					else ImGui::Spacing();
					ImGui::BeginGroup();
					ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "B. Numerische Messwerte (Optional):" : "B. Numerical Scores (Optional):");
					ImGui::Spacing();

					ImGui::TextDisabled("%s:", isDe ? "Nagel-AQ (Anomalie-Quotient)" : "Nagel-AQ (Anomalous Quotient)");
					ImGui::SetNextItemWidth(colW);
					ImGui::InputFloat("##rep_aq_in", &s_repAqInput, 0.1f, 0.5f, "%.2f");

					ImGui::Spacing();
					ImGui::TextDisabled("%s:", isDe ? "HRR-Tafeln / Ishihara Score" : "HRR Plates / Ishihara Score");
					ImGui::SetNextItemWidth(colW);
					ImGui::InputText("##rep_hrr_in", s_repHrrInput, sizeof(s_repHrrInput));

					ImGui::EndGroup();

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					// ICD-10 Code & Diagnostic Mapping Display
					const char* icdCode = (s_repType == 0) ? (isDe ? "ICD-10 H53.51 (Protanomalie)" : "ICD-10 H53.51 (Protanomaly)") :
					                      (s_repType == 1) ? (isDe ? "ICD-10 H53.52 (Deuteranomalie)" : "ICD-10 H53.52 (Deuteranomaly)") :
					                                         (isDe ? "ICD-10 H53.53 (Tritanomalie)" : "ICD-10 H53.53 (Tritanomaly)");

					ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.15f, 0.90f));
					ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.25f, 0.35f, 0.50f, 0.45f));
					ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));

					float icdCardH = ImGui::GetTextLineHeightWithSpacing() * 4.0f + 20.0f;
					if (ImGui::BeginChild("##icd_summary_card", ImVec2(availW, icdCardH), true, ImGuiWindowFlags_NoScrollbar))
					{
						ImGui::TextColored(Theme::kTextGoldLabel, "%s", icdCode);
						double targetSev = (s_repSeverity == 0) ? 0.35 :
						                   (s_repSeverity == 1) ? 0.65 :
						                   (s_repSeverity == 2) ? 0.95 : 1.25;

						ImGui::Text(isDe ? "Empfohlene Matrix-Kompensation: %.0f%% (Severity01 = %.2f)" 
						                 : "Recommended Matrix Compensation: %.0f%% (Severity01 = %.2f)", targetSev * 100.0, targetSev);

						ImGui::TextDisabled("%s", isDe ? "Entspricht dem klinischen Standardprofil fuer optimale Zapfenunterstuetzung in Guild Wars 2."
						                               : "Matches the clinical baseline profile for optimal cone differentiation in Guild Wars 2.");
					}
					ImGui::EndChild();
					ImGui::PopStyleVar(2);
					ImGui::PopStyleColor(2);

					ImGui::Spacing();

					// Apply Button
					ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
					ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);

					if (ImGui::Button(isDe ? "Befund-Werte in CBA uebernehmen##apply_report" : "Apply Findings to CBA Settings##apply_report", ImVec2(0.0f, 25.0f)))
					{
						EnsureDeferredInitialized();
						CurrentSettings.Enabled = true;
						CurrentSettings.Mixed = false;
						CurrentSettings.Type = (s_repType == 0) ? BalanceType::Protan :
						                       (s_repType == 1) ? BalanceType::Deutan : BalanceType::Tritan;

						CurrentSettings.Severity01 = (s_repSeverity == 0) ? 0.35f :
						                             (s_repSeverity == 1) ? 0.65f :
						                             (s_repSeverity == 2) ? 0.95f : 1.25f;

						CurrentSettings.GammaGain = (s_repSeverity >= 2) ? 1.08f : 1.00f;
						CurrentSettings.Save(AddonDir);
						Recompute(/*aForce=*/true);
					}
					ImGui::PopStyleColor(4);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(isDe ? "Stellt Farbbalance, Kompensationsstaerke und Helligkeit sofort gemaess des Befunds ein"
						                       : "Immediately configures color balance, compensation strength, and brightness according to the findings");

					ImGui::EndTabItem();
				}

				// ══════════════════════════════════════════════════════════════════
				// TAB 3: FARBTAFELN (PSEUDOISOCHROMATISCHE TAFELN / HRR & ISHIHARA)
				// ══════════════════════════════════════════════════════════════════
				if (ImGui::BeginTabItem(isDe ? "3. Farbtafeln (HRR / Ishihara)##tab_plates" : "3. Color Plates (HRR / Ishihara)##tab_plates"))
				{
					InitPlateDots();
					ImGui::Spacing();

					static int s_plateType = 0; // 0=Protan/Deutan, 1=Tritan, 2=Feiner Kontrast
					static int s_shapeType = 0; // 0=Kreis, 1=Dreieck, 2=Kreuz, 3=Quadrat
					static bool s_plateFilter = false;
					static int s_lastAnswer = -1;
					static bool s_answered = false;

					ImGui::TextDisabled("%s:", isDe ? "Tafel-Typ" : "Plate Type");
					ImGui::SameLine(0, 8.0f);
					if (ImGui::RadioButton(isDe ? "Rot / Gruen (Protan / Deutan)##plt_rg" : "Red / Green (Protan / Deutan)##plt_rg", s_plateType == 0))
					{
						s_plateType = 0;
						s_answered = false;
					}
					ImGui::SameLine(0, 10.0f);
					if (ImGui::RadioButton(isDe ? "Blau / Cyan (Tritan)##plt_by" : "Blue / Cyan (Tritan)##plt_by", s_plateType == 1))
					{
						s_plateType = 1;
						s_answered = false;
					}

					ImGui::Spacing();

					// Base Colors for plate
					float bgR = 0.55f, bgG = 0.52f, bgB = 0.42f; // Neutral grey-beige
					float fgR = 0.82f, fgG = 0.40f, fgB = 0.32f; // Orange-red test shape

					if (s_plateType == 1)
					{
						bgR = 0.45f; bgG = 0.48f; bgB = 0.60f; // Soft bluish grey
						fgR = 0.20f; fgG = 0.72f; fgB = 0.78f; // Cyan-green shape
					}

					// Render Dot Plate
					ImGui::BeginGroup();
					ImVec2 p0 = ImGui::GetCursorScreenPos();
					float plateR = 82.0f;
					float plateBoxW = plateR * 2.0f + 16.0f;
					float plateBoxH = plateR * 2.0f + 16.0f;

					ImDrawList* dl = ImGui::GetWindowDrawList();
					ImVec2 center(p0.x + plateR + 8.0f, p0.y + plateR + 8.0f);

					// Plate background disc
					dl->AddCircleFilled(center, plateR + 3.0f, IM_COL32(18, 22, 30, 255));
					dl->AddCircle(center, plateR + 3.0f, IM_COL32(70, 95, 135, 180), 0, 2.0f);

					for (const auto& dot : s_plateDots)
					{
						bool inside = IsInsideShape(dot.nx, dot.ny, s_shapeType);
						float r = inside ? fgR : bgR;
						float g = inside ? fgG : bgG;
						float b = inside ? fgB : bgB;

						// Apply luminance jitter
						r = std::clamp(r + dot.lumVar, 0.0f, 1.0f);
						g = std::clamp(g + dot.lumVar, 0.0f, 1.0f);
						b = std::clamp(b + dot.lumVar, 0.0f, 1.0f);

						// Apply CBA filter preview if toggled
						if (s_plateFilter && CurrentSettings.Enabled)
						{
							float cr = (float)std::clamp(corrMat[0][0]*r + corrMat[0][1]*g + corrMat[0][2]*b, 0.0, 1.0);
							float cg = (float)std::clamp(corrMat[1][0]*r + corrMat[1][1]*g + corrMat[1][2]*b, 0.0, 1.0);
							float cb = (float)std::clamp(corrMat[2][0]*r + corrMat[2][1]*g + corrMat[2][2]*b, 0.0, 1.0);
							r = cr; g = cg; b = cb;
						}

						ImVec2 dotPos(center.x + dot.nx * plateR, center.y + dot.ny * plateR);
						dl->AddCircleFilled(dotPos, dot.r, IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255));
					}

					ImGui::Dummy(ImVec2(plateBoxW, plateBoxH));
					ImGui::EndGroup();

					// Right side: Quiz Controls & Filter Switch
					float availPlates = ImGui::GetContentRegionAvail().x;
					if (availPlates >= 410.0f) ImGui::SameLine(0, 16.0f);
					else ImGui::Spacing();
					ImGui::BeginGroup();

					ImGui::TextDisabled("%s:", isDe ? "Welche geometrische Form ist in der Tafel zu sehen?" 
					                                : "Which geometric shape is hidden in the plate?");
					ImGui::Spacing();

					auto quizBtn = [&](const char* lbl, int shapeIdx) {
						if (ImGui::Button(lbl, ImVec2(92.0f, 23.0f)))
						{
							s_lastAnswer = shapeIdx;
							s_answered = true;
						}
					};

					quizBtn(isDe ? "Kreis##q0" : "Circle##q0", 0);
					ImGui::SameLine(0, 6.0f);
					quizBtn(isDe ? "Dreieck##q1" : "Triangle##q1", 1);

					quizBtn(isDe ? "Kreuz##q2" : "Cross##q2", 2);
					ImGui::SameLine(0, 6.0f);
					quizBtn(isDe ? "Quadrat##q3" : "Square##q3", 3);

					if (ImGui::Button(isDe ? "Nichts erkennbar##q4" : "Nothing visible##q4", ImVec2(190.0f, 23.0f)))
					{
						s_lastAnswer = -1;
						s_answered = true;
					}

					ImGui::Spacing();
					if (s_answered)
					{
						if (s_lastAnswer == s_shapeType)
						{
							ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.50f, 1.0f), "%s", 
								isDe ? "Richtig erkannt! Die Kontrastgrenze ist fuer dein Auge differenzierbar." 
								     : "Correct! The contrast boundary is discernible to your vision.");
						}
						else
						{
							ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.35f, 1.0f), "%s", 
								isDe ? "Schwer erkennbar oder verwechselt. Aktiviere den CBA-Filter unten zum Vergleich!" 
								     : "Hard to distinguish. Activate the CBA filter below for comparison!");
						}
					}

					ImGui::Spacing();
					ImGui::Checkbox(isDe ? "CBA-Filter auf Tafel anwenden##plt_flt" : "Apply CBA Filter to Plate##plt_flt", &s_plateFilter);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(isDe ? "Demonstriert, wie die Verwechslungsfarben durch die CBA-Matrix verschoben werden, sodass die Form hervorspringt"
						                       : "Demonstrates how confusion colors are shifted by the CBA matrix so the shape becomes prominent");

					ImGui::SameLine(0, 10.0f);
					if (ImGui::Button(isDe ? "Naechste Form##next_shape" : "Next Shape##next_shape", ImVec2(0.0f, 23.0f)))
					{
						s_shapeType = (s_shapeType + 1) % 4;
						s_answered = false;
					}

					ImGui::EndGroup();

					ImGui::EndTabItem();
				}

				// ══════════════════════════════════════════════════════════════════
				// TAB 4: GW2 PRAXISTEST (GAME USABILITY BENCH)
				// ══════════════════════════════════════════════════════════════════
				if (ImGui::BeginTabItem(isDe ? "4. GW2 Praxistest & Usability##tab_gw2" : "4. GW2 Usability Bench##tab_gw2"))
				{
					ImGui::Spacing();
					static int s_sceneIdx = 0; // 0=AoE Ring, 1=Com Tags, 2=HP Bars, 3=Targeting
					static float s_ambientLight = 0.85f; // Day / Dusk / Night in Tyria

					ImGui::TextDisabled("%s:", isDe ? "Spielsituation" : "Game Scenario");
					ImGui::SameLine(0, 8.0f);
					const char* scenesDe[] = {
						"1. AoE-Gefahrenzone auf WvW-Gras",
						"2. Commander-Tag Pulk",
						"3. Lebensbalken & Zustaende (HP/Downed)",
						"4. Zielerfassung (Feind vs Verbuendeter)"
					};
					const char* scenesEn[] = {
						"1. Red AoE Ring on WvW Grass",
						"2. Commander Tag Stack",
						"3. Health Bars & States (HP/Downed)",
						"4. Target Reticle (Enemy vs Ally)"
					};
					ImGui::SetNextItemWidth(310.0f);
					ImGui::Combo("##scene_combo", &s_sceneIdx, isDe ? scenesDe : scenesEn, 4);

					ImGui::SameLine(0, 14.0f);
					ImGui::TextDisabled("%s:", isDe ? "Licht" : "Light");
					ImGui::SameLine(0, 6.0f);
					ImGui::SetNextItemWidth(120.0f);
					ImGui::SliderFloat("##ambient_slider", &s_ambientLight, 0.40f, 1.0f, isDe ? "%.0f%% Licht" : "%.0f%% Light");

					ImGui::Spacing();

					float availW = ImGui::GetContentRegionAvail().x;
					float cardW = (availW - 12.0f) * 0.5f;
					if (cardW < 220.0f) cardW = availW;
					float cardH = 150.0f;

					// Render a scenario card
					auto drawScenario = [&](bool withFilter, const char* cardId, const char* titleText) {
						ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 0.95f));
						ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.25f, 0.35f, 0.50f, 0.45f));
						ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));

						if (ImGui::BeginChild(cardId, ImVec2(cardW, cardH), true, ImGuiWindowFlags_NoScrollbar))
						{
							ImGui::TextDisabled("%s", titleText);
							ImVec2 sp = ImGui::GetCursorScreenPos();
							ImDrawList* cdl = ImGui::GetWindowDrawList();
							float canvasW = cardW - 16.0f;
							float canvasH = cardH - 34.0f;

							auto transformCol = [&](float r, float g, float b, float alpha = 1.0f) -> ImU32 {
								// Ambient scale
								r *= s_ambientLight;
								g *= s_ambientLight;
								b *= s_ambientLight;

								if (withFilter && CurrentSettings.Enabled)
								{
									float cr = (float)std::clamp(corrMat[0][0]*r + corrMat[0][1]*g + corrMat[0][2]*b, 0.0, 1.0);
									float cg = (float)std::clamp(corrMat[1][0]*r + corrMat[1][1]*g + corrMat[1][2]*b, 0.0, 1.0);
									float cb = (float)std::clamp(corrMat[2][0]*r + corrMat[2][1]*g + corrMat[2][2]*b, 0.0, 1.0);
									r = cr; g = cg; b = cb;
								}
								else
								{
									// Simulate CVD perception
									double sr = r, sg = g, sb = b;
									ColorMatrix::SimulatePixel(r, g, b, CurrentSettings.Type, sr, sg, sb);
									r = (float)sr; g = (float)sg; b = (float)sb;
								}
								return IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), (int)(alpha*255));
							};

							// Draw background terrain
							ImU32 terrainCol = transformCol(0.24f, 0.45f, 0.22f); // GW2 grass green
							cdl->AddRectFilled(sp, ImVec2(sp.x + canvasW, sp.y + canvasH), terrainCol, 3.0f);

							if (s_sceneIdx == 0) // Scenario 1: Red AoE on Grass
							{
								ImVec2 aoeCenter(sp.x + canvasW * 0.5f, sp.y + canvasH * 0.5f);
								float aoeRadius = 38.0f;
								ImU32 aoeFill = transformCol(0.85f, 0.22f, 0.18f, 0.35f);
								ImU32 aoeRing = transformCol(0.95f, 0.15f, 0.10f, 0.95f);

								cdl->AddCircleFilled(aoeCenter, aoeRadius, aoeFill);
								cdl->AddCircle(aoeCenter, aoeRadius, aoeRing, 0, 2.5f);
								cdl->AddCircle(aoeCenter, aoeRadius * 0.65f, aoeRing, 0, 1.2f);
							}
							else if (s_sceneIdx == 1) // Scenario 2: Tag Stack
							{
								float tagRadius = 14.0f;
								float cx = sp.x + canvasW * 0.5f;
								float cy = sp.y + canvasH * 0.5f;

								ImU32 tRed = transformCol(0.92f, 0.25f, 0.20f);
								ImU32 tGrn = transformCol(0.22f, 0.78f, 0.32f);
								ImU32 tYel = transformCol(0.95f, 0.85f, 0.15f);
								ImU32 tBlu = transformCol(0.20f, 0.50f, 0.92f);
								ImU32 tPrp = transformCol(0.70f, 0.28f, 0.90f);

								cdl->AddCircleFilled(ImVec2(cx - 24.0f, cy - 10.0f), tagRadius, tRed);
								cdl->AddCircleFilled(ImVec2(cx + 24.0f, cy - 10.0f), tagRadius, tGrn);
								cdl->AddCircleFilled(ImVec2(cx, cy + 12.0f), tagRadius, tYel);
								cdl->AddCircleFilled(ImVec2(cx - 36.0f, cy + 14.0f), tagRadius, tBlu);
								cdl->AddCircleFilled(ImVec2(cx + 36.0f, cy + 14.0f), tagRadius, tPrp);
							}
							else if (s_sceneIdx == 2) // Scenario 3: Health Bars
							{
								float barW = canvasW - 40.0f;
								float barH = 14.0f;
								float bx = sp.x + 20.0f;

								// Full HP (Green)
								cdl->AddRectFilled(ImVec2(bx, sp.y + 16.0f), ImVec2(bx + barW, sp.y + 16.0f + barH), transformCol(0.25f, 0.80f, 0.28f), 2.0f);
								// Downed (Red)
								cdl->AddRectFilled(ImVec2(bx, sp.y + 42.0f), ImVec2(bx + barW * 0.65f, sp.y + 42.0f + barH), transformCol(0.90f, 0.22f, 0.20f), 2.0f);
								// Barrier (Yellow-Gold)
								cdl->AddRectFilled(ImVec2(bx, sp.y + 68.0f), ImVec2(bx + barW * 0.85f, sp.y + 68.0f + barH), transformCol(0.95f, 0.78f, 0.15f), 2.0f);
							}
							else // Scenario 4: Target Reticle
							{
								ImVec2 fCenter(sp.x + canvasW * 0.32f, sp.y + canvasH * 0.5f);
								ImVec2 aCenter(sp.x + canvasW * 0.68f, sp.y + canvasH * 0.5f);
								float r = 24.0f;

								// Foe (Red)
								ImU32 foeCol = transformCol(0.95f, 0.20f, 0.18f);
								cdl->AddCircle(fCenter, r, foeCol, 0, 2.0f);
								cdl->AddLine(ImVec2(fCenter.x - r - 4, fCenter.y), ImVec2(fCenter.x + r + 4, fCenter.y), foeCol, 1.5f);
								cdl->AddLine(ImVec2(fCenter.x, fCenter.y - r - 4), ImVec2(fCenter.x, fCenter.y + r + 4), foeCol, 1.5f);

								// Ally (Blue/Cyan)
								ImU32 allyCol = transformCol(0.20f, 0.75f, 0.95f);
								cdl->AddCircle(aCenter, r, allyCol, 0, 2.0f);
								cdl->AddLine(ImVec2(aCenter.x - r - 4, aCenter.y), ImVec2(aCenter.x + r + 4, aCenter.y), allyCol, 1.5f);
								cdl->AddLine(ImVec2(aCenter.x, aCenter.y - r - 4), ImVec2(aCenter.x, aCenter.y + r + 4), allyCol, 1.5f);
							}
						}
						ImGui::EndChild();
						ImGui::PopStyleVar(2);
						ImGui::PopStyleColor(2);
					};

					drawScenario(false, "##gw2_card_raw", isDe ? "Ohne CBA-Filter (CVD-Simulation)" : "Without Filter (CVD Simulation)");
					if (cardW < availW) ImGui::SameLine(0, 12.0f);
					else ImGui::Spacing();
					drawScenario(true, "##gw2_card_cba", isDe ? "Mit CBA-Filter (Kompensation & Boost)" : "With CBA Filter (Compensation & Boost)");

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(6);
	}
}
