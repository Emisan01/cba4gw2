#pragma once
#include <unordered_map>

namespace cba
{
	// Single source of truth for every user-controllable parameter.
	// UI widgets and logic modules read/write through a ParamId, never
	// through a Settings field or a UIState global directly.
	// See CLAUDE.md ("Registry / Control Layer") for why this exists.
	//
	// Storage still physically lives in `CurrentSettings` (no duplication,
	// no change to Save/Load/Export/Import) - the registry just holds a
	// pointer + metadata per parameter and is the only thing new UI code
	// is allowed to talk to.
	//
	// Migration is incremental: existing direct CurrentSettings.* access
	// keeps working untouched. Only newly migrated call sites go through
	// this. See CLAUDE.md for the current migration status.

	enum class ParamId
	{
		EnhancerTolerance,
		GammaGain,
		CommanderTagMode,
		SmartEnhancer,
		Severity01,
		MixedRgSeverity01,
		MixedBySeverity01,
		// Eye-Sensitive Mode (2026-09-09) - its own independent module, see
		// ColorMatrix::EyeComfortMatrix / Settings.h for the full writeup.
		BlueFilter01,
		WarmTint01,
		SaturationReduction01,
		// Not a Settings field (session-only, not persisted) - storage is
		// UIState.cpp's s_activeSlotIdx. Registered anyway: this is the exact
		// "two widgets sharing one cross-window value" case the registry
		// exists for (Main Window + Nexus-embedded panel both read/write it),
		// and it used to be clamped by hand at each of its ~4 write sites
		// (std::clamp(s_activeSlotIdx, 0, 2)) before migration.
		ActiveSlotIdx,
		// Add new IDs here as parameters get migrated. Do not remove or
		// reorder existing entries - do not assume the underlying integer
		// value is stable across builds (no serialization keys off it;
		// ParamMeta::key is what's stable long-term).
		//
		// Not registered here on purpose: BalanceType Type and bool Mixed
		// (radio-button *selection*, not a continuous slider value - the
		// overscaling-via-clamp problem this registry exists to fix doesn't
		// apply to them the same way) and bool Enabled (touched from many
		// non-widget places - keybind, ResetFilterSettingsAndDisable, Watchdog
		// - where registry indirection wouldn't add much over the plain field).
		_Count
	};

	enum class ParamKind { Bool, Float, Int };

	// Which panel a parameter belongs to once the Core/Advanced UI split
	// (CLAUDE.md step 2) happens. Not consumed by anything yet.
	enum class ParamGroup { Core, Advanced };

	struct ParamMeta
	{
		const char* key = "";       // stable string id (debug/log/future use)
		const char* labelEn = "";
		const char* labelDe = "";
		ParamKind   kind = ParamKind::Float;
		ParamGroup  group = ParamGroup::Advanced;
		float minF = 0.0f, maxF = 0.0f; // maxF <= minF means "no clamp"
		int   minI = 0,    maxI = 0;    // maxI <= minI means "no clamp"
	};

	class ParameterRegistry
	{
	public:
		static ParameterRegistry& Get();

		void RegisterFloat(ParamId aId, float* aStorage, ParamMeta aMeta);
		void RegisterBool(ParamId aId, bool* aStorage, ParamMeta aMeta);
		void RegisterInt(ParamId aId, int* aStorage, ParamMeta aMeta);

		float GetFloat(ParamId aId) const;
		void  SetFloat(ParamId aId, float aValue); // clamps to meta min/max

		bool  GetBool(ParamId aId) const;
		void  SetBool(ParamId aId, bool aValue);

		int   GetInt(ParamId aId) const;
		void  SetInt(ParamId aId, int aValue); // clamps to meta min/max

		const ParamMeta& GetMeta(ParamId aId) const;
		bool  IsRegistered(ParamId aId) const;

	private:
		struct Entry
		{
			ParamMeta meta{};
			union Storage { float* f; bool* b; int* i; };
			Storage storage{};
		};
		std::unordered_map<ParamId, Entry> _entries;
	};

	// Wires up every currently-migrated ParamId to its CurrentSettings
	// field. Call once from ModuleMain.cpp AddonLoad, after
	// CurrentSettings has been loaded from disk.
	void RegisterAllParameters();
}
