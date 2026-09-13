#pragma once
#include "ui/L10n.h"

namespace cba
{
	// One function per Main Window tab, extracted from MainWindow.cpp
	// 2026-09-13 so no single file carries the whole Studio UI.
	void RenderDashboardTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded);
	void RenderEyeComfortTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded);
	void RenderSystemTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded);
}
