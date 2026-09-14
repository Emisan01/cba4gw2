#pragma once
#include <imgui.h>
#include "ui/L10n.h"

namespace cba
{
	void RenderGraphWindow();

	void DrawSpectralGraphPanel(ImDrawList* aDraw, ImVec2 aOrigin, float aW, float aH,
	                           const double aM[3][3], bool aIsDetached, float aOpacity, int aMode);

	// Chip row of currently-active filter/feature modules (Filter, Commander
	// Tag, Hybrid Mode, Eye-Sensitive, ...). Shared between the Sensor Graph
	// HUD and the Main Window Dashboard tile - one implementation, two homes.
	void RenderActiveModulesChips(bool isDe);

	// Manual profile controls (2026-09-12): type buttons, Mixed radio, the
	// strength slider(s). "Set profile, strength and contrast directly - no
	// guided questions." Originally the Sensor Graph HUD's own section;
	// shared with the Main Window Dashboard's Curve View tile 2026-09-14
	// (Emi: "unsere manuell setzbaren Filter... die brauchen wir zurueck")
	// - one implementation, two homes, same pattern as
	// RenderActiveModulesChips above. The Advanced (tolerance/reference
	// values) tree stays Sensor-Graph-only, next to the sensor readouts it
	// was moved beside on 2026-09-12.
	void RenderManualProfileControls(bool isDe, const L10n& t, bool& changed, bool& saveNeeded);
}
