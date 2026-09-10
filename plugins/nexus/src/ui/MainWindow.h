#pragma once

namespace cba
{
	void RenderMainWindow();
	void RenderEmbeddedOptions();
	void RenderMovableToolbarIcon();
	// "Without Filter (CVD)" vs "With CBA Filter (Boost)" comparison cards +
	// the color-pair picker above them. Shared between the Main Window's
	// Section 1 and the Nexus-embedded panel so there's one implementation.
	void DrawContrastTestSwatches(bool isDe, const double aCorrMat[3][3], bool& saveNeeded);
}
