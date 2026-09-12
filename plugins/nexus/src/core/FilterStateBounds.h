#pragma once

namespace cba
{
	// ── The one place a filter value's legal range is written down ──────────
	//
	// There were three hand-maintained enumerations of "what is a filter
	// setting", all different sizes and none agreeing: ProfileSlot carried 6
	// fields, ExportPresetString 13, ParameterRegistry 10. A profile shared as
	// a code therefore carried MORE state than the same profile saved into a
	// local slot - Eye Comfort and Commander Tag settings survived sharing and
	// were lost by saving.
	//
	// ParameterRegistry already owns the bounds for the params it knows, but it
	// cannot be the single source here: ModuleMain.cpp runs
	// `CurrentSettings = Settings::Load(...)` BEFORE `RegisterAllParameters()`,
	// so GetMeta() asserts on an empty registry during load - exactly when an
	// untrusted, possibly hand-edited ini most needs bounding. These constants
	// exist at compile time, so both Settings::Load and RegisterAllParameters
	// read the same numbers, and a range can no longer be "fixed" in one place
	// and left stale in the other.
	//
	// Rule for changing anything here: a bound may be widened or narrowed, but
	// never silently - it governs both what a live slider allows and what an
	// existing settings.ini is allowed to restore.
	namespace FilterBounds
	{
		// Severity and the two mixed axes deliberately reach past 1.0: above
		// 100% corrects harder than the plain simulation prescribes, for cases
		// where the neutral correction is not enough yet.
		constexpr float kSeverityMin = 0.0f;
		constexpr float kSeverityMax = 1.25f;

		// Brightness scaling. Below 1.0 darkens, above brightens; the ceiling
		// is where clipping starts eating the gain rather than delivering it.
		constexpr float kGammaGainMin = 0.70f;
		constexpr float kGammaGainMax = 1.30f;

		// The three Eye-Sensitive axes are plain 0..1 fractions.
		constexpr float kEyeAxisMin = 0.0f;
		constexpr float kEyeAxisMax = 1.0f;

		// How far a screen colour may differ from a reference tag colour and
		// still count as that tag.
		constexpr float kToleranceMin = 0.04f;
		constexpr float kToleranceMax = 0.20f;

		// Commander Tag contrast: off or on.
		constexpr int kCommanderTagModeMin = 0;
		constexpr int kCommanderTagModeMax = 1;

		// A slot name shares an ini line with its key, and Load splits on the
		// first '=' and takes the rest of the line - so a CR or LF inside a
		// name would become a live top-level key on the next load. Names are
		// stripped of control characters on write and bounded on read; 63 is
		// the rename control's own char[64] buffer.
		constexpr int kSlotNameMaxLength = 63;
	}
}
