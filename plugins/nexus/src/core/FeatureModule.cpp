#include "FeatureModule.h"
#include "Shared.h"
#include "../ui/Theme.h"
#include "../platform/HybridScanner.h"

namespace cba
{
	FeatureModuleRegistry& FeatureModuleRegistry::Get()
	{
		static FeatureModuleRegistry instance;
		return instance;
	}

	void FeatureModuleRegistry::Register(FeatureModuleDef aModule)
	{
		_modules.push_back(std::move(aModule));
	}

	void FeatureModuleRegistry::Clear()
	{
		_modules.clear();
	}

	const std::vector<FeatureModuleDef>& FeatureModuleRegistry::GetAll() const
	{
		return _modules;
	}

	void RegisterAllFeatureModules()
	{
		FeatureModuleRegistry& reg = FeatureModuleRegistry::Get();
		reg.Clear();

		reg.Register(FeatureModuleDef{
			"commander_tag", "Commander Tag", "Commander-Tag",
			[]() { return Theme::kTextGoldLabel; },
			[]() { return CurrentSettings.CommanderTagMode != 0; },
			[](bool) -> std::string {
				return CurrentSettings.SmartEnhancer ? "Com-Tag: Smart-Auto" : "Com-Tag: Preset";
			},
			[]() { CurrentSettings.CommanderTagMode = 0; }
		});

		reg.Register(FeatureModuleDef{
			"hybrid_mode", "Hybrid Mode", "Hybrid-Modus",
			// Was a hardcoded literal, not Theme-driven like commander_tag's
			// kTextGoldLabel - invisible only because Symbiont currently
			// aliases Classic (found in the 2026-09-09 codebase review).
			[]() { return Theme::kHudHybridMode; },
			[]() { return CurrentSettings.EnableHybridMode; },
			nullptr,
			[]() { CurrentSettings.EnableHybridMode = false; GetHybridScanner().SetEnabled(false); }
		});

		reg.Register(FeatureModuleDef{
			"filter_lab", "Filter Lab", "Filter-Labor",
			[]() { return Theme::kHudFilterLab; },
			[]() { return CurrentSettings.LabModeEnabled; },
			nullptr,
			[]() { CurrentSettings.LabModeEnabled = false; }
		});

		reg.Register(FeatureModuleDef{
			"eye_comfort", "Eye-Sensitive", "Eye-Sensitive",
			[]() { return Theme::kHudEyeComfort; },
			[]() { return CurrentSettings.EyeComfortModeEnabled; },
			nullptr,
			[]() {
				CurrentSettings.EyeComfortModeEnabled = false;
				CurrentSettings.BlueFilter01 = 0.0f;
				CurrentSettings.WarmTint01 = 0.0f;
				CurrentSettings.SaturationReduction01 = 0.0f;
			}
		});
	}
}
