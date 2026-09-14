#pragma once
#include "ui/L10n.h"

namespace cba
{
	// One function per Main Window tab, extracted from MainWindow.cpp
	// 2026-09-13 so no single file carries the whole Studio UI.
	void RenderDashboardTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded);
	void RenderEyeComfortTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded);
	// The Gamma/Auto-Brightness pair, Eye-Sensitive toggle, and its three
	// sliders (Blue Filter/Warm Tint/Saturation) - the part of the Eye
	// Comfort tab that is genuinely controls, not the HDR-detection dot.
	// Shared with the Main Window Dashboard's own Eye Comfort tile
	// (2026-09-14, Emi: wants it directly under the Curve View, not only
	// behind its own tab) - one implementation, two homes, same pattern as
	// RenderActiveModulesChips / RenderManualProfileControls.
	void RenderEyeComfortControls(bool isDe, const L10n& t, bool& changed, bool& saveNeeded);
	void RenderSystemTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded);

	// "Start automatically with GW2" - the single AutoStartSlot value's one
	// control, shared between the Dashboard's Profile Management tile and
	// the embedded Nexus panel (2026-09-14, Emi: the panel's Commander-Tag
	// checkbox "wird zu Start with GW2" - replaced, not duplicated).
	// aCompact drops the "Profile at launch:" label row for tight spaces.
	void DrawAutoStartControl(bool& aSaveNeeded, bool aIsDe, bool aCompact);
}
