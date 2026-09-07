#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SensorGraphHUD.h"
#include "UIState.h"
#include "Theme.h"
#include "L10n.h"
#include "ColorMatrix.h"
#include "HybridScanner.h"
#include "ColorEffectController.h"

#include <imgui.h>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace cba
{
	// ── Mode 1: Spektrale Transferfunktion (Polygonal / PWL) ─────────────────
	static void DrawCurvePanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                           const double aM[3][3], bool aIsDetached, float aOpacity)
	{
		const float pad = 8.0f;
		const float labelSpaceLeft = 28.0f;
		const float labelSpaceBottom = 20.0f;
		const float badgeSpaceRight = 6.0f;

		const float plotX = aOrigin.x + pad + labelSpaceLeft;
		const float plotY = aOrigin.y + pad + 4.0f;
		const float plotW = aW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		const float plotH = aH - pad * 2 - labelSpaceBottom - 4.0f;

		// Panel background: transparent glass when detached, deep slate when docked
		float panelA = aIsDetached ? std::clamp(aOpacity * 0.92f, 0.0f, 0.92f) : 0.92f;
		int bgAlpha = (int)(panelA * 255.0f);
		int borderAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f, 20.0f, 150.0f)) : 150;

		aDraw->AddRectFilled(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(12, 15, 22, bgAlpha), 6.0f);
		aDraw->AddRect(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(65, 85, 125, borderAlpha), 6.0f, 0, 1.2f);

		// Grid with axis tick labels
		int gridAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 65.0f, 15.0f, 65.0f)) : 65;
		int textAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f + 60.0f, 60.0f, 210.0f)) : 210;

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
	}

	// ── Mode 2: Harmonische Resonanz (Gauß / Sinusoidale LMS-Wellen) ──────────
	static void DrawHarmonicCurvePanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                                   const double aM[3][3], bool aIsDetached, float aOpacity)
	{
		const float pad = 8.0f;
		const float labelSpaceLeft = 28.0f;
		const float labelSpaceBottom = 20.0f;
		const float badgeSpaceRight = 6.0f;

		const float plotX = aOrigin.x + pad + labelSpaceLeft;
		const float plotY = aOrigin.y + pad + 4.0f;
		const float plotW = aW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		const float plotH = aH - pad * 2 - labelSpaceBottom - 4.0f;

		float panelA = aIsDetached ? std::clamp(aOpacity * 0.92f, 0.0f, 0.92f) : 0.92f;
		int bgAlpha = (int)(panelA * 255.0f);
		int borderAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f, 20.0f, 150.0f)) : 150;

		aDraw->AddRectFilled(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(12, 15, 22, bgAlpha), 6.0f);
		aDraw->AddRect(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(65, 85, 125, borderAlpha), 6.0f, 0, 1.2f);

		int gridAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 65.0f, 15.0f, 65.0f)) : 65;
		int textAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f + 60.0f, 60.0f, 210.0f)) : 210;

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

		aDraw->AddRect(ImVec2(plotX, plotY), ImVec2(plotX + plotW, plotY + plotH), IM_COL32(70, 90, 130, borderAlpha), 0.0f, 0, 1.0f);
		aDraw->PushClipRect(ImVec2(plotX - 0.5f, plotY - 0.5f), ImVec2(plotX + plotW + 0.5f, plotY + plotH + 0.5f), true);

		constexpr int kSteps = 64;
		static const ImU32 kChanCol[3] = {
			IM_COL32(255, 75, 75, 255),
			IM_COL32(65, 240, 110, 255),
			IM_COL32(75, 170, 255, 255)
		};
		static const ImU32 kChanGlow[3] = {
			IM_COL32(255, 75, 75, 50),
			IM_COL32(65, 240, 110, 50),
			IM_COL32(75, 170, 255, 50)
		};

		auto sampleHarmonicLMS = [](float u, float& r0, float& g0, float& b0) {
			float db = (u - 0.22f);
			b0 = std::exp(-(db * db) / 0.035f);

			float dg = (u - 0.50f);
			g0 = std::exp(-(dg * dg) / 0.040f);

			float dr1 = (u - 0.78f);
			float dr2 = (u - 0.02f);
			r0 = std::exp(-(dr1 * dr1) / 0.045f) + 0.25f * std::exp(-(dr2 * dr2) / 0.012f);

			r0 = std::clamp(r0, 0.0f, 1.0f);
			g0 = std::clamp(g0, 0.0f, 1.0f);
			b0 = std::clamp(b0, 0.0f, 1.0f);
		};

		ImVec2 prevPts[3];
		for (int step = 0; step <= kSteps; ++step) {
			float u = (float)step / (float)kSteps;
			float r0, g0, b0;
			sampleHarmonicLMS(u, r0, g0, b0);

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
					aDraw->AddLine(prevPts[ch], curPts[ch], kChanCol[ch], 2.2f);
				}
			}

			for (int ch = 0; ch < 3; ++ch) {
				prevPts[ch] = curPts[ch];
			}
		}

		aDraw->PopClipRect();
	}

	// ── Mode 3: Diskrete Strahlen-Zerlegung (Lineare Strahlen / Ray Scope) ─────
	static void DrawRayCurvePanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                              const double aM[3][3], bool aIsDetached, float aOpacity)
	{
		const float pad = 8.0f;
		const float labelSpaceLeft = 28.0f;
		const float labelSpaceBottom = 20.0f;
		const float badgeSpaceRight = 6.0f;

		const float plotX = aOrigin.x + pad + labelSpaceLeft;
		const float plotY = aOrigin.y + pad + 4.0f;
		const float plotW = aW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		const float plotH = aH - pad * 2 - labelSpaceBottom - 4.0f;

		float panelA = aIsDetached ? std::clamp(aOpacity * 0.92f, 0.0f, 0.92f) : 0.92f;
		int bgAlpha = (int)(panelA * 255.0f);
		int borderAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f, 20.0f, 150.0f)) : 150;

		aDraw->AddRectFilled(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(12, 15, 22, bgAlpha), 6.0f);
		aDraw->AddRect(aOrigin, ImVec2(aOrigin.x + aW, aOrigin.y + aH), IM_COL32(65, 85, 125, borderAlpha), 6.0f, 0, 1.2f);

		int gridAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 65.0f, 15.0f, 65.0f)) : 65;
		int textAlpha = aIsDetached ? (int)(std::clamp(aOpacity * 150.0f + 60.0f, 60.0f, 210.0f)) : 210;

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

		aDraw->AddRect(ImVec2(plotX, plotY), ImVec2(plotX + plotW, plotY + plotH), IM_COL32(70, 90, 130, borderAlpha), 0.0f, 0, 1.0f);
		aDraw->PushClipRect(ImVec2(plotX - 0.5f, plotY - 0.5f), ImVec2(plotX + plotW + 0.5f, plotY + plotH + 0.5f), true);

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

		constexpr int kRays = 32;
		for (int i = 0; i <= kRays; ++i) {
			float u = (float)i / (float)kRays;
			float r0, g0, b0;
			sampleSpectrumRGB(u, r0, g0, b0);

			double cr = std::clamp(aM[0][0]*r0 + aM[0][1]*g0 + aM[0][2]*b0, 0.0, 1.0);
			double cg = std::clamp(aM[1][0]*r0 + aM[1][1]*g0 + aM[1][2]*b0, 0.0, 1.0);
			double cb = std::clamp(aM[2][0]*r0 + aM[2][1]*g0 + aM[2][2]*b0, 0.0, 1.0);

			float curX = plotX + u * plotW;
			float baselineY = plotY + plotH;

			float yr = baselineY - (float)cr * plotH;
			float yg = baselineY - (float)cg * plotH;
			float yb = baselineY - (float)cb * plotH;

			aDraw->AddLine(ImVec2(curX - 1.5f, baselineY), ImVec2(curX - 1.5f, yr), IM_COL32(255, 75, 75, 140), 1.2f);
			aDraw->AddCircleFilled(ImVec2(curX - 1.5f, yr), 2.2f, IM_COL32(255, 95, 95, 230));

			aDraw->AddLine(ImVec2(curX, baselineY), ImVec2(curX, yg), IM_COL32(65, 240, 110, 140), 1.2f);
			aDraw->AddCircleFilled(ImVec2(curX, yg), 2.2f, IM_COL32(85, 255, 130, 230));

			aDraw->AddLine(ImVec2(curX + 1.5f, baselineY), ImVec2(curX + 1.5f, yb), IM_COL32(75, 170, 255, 140), 1.2f);
			aDraw->AddCircleFilled(ImVec2(curX + 1.5f, yb), 2.2f, IM_COL32(95, 190, 255, 230));
		}

		aDraw->PopClipRect();
	}

	void DrawSpectralGraphPanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                           const double aM[3][3], bool aIsDetached, float aOpacity, int aMode)
	{
		if (aMode == 1) {
			DrawHarmonicCurvePanel(aDraw, aOrigin, aW, aH, aM, aIsDetached, aOpacity);
		} else if (aMode == 2) {
			DrawRayCurvePanel(aDraw, aOrigin, aW, aH, aM, aIsDetached, aOpacity);
		} else {
			DrawCurvePanel(aDraw, aOrigin, aW, aH, aM, aIsDetached, aOpacity);
		}
	}

	// ── Dedicated Sensor & Spectral Graph HUD Window ─────────────────────────
	void RenderGraphWindow()
	{
		if (!ImGui::GetCurrentContext()) return;
		const L10n& t = Strings();
		bool changed = false;
		bool saveNeeded = false;
		bool isDe = (t.Enabled[0] == 'A');

		ImGui::PushID("CBA_GraphHUD");

		ImGuiStyle& style = ImGui::GetStyle();
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);

		// ── Header Bar: Live Status Dot, Live Profile & Brightness Info, Reset Button, Mischpult Opacity Button ──
		DrawFilterStatusIndicator(false);
		ImGui::SameLine(0, 6.0f);

		// Status text
		{
			std::string profStr;
			if (CurrentSettings.Mixed) {
				char b[64];
				std::snprintf(b, sizeof(b), "%s (%d%%/%d%%)", isDe ? "Gemischt" : "Mixed",
					(int)(CurrentSettings.MixedRgSeverity01 * 100.0), (int)(CurrentSettings.MixedBySeverity01 * 100.0));
				profStr = b;
			} else {
				const char* name = (CurrentSettings.Type == BalanceType::Protan) ? "Protan" :
				                   (CurrentSettings.Type == BalanceType::Deutan) ? "Deutan" : "Tritan";
				char b[64];
				std::snprintf(b, sizeof(b), "%s (%d%%)", name, (int)(CurrentSettings.Severity01 * 100.0));
				profStr = b;
			}
			ImGui::TextColored(ImVec4(0.70f, 0.78f, 0.90f, 0.95f), "%s: %s | %s: %.2fx", 
				isDe ? "Profil" : "Profile", profStr.c_str(),
				isDe ? "Helligkeit" : "Brightness", CurrentSettings.GammaGain);
		}

		ImGui::SameLine(0, 8.0f);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextPrimary);
		if (ImGui::Button("Reset##graph", ImVec2(56.0f, 22.0f)))
		{
			CurrentSettings.Enabled = false;
			CurrentSettings.Severity01 = 0.0;
			CurrentSettings.Mixed = false;
			CurrentSettings.MixedRgSeverity01 = 0.0;
			CurrentSettings.MixedBySeverity01 = 0.0;
			CurrentSettings.GammaGain = 1.0f;
			CurrentSettings.AutoBrightness = false;
			CurrentSettings.CommanderTagMode = 0;
			CurrentSettings.EnableHybridMode = false;
			GetHybridScanner().SetEnabled(false);
			CurrentSettings.FreeFilterEnabled = false;
			CurrentSettings.LabModeEnabled = false;
			CurrentSettings.Save(AddonDir);
			GetColorEffectController().Clear();
			Recompute(/*aForce=*/true);
			changed = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt alle aktiven Kurven, Filter und Effekte komplett auf neutral zurueck (Filter AUS)."
			                       : "Resets all active curves, filters and effects to neutral (Filter OFF).");
		}

		// Opacity slider toggle
		ImGui::SameLine(0, 8.0f);
		bool opActive = s_showGraphOpacityDrawer;
		if (opActive) {
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
		if (ImGui::Button(isDe ? "Mischpult" : "Deck", ImVec2(68.0f, 22.0f)))
		{
			s_showGraphOpacityDrawer = !s_showGraphOpacityDrawer;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "HUD-Transparenz und Glas-Effekt anpassen" : "Adjust HUD opacity and glass effect");
		}

		if (s_showGraphOpacityDrawer)
		{
			ImGui::Spacing();
			float availW = ImGui::GetContentRegionAvail().x;
			ImGui::SetNextItemWidth(availW);
			if (ImGui::SliderFloat("##hud_opacity", &CurrentSettings.UiOpacity, 0.10f, 1.0f, isDe ? "HUD Glas-Deckkraft: %.0f%%" : "HUD Glass Opacity: %.0f%%"))
			{
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ── 3-Way Graph Visualization Mode Selector ─────────────────────────
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));

		auto graphModeBtn = [&](const char* aName, int aModeVal, const char* aTip) {
			bool active = (CurrentSettings.GraphMode == aModeVal);
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
				CurrentSettings.GraphMode = aModeVal;
				saveNeeded = true;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", aTip);
		};

		ImGui::TextDisabled("%s:", isDe ? "Ansicht" : "View");
		ImGui::SameLine(0, 8.0f);
		graphModeBtn("Polygonal", 0, isDe ? "1. Spektrale Transferfunktion (Polygonal / PWL)\nStueckweise lineare Farbvektor-Projektion ueber die Hue-Winkel."
		                                  : "1. Spectral Transfer Function (Piecewise-Linear / PWL)\nPiecewise linear color vector projection across hue angles.");
		ImGui::SameLine();
		graphModeBtn(isDe ? "Harmonisch" : "Harmonic", 1, isDe ? "2. Harmonische Resonanz (Gauss / Sinusoidale LMS-Kurven)\nFliessende, stetige Wellenkurven nach dem LMS-Zapfenmodell des menschlichen Auges."
		                                   : "2. Harmonic Spectral Response (Gaussian / Smooth Spline)\nFlowing, continuous wave curves based on the human LMS cone model.");
		ImGui::SameLine();
		graphModeBtn(isDe ? "Strahlen" : "Rays", 2, isDe ? "3. Diskrete Strahlen-Zerlegung (Lineare Strahlen / Ray Scope)\nPhysikalische Strahlenzerlegung der Farbkanaele wie bei einem Gitterspektrometer."
		                                 : "3. Linear Spectral Rays (Ray Scope / Dispersion Bars)\nPhysical ray-optics decomposition of channels like a diffraction spectrometer.");

		ImGui::PopStyleVar(2);
		ImGui::Spacing();

		// Correction curves
		double corrMat[3][3];
		if (CurrentSettings.Mixed)
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01, corrMat);
		else
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, corrMat);

		float availW = ImGui::GetContentRegionAvail().x;
		float graphW = (availW > 260.0f) ? availW : 260.0f;
		float graphH = 120.0f;

		ImVec2 cp = ImGui::GetCursorScreenPos();
		
		auto t0 = std::chrono::high_resolution_clock::now();
		DrawSpectralGraphPanel(ImGui::GetWindowDrawList(), cp, graphW, graphH, corrMat, /*isDetached=*/true, CurrentSettings.UiOpacity, CurrentSettings.GraphMode);
		auto t1 = std::chrono::high_resolution_clock::now();
		g_perfCurvesMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

		ImGui::InvisibleButton("##curve_panel_hud", ImVec2(graphW, graphH));

		// Live filtered color beam preview (Strahl-Anzeiger)
		const float pad = 8.0f;
		const float labelSpaceLeft = 28.0f;
		const float badgeSpaceRight = 6.0f;
		float plotX = cp.x + pad + labelSpaceLeft;
		float plotW = graphW - pad * 2 - labelSpaceLeft - badgeSpaceRight;
		float beamH = 12.0f;

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

			int beamAlpha = (int)(std::clamp(CurrentSettings.UiOpacity * 210.0f + 45.0f, 45.0f, 255.0f));
			ImU32 col = IM_COL32((int)(cr*255), (int)(cg*255), (int)(cb*255), beamAlpha);
			dl->AddRectFilled(ImVec2(plotX + u0 * plotW, beamPos.y), ImVec2(plotX + u1 * plotW, beamPos.y + beamH), col, (b == 0 || b == kBeamSteps - 1) ? 2.0f : 0.0f);
		}
		int borderAlpha = (int)(std::clamp(CurrentSettings.UiOpacity * 140.0f, 20.0f, 140.0f));
		dl->AddRect(ImVec2(plotX, beamPos.y), ImVec2(plotX + plotW, beamPos.y + beamH), IM_COL32(80, 100, 140, borderAlpha), 2.0f);
		ImGui::Dummy(ImVec2(graphW, beamH));
		ImGui::TextDisabled("%s", isDe ? "Echtzeit-Spektrum (gefiltert)" : "Real-time spectrum (filtered)");

		// ── Lower Controls ───────────────────────────────────────────────────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		auto typeBtnHUD = [&](const char* aLabel, bool aActive, BalanceType aType) {
			if (ImGui::RadioButton(aLabel, aActive)) {
				if (CurrentSettings.Mixed || CurrentSettings.Type != aType) {
					CurrentSettings.Mixed = false;
					CurrentSettings.Type  = aType;
					CurrentSettings.Severity01 = 0.0;
					changed = true;
					saveNeeded = true;
				}
			}
		};
		typeBtnHUD(t.Protan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Protan, BalanceType::Protan);
		ImGui::SameLine();
		typeBtnHUD(t.Deutan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Deutan, BalanceType::Deutan);
		ImGui::SameLine();
		typeBtnHUD(t.Tritan, !CurrentSettings.Mixed && CurrentSettings.Type == BalanceType::Tritan, BalanceType::Tritan);
		ImGui::SameLine();
		if (ImGui::RadioButton(t.Mixed, CurrentSettings.Mixed)) {
			if (!CurrentSettings.Mixed) {
				CurrentSettings.Mixed = true;
				CurrentSettings.MixedRgSeverity01 = 0.0;
				CurrentSettings.MixedBySeverity01 = 0.0;
				changed = true;
				saveNeeded = true;
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
			if (ImGui::SliderFloat("##rg_hud", &rg, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.MixedRgSeverity01 = std::clamp(rg, 0.0f, 1.25f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine(0, sp);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			if (ImGui::Button("Reset##rg_hud", ImVec2(btnW, 0.0f))) { 
				CurrentSettings.MixedRgSeverity01 = 0.0f; 
				changed = true; 
				saveNeeded = true; 
			}
			ImGui::PopStyleColor(4);
			
			ImGui::TextUnformatted(t.ByStrength);
			ImGui::SetNextItemWidth(sW);
			if (ImGui::SliderFloat("##by_hud", &by, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.MixedBySeverity01 = std::clamp(by, 0.0f, 1.25f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine(0, sp);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			if (ImGui::Button("Reset##by_hud", ImVec2(btnW, 0.0f))) { 
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
			if (ImGui::SliderFloat("##sev_hud", &sev, 0.0f, 1.25f, "%.3f", ImGuiSliderFlags_NoInput)) {
				CurrentSettings.Severity01 = std::clamp(sev, 0.0f, 1.25f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			ImGui::SameLine(0, sp);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			if (ImGui::Button("Reset##sev_hud", ImVec2(btnW, 0.0f))) { 
				CurrentSettings.Severity01 = 0.0f; 
				changed = true; 
				saveNeeded = true; 
			}
			ImGui::PopStyleColor(4);
		}

		// ── Section: Eye Comfort / Helligkeits-Logik ────────────────────────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

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

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.08f, 0.12f, 0.95f));
		ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.12f, 0.28f, 0.40f, 0.75f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 7.0f));

		if (ImGui::BeginChild("##eye_comfort_hud_card", ImVec2(0.0f, 54.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Helligkeits-Kompensation (Eye Comfort)" : "Brightness Compensation (Eye Comfort)");
			ImGui::Spacing();

			ImGui::TextUnformatted(isDe ? "Luminanz-Retention:" : "Luminance Retention:");
			ImGui::SameLine(0, 6.0f);
			ImGui::TextColored(Theme::kTextCyanLicht, "%.1f%%", retention.retentionRatio * 100.0f);

			ImGui::SameLine(0, 14.0f);
			ImGui::TextUnformatted(isDe ? "Empfehlung:" : "Target:");
			ImGui::SameLine(0, 6.0f);
			ImGui::TextColored(Theme::kTextGoldLabel, "%.2fx", retention.recommendedGain);
		}
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);

		ImGui::Spacing();

		float impactPct = (retention.retentionRatio - 1.0f) * 100.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);

		char applyBtnLabel[64];
		std::snprintf(applyBtnLabel, sizeof(applyBtnLabel), isDe ? "Optimalwert (%.2fx)##hud_apply" : "Apply Target (%.2fx)##hud_apply", retention.recommendedGain);
		float btnWHUDApply = isDe ? 155.0f : 145.0f;

		if (ImGui::Button(applyBtnLabel, ImVec2(btnWHUDApply, 24.0f)))
		{
			CurrentSettings.GammaGain = retention.recommendedGain;
			changed = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Setzt den GammaGain einmalig auf den berechneten Optimalwert (%.2fx).\nProfil-Impact auf Helligkeit: %+.1f%% (Retention: %.1f%%)"
			                       : "Applies the calculated optimal gain (%.2fx) once.\nProfile impact on brightness: %+.1f%% (Retention: %.1f%%)",
			                       retention.recommendedGain, impactPct, retention.retentionRatio * 100.0f);
		}

		ImGui::SameLine(0, 8.0f);
		ImGui::TextDisabled(isDe ? "Impact: %+.1f%%" : "Impact: %+.1f%%", impactPct);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Errechnete Luminanz-Verschiebung durch das aktive Farbprofil (%+.1f%%).\nDer Optimalwert gleicht diesen Helligkeitsverlust praezise aus."
			                       : "Calculated luminance shift caused by active color profile (%+.1f%%).\nThe target value accurately compensates this difference.", impactPct);
		}

		ImGui::SameLine(0, 10.0f);
		if (ImGui::Checkbox(isDe ? "Auto-Sync##hud_auto" : "Auto-Sync##hud_auto", &CurrentSettings.AutoBrightness))
		{
			if (CurrentSettings.AutoBrightness)
			{
				CurrentSettings.GammaGain = retention.recommendedGain;
				changed = true;
			}
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Wenn aktiv: Passt die Helligkeit bei jeder Profil-Aenderung dynamisch und vollautomatisch an."
			                       : "When active: Dynamically synchronizes brightness compensation in real time.");
		}

		ImGui::Spacing();
		float availHUD = ImGui::GetContentRegionAvail().x;
		float btnWHUD = 56.0f;
		float spHUD = 6.0f;
		float sliderWHUD = (availHUD > (btnWHUD + spHUD + 60.0f)) ? (availHUD - btnWHUD - spHUD) : 180.0f;

		ImGui::TextDisabled("%s (%.2fx):", isDe ? "Manuelle Helligkeit" : "Manual Brightness", CurrentSettings.GammaGain);
		ImGui::SetNextItemWidth(sliderWHUD);
		if (ImGui::SliderFloat("##GammaSliderHUD", &CurrentSettings.GammaGain, 0.70f, 1.30f, "%.2fx"))
		{
			CurrentSettings.AutoBrightness = false;
			changed = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
		ImGui::SameLine(0, spHUD);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
		if (ImGui::Button("Reset##gamma_hud", ImVec2(btnWHUD, 0.0f)))
		{
			CurrentSettings.GammaGain = 1.0f;
			CurrentSettings.AutoBrightness = false;
			changed = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip(isDe ? "Setzt Helligkeit auf 1.00x zurueck" : "Resets brightness to 1.00x");

		// ── Bottom Status Bar: Active Features & Modules ─────────────────────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		struct ActiveModule {
			std::string label;
			ImVec4 col;
		};
		std::vector<ActiveModule> activeModules;

		if (CurrentSettings.Enabled) {
			if (CurrentSettings.Mixed) {
				char b[64];
				std::snprintf(b, sizeof(b), "%s (%d%%/%d%%)", isDe ? "Gemischt" : "Mixed",
					(int)(CurrentSettings.MixedRgSeverity01 * 100.0), (int)(CurrentSettings.MixedBySeverity01 * 100.0));
				activeModules.push_back({ std::string("Filter: ") + b, Theme::kTextCyanLicht });
			} else if (CurrentSettings.Severity01 > 0.001) {
				const char* name = (CurrentSettings.Type == BalanceType::Protan) ? "Protan" :
				                   (CurrentSettings.Type == BalanceType::Deutan) ? "Deutan" : "Tritan";
				char b[64];
				std::snprintf(b, sizeof(b), "%s (%d%%)", name, (int)(CurrentSettings.Severity01 * 100.0));
				activeModules.push_back({ std::string("Filter: ") + b, Theme::kTextCyanLicht });
			} else {
				activeModules.push_back({ isDe ? "Filter: Bereit (0%)" : "Filter: Ready (0%)", Theme::kTextBlauPeak });
			}
		}

		if (CurrentSettings.CommanderTagMode != 0) {
			activeModules.push_back({ CurrentSettings.SmartEnhancer ? (isDe ? "Com-Tag: Smart-Auto" : "Com-Tag: Smart-Auto")
			                                                        : (isDe ? "Com-Tag: Preset" : "Com-Tag: Preset"),
			                          Theme::kTextGoldLabel });
		}

		if (CurrentSettings.EnableHybridMode) {
			activeModules.push_back({ isDe ? "Hybrid-Modus" : "Hybrid Mode", ImVec4(0.40f, 0.90f, 0.70f, 1.0f) });
		}

		if (CurrentSettings.AutoBrightness) {
			char b[64];
			std::snprintf(b, sizeof(b), "%s (%.2fx)", isDe ? "Auto-Helligkeit" : "Auto Brightness", CurrentSettings.GammaGain);
			activeModules.push_back({ b, Theme::kTextCyanLicht });
		} else if (std::abs(CurrentSettings.GammaGain - 1.0f) > 0.01f) {
			char b[64];
			std::snprintf(b, sizeof(b), "Gamma: %.2fx", CurrentSettings.GammaGain);
			activeModules.push_back({ b, Theme::kTextBlauPeak });
		}

		if (CurrentSettings.FreeFilterEnabled) {
			activeModules.push_back({ isDe ? "Farb-Tausch" : "Color Swap", ImVec4(0.95f, 0.65f, 0.35f, 1.0f) });
		}

		if (CurrentSettings.LabModeEnabled) {
			activeModules.push_back({ isDe ? "Filter-Labor" : "Filter Lab", ImVec4(0.85f, 0.50f, 0.95f, 1.0f) });
		}

		ImGui::TextDisabled("%s:", isDe ? "Aktiv" : "Active");
		ImGui::SameLine(0, 8.0f);

		if (activeModules.empty()) {
			ImGui::TextColored(Theme::kTextSecondary, "%s", isDe ? "Keine Effekte aktiv (Neutral)" : "No effects active (Neutral)");
		} else {
			for (size_t i = 0; i < activeModules.size(); ++i) {
				if (i > 0) ImGui::SameLine(0, 6.0f);

				ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.08f, 0.14f, 0.20f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.20f, 0.28f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.06f, 0.10f, 0.16f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_Text,          activeModules[i].col);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 1.0f));

				char chipId[32];
				std::snprintf(chipId, sizeof(chipId), "##mod_chip_%zu", i);
				std::string chipText = std::string("[+] ") + activeModules[i].label + chipId;
				ImGui::Button(chipText.c_str());

				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(4);
			}
		}

		if (saveNeeded) {
			CurrentSettings.Save(AddonDir);
			Recompute(/*aForce=*/true);
		} else if (changed) {
			Recompute(/*aForce=*/false);
		}

		ImGui::PopID();
	}
}
