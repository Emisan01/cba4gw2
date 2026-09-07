#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Shared.h"
#include "ColorMatrix.h"
#include "ColorEffectController.h"
#include "WindowMode.h"
#include "Settings.h"
#include "CbaIcon.h"
#include "HybridScanner.h"
#include "ColorMath.h"
#include "Theme.h"
#include "L10n.h"
#include "UIState.h"
#include "MainWindow.h"
#include "SensorGraphHUD.h"
#include "FilterLab.h"
#include "VisionLab.h"
#include "SafeStartGate.h"
#include "CreditsDialog.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <chrono>
#include <array>
#include <cstdio>
#include <string>
#include <cmath>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <vector>

using namespace cba;

namespace
{
	AddonDefinition AddonDef{};

	struct MumbleContext {
		unsigned char serverAddress[28];
		uint32_t mapId;
		uint32_t mapType;
		uint32_t shardId;
		uint32_t instance;
		uint32_t buildId;
		uint32_t uiState;
		uint16_t compassWidth;
		uint16_t compassHeight;
		float compassRotation;
		float playerX;
		float playerY;
		float mapCenterX;
		float mapCenterY;
		float mapScale;
		uint32_t processId;
		uint8_t mountIndex;
	};

	struct GW2MumbleLink {
		uint32_t uiVersion;
		uint32_t uiTick;
		float fAvatarPosition[3];
		float fAvatarFront[3];
		float fAvatarTop[3];
		wchar_t name[256];
		float fCameraPosition[3];
		float fCameraFront[3];
		float fCameraTop[3];
		wchar_t identity[256];
		uint32_t context_len;
		MumbleContext context;
		wchar_t description[2048];
	};
	NexusLinkData*  NexusLink = nullptr;
	GW2MumbleLink* MumbleLinkData = nullptr;

	std::chrono::steady_clock::time_point s_lastApply{};
	MAGCOLOREFFECT s_lastAppliedEffect{};
	bool s_hasApplied = false;
	std::mutex s_recomputeMutex;

	std::atomic<bool> s_watchdogRunning{false};
	std::thread s_watchdogThread;

	bool RoughlyEqual(const MAGCOLOREFFECT& a, const MAGCOLOREFFECT& b)
	{
		for (int i = 0; i < 5; i++)
			for (int j = 0; j < 5; j++)
				if (std::fabs(a.transform[i][j] - b.transform[i][j]) > 0.0005f)
					return false;
		return true;
	}

	void ApplyThrottled(const MAGCOLOREFFECT& aEffect, bool aForce = false)
	{
		if (!s_deferredInitDone.load())
			return;

		if (!aForce && s_hasApplied && RoughlyEqual(aEffect, s_lastAppliedEffect))
			return; // Identical, save DWM IPC call

		auto now = std::chrono::steady_clock::now();
		auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastApply).count();
		if (!aForce && elapsedMs < 16) // ~60 Hz DWM cap
			return;

		GetColorEffectController().Apply(aEffect);
		s_lastApply = now;
		s_lastAppliedEffect = aEffect;
		s_hasApplied = true;
	}
}

namespace cba
{
	void EnsureDeferredInitialized()
	{
		if (s_deferredInitDone.load()) return;

		if (GetColorEffectController().Initialize())
		{
			s_deferredInitDone.store(true);
			if (CurrentSettings.Enabled)
			{
				Recompute(/*aForce=*/true);
			}
		}
		else
		{
			s_deferredInitDone.store(true);
			if (APIDefs && APIDefs->Log)
			{
				APIDefs->Log(ELogLevel_WARNING, "cba4gw2", "MagInitialize deferred init failed.");
			}
		}
	}

	void UpdateTagEnhancerConflicts()
	{
		static bool s_lastActive = false;
		static float s_lastTolerance = -1.0f;
		static std::vector<TargetColor> s_lastTargets;

		bool anyActive = (CurrentSettings.CommanderTagMode != 0) || CurrentSettings.FreeFilterEnabled || CurrentSettings.LabModeEnabled;
		if (!anyActive)
		{
			if (s_lastActive)
			{
				GetHybridScanner().SetHighlighterParams(false, {}, CurrentSettings.EnhancerTolerance);
				s_lastActive = false;
				s_lastTargets.clear();
			}
			for (auto& st : s_tagConflictStates) st = {};
			return;
		}

		std::vector<TargetColor> targetColors;
		targetColors.reserve(10);

		if (CurrentSettings.CommanderTagMode != 0)
		{
			BalanceType defType = CurrentSettings.Mixed 
				? (CurrentSettings.MixedBySeverity01 > CurrentSettings.MixedRgSeverity01 ? BalanceType::Tritan : BalanceType::Deutan)
				: CurrentSettings.Type;
			double sev = CurrentSettings.Mixed 
				? (CurrentSettings.MixedRgSeverity01 > CurrentSettings.MixedBySeverity01 ? CurrentSettings.MixedRgSeverity01 : CurrentSettings.MixedBySeverity01)
				: CurrentSettings.Severity01;

			struct SimTag {
				float origR, origG, origB;
				float simR, simG, simB;
				float luma;
			};
			std::array<SimTag, 9> simTags{};
			for (int i = 0; i < 9; ++i)
			{
				simTags[i].origR = kGw2TagRefs[i].r;
				simTags[i].origG = kGw2TagRefs[i].g;
				simTags[i].origB = kGw2TagRefs[i].b;
				simTags[i].luma = RelativeLuma(kGw2TagRefs[i].r, kGw2TagRefs[i].g, kGw2TagRefs[i].b);

				double outR = 0.0, outG = 0.0, outB = 0.0;
				ColorMatrix::SimulatePixel(simTags[i].origR, simTags[i].origG, simTags[i].origB, defType, outR, outG, outB);

				simTags[i].simR = (float)(simTags[i].origR + sev * (outR - simTags[i].origR));
				simTags[i].simG = (float)(simTags[i].origG + sev * (outG - simTags[i].origG));
				simTags[i].simB = (float)(simTags[i].origB + sev * (outB - simTags[i].origB));
			}

			std::array<bool, 9> hasConflict{};
			if (CurrentSettings.SmartEnhancer)
			{
				if (defType == BalanceType::Protan) {
					hasConflict[0] = true; // Rot
					hasConflict[1] = true; // Orange
					hasConflict[3] = true; // Gruen
					hasConflict[5] = true; // Blau
					hasConflict[6] = true; // Lila
				} else if (defType == BalanceType::Deutan) {
					hasConflict[0] = true; // Rot
					hasConflict[1] = true; // Orange
					hasConflict[3] = true; // Gruen
				} else {
					hasConflict[2] = true; // Gelb
					hasConflict[3] = true; // Gruen
					hasConflict[4] = true; // Cyan
					hasConflict[5] = true; // Blau
					hasConflict[7] = true; // Magenta
				}
			}
			else
			{
				constexpr float kConflictThreshold = 0.16f;
				for (int i = 0; i < 9; ++i)
				{
					for (int j = i + 1; j < 9; ++j)
					{
						float dr = simTags[i].simR - simTags[j].simR;
						float dg = simTags[i].simG - simTags[j].simG;
						float db = simTags[i].simB - simTags[j].simB;
						float dist = std::sqrt(dr * dr + dg * dg + db * db);
						if (dist < kConflictThreshold)
						{
							hasConflict[i] = true;
							hasConflict[j] = true;
						}
					}
				}
			}

			for (int i = 0; i < 9; ++i)
			{
				s_tagConflictStates[i].inConflict = hasConflict[i];
				if (!hasConflict[i])
				{
					s_tagConflictStates[i].repR = simTags[i].origR;
					s_tagConflictStates[i].repG = simTags[i].origG;
					s_tagConflictStates[i].repB = simTags[i].origB;
					continue;
				}

				float bestR = simTags[i].origR, bestG = simTags[i].origG, bestB = simTags[i].origB;
				float bestScore = -1.0f;

				float h = 0, s = 0, v = 0;
				RgbToHsv(simTags[i].origR, simTags[i].origG, simTags[i].origB, h, s, v);

				for (int step = 1; step <= 23; ++step)
				{
					float testH = std::fmod(h + step * 15.0f, 360.0f);
					float r = 0, g = 0, b = 0;
					HsvToRgb(testH, s, v, r, g, b);

					float newLuma = RelativeLuma(r, g, b);
					float minLuma = simTags[i].luma * 0.85f;
					if (newLuma < minLuma && newLuma > 1e-4f)
					{
						float scale = minLuma / newLuma;
						r = std::clamp(r * scale, 0.0f, 1.0f);
						g = std::clamp(g * scale, 0.0f, 1.0f);
						b = std::clamp(b * scale, 0.0f, 1.0f);
					}

					double candSimR = 0, candSimG = 0, candSimB = 0;
					ColorMatrix::SimulatePixel(r, g, b, defType, candSimR, candSimG, candSimB);
					candSimR = r + sev * (candSimR - r);
					candSimG = g + sev * (candSimG - g);
					candSimB = b + sev * (candSimB - b);

					float minDist = 999.0f;
					for (int j = 0; j < 9; ++j)
					{
						if (i == j) continue;
						float dr = (float)candSimR - simTags[j].simR;
						float dg = (float)candSimG - simTags[j].simG;
						float db = (float)candSimB - simTags[j].simB;
						float d = std::sqrt(dr * dr + dg * dg + db * db);
						if (d < minDist) minDist = d;
					}

					if (minDist > bestScore)
					{
						bestScore = minDist;
						bestR = r;
						bestG = g;
						bestB = b;
					}
				}

				s_tagConflictStates[i].repR = bestR;
				s_tagConflictStates[i].repG = bestG;
				s_tagConflictStates[i].repB = bestB;

				TargetColor tc;
				tc.r = simTags[i].origR;
				tc.g = simTags[i].origG;
				tc.b = simTags[i].origB;
				tc.repR = (uint8_t)(std::clamp(bestR * 255.0f, 0.0f, 255.0f));
				tc.repG = (uint8_t)(std::clamp(bestG * 255.0f, 0.0f, 255.0f));
				tc.repB = (uint8_t)(std::clamp(bestB * 255.0f, 0.0f, 255.0f));
				targetColors.push_back(tc);
			}
		}
		else
		{
			for (auto& st : s_tagConflictStates) st = {};
		}

		if (CurrentSettings.FreeFilterEnabled)
		{
			TargetColor freeTc;
			freeTc.r = CurrentSettings.FreeFilterTargetRgb[0];
			freeTc.g = CurrentSettings.FreeFilterTargetRgb[1];
			freeTc.b = CurrentSettings.FreeFilterTargetRgb[2];
			freeTc.repR = (uint8_t)(std::clamp(CurrentSettings.FreeFilterReplaceRgb[0] * 255.0f, 0.0f, 255.0f));
			freeTc.repG = (uint8_t)(std::clamp(CurrentSettings.FreeFilterReplaceRgb[1] * 255.0f, 0.0f, 255.0f));
			freeTc.repB = (uint8_t)(std::clamp(CurrentSettings.FreeFilterReplaceRgb[2] * 255.0f, 0.0f, 255.0f));
			targetColors.push_back(freeTc);
		}

		if (CurrentSettings.LabModeEnabled)
		{
			for (const auto& filter : CurrentSettings.LabFilters)
			{
				if (!filter.Enabled) continue;
				TargetColor labTc;
				labTc.r = filter.TargetRgb[0];
				labTc.g = filter.TargetRgb[1];
				labTc.b = filter.TargetRgb[2];
				labTc.repR = (uint8_t)(std::clamp(filter.ReplaceRgb[0] * 255.0f, 0.0f, 255.0f));
				labTc.repG = (uint8_t)(std::clamp(filter.ReplaceRgb[1] * 255.0f, 0.0f, 255.0f));
				labTc.repB = (uint8_t)(std::clamp(filter.ReplaceRgb[2] * 255.0f, 0.0f, 255.0f));
				labTc.tolerance = (filter.ToleranceTones / 255.0f) * 1.732f;
				labTc.diffusion = filter.Diffusion;
				labTc.actionType = filter.ActionType;
				targetColors.push_back(labTc);
			}
		}

		float effectiveTol = (CurrentSettings.CommanderTagMode != 0) 
			? CurrentSettings.EnhancerTolerance 
			: ((CurrentSettings.FreeFilterToleranceTones / 255.0f) * 1.732f);

		bool paramsChanged = (!s_lastActive) || (effectiveTol != s_lastTolerance) || (targetColors.size() != s_lastTargets.size());
		if (!paramsChanged)
		{
			for (size_t k = 0; k < targetColors.size(); ++k)
			{
				if (targetColors[k].r != s_lastTargets[k].r || targetColors[k].g != s_lastTargets[k].g || targetColors[k].b != s_lastTargets[k].b ||
					targetColors[k].repR != s_lastTargets[k].repR || targetColors[k].repG != s_lastTargets[k].repG || targetColors[k].repB != s_lastTargets[k].repB)
				{
					paramsChanged = true;
					break;
				}
			}
		}

		if (paramsChanged)
		{
			s_lastActive = true;
			s_lastTolerance = effectiveTol;
			s_lastTargets = targetColors;
			GetHybridScanner().SetHighlighterParams(true, targetColors, effectiveTol);
		}
	}

	BrightnessRetentionResult GetBrightnessRetention()
	{
		static const struct { float r, g, b; } kAmbientColors[5] = {
			{ 0.45f, 0.32f, 0.20f }, // Erdbraun
			{ 0.22f, 0.48f, 0.20f }, // Laub
			{ 0.35f, 0.60f, 0.85f }, // Himmel
			{ 0.52f, 0.52f, 0.52f }, // Stein
			{ 0.95f, 0.90f, 0.70f }  // Sonnenlicht
		};

		double m3x3[3][3];
		if (CurrentSettings.Mixed)
		{
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01,
				m3x3);
		}
		else
		{
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, m3x3);
		}

		float sumOrig = 0.0f;
		float sumTrans = 0.0f;

		auto processColor = [&](float r, float g, float b) {
			float origLuma = RelativeLuma(r, g, b);
			sumOrig += origLuma;

			float trR = (float)(m3x3[0][0] * r + m3x3[0][1] * g + m3x3[0][2] * b);
			float trG = (float)(m3x3[1][0] * r + m3x3[1][1] * g + m3x3[1][2] * b);
			float trB = (float)(m3x3[2][0] * r + m3x3[2][1] * g + m3x3[2][2] * b);

			trR = std::clamp(trR, 0.0f, 1.0f);
			trG = std::clamp(trG, 0.0f, 1.0f);
			trB = std::clamp(trB, 0.0f, 1.0f);

			float transLuma = RelativeLuma(trR, trG, trB);
			sumTrans += transLuma;
		};

		for (int i = 0; i < 8; ++i)
		{
			processColor(kGw2TagRefs[i].r, kGw2TagRefs[i].g, kGw2TagRefs[i].b);
		}
		for (int i = 0; i < 5; ++i)
		{
			processColor(kAmbientColors[i].r, kAmbientColors[i].g, kAmbientColors[i].b);
		}

		BrightnessRetentionResult res{};
		if (sumOrig > 1e-4f)
		{
			res.retentionRatio = sumTrans / sumOrig;
		}
		else
		{
			res.retentionRatio = 1.0f;
		}

		float inv = (res.retentionRatio > 1e-4f) ? (1.0f / res.retentionRatio) : 1.0f;
		res.recommendedGain = std::clamp(inv, 0.70f, 1.30f);
		return res;
	}

	void Recompute(bool aForce)
	{
		std::lock_guard<std::mutex> lock(s_recomputeMutex);
		auto& controller = GetColorEffectController();

		if (!s_deferredInitDone.load())
		{
			return;
		}

		UpdateTagEnhancerConflicts();

		if (!CurrentSettings.Enabled)
		{
			controller.Clear();
			s_hasApplied = false;
			return;
		}

		HWND fg = GetForegroundWindow();
		DWORD fgPid = 0;
		if (fg) GetWindowThreadProcessId(fg, &fgPid);
		bool isGw2Foreground = (fg && fgPid == GetCurrentProcessId());
		bool isMinimized = s_gw2Minimized.load() || (s_gw2Hwnd.load() && IsIconic(s_gw2Hwnd.load()));

		bool shouldBeActive = (!isMinimized && (isGw2Foreground || CurrentSettings.SystemWide));

		if (!shouldBeActive)
		{
			controller.Clear();
			s_hasApplied = false;
			return;
		}

		double m3x3[3][3];
		if (CurrentSettings.Mixed)
		{
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01,
				m3x3);
		}
		else
		{
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, m3x3);
		}

		// Linear brightness scaling (GammaGain, range 0.70 - 1.30)
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c)
			{
				m3x3[r][c] *= CurrentSettings.GammaGain;
			}
		}

		MAGCOLOREFFECT effect = ColorMatrix::ToMagColorEffect(m3x3);
		ApplyThrottled(effect, aForce);
	}

	namespace
	{
		void WatchdogLoop()
		{
			while (s_watchdogRunning)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				if (!s_deferredInitDone.load())
				{
					continue;
				}

				if (!CurrentSettings.Enabled)
				{
					if (s_hasApplied)
					{
						std::lock_guard<std::mutex> lock(s_recomputeMutex);
						GetColorEffectController().Clear();
						s_hasApplied = false;
					}
					continue;
				}

				HWND fg = GetForegroundWindow();
				DWORD fgPid = 0;
				if (fg) GetWindowThreadProcessId(fg, &fgPid);
				bool isGw2Foreground = (fg && fgPid == GetCurrentProcessId());
				bool isMinimized = s_gw2Minimized.load() || (s_gw2Hwnd.load() && IsIconic(s_gw2Hwnd.load()));

				bool shouldBeActive = (!isMinimized && (isGw2Foreground || CurrentSettings.SystemWide));

				if (!shouldBeActive)
				{
					if (s_hasApplied)
					{
						std::lock_guard<std::mutex> lock(s_recomputeMutex);
						GetColorEffectController().Clear();
						s_hasApplied = false;
					}
				}
				else
				{
					if (!s_hasApplied)
					{
						Recompute(/*aForce=*/true);
					}
				}
			}
		}
	}

	UINT AddonWndProc(HWND aWnd, UINT aMsg, WPARAM aWParam, LPARAM aLParam)
	{
		if (aWnd) s_gw2Hwnd = aWnd;
		switch (aMsg)
		{
			case WM_ACTIVATE:
			{
				WORD state = LOWORD(aWParam);
				if (state == WA_INACTIVE)
				{
					if (!CurrentSettings.SystemWide)
					{
						GetColorEffectController().Clear();
						s_hasApplied = false;
					}
				}
				else
				{
					s_gw2Minimized = false;
					Recompute(/*aForce=*/true);
				}
				break;
			}
			case WM_SIZE:
			{
				if (aWParam == SIZE_MINIMIZED)
				{
					s_gw2Minimized = true;
					GetColorEffectController().Clear();
					s_hasApplied = false;
				}
				else if (aWParam == SIZE_RESTORED || aWParam == SIZE_MAXIMIZED)
				{
					s_gw2Minimized = false;
					Recompute(/*aForce=*/true);
				}
				break;
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				bool altDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
				bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
				bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

				// Filter Off: Strg + Shift + O (or legacy Strg + Shift + Q)
				if ((aWParam == 'O' || aWParam == 'Q') && ctrlDown && shiftDown)
				{
					CurrentSettings.Enabled = false;
					CurrentSettings.Save(AddonDir);
					{
						std::lock_guard<std::mutex> lock(s_recomputeMutex);
						GetColorEffectController().Clear();
						s_hasApplied = false;
					}
					Recompute(/*aForce=*/true);
					return aMsg;
				}
				// Main Window: Strg + Shift + C (or Ctrl + Alt + C)
				else if (aWParam == 'C')
				{
					if ((ctrlDown && shiftDown) || (ctrlDown && altDown))
					{
						EnsureDeferredInitialized();
						CurrentSettings.ShowMainWindow = !CurrentSettings.ShowMainWindow;
						if (CurrentSettings.ShowMainWindow) s_focusMainWindow = true;
					}
				}
				// Sensor Graph HUD: Strg + Shift + G (or Ctrl + Alt + G)
				else if (aWParam == 'G')
				{
					if ((ctrlDown && shiftDown) || (ctrlDown && altDown))
					{
						EnsureDeferredInitialized();
						CurrentSettings.ShowGraphWindow = !CurrentSettings.ShowGraphWindow;
						if (CurrentSettings.ShowGraphWindow) s_focusGraphWindow = true;
					}
				}
				break;
			}
		}
		return aMsg;
	}

	void ProcessKeybind(const char* aIdentifier, bool aIsRelease)
	{
		if (aIsRelease) return;

		if (strcmp(aIdentifier, "CBA - Filter Off") == 0 || strcmp(aIdentifier, "CBA - Not-Aus") == 0 || strcmp(aIdentifier, "KB_CBA_PANIC") == 0)
		{
			CurrentSettings.Enabled = false;
			CurrentSettings.Save(AddonDir);
			{
				std::lock_guard<std::mutex> lock(s_recomputeMutex);
				GetColorEffectController().Clear();
				s_hasApplied = false;
			}
			Recompute(/*aForce=*/true);
		}
		else if (strcmp(aIdentifier, "CBA - Main Window") == 0 || strcmp(aIdentifier, "KB_CBA_WINDOW") == 0)
		{
			EnsureDeferredInitialized();
			CurrentSettings.ShowMainWindow = !CurrentSettings.ShowMainWindow;
			if (CurrentSettings.ShowMainWindow) s_focusMainWindow = true;
		}
		else if (strcmp(aIdentifier, "CBA - Sensor Graph") == 0 || strcmp(aIdentifier, "KB_CBA_GRAPH") == 0)
		{
			EnsureDeferredInitialized();
			CurrentSettings.ShowGraphWindow = !CurrentSettings.ShowGraphWindow;
			if (CurrentSettings.ShowGraphWindow) s_focusGraphWindow = true;
		}
	}

	void UpdateQuickAccessIcon()
	{
		if (!APIDefs) return;
		if (CurrentSettings.ShowQuickAccessIcon)
		{
			if (APIDefs->QuickAccess.Add)
			{
				APIDefs->QuickAccess.Add("QA_CBA", "CBA_ICON", "CBA_ICON", "CBA - Main Window", "cba4gw2 (Strg+Shift+C / Filter Off: Strg+Shift+O)");
			}
		}
		else
		{
			if (APIDefs->QuickAccess.Remove)
			{
				APIDefs->QuickAccess.Remove("QA_CBA");
			}
		}
	}

	void AddonOptions()
	{
		if (!ImGui::GetCurrentContext()) return;
		EnsureDeferredInitialized();
		RenderEmbeddedOptions();
	}

	void AddonRenderWindow()
	{
		auto tStartTotal = std::chrono::high_resolution_clock::now();

		// Safe Start Dialog Gate
		if (s_safeStartPending.load() && ImGui::GetCurrentContext())
		{
			auto t0 = std::chrono::high_resolution_clock::now();
			RenderSafeStartDialog();
			auto t1 = std::chrono::high_resolution_clock::now();
			g_perfSafeStartMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
		}
		else
		{
			g_perfSafeStartMs = 0.0;
		}

		// ── Deferred Warmup Gate ────────────────────────────────────────────────
		// Give GW2, D3D11 swapchains, ArcDPS, FastLoad and NVIDIA Overlay
		// 30 frames of stable rendering before touching DWM magnification.
		if (!s_deferredInitDone.load())
		{
			static int s_renderWarmupFrames = 0;
			if (++s_renderWarmupFrames >= 30)
			{
				EnsureDeferredInitialized();
			}
		}

		// ── Hybrid Scanner Frame Capture & Overlay ──────────────────────────────
		if ((CurrentSettings.EnableHybridMode || CurrentSettings.CommanderTagMode != 0) && s_deferredInitDone.load())
		{
			IDXGISwapChain* swapChain = APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr;
			if (swapChain)
			{
				GetHybridScanner().ScanFrame(swapChain);
			}

			int texW = 0, texH = 0;
			ID3D11ShaderResourceView* srv = GetHybridScanner().GetOverlaySRV(texW, texH);
			if (srv)
			{
				ImVec2 disp = ImGui::GetIO().DisplaySize;
				ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)srv, ImVec2(0, 0), disp);
			}
		}

		if (s_showC64Credits.load())
		{
			RenderC64CreditsOverlay();
		}

		// ── Window 1: CBA Main Window ─────────────────────────────────────────
		if (CurrentSettings.ShowMainWindow && ImGui::GetCurrentContext())
		{
			auto t0 = std::chrono::high_resolution_clock::now();

			if (s_focusMainWindow)
			{
				ImGui::SetNextWindowFocus();
				s_focusMainWindow = false;
			}

			ImVec2 disp = ImGui::GetIO().DisplaySize;
			float screenH = (disp.y > 400.0f) ? disp.y : 1080.0f;
			float defaultW = 530.0f;
			float defaultH = std::clamp(screenH * 0.76f, 620.0f, 860.0f);

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

			// Sleek, variable sidebar sizing: min 480x420, max 1600 x screenH
			ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f, 420.0f), ImVec2(1600.0f, screenH - 40.0f));

			if (s_resetMainWindowPos)
			{
				float posX = 40.0f;
				float posY = 60.0f;
				ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(defaultW, defaultH), ImGuiCond_Always);
				s_resetMainWindowPos = false;
			}
			else
			{
				ImGui::SetNextWindowSize(ImVec2(defaultW, defaultH), ImGuiCond_FirstUseEver);
			}

			bool isDe = (Strings().Enabled[0] == 'A');
			ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
			if (ImGui::Begin(isDe ? "cba4gw2 - Hauptfenster###CBA_MainWindow" : "cba4gw2 - Main Window###CBA_MainWindow", &CurrentSettings.ShowMainWindow, winFlags))
			{
				RenderMainWindow();
			}
			ImGui::End();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor(6);

			auto t1 = std::chrono::high_resolution_clock::now();
			g_perfMainWindowMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
		}
		else
		{
			g_perfMainWindowMs = 0.0;
		}

		// ── Window 2: Sensor & Spectral Graph HUD Window ─────────────────────
		if (CurrentSettings.ShowGraphWindow && ImGui::GetCurrentContext())
		{
			auto t0 = std::chrono::high_resolution_clock::now();

			if (s_focusGraphWindow)
			{
				ImGui::SetNextWindowFocus();
				s_focusGraphWindow = false;
			}

			float clampedOpacity = std::clamp(CurrentSettings.UiOpacity, 0.0f, 1.0f);
			float winBgAlpha = std::clamp(clampedOpacity * 0.96f, 0.05f, 0.96f);
			ImGui::SetNextWindowBgAlpha(winBgAlpha);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.06f, 0.10f, winBgAlpha));

			// Sleek, variable HUD sizing: min 340x280, max 1600x1200
			ImGui::SetNextWindowSizeConstraints(ImVec2(340.0f, 280.0f), ImVec2(1600.0f, 1200.0f));

			if (s_resetGraphWindowPos)
			{
				float w = 440.0f;
				float h = 405.0f;
				float posX = 40.0f;
				float posY = 60.0f;
				ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
				s_resetGraphWindowPos = false;
			}
			else
			{
				ImGui::SetNextWindowSize(ImVec2(440.0f, 405.0f), ImGuiCond_FirstUseEver);
			}

			ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse;
			if (ImGui::Begin("cba graph###CBA_GraphWindow", &CurrentSettings.ShowGraphWindow, winFlags))
			{
				RenderGraphWindow();
			}
			ImGui::End();
			ImGui::PopStyleColor();

			auto t1 = std::chrono::high_resolution_clock::now();
			g_perfSensorGraphMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
		}
		else
		{
			g_perfSensorGraphMs = 0.0;
			g_perfCurvesMs = 0.0;
		}

		// ── Window 3: Filter Lab Detached Floating Window ────────────────────
		if (CurrentSettings.ShowLabWindow && ImGui::GetCurrentContext())
		{
			auto t0 = std::chrono::high_resolution_clock::now();

			if (s_focusLabWindow)
			{
				ImGui::SetNextWindowFocus();
				s_focusLabWindow = false;
			}

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

			ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f, 400.0f), ImVec2(1600.0f, 1200.0f));

			if (s_resetLabWindowPos)
			{
				float posX = 40.0f;
				float posY = 60.0f;
				ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
				ImGui::SetNextWindowSize(ImVec2(620.0f, 520.0f), ImGuiCond_Always);
				s_resetLabWindowPos = false;
			}
			else
			{
				ImGui::SetNextWindowSize(ImVec2(620.0f, 520.0f), ImGuiCond_FirstUseEver);
			}

			bool isDe = (Strings().Enabled[0] == 'A');
			ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse;
			if (ImGui::Begin(isDe ? "cba4gw2 - Filter-Labor###CBA_LabWindow" : "cba4gw2 - Filter Lab###CBA_LabWindow", &CurrentSettings.ShowLabWindow, winFlags))
			{
				bool labChanged = false;
				bool labSaveNeeded = false;
				DrawFilterLabWidget(isDe, labChanged, labSaveNeeded);
				if (labSaveNeeded)
				{
					CurrentSettings.Save(AddonDir);
					Recompute(/*aForce=*/true);
				}
				else if (labChanged)
				{
					Recompute(/*aForce=*/false);
				}
			}
			ImGui::End();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor(6);

			auto t1 = std::chrono::high_resolution_clock::now();
			g_perfFilterLabMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
		}
		else
		{
			g_perfFilterLabMs = 0.0;
		}

		// ── Window 4: Vision Lab Floating Window ─────────────────────────────
		if (CurrentSettings.ShowVisionLabWindow && ImGui::GetCurrentContext())
		{
			auto t0 = std::chrono::high_resolution_clock::now();
			RenderVisionLabWindow();
			auto t1 = std::chrono::high_resolution_clock::now();
			g_perfVisionLabMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
		}
		else
		{
			g_perfVisionLabMs = 0.0;
		}

		auto tEndTotal = std::chrono::high_resolution_clock::now();
		g_perfTotalImGuiMs = std::chrono::duration<double, std::milli>(tEndTotal - tStartTotal).count();
	}

	void AddonLoad(AddonAPI* aApi)
	{
		try
		{
			APIDefs = aApi;
			if (!APIDefs) return;

			if (APIDefs->DataLink.Get)
			{
				NexusLink = (NexusLinkData*)APIDefs->DataLink.Get("DL_NEXUS_LINK");
				MumbleLinkData = (GW2MumbleLink*)APIDefs->DataLink.Get("GW2_MUMBLE_LINK");
			}

			if (APIDefs->ImguiContext)
				ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
			if (APIDefs->ImguiMalloc && APIDefs->ImguiFree)
				ImGui::SetAllocatorFunctions(
					(void* (*)(size_t, void*))APIDefs->ImguiMalloc,
					(void(*)(void*, void*))APIDefs->ImguiFree);

			const char* dir = (APIDefs->Paths.GetAddonDirectory) ? APIDefs->Paths.GetAddonDirectory("cba") : nullptr;
			AddonDir = dir ? dir : "";
			if (!AddonDir.empty())
			{
				std::error_code ec;
				std::filesystem::create_directories(AddonDir, ec);
			}
			CurrentSettings = Settings::Load(AddonDir);

			// Backward compatibility migration: If DetachedWindow was previously set, open Main Window
			if (CurrentSettings.DetachedWindow)
			{
				CurrentSettings.ShowMainWindow = true;
				CurrentSettings.DetachedWindow = false;
			}

			// Initialize hybrid background scanner
			GetHybridScanner().Initialize();
			GetHybridScanner().SetEnabled(CurrentSettings.EnableHybridMode);
			UpdateTagEnhancerConflicts();

			// Session Breadcrumb crash guard: mark session running
			Settings::MarkRunning(AddonDir);

			// Safe-Start Gate: If previous crash happened or AlwaysDirectStart is false, hold at safe gate
			if (CurrentSettings.SafeModeTriggered || !CurrentSettings.AlwaysDirectStart)
			{
				s_safeStartPending.store(true);
				CurrentSettings.Enabled = false; // Filter starts disarmed
			}
			else if (!CurrentSettings.LoadOnStartup)
			{
				CurrentSettings.Enabled = false;
			}

			// Escape closes windows
			if (APIDefs->UI.RegisterCloseOnEscape)
			{
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Hauptfenster###CBA_MainWindow", &CurrentSettings.ShowMainWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Main Window###CBA_MainWindow", &CurrentSettings.ShowMainWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba graph###CBA_GraphWindow", &CurrentSettings.ShowGraphWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Sensor Graph###CBA_GraphWindow", &CurrentSettings.ShowGraphWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Filter-Labor###CBA_LabWindow", &CurrentSettings.ShowLabWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Filter Lab###CBA_LabWindow", &CurrentSettings.ShowLabWindow);
				APIDefs->UI.RegisterCloseOnEscape("cba4gw2 - Vision Lab###CBA_VisionLabWindow", &CurrentSettings.ShowVisionLabWindow);
			}

			// Renderers
			if (APIDefs->Renderer.Register)
			{
				APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);
				APIDefs->Renderer.Register(ERenderType_Render, AddonRenderWindow);
			}

			// WndProc
			if (APIDefs->WndProc.Register)
			{
				APIDefs->WndProc.Register(AddonWndProc);
			}

			// QuickAccess toolbar icon & window toggle keybinds
			if (APIDefs->InputBinds.RegisterWithString)
			{
				APIDefs->InputBinds.RegisterWithString("CBA - Main Window", ProcessKeybind, "CTRL+SHIFT+C");
				APIDefs->InputBinds.RegisterWithString("CBA - Sensor Graph", ProcessKeybind, "CTRL+SHIFT+G");
				APIDefs->InputBinds.RegisterWithString("CBA - Filter Off", ProcessKeybind, "CTRL+SHIFT+O");
			}
			if (APIDefs->Textures.GetOrCreateFromMemory)
			{
				APIDefs->Textures.GetOrCreateFromMemory("CBA_ICON", (void*)kCbaIconPng, kCbaIconPngSize);
			}
			if (APIDefs->QuickAccess.Add && CurrentSettings.ShowQuickAccessIcon)
			{
				APIDefs->QuickAccess.Add("QA_CBA", "CBA_ICON", "CBA_ICON", "CBA - Main Window", "cba4gw2 (Strg+Shift+C / Filter Off: Strg+Shift+O)");
			}

			// Start state watchdog thread (monitors focus transitions every 50ms)
			s_watchdogRunning = true;
			s_watchdogThread = std::thread(WatchdogLoop);
		}
		catch (...)
		{
			// Never allow an unhandled exception to escape AddonLoad
		}
	}

	void AddonUnload()
	{
		try
		{
			// Session Breadcrumb: mark graceful exit
			Settings::MarkCleanExit(AddonDir);

			// Always close windows on game exit so they start closed on next launch
			CurrentSettings.ShowMainWindow = false;
			CurrentSettings.ShowGraphWindow = false;
			CurrentSettings.ShowLabWindow = false;
			CurrentSettings.DetachedWindow = false;
			CurrentSettings.Save(AddonDir);

			s_showC64Credits.store(false);
			StopC64Audio();

			// Stop state watchdog thread
			s_watchdogRunning = false;
			if (s_watchdogThread.joinable())
			{
				s_watchdogThread.join();
			}

			if (APIDefs)
			{
				if (APIDefs->QuickAccess.Remove)
				{
					APIDefs->QuickAccess.Remove("QA_CBA");
				}
				if (APIDefs->InputBinds.Deregister)
				{
					APIDefs->InputBinds.Deregister("CBA - Main Window");
					APIDefs->InputBinds.Deregister("CBA - Filter Off");
					APIDefs->InputBinds.Deregister("CBA - Sensor Graph");
					APIDefs->InputBinds.Deregister("CBA - Not-Aus");
					APIDefs->InputBinds.Deregister("KB_CBA_WINDOW");
				}
				if (APIDefs->UI.DeregisterCloseOnEscape)
				{
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Hauptfenster###CBA_MainWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Main Window###CBA_MainWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba graph###CBA_GraphWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Sensor Graph###CBA_GraphWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Filter-Labor###CBA_LabWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Filter Lab###CBA_LabWindow");
					APIDefs->UI.DeregisterCloseOnEscape("cba4gw2 - Vision Lab###CBA_VisionLabWindow");
				}
				if (APIDefs->WndProc.Deregister)
					APIDefs->WndProc.Deregister(AddonWndProc);
				if (APIDefs->Renderer.Deregister)
				{
					APIDefs->Renderer.Deregister(AddonRenderWindow);
					APIDefs->Renderer.Deregister(AddonOptions);
				}
			}

			GetHybridScanner().Shutdown();
			GetColorEffectController().Shutdown();
		}
		catch (...)
		{
		}
	}
}

BOOL APIENTRY DllMain(HMODULE aModule, DWORD aReasonForCall, LPVOID)
{
	if (aReasonForCall == DLL_PROCESS_ATTACH)
		AddonModuleHandle = aModule;

	return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef.Signature = -78341;
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "cba4gw2";
	AddonDef.Version.Major = 1;
	AddonDef.Version.Minor = 0;
	AddonDef.Version.Build = 2;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "Emisan01";
	AddonDef.Description =
		"Color balance, contrast enhancement and visual assist for Guild Wars 2. "
		"Applied via the Windows Magnification API.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	AddonDef.Provider = EUpdateProvider_GitHub;
	AddonDef.UpdateLink = "https://github.com/Emisan01/cba4gw2";

	return &AddonDef;
}
