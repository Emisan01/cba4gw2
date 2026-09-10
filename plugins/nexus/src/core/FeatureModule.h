#pragma once
#include <imgui.h>
#include <functional>
#include <string>
#include <vector>

namespace cba
{
	// Self-registering "feature module" (2026-09-09) - the fix for the
	// wiring tax Emi flagged: every optional feature (Commander Tag, Hybrid
	// Mode, Filter Lab, Eye-Sensitive Mode) used to need its own hand-written
	// line in BOTH ResetFilterSettingsAndDisable() AND the Sensor Graph
	// HUD's "active modules" status list - miss one and Reset Filter leaves
	// a feature half-on, or the HUD silently doesn't show it running.
	//
	// A module registers once (RegisterAllFeatureModules(), ModuleMain.cpp
	// AddonLoad) and both call sites become generic loops over
	// FeatureModuleRegistry::Get().GetAll() instead of growing a new
	// hand-written branch per feature. Deliberately scoped to just these two
	// call sites (not Export/Import preset string, not Self-Test) - those
	// were already working and tested, this only generalizes the two spots
	// that actually caused repeated manual wiring today.
	//
	// NOT for the base CVD state (Type/Severity/Mixed/Enabled) - same
	// reasoning ParameterRegistry.h already documents for excluding those:
	// they're the core always-present behavior, not an optional add-on
	// module, and Reset Filter's handling of them is already a single,
	// obvious block at the top of the function.
	struct FeatureModuleDef
	{
		const char* key;      // stable id (debug/log/future use)
		const char* labelEn;
		const char* labelDe;
		// A function, not a stored ImVec4 - Theme::kXxx are runtime-mutable
		// globals (the Advanced UI Theme switch, also 2026-09-09), so this
		// must read the CURRENT value each time, not freeze a copy from
		// registration time.
		std::function<ImVec4()> hudColor;

		std::function<bool()> isActive;
		// Optional: full status text (e.g. "Com-Tag: Smart-Auto") instead of
		// the plain label. Leave null to just use labelEn/labelDe.
		std::function<std::string(bool aIsDe)> statusText;
		// Turns this module fully off / back to neutral. Required.
		std::function<void()> resetToNeutral;
	};

	class FeatureModuleRegistry
	{
	public:
		static FeatureModuleRegistry& Get();
		void Register(FeatureModuleDef aModule);
		// Drops every registered module. RegisterAllFeatureModules() calls
		// this before re-registering - AddonLoad() can run more than once
		// per DLL lifetime (Nexus disable/enable, no full reload), and this
		// registry is a vector, not ParameterRegistry's naturally-idempotent
		// map: without this, a second AddonLoad would duplicate every
		// module (the HUD would show "Commander Tag" twice).
		void Clear();
		const std::vector<FeatureModuleDef>& GetAll() const;

	private:
		std::vector<FeatureModuleDef> _modules;
	};

	// Wires up every currently-migrated feature module. Call once from
	// ModuleMain.cpp AddonLoad, after Settings has been loaded. Safe to call
	// more than once (clears first).
	void RegisterAllFeatureModules();
}
