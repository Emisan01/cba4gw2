#include "ParameterRegistry.h"
#include "Shared.h"
#include "../ui/UIState.h"
#include <algorithm>
#include <cassert>

namespace cba
{
	ParameterRegistry& ParameterRegistry::Get()
	{
		static ParameterRegistry instance;
		return instance;
	}

	void ParameterRegistry::RegisterFloat(ParamId aId, float* aStorage, ParamMeta aMeta)
	{
		aMeta.kind = ParamKind::Float;
		Entry e;
		e.meta = aMeta;
		e.storage.f = aStorage;
		_entries[aId] = e;
	}

	void ParameterRegistry::RegisterBool(ParamId aId, bool* aStorage, ParamMeta aMeta)
	{
		aMeta.kind = ParamKind::Bool;
		Entry e;
		e.meta = aMeta;
		e.storage.b = aStorage;
		_entries[aId] = e;
	}

	void ParameterRegistry::RegisterInt(ParamId aId, int* aStorage, ParamMeta aMeta)
	{
		aMeta.kind = ParamKind::Int;
		Entry e;
		e.meta = aMeta;
		e.storage.i = aStorage;
		_entries[aId] = e;
	}

	// assert() alone used to guard every one of these lookups - fine in a
	// Debug build, but CLAUDE.md's own build workflow uses
	// `cmake --build build --config Release`, which defines NDEBUG and
	// compiles asserts out entirely. A Get/Set on an unregistered ParamId in
	// the actual shipping build used to silently dereference an end()
	// iterator - undefined behavior, not a caught bug (found in the
	// 2026-09-09 codebase review). Every accessor below now has an explicit
	// branch that returns a safe default (Get) or no-ops (Set) instead of
	// relying on assert() as the only line of defense. The assert() calls
	// stay too - still useful to catch this immediately in a Debug build.

	float ParameterRegistry::GetFloat(ParamId aId) const
	{
		auto it = _entries.find(aId);
		assert(it != _entries.end() && it->second.meta.kind == ParamKind::Float);
		if (it == _entries.end() || it->second.meta.kind != ParamKind::Float) return 0.0f;
		return *it->second.storage.f;
	}

	void ParameterRegistry::SetFloat(ParamId aId, float aValue)
	{
		auto it = _entries.find(aId);
		assert(it != _entries.end() && it->second.meta.kind == ParamKind::Float);
		if (it == _entries.end() || it->second.meta.kind != ParamKind::Float) return;
		const ParamMeta& meta = it->second.meta;
		if (meta.maxF > meta.minF)
			aValue = std::clamp(aValue, meta.minF, meta.maxF);
		*it->second.storage.f = aValue;
	}

	bool ParameterRegistry::GetBool(ParamId aId) const
	{
		auto it = _entries.find(aId);
		assert(it != _entries.end() && it->second.meta.kind == ParamKind::Bool);
		if (it == _entries.end() || it->second.meta.kind != ParamKind::Bool) return false;
		return *it->second.storage.b;
	}

	void ParameterRegistry::SetBool(ParamId aId, bool aValue)
	{
		auto it = _entries.find(aId);
		assert(it != _entries.end() && it->second.meta.kind == ParamKind::Bool);
		if (it == _entries.end() || it->second.meta.kind != ParamKind::Bool) return;
		*it->second.storage.b = aValue;
	}

	int ParameterRegistry::GetInt(ParamId aId) const
	{
		auto it = _entries.find(aId);
		assert(it != _entries.end() && it->second.meta.kind == ParamKind::Int);
		if (it == _entries.end() || it->second.meta.kind != ParamKind::Int) return 0;
		return *it->second.storage.i;
	}

	void ParameterRegistry::SetInt(ParamId aId, int aValue)
	{
		auto it = _entries.find(aId);
		assert(it != _entries.end() && it->second.meta.kind == ParamKind::Int);
		if (it == _entries.end() || it->second.meta.kind != ParamKind::Int) return;
		const ParamMeta& meta = it->second.meta;
		if (meta.maxI > meta.minI)
			aValue = std::clamp(aValue, meta.minI, meta.maxI);
		*it->second.storage.i = aValue;
	}

	const ParamMeta& ParameterRegistry::GetMeta(ParamId aId) const
	{
		auto it = _entries.find(aId);
		assert(it != _entries.end());
		if (it == _entries.end())
		{
			static const ParamMeta kEmptyMeta{};
			return kEmptyMeta;
		}
		return it->second.meta;
	}

	bool ParameterRegistry::IsRegistered(ParamId aId) const
	{
		return _entries.find(aId) != _entries.end();
	}

	void RegisterAllParameters()
	{
		ParameterRegistry& reg = ParameterRegistry::Get();

		reg.RegisterFloat(ParamId::EnhancerTolerance, &CurrentSettings.EnhancerTolerance,
			ParamMeta{ "enhancer_tolerance", "Tag Contrast Tolerance", "Tag-Kontrast Toleranz",
				ParamKind::Float, ParamGroup::Core, 0.04f, 0.20f });

		reg.RegisterFloat(ParamId::GammaGain, &CurrentSettings.GammaGain,
			ParamMeta{ "gamma_gain", "Eye Comfort Gain", "Augenschonend-Verstaerkung",
				ParamKind::Float, ParamGroup::Advanced, 0.70f, 1.30f });

		reg.RegisterInt(ParamId::CommanderTagMode, &CurrentSettings.CommanderTagMode,
			ParamMeta{ "commander_tag_mode", "Commander Tag Contrast", "Commander-Tag-Kontrast",
				ParamKind::Int, ParamGroup::Core, 0, 1 });

		reg.RegisterBool(ParamId::SmartEnhancer, &CurrentSettings.SmartEnhancer,
			ParamMeta{ "smart_enhancer", "Smart Auto", "Smart-Auto",
				ParamKind::Bool, ParamGroup::Core, 0, 0 });

		reg.RegisterFloat(ParamId::Severity01, &CurrentSettings.Severity01,
			ParamMeta{ "severity01", "Strength", "Staerke",
				ParamKind::Float, ParamGroup::Core, 0.0f, 1.25f });

		reg.RegisterFloat(ParamId::MixedRgSeverity01, &CurrentSettings.MixedRgSeverity01,
			ParamMeta{ "mixed_rg_severity01", "Red-Green Strength", "Rot-Gruen Staerke",
				ParamKind::Float, ParamGroup::Core, 0.0f, 1.25f });

		reg.RegisterFloat(ParamId::MixedBySeverity01, &CurrentSettings.MixedBySeverity01,
			ParamMeta{ "mixed_by_severity01", "Blue-Yellow Strength", "Blau-Gelb Staerke",
				ParamKind::Float, ParamGroup::Core, 0.0f, 1.25f });

		reg.RegisterInt(ParamId::ActiveSlotIdx, &s_activeSlotIdx,
			ParamMeta{ "active_slot_idx", "Active Profile Slot", "Aktiver Profil-Slot",
				ParamKind::Int, ParamGroup::Core, 0, 2 });

		reg.RegisterFloat(ParamId::BlueFilter01, &CurrentSettings.BlueFilter01,
			ParamMeta{ "blue_filter01", "Blue Light Filter", "Blaufilter",
				ParamKind::Float, ParamGroup::Core, 0.0f, 1.0f });

		reg.RegisterFloat(ParamId::WarmTint01, &CurrentSettings.WarmTint01,
			ParamMeta{ "warm_tint01", "Warm Tint", "Warmton",
				ParamKind::Float, ParamGroup::Core, 0.0f, 1.0f });

		reg.RegisterFloat(ParamId::SaturationReduction01, &CurrentSettings.SaturationReduction01,
			ParamMeta{ "saturation_reduction01", "Saturation Reduction", "Saettigungsreduktion",
				ParamKind::Float, ParamGroup::Core, 0.0f, 1.0f });
	}
}
