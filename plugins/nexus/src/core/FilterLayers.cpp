#include "FilterLayers.h"
#include "Shared.h"
#include "ColorMatrix.h"

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

	void ActiveCorrectionMatrix(double aOut3x3[3][3])
	{
		if (CurrentSettings.Mixed)
		{
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01,
				aOut3x3);
		}
		else
		{
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, aOut3x3);
		}
	}

	void ColorStackMatrix(double aOut3x3[3][3])
	{
		ActiveCorrectionMatrix(aOut3x3);

		// Eye-Sensitive Mode composes ON TOP of the CVD correction (tinted
		// lens in front of an already-corrected image), never replaces it -
		// so it left-multiplies. Off by default: EyeComfortMatrix(0,0,0) is
		// identity, making this branch a true no-op unless actually turned up.
		if (CurrentSettings.EyeComfortModeEnabled)
		{
			double eyeComfort[3][3];
			ColorMatrix::EyeComfortMatrix(CurrentSettings.BlueFilter01, CurrentSettings.WarmTint01,
				CurrentSettings.SaturationReduction01, eyeComfort);
			double composed[3][3];
			for (int r = 0; r < 3; ++r)
				for (int c = 0; c < 3; ++c)
				{
					double sum = 0.0;
					for (int k = 0; k < 3; ++k) sum += eyeComfort[r][k] * aOut3x3[k][c];
					composed[r][c] = sum;
				}
			for (int r = 0; r < 3; ++r)
				for (int c = 0; c < 3; ++c)
					aOut3x3[r][c] = composed[r][c];
		}
	}

	void EffectiveDisplayMatrix(double aOut3x3[3][3])
	{
		if (!CurrentSettings.Enabled)
		{
			for (int r = 0; r < 3; ++r)
				for (int c = 0; c < 3; ++c)
					aOut3x3[r][c] = (r == c) ? 1.0 : 0.0;
			return;
		}

		ColorStackMatrix(aOut3x3);

		// Linear brightness scaling (GammaGain, 0.70 - 1.30).
		for (int r = 0; r < 3; ++r)
			for (int c = 0; c < 3; ++c)
				aOut3x3[r][c] *= CurrentSettings.GammaGain;
	}
}
