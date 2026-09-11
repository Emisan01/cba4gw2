#include "FilterLayers.h"
#include "Shared.h"

#include <algorithm>

namespace cba
{
	std::vector<FilterLayerInfo> GetFilterLayerOrder()
	{
		std::vector<FilterLayerInfo> layers;

		if (CurrentSettings.CommanderTagMode != 0)
		{
			layers.push_back({ -1, "Commander Tag Auto-Contrast", "Commander-Tag Auto-Kontrast",
				true, CurrentSettings.CommanderTagLayerPriority });
		}

		for (size_t i = 0; i < CurrentSettings.LabFilters.size(); ++i)
		{
			const auto& f = CurrentSettings.LabFilters[i];
			layers.push_back({ (int)i, f.Name, f.Name, f.Enabled, f.LayerPriority });
		}

		std::stable_sort(layers.begin(), layers.end(),
			[](const FilterLayerInfo& a, const FilterLayerInfo& b) { return a.priority < b.priority; });

		return layers;
	}

	void MoveFilterLayer(int aId, int aDirection)
	{
		if (aDirection != -1 && aDirection != 1) return;

		std::vector<FilterLayerInfo> layers = GetFilterLayerOrder();
		int idx = -1;
		for (size_t i = 0; i < layers.size(); ++i)
		{
			if (layers[i].id == aId) { idx = (int)i; break; }
		}
		if (idx < 0) return;

		int otherIdx = idx + aDirection;
		if (otherIdx < 0 || otherIdx >= (int)layers.size()) return;

		auto priorityOf = [](int aLayerId) -> int& {
			if (aLayerId == -1) return CurrentSettings.CommanderTagLayerPriority;
			return CurrentSettings.LabFilters[aLayerId].LayerPriority;
		};

		std::swap(priorityOf(layers[idx].id), priorityOf(layers[otherIdx].id));
	}
}
