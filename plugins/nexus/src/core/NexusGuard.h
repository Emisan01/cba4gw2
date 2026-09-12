#pragma once
#include <imgui.h>

namespace cba
{
	// ── Render Context Safeguards ───────────────────────────────────────────
	// Validates that the host environment (Guild Wars 2, Nexus, and ArcDPS)
	// has provided a healthy ImGui context. If this fails, the addon must
	// gracefully yield the frame rather than crashing the game via Access
	// Violation.

	inline bool IsRenderContextSafe()
	{
		// Context must exist
		if (!ImGui::GetCurrentContext())
			return false;

		// The display size must be reasonable (not tearing down)
		ImVec2 dispSize = ImGui::GetIO().DisplaySize;
		if (dispSize.x <= 0 || dispSize.y <= 0)
			return false;

		return true;
	}
}

// Drops the current function immediately if the context is unsafe.
#define CBA_GUARD_RENDER_CONTEXT() \
	if (!cba::IsRenderContextSafe()) return;

