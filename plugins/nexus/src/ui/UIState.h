#pragma once
#include <atomic>
#include <array>
#include "Shared.h"

namespace cba
{
	extern std::atomic<bool> s_resetMainWindowPos;
	extern std::atomic<bool> s_resetGraphWindowPos;
	extern std::atomic<bool> s_resetDetachedWindowPos;
	extern std::atomic<bool> s_resetLabWindowPos;

	extern std::atomic<bool> s_focusMainWindow;
	extern std::atomic<bool> s_focusGraphWindow;
	extern std::atomic<bool> s_focusLabWindow;

	extern std::atomic<bool> s_safeStartPending;
	extern std::atomic<bool> s_showC64Credits;
	extern std::atomic<bool> s_deferredInitDone;
	extern bool s_showGraphOpacityDrawer;

	extern std::atomic<HWND> s_gw2Hwnd;
	extern std::atomic<bool> s_gw2Minimized;

	struct TagConflictState
	{
		bool inConflict = false;
		float repR = 0.0f, repG = 0.0f, repB = 0.0f;
	};
	extern std::array<TagConflictState, 9> s_tagConflictStates;

	struct BrightnessRetentionResult
	{
		float retentionRatio = 1.0f;
		float recommendedGain = 1.0f;
	};

	void Recompute(bool aForce = false);
	void EnsureDeferredInitialized();
	void UpdateTagEnhancerConflicts();
	void DrawFilterStatusIndicator(bool aWithText);
	BrightnessRetentionResult GetBrightnessRetention();
	void UpdateQuickAccessIcon();

	void StartC64Audio();
	void StopC64Audio();

	// ── Per-Window Performance Watchdog Timers (Debug Mode) ──────────────
	extern double g_perfMainWindowMs;
	extern double g_perfSensorGraphMs;
	extern double g_perfCurvesMs;
	extern double g_perfFilterLabMs;
	extern double g_perfSafeStartMs;
	extern double g_perfTotalImGuiMs;
}
