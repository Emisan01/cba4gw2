#pragma once
#include <imgui.h>

namespace cba
{
	void RenderGraphWindow();

	void DrawSpectralGraphPanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                           const double aM[3][3], bool aIsDetached, float aOpacity, int aMode);

	// Chip row of currently-active filter/feature modules (Filter, Commander
	// Tag, Hybrid Mode, Eye-Sensitive, ...). Shared between the Sensor Graph
	// HUD and the Main Window Dashboard tile - one implementation, two homes.
	void RenderActiveModulesChips(bool isDe);
}
