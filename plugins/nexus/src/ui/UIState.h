#pragma once
#include <atomic>
#include <array>
#include "Shared.h"

namespace cba
{
	// ── Transient UI signals - deliberately NOT ParameterRegistry params ──────
	// Everything below is a one-shot cross-thread command or internal runtime
	// state, not a persisted user-adjustable value: no min/max, no label, never
	// bound to a generic slider/checkbox, usually self-resetting within a
	// frame. Categorically checked against the registry 2026-09-09 (see
	// CLAUDE.md) - the one field in this file that WAS a genuine fit
	// (s_activeSlotIdx, a value two separate windows read/write and used to
	// clamp by hand) has been migrated to ParamId::ActiveSlotIdx below. The
	// rest stays here on purpose.
	extern std::atomic<bool> s_resetMainWindowPos;
	extern std::atomic<bool> s_resetGraphWindowPos;
	extern std::atomic<bool> s_resetLabWindowPos;
	extern std::atomic<bool> s_resetVisionLabWindowPos;

	extern std::atomic<bool> s_focusMainWindow;
	extern std::atomic<bool> s_focusGraphWindow;
	extern std::atomic<bool> s_focusLabWindow;
	extern std::atomic<bool> s_focusVisionLabWindow;

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
	// The one "turn everything off and back to neutral" reset: base correction,
	// Commander Tag Enhancer, Hybrid Mode, Free Filter, Filter Lab - all off.
	// Used by the "CBA - Filter Off" keybind, the Sensor Graph HUD's Reset
	// button, and the Main Window's Reset Filter button, so there's exactly one
	// implementation instead of three that can drift apart (see CLAUDE.md,
	// 2026-09-09 - the keybind used to only clear Enabled, leaving Commander Tag
	// Enhancer's overlay running, which looked like "the filter is still on").
	void ResetFilterSettingsAndDisable();
	// One-click "make commander tags contrast-safe for my CVD type" - used by
	// the quick-profile buttons in both the Nexus-embedded panel and the Main
	// Window's Section 1, so there's one implementation, not two that can
	// drift (same reasoning as ResetFilterSettingsAndDisable above).
	void ActivateCommanderTagProfile(BalanceType aType);
	// The master ON/OFF toggle's actual state change (flip Enabled, Save,
	// Recompute, refresh the QuickAccess icon) - used by both the Main
	// Window's and the Nexus-embedded panel's own toggle button, which used
	// to duplicate this exact sequence. Each caller still draws its own
	// button/styling/banner around it (the Studio version has an OS-block
	// banner and a pulsing border the compact panel doesn't need), only the
	// state-changing logic is shared.
	void ToggleMasterEnabled();

	// Which Slots[] index the UI currently treats as "active" (highlighted,
	// and where "Save" writes to). Physical storage for
	// ParamId::ActiveSlotIdx (see ParameterRegistry.cpp) - read/write through
	// ParameterRegistry::Get().GetInt/SetInt(ParamId::ActiveSlotIdx), not
	// this variable directly, so the Main Window and the Nexus-embedded
	// panel's Profile Slot pickers stay in sync and share one clamp (used to
	// be two independent statics, then a shared extern with std::clamp
	// repeated at each write site).
	extern int s_activeSlotIdx;

	// Hold-to-compare (2026-09-11, PRODUCT_CONCEPT.md section 3.2): true only
	// while the compare keybind is physically held down. Suspends BOTH filter
	// stages - the DWM matrix in Recompute() and the tag overlay in
	// AddonRender - so the user sees the completely unfiltered game and can
	// answer "is this actually doing anything?" against real content instead
	// of an abstract swatch.
	//
	// Deliberately NOT a Settings field: it is transient input state, must
	// never persist across a session, and a crash while held must not leave
	// the filter permanently suppressed. Nexus's own InputBinds reports the
	// release edge (KEYBINDS_PROCESS's aIsRelease), so this needs no second
	// raw-WM_KEYDOWN path - that one was removed deliberately, see CLAUDE.md.
	extern std::atomic<bool> s_compareHoldActive;

	void DrawFilterStatusIndicator(bool aWithText);
	BrightnessRetentionResult GetBrightnessRetention();
	void UpdateQuickAccessIcon();

	// Gamma/Auto-Brightness logic, deduplicated 2026-09-09 - this exact
	// sequence used to be copy-pasted at ~10 call sites across MainWindow.cpp
	// (Section 1 and the Eye Comfort section) and SensorGraphHUD.cpp:
	// - SetGammaGainManual: what every manual Gamma slider drag and every
	//   "Reset" button does - write the value, and manual input always wins
	//   over Auto-Brightness (clears the flag).
	// - ApplyAutoBrightnessGain: what both "enable the Auto-Brightness
	//   checkbox" and both "Apply Target" buttons do - jump GammaGain to the
	//   currently recommended retention value. Deliberately does not touch
	//   the AutoBrightness flag itself (the checkbox sets it via its own
	//   bool&, the Apply-Target buttons are one-shot and don't turn Auto on).
	// - SyncAutoBrightnessGain: the passive per-frame drift correction (if
	//   Auto-Brightness is on and the CVD profile changed elsewhere since the
	//   last recommended-gain calculation, catch GammaGain back up) - was
	//   duplicated in the Main Window's Eye Comfort section and the Sensor
	//   Graph HUD; Section 1 never had it, so this is dedup only, not new
	//   behavior added anywhere it wasn't already.
	void SetGammaGainManual(float aGain);
	void ApplyAutoBrightnessGain();
	void SyncAutoBrightnessGain(bool& aChanged, bool& aSaveNeeded);

	// Resets every CBA window's position/size to its default top-left layout
	// plus the movable toolbar icon's position - used by both "Reset UI"
	// buttons (Main Window and the Nexus-embedded panel), which had drifted
	// apart (found 2026-09-09): the embedded panel's version was missing the
	// Vision Lab window entirely and never reset the toolbar icon position,
	// even though both buttons say "Reset UI" and a user has no way to know
	// the two aren't equivalent.
	void ResetUiLayout();

	void StartC64Audio();
	void StopC64Audio();

	// ── Per-Window Performance Watchdog Timers (Debug Mode) ──────────────
	extern double g_perfMainWindowMs;
	extern double g_perfSensorGraphMs;
	extern double g_perfCurvesMs;
	extern double g_perfFilterLabMs;
	extern double g_perfVisionLabMs;
	extern double g_perfSafeStartMs;
	extern double g_perfTotalImGuiMs;
}
