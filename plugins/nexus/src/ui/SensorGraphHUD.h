#pragma once
#include <imgui.h>

namespace cba
{
	void RenderGraphWindow();

	void DrawSpectralGraphPanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                           const double aM[3][3], bool aIsDetached, float aOpacity, int aMode);
}
