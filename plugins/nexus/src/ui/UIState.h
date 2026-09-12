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

	// ── Profile slots: one writer, one reader ───────────────────────────────
	//
	// Replaces three hand-rolled slot writers that had each drifted to a
	// different subset of the filter state. The worst of them wrote
	// Mixed = false and never wrote the mixed severities, so a mixed-profile
	// user silently lost them on save.
	//
	// SaveSettingsToSlot captures the complete current filter state (see
	// Settings::ProfileSlot) and marks the slot used. It auto-names an empty
	// slot and never overwrites a name the user chose. It does NOT save the
	// settings file - the caller decides when that happens, because a slot
	// write is usually one of several changes in the same frame.
	//
	// LoadSettingsFromSlot applies a slot to the live settings. It writes
	// through ParameterRegistry wherever a parameter is registered, so every
	// value is clamped by the same ParamMeta a slider obeys. It deliberately
	// does NOT touch Enabled: a slot stores what the filter looks like, not
	// whether it is running. Returns false for an unused or out-of-range
	// slot, in which case nothing is written at all.
	void SaveSettingsToSlot(int aSlotIndex);
	bool LoadSettingsFromSlot(int aSlotIndex);

	// ── The screen-effect gate (one rule, one place, 2026-09-12) ───────────
	//
	// ShouldScreenEffectBeActive() is the single answer to "should the
	// screen-wide colour effect be installed right now": not minimized, and
	// either GW2 is the foreground process or the user asked for it to stay
	// on in the background (Settings.SystemWide). It used to be written out
	// by hand in three places - Recompute(), WatchdogLoop() and
	// DrawFilterStatusIndicator()'s inverse - which is how a rule this small
	// becomes three rules.
	//
	// SyncScreenEffectToGate() makes the actual screen match that answer, and
	// is the only thing any caller needs. It is idempotent (guarded by
	// s_hasApplied) and safe to call from any thread, so the render callback,
	// the Watchdog and the WndProc all just poke it instead of each
	// hand-rolling apply/clear.
	//
	// Why the render thread pokes it at all (this is the actual bug fix):
	// the Nexus log from 2026-09-12 contains only "(Clear) REJECTED" lines,
	// never an Apply rejection. Apply runs from the render thread, which is
	// also where MagInitialize() ran; Clear on focus loss ran from the WndProc
	// or Watchdog thread, while GW2 was already in the background. So the
	// filter reliably switched ON and unreliably switched OFF - it could stay
	// applied across the whole desktop while the user was in another app.
	// Evaluating the gate on the render thread gives the clear the same
	// conditions the apply has.
	bool ShouldScreenEffectBeActive();
	void SyncScreenEffectToGate();
	// Whether a non-identity effect is currently believed to be installed in
	// DWM. Exposed so SelfTest can compare it against the gate - "applied
	// while the gate says it should be off" is the stuck-on-the-desktop bug,
	// and it is the one state nobody can see from inside the game.
	bool IsScreenEffectApplied();
	// Whether the shader backend should be painting this frame. Short by
	// design - see its definition.
	bool ShouldShaderPassRun();
	// Is the 50ms safety-net thread alive? It carries the stuck-effect
	// recovery, focus tracking and the HybridScanner self-heal, so its
	// death is silent and expensive - exactly the shape of bug it was
	// built to catch in other threads.
	bool IsWatchdogRunning();

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
	// Auto-Brightness steered by the sensor instead of the matrix
	// prediction. No-op unless the shader backend is painting,
	// Auto-Brightness is on, its source is set to the sensor, and a fresh
	// reading exists. Returns true if it moved the gain.
	bool ApplySensorBrightnessCorrection();
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
	// CPU time spent submitting the colour pass (copy + one draw). GPU
	// time is not visible from here, so read this as "what the pass costs
	// the game's render thread", not as the full cost.
	extern double g_perfShaderPassMs;
	extern double g_perfFilterLabMs;
	extern double g_perfVisionLabMs;
	extern double g_perfSafeStartMs;
	extern double g_perfTotalImGuiMs;
}
