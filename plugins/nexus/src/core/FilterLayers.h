#pragma once
#include <string>
#include <vector>

namespace cba
{
	// Filter Layer Matrix (2026-09-11) - Emi's proposal to replace guessing
	// at "should automation override manual settings" with a simple,
	// user-visible, user-reorderable priority list instead. List order IS
	// the precedence: a pixel matching a higher (earlier) layer's target
	// always wins over a lower layer's, no hidden rules.
	struct FilterLayerInfo
	{
		int id;             // -1 = Commander Tag Auto-Contrast, >=0 = index into CurrentSettings.LabFilters
		std::string nameEn;
		std::string nameDe;
		bool enabled;       // whether this layer is currently contributing targets
		int priority;       // raw stored value, lower = higher priority
	};

	// Every currently-relevant layer (Commander Tag Auto-Contrast if
	// CommanderTagMode != 0, every LabFilters entry regardless of its own
	// .Enabled so disabled ones stay visible/reorderable), sorted by
	// priority ascending (index 0 = wins first for any pixel it matches).
	// This is the single source of truth both the Filter Layer Matrix UI
	// widget and UpdateTagEnhancerConflicts() (which assigns sequential
	// match-priority ranks from this same order) read from.
	std::vector<FilterLayerInfo> GetFilterLayerOrder();

	// Swaps this layer's stored priority with its neighbor in the given
	// direction (-1 = move up/higher-priority, +1 = move down/lower-
	// priority). No-op if the layer is already at that end of the list.
	void MoveFilterLayer(int aId, int aDirection);
}
