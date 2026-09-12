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
#include "ParameterRegistry.h"
#include "FeatureModule.h"
#include "FilterLayers.h"
#include "FilterSensor.h"

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
		bool isDe = cba::IsGerman(); // was a fragile first-letter check - see CLAUDE.md 2026-09-09

		ImGui::PushID("CBA_GraphHUD");

		// Scoped, not global - same fix as RenderMainWindow, same reason: this
		// wrote into the style struct shared with every other addon in Nexus's
		// ImGui context and never put it back.
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

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
		if (ImGui::Button("Reset##graph", ImVec2(0.0f, 22.0f)))
		{
			ResetFilterSettingsAndDisable();
			changed = true;
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
		// Named after the job again (2026-09-12). Emi built this to fade the
		// panels down so they stop covering the game WHILE he adjusts the
		// filter by hand - it drifted to "Mischpult"/"Deck" at some point,
		// which names a piece of furniture rather than the thing it does.
		if (ImGui::Button(isDe ? "Durchsicht" : "See-through", ImVec2(0.0f, 22.0f)))
		{
			s_showGraphOpacityDrawer = !s_showGraphOpacityDrawer;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe ? "Blendet die CBA-Fenster herunter, damit sie beim manuellen Einstellen nicht im Weg sind.\nDer Filter laeuft dabei normal weiter."
			                       : "Fades the CBA windows down so they stop covering the game while you adjust by hand.\nThe filter keeps running normally.");
		}

		if (s_showGraphOpacityDrawer)
		{
			ImGui::Spacing();
			float availW = ImGui::GetContentRegionAvail().x;
			ImGui::SetNextItemWidth(availW);
			// Emi: "verbuggterweise nur 2 Stellungen, was nicht immer so war."
			// It was never the slider - it is the readout. UiOpacity lives in
			// 0.10..1.00 and the format string was "%.0f%%", so every value
			// below 0.5 printed "0%" and everything above printed "1%". The
			// handle moved continuously the whole time; the number beside it
			// had two states, which is what a person sees and therefore what
			// the control IS. Exactly the same defect as the Eye-Sensitive
			// sliders (CLAUDE.md, 2026-09-09): ImGui does not scale a value to
			// match its format string. Widget runs in 0-100 display units,
			// converted at the boundary.
			{
				float opacityPct = CurrentSettings.UiOpacity * 100.0f;
				if (ImGui::SliderFloat("##hud_opacity", &opacityPct, 10.0f, 100.0f,
					isDe ? "Fenster-Deckkraft: %.0f%%" : "Window opacity: %.0f%%", ImGuiSliderFlags_AlwaysClamp))
				{
					CurrentSettings.UiOpacity = std::clamp(opacityPct * 0.01f, 0.10f, 1.00f);
					changed = true;
				}
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
			if (ImGui::Button(aName, ImVec2(0.0f, 22.0f))) {
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
		ActiveCorrectionMatrix(corrMat);

		float availW = ImGui::GetContentRegionAvail().x;
		float graphW = (availW > 260.0f) ? availW : 260.0f;
		float graphH = 120.0f;

		ImVec2 cp = ImGui::GetCursorScreenPos();
		
		// t1/g_perfCurvesMs used to be captured right here, before the
		// 48-iteration beam-preview loop below - so this counter, surfaced
		// verbatim in MainWindow.cpp's HUD cost breakdown, silently excluded
		// roughly half the actual per-frame drawing cost of this panel
		// (found in the 2026-09-09 codebase review). Moved past the beam
		// preview so it covers the whole visual block.
		auto t0 = std::chrono::high_resolution_clock::now();
		DrawSpectralGraphPanel(ImGui::GetWindowDrawList(), cp, graphW, graphH, corrMat, /*isDetached=*/true, CurrentSettings.UiOpacity, CurrentSettings.GraphMode);

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

		auto t1 = std::chrono::high_resolution_clock::now();
		g_perfCurvesMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

		// ── Measured, not predicted ──────────────────────────────────────────
		// Everything else this tool says about its own effect is derived from
		// the matrix. This is the one thing it can honestly measure: the same
		// frame before and after the correction, differenced.
		//
		// The first version of this block compared the sensor's ratio directly
		// against GetBrightnessRetention() and reported the gap as a
		// disagreement. That was wrong, and wrong in the way this project keeps
		// having to correct: retentionRatio is computed from ColorStackMatrix,
		// which deliberately EXCLUDES GammaGain (CLAUDE.md - Auto-Brightness
		// solves for the gain, so its own measurement must not contain it). The
		// sensor measures the whole pipeline, gain included. At severity 0 with
		// a 1.25x gain the panel therefore announced "116.2% vs 100.0%, off by
		// 16.2 points" while both numbers were exactly right about different
		// questions. Comparing like with like below: predicted TOTAL change is
		// retention x gain, and the gap against the measurement is a real
		// finding rather than a unit mismatch.
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Sensor - gemessen am echten Bild" : "Sensor - measured on the real frame");
		ImGui::TextDisabled("%s", isDe
			? "Derselbe Frame vor und nach der Korrektur. Die Differenz ist, was der Filter tut."
			: "The same frame before and after the correction. The difference is what the filter does.");

		if (CurrentSettings.RenderBackend != 1)
		{
			ImGui::TextDisabled("%s", isDe
				? "Nur im Modus \"Nur GW2 (Shader)\" messbar - im DWM-Modus faerbt Windows das Bild, nachdem wir es zuletzt sehen."
				: "Only measurable in \"GW2 only (shader)\" mode - under DWM, Windows tints the image after the last point we can see it.");
		}
		else
		{
			FilterSensor::Reading reading = GetFilterSensor().Latest();
			if (!reading.valid)
			{
				ImGui::TextDisabled("%s", isDe ? "Misst... (erste Messung nach ~0,2 s)" : "Measuring... (first sample after ~0.2 s)");
			}
			else
			{
				const float lumBefore = SensorLuma(reading.beforeR, reading.beforeG, reading.beforeB);
				const float lumAfter  = SensorLuma(reading.afterR,  reading.afterG,  reading.afterB);
				const float measuredRatio = (lumBefore > 1e-5f) ? (lumAfter / lumBefore) : 1.0f;

				ImDrawList* sdl = ImGui::GetWindowDrawList();
				auto swatch = [&](const char* aLabel, float r, float g, float b, float lum)
				{
					ImVec2 p = ImGui::GetCursorScreenPos();
					const float sz = ImGui::GetTextLineHeight() + 4.0f;
					sdl->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255), 3.0f);
					sdl->AddRect(p, ImVec2(p.x + sz, p.y + sz), IM_COL32(90, 110, 140, 180), 3.0f);
					ImGui::Dummy(ImVec2(sz, sz));
					ImGui::SameLine(0, 6.0f);
					ImGui::TextDisabled("%s", aLabel);
					ImGui::SameLine(0, 6.0f);
					ImGui::Text("%.3f %.3f %.3f", r, g, b);
					ImGui::SameLine(0, 10.0f);
					ImGui::TextDisabled(isDe ? "Helligkeit %.4f" : "luminance %.4f", lum);
				};
				swatch(isDe ? "So zeichnet GW2 " : "As GW2 draws it ", reading.beforeR, reading.beforeG, reading.beforeB, lumBefore);
				swatch(isDe ? "So siehst du es" : "As you see it   ", reading.afterR, reading.afterG, reading.afterB, lumAfter);

				ImGui::Spacing();
				ImGui::TextUnformatted(isDe ? "Aenderung durch den Filter:" : "Change caused by the filter:");
				ImGui::SameLine(0, 6.0f);
				ImGui::TextColored(Theme::kTextGoldLabel, "%+.1f%%", (measuredRatio - 1.0f) * 100.0f);

				// Like with like: the matrix's prediction for the SAME thing the
				// sensor measured, which means retention times the gain.
				const BrightnessRetentionResult predicted = GetBrightnessRetention();
				const float gain = CurrentSettings.GammaGain;
				const float predictedTotal = predicted.retentionRatio * gain;
				const float gapPoints = (measuredRatio - predictedTotal) * 100.0f;

				ImGui::TextDisabled(isDe ? "Erwartet laut Matrix: %+.1f%%  (Korrektur %.0f%% x Helligkeit %.2fx)"
				                         : "Expected from the matrix: %+.1f%%  (correction %.0f%% x brightness %.2fx)",
					(predictedTotal - 1.0f) * 100.0f, predicted.retentionRatio * 100.0f, gain);

				if (std::abs(gapPoints) >= 2.0f)
				{
					ImGui::TextColored(Theme::kTextGoldLabel, isDe ? "Differenz: %+.1f Punkte" : "Gap: %+.1f points", gapPoints);
					ImGui::SameLine(0, 6.0f);
					// The overwhelmingly likely cause, and the one the sensor
					// exists to make visible: the shader clamps, so a gain above
					// 1.0 cannot brighten pixels that are already at maximum.
					// The matrix cannot know that; only a measurement can.
					ImGui::TextDisabled("%s", (gain > 1.01f && gapPoints < 0.0f)
						? (isDe ? "- helle Bildbereiche sind bereits am Anschlag (Clipping)"
						        : "- bright areas are already at maximum (clipping)")
						: (isDe ? "- die Vorhersage kennt nur neun Tagfarben, die Messung das ganze Bild"
						        : "- the prediction knows nine tag colours, the measurement the whole image"));
				}
				else
				{
					ImGui::TextDisabled("%s", isDe ? "Deckt sich mit der Messung." : "Matches the measurement.");
				}

				ImGui::Spacing();
				ImGui::TextDisabled(isDe ? "Kanal-Delta:  R %+.3f   G %+.3f   B %+.3f" : "Channel delta:  R %+.3f   G %+.3f   B %+.3f",
					reading.afterR - reading.beforeR,
					reading.afterG - reading.beforeG,
					reading.afterB - reading.beforeB);

				// ── Regulation ────────────────────────────────────────────────
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Helligkeit nach Messung regeln" : "Steer brightness by measurement");

				if (!CurrentSettings.AutoBrightness)
				{
					ImGui::TextColored(Theme::kTextGoldLabel, "%s", isDe
						? "Auto-Helligkeit ist aus - ohne sie regelt hier nichts."
						: "Auto-Brightness is off - nothing steers without it.");
					ImGui::SameLine(0, 8.0f);
					if (ImGui::SmallButton(isDe ? "Einschalten##ab_on" : "Turn on##ab_on"))
					{
						CurrentSettings.AutoBrightness = true;
						changed = true;
						saveNeeded = true;
					}
				}

				int src = CurrentSettings.AutoBrightnessSource;
				auto srcRadio = [&](const char* aLabel, int aValue, const char* aTip)
				{
					if (ImGui::RadioButton(aLabel, src == aValue))
					{
						CurrentSettings.AutoBrightnessSource = aValue;
						changed = true;
						saveNeeded = true;
					}
					if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", aTip);
				};
				srcRadio(isDe ? "Vorhersage##abs0" : "Prediction##abs0", 0, isDe
					? "Die Empfehlung kommt aus der Matrix, gerechnet auf neun Referenz-Tagfarben.\nFunktioniert in jedem Modus, sieht aber nie das Bild."
					: "The target comes from the matrix, computed on nine reference tag colours.\nWorks in every mode, but never sees the image.");
				ImGui::SameLine(0, 12.0f);
				srcRadio(isDe ? "Filter neutral halten##abs1" : "Keep the filter neutral##abs1", 1, isDe
					? "Regelt so, dass die KORREKTUR keine Helligkeit kostet - gemessen, nicht geschaetzt.\nUnabhaengig von der Szene, weil auf ein Verhaeltnis geregelt wird."
					: "Steers so the CORRECTION costs no brightness - measured, not estimated.\nScene-independent, because it steers on a ratio.");
				ImGui::SameLine(0, 12.0f);
				srcRadio(isDe ? "Niveau halten##abs2" : "Hold a level##abs2", 2, isDe
					? "Regelt die gemessene Helligkeit auf einen gemerkten Wert.\nDas ist Belichtungsautomatik - sie arbeitet GEGEN die Beleuchtung des Spiels.\nHoehle und Wueste sollen sich unterscheiden; das hier gleicht sie an."
					: "Steers measured brightness towards a remembered value.\nThis is auto-exposure - it works AGAINST the game's own lighting.\nA cave and a desert are meant to differ; this evens them out.");

				if (CurrentSettings.AutoBrightnessSource == 2)
				{
					ImGui::Indent(12.0f);
					const float target = CurrentSettings.SensorBrightnessTarget;
					if (target <= 0.0f)
					{
						ImGui::TextColored(Theme::kTextGoldLabel, "%s", isDe
							? "Noch kein Niveau gemerkt - bis dahin regelt nichts."
							: "No level captured yet - nothing steers until there is one.");
					}
					else
					{
						ImGui::TextDisabled(isDe ? "Ziel: %.4f   (gerade gemessen: %.4f)" : "Target: %.4f   (measured now: %.4f)",
							target, lumAfter);
					}
					if (ImGui::SmallButton(isDe ? "Jetzige Helligkeit merken##cap" : "Remember current brightness##cap"))
					{
						CurrentSettings.SensorBrightnessTarget = lumAfter;
						changed = true;
						saveNeeded = true;
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("%s", isDe
							? "Nimmt die gerade gemessene Helligkeit als Ziel.\nStell das Bild vorher so ein, wie du es haben willst."
							: "Takes the brightness measured right now as the target.\nSet the image the way you want it first.");
					}
					if (target > 0.0f)
					{
						ImGui::SameLine(0, 8.0f);
						if (ImGui::SmallButton(isDe ? "Vergessen##capclr" : "Forget##capclr"))
						{
							CurrentSettings.SensorBrightnessTarget = 0.0f;
							changed = true;
							saveNeeded = true;
						}
					}
					ImGui::Unindent(12.0f);
				}
			}

			// Stated, not implied. A sensor that let someone believe it knew
			// more than it does would be worse than no sensor.
			ImGui::Spacing();
			ImGui::TextDisabled("%s", isDe
				? "Gemessen wird der Bildmittelwert ueber den ganzen Frame, CBA-Oberflaeche eingeschlossen -"
				: "Measured as the frame-wide mean, CBA's own interface included -");
			ImGui::TextDisabled("%s", isDe
				? "also dieses Fenster auch. Monitor-Helligkeit, Panel-Gamma und HDR-Tonemapping"
				: "including this window. Monitor brightness, panel gamma and HDR tone mapping happen");
			ImGui::TextDisabled("%s", isDe
				? "liegen ausserhalb des Prozesses und sind von hier aus nicht messbar."
				: "outside this process and cannot be measured from here.");
		}

		// ── Manual Filter Controls ────────────────────────────────────────────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// The manual path has a home again (2026-09-12, Emi's call after
		// seeing every window open at once).
		//
		// These controls were removed on 2026-09-11 as a third duplicate
		// editor, which they were - but that left the *manual* path with no
		// home at all. The Nexus panel asks perception questions and sets the
		// values for you; that is the right entry point and the wrong tool for
		// someone who wants to dial a number in by hand. So this is not the
		// old duplicate coming back: the base panel owns the guided path, this
		// window owns the manual one, and Main Window Section 1 stays
		// read-only status.
		//
		// Everything the manual path needs is collected here rather than
		// scattered across windows: the three base profiles, Mixed, their
		// strength sliders, the contrast tolerance and reference values (moved
		// out of Main Window Section 1, where they sat two windows away from
		// the contrast logic they belong to), and the brightness block already
		// below - the last slider of the module.
		ImGui::TextColored(Theme::kTextCyanLicht, "%s", isDe ? "Manuelle Filter-Regler" : "Manual Filter Controls");
		ImGui::TextDisabled("%s", isDe ? "Profil, Staerke und Kontrast direkt setzen - ohne gefuehrte Abfrage."
		                               : "Set profile, strength and contrast directly - no guided questions.");

		if (!CurrentSettings.Enabled)
		{
			// Without this line every slider below stores a value and changes
			// nothing on screen, i.e. the panel would be supplying false
			// evidence - the one thing PRODUCT_CONCEPT.md exists to prevent.
			ImGui::TextColored(Theme::kTextGoldLabel, "%s", isDe ? "Filter ist AUS - Werte werden gespeichert, aber nicht angewendet."
			                                                     : "Filter is OFF - values are stored but not applied.");
			ImGui::SameLine(0, 8.0f);
			// Same shape as the Eye Comfort block's own off-state hint in the
			// Nexus panel: state the problem, then offer the single click that
			// solves it. ToggleMasterEnabled() is the shared owner of this
			// flag - see CLAUDE.md's ungoverned-Enabled ratchet for why no new
			// site writes it directly.
			if (ImGui::SmallButton(isDe ? "Jetzt einschalten##hud_enable" : "Turn on now##hud_enable"))
			{
				ToggleMasterEnabled();
				changed = true;
				saveNeeded = true;
			}
		}

		ImGui::Spacing();

		auto typeBtnHUD = [&](const char* aLabel, BalanceType aType) {
			bool active = (!CurrentSettings.Mixed && CurrentSettings.Type == aType);
			if (ImGui::RadioButton(aLabel, active)) {
				if (CurrentSettings.Mixed || CurrentSettings.Type != aType) {
					CurrentSettings.Mixed = false;
					CurrentSettings.Type  = aType;
					// Deliberate: picking a type by hand starts at 0% and lets
					// the user walk it up while watching the screen. The
					// guided path jumps straight to a working strength
					// instead, because there the user answered a question
					// rather than asked for a knob.
					ParameterRegistry::Get().SetFloat(ParamId::Severity01, 0.0f);
					changed = true;
					saveNeeded = true;
				}
			}
		};
		typeBtnHUD(t.Protan, BalanceType::Protan);
		ImGui::SameLine();
		typeBtnHUD(t.Deutan, BalanceType::Deutan);
		ImGui::SameLine();
		typeBtnHUD(t.Tritan, BalanceType::Tritan);
		ImGui::SameLine();
		if (ImGui::RadioButton(t.Mixed, CurrentSettings.Mixed)) {
			if (!CurrentSettings.Mixed) {
				CurrentSettings.Mixed = true;
				ParameterRegistry::Get().SetFloat(ParamId::MixedRgSeverity01, 0.0f);
				ParameterRegistry::Get().SetFloat(ParamId::MixedBySeverity01, 0.0f);
				changed = true;
				saveNeeded = true;
			}
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", isDe ? "Korrigiert Rot-Gruen und Blau-Gelb unabhaengig voneinander, mit je eigenem Regler."
			                             : "Corrects red-green and blue-yellow independently, each with its own slider.");
		}

		ImGui::Spacing();

		// Slider + Reset, written once. This block used to carry three
		// near-identical ~25-line copies of it (Strength, RG, BY).
		//
		// Percent is display-only: ImGui does not scale the value it is given
		// to match the format string (see CLAUDE.md's Eye-Sensitive "0%/1%"
		// bug), so the widget runs in 0-125 display units and converts at the
		// boundary. The 1.25 ceiling is not the widget's to enforce either -
		// ParamMeta clamps on every Set, CTRL-click text entry included.
		auto severitySlider = [&](const char* aLabel, ParamId aId, const char* aSliderId, const char* aResetId) {
			ImGui::TextUnformatted(aLabel);
			float avail = ImGui::GetContentRegionAvail().x;
			float padX  = ImGui::GetStyle().FramePadding.x * 2.0f;
			float btnW  = ImGui::CalcTextSize("Reset").x + padX + 8.0f;
			const float sp = 6.0f;
			float sW = (avail > (btnW + sp + 60.0f)) ? (avail - btnW - sp) : 180.0f;

			float pct = ParameterRegistry::Get().GetFloat(aId) * 100.0f;
			ImGui::SetNextItemWidth(sW);
			if (ImGui::SliderFloat(aSliderId, &pct, 0.0f, 125.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
			{
				ParameterRegistry::Get().SetFloat(aId, pct * 0.01f);
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", isDe ? "Ueber 100% korrigiert staerker, als die Simulation vorgibt - fuer Faelle, in denen die neutrale Korrektur noch nicht reicht."
				                             : "Above 100% corrects harder than the simulation prescribes - for cases where the neutral correction is not enough yet.");
			}

			ImGui::SameLine(0, sp);
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
			if (ImGui::Button(aResetId, ImVec2(btnW, 0.0f)))
			{
				ParameterRegistry::Get().SetFloat(aId, 0.0f);
				changed = true;
				saveNeeded = true;
			}
			ImGui::PopStyleColor(4);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", isDe ? "Wert auf 0% zuruecksetzen" : "Reset value to 0%");
		};

		if (CurrentSettings.Mixed) {
			severitySlider(t.RgStrength, ParamId::MixedRgSeverity01, "##rg_hud", "Reset##rg_hud");
			severitySlider(t.ByStrength, ParamId::MixedBySeverity01, "##by_hud", "Reset##by_hud");
		} else {
			severitySlider(t.Strength, ParamId::Severity01, "##sev_hud", "Reset##sev_hud");
		}

		// ── Advanced: contrast tolerance & reference values ──────────────────
		// Moved here from Main Window Section 1 (2026-09-12). Emi's reading of
		// the live windows: the detection radius decides how much the
		// Commander-Tag contrast logic grabs, so having it sit alone in
		// another window read as an unrelated leftover instead of as part of
		// the contrast logic it drives. One home, next to what it acts on.
		ImGui::Spacing();
		if (ImGui::TreeNodeEx(isDe ? "Erweitert: Kontrast & Referenzwerte##hud_advanced"
		                           : "Advanced: Contrast & Reference Values##hud_advanced", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextUnformatted(isDe ? "Toleranz (Erkennungsradius):" : "Tolerance (Detection Radius):");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			{
				float tol = ParameterRegistry::Get().GetFloat(ParamId::EnhancerTolerance);
				if (ImGui::SliderFloat("##enhancer_tol_hud", &tol, 0.04f, 0.20f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
				{
					ParameterRegistry::Get().SetFloat(ParamId::EnhancerTolerance, tol);
					UpdateTagEnhancerConflicts();
					changed = true;
				}
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", isDe ? "Wie weit eine Bildschirmfarbe von einer Referenz-Tagfarbe abweichen darf und trotzdem als diese erkannt wird.\nGroesser = mehr Treffer, aber auch mehr Fehltreffer."
				                             : "How far a screen colour may differ from a reference tag colour and still count as that tag.\nLarger = more matches, but also more false ones.");
			}

			ImGui::Spacing();
			const char* defaultHintHUD = CurrentSettings.Mixed ? "RG: 50% | BY: 50%" :
				(CurrentSettings.Type == BalanceType::Protan ? "AQ: 0.35 | HRR: 8/10" :
				(CurrentSettings.Type == BalanceType::Deutan ? "AQ: 3.20 | HRR: 8/10" : "Moreland: 1.15 | HRR: 6/10"));

			ImGui::TextDisabled("%s:", isDe ? "Referenzwerte / Kalibrierung (AQ / HRR)" : "Reference Values / Calibration (AQ / HRR)");
			char diagBufHUD[128]{};
			std::snprintf(diagBufHUD, sizeof(diagBufHUD), "%s", CurrentSettings.DiagnosisHint.c_str());

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if (ImGui::InputTextWithHint("##ref_values_hud", defaultHintHUD, diagBufHUD, sizeof(diagBufHUD)))
			{
				// The assignment has to happen per keystroke - diagBufHUD is
				// refilled from DiagnosisHint every frame, so skipping it
				// would undo each character as it is typed. The disk write
				// must not: this used to set saveNeeded here, i.e. a
				// Settings::Save + forced Recompute for every letter, which
				// is the same defect the Filter Lab colour pickers had.
				CurrentSettings.DiagnosisHint = diagBufHUD;
				changed = true;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isDe
					? "Notizfeld fuer persoenliche Kalibrier- oder Testwerte (z.B. Nagel-AQ, HRR-Plates) - wird mit dem Profil gespeichert.\nEinen echten Befund in Filterwerte uebersetzen: Vision Lab -> Klinischer Report.\nTypische Werte fuer dieses Profil: %s"
					: "Note field for personal calibration or test scores (e.g. Nagel AQ, HRR plates) - stored with the profile.\nTo translate a real diagnosis into filter values: Vision Lab -> Clinical Report.\nTypical values for this profile: %s",
					defaultHintHUD);
			}
			ImGui::TreePop();
		}

		// ── Section: Eye Comfort / Helligkeits-Logik ────────────────────────
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		SyncAutoBrightnessGain(changed, saveNeeded);
		BrightnessRetentionResult retention = GetBrightnessRetention();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.08f, 0.12f, 0.95f));
		ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.12f, 0.28f, 0.40f, 0.75f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 7.0f));

		float hudCardH = ImGui::GetTextLineHeightWithSpacing() * 2.0f + 20.0f;
		if (ImGui::BeginChild("##eye_comfort_hud_card", ImVec2(0.0f, hudCardH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
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

		if (ImGui::Button(applyBtnLabel, ImVec2(0.0f, 24.0f)))
		{
			ApplyAutoBrightnessGain();
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
				ApplyAutoBrightnessGain();
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
		float padXHUD = ImGui::GetStyle().FramePadding.x * 2.0f;
		float btnWHUD = ImGui::CalcTextSize("Reset").x + padXHUD + 8.0f;
		float spHUD = 6.0f;
		float sliderWHUD = (availHUD > (btnWHUD + spHUD + 60.0f)) ? (availHUD - btnWHUD - spHUD) : 180.0f;

		ImGui::TextDisabled("%s (%.2fx):", isDe ? "Manuelle Helligkeit" : "Manual Brightness", CurrentSettings.GammaGain);
		ImGui::SetNextItemWidth(sliderWHUD);
		// Registry-backed (CLAUDE.md, Registry/Control Layer step 1) - same
		// ParamId::GammaGain as the Main Window slider, both windows now
		// share one clamp source instead of duplicating 0.70f/1.30f here.
		{
			float gain = ParameterRegistry::Get().GetFloat(ParamId::GammaGain);
			if (ImGui::SliderFloat("##GammaSliderHUD", &gain, 0.70f, 1.30f, "%.2fx", ImGuiSliderFlags_AlwaysClamp))
			{
				SetGammaGainManual(gain);
				changed = true;
			}
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;
		ImGui::SameLine(0, spHUD);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
		if (ImGui::Button("Reset##gamma_hud", ImVec2(btnWHUD, 0.0f)))
		{
			SetGammaGainManual(1.0f);
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

		// Every registered feature module contributes its own status line if
		// active (2026-09-09, FeatureModuleRegistry - see core/FeatureModule.h).
		// Used to be one hand-written `if` block per feature here (Commander
		// Tag, Hybrid Mode, Filter Lab, Eye-Sensitive) - the same wiring tax
		// ResetFilterSettingsAndDisable() had.
		for (const auto& module : FeatureModuleRegistry::Get().GetAll())
		{
			if (!module.isActive || !module.isActive()) continue;
			std::string text = module.statusText ? module.statusText(isDe) : std::string();
			if (text.empty()) text = isDe ? module.labelDe : module.labelEn;
			ImVec4 col = module.hudColor ? module.hudColor() : Theme::kTextPrimary;
			activeModules.push_back({ text, col });
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

		ImGui::TextDisabled("%s:", isDe ? "Aktiv" : "Active");
		ImGui::SameLine(0, 8.0f);

		if (activeModules.empty()) {
			ImGui::TextColored(Theme::kTextSecondary, "%s", isDe ? "Keine Effekte aktiv (Neutral)" : "No effects active (Neutral)");
		} else {
			for (size_t i = 0; i < activeModules.size(); ++i) {
				char chipId[32];
				std::snprintf(chipId, sizeof(chipId), "##mod_chip_%zu", i);
				std::string chipText = std::string("[+] ") + activeModules[i].label + chipId;
				float chipW = ImGui::CalcTextSize(chipText.c_str()).x + 16.0f;

				if (i > 0) {
					if (ImGui::GetContentRegionAvail().x >= chipW + 6.0f) ImGui::SameLine(0, 6.0f);
					else ImGui::Spacing();
				}

				ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.08f, 0.14f, 0.20f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.20f, 0.28f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.06f, 0.10f, 0.16f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_Text,          activeModules[i].col);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 1.0f));

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

		ImGui::PopStyleVar(1); // pairs with ButtonTextAlign at the top
		ImGui::PopID();
	}
}
