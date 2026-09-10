#include "SelfTest.h"
#include "ColorMatrix.h"
#include "ParameterRegistry.h"
#include "Settings.h"
#include "ColorEffectController.h"
#include "Shared.h"
#include "../platform/HybridScanner.h"
#include "../ui/UIState.h"
#include <cmath>

namespace cba
{
	namespace
	{
		bool Near(double aA, double aB, double aEps = 1e-6) { return std::fabs(aA - aB) <= aEps; }

		bool MatrixNear(const double aA[3][3], const double aB[3][3], double aEps = 1e-6)
		{
			for (int i = 0; i < 3; ++i)
				for (int j = 0; j < 3; ++j)
					if (!Near(aA[i][j], aB[i][j], aEps)) return false;
			return true;
		}

		const double kIdentity[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };

		void Add(std::vector<SelfTestCheck>& aOut, const char* aCategory, const char* aName, bool aPassed, std::string aDetail = "")
		{
			aOut.push_back({ aCategory, aName, aPassed, false, std::move(aDetail) });
		}

		void AddInfo(std::vector<SelfTestCheck>& aOut, const char* aCategory, const char* aName, bool aPassed, std::string aDetail = "")
		{
			aOut.push_back({ aCategory, aName, aPassed, true, std::move(aDetail) });
		}
	}

	std::vector<SelfTestCheck> RunSelfTest()
	{
		std::vector<SelfTestCheck> r;
		r.reserve(24);

		// ── Color math invariants - same properties tests/test_color_matrix.cpp
		// checks offline, run live in the actual loaded DLL so a math
		// regression shows up here too even if someone forgot to run the
		// standalone suite before deploying ─────────────────────────────
		{
			bool allIdentity = true;
			for (BalanceType t : { BalanceType::Protan, BalanceType::Deutan, BalanceType::Tritan })
			{
				double m[3][3];
				ColorMatrix::CorrectionMatrix(t, 0.0, m);
				if (!MatrixNear(m, kIdentity)) allIdentity = false;
			}
			Add(r, "Color Math", "CorrectionMatrix(severity=0) is identity for all 3 types", allIdentity);
		}
		{
			double wr, wg, wb, br, bg, bb;
			ColorMatrix::SimulatePixel(1.0, 1.0, 1.0, BalanceType::Deutan, wr, wg, wb);
			ColorMatrix::SimulatePixel(0.0, 0.0, 0.0, BalanceType::Deutan, br, bg, bb);
			Add(r, "Color Math", "SimulatePixel preserves white and black",
				Near(wr, 1.0, 1e-3) && Near(wg, 1.0, 1e-3) && Near(wb, 1.0, 1e-3) &&
				Near(br, 0.0, 1e-3) && Near(bg, 0.0, 1e-3) && Near(bb, 0.0, 1e-3));
		}
		{
			double m[3][3];
			ColorMatrix::CorrectionMatrix(BalanceType::Protan, 1.0, m);
			double outR, outG, outB;
			ColorMatrix::ApplyPixel(0.85, 0.27, 0.24, m, outR, outG, outB); // GW2's reference "red" tag color
			bool changed = !Near(outR, 0.85, 1e-3) || !Near(outG, 0.27, 1e-3) || !Near(outB, 0.24, 1e-3);
			Add(r, "Color Math", "Full-severity correction visibly shifts a saturated pixel (not a silent no-op)", changed);
		}
		{
			double m[3][3];
			ColorMatrix::MixedCorrectionMatrix(0.0, 0.0, m);
			Add(r, "Color Math", "MixedCorrectionMatrix(0,0) is identity", MatrixNear(m, kIdentity));
		}
		{
			// Eye-Sensitive Mode (2026-09-09) - fully off must be a true
			// no-op, same invariant the CVD correction matrices already
			// guarantee at severity=0.
			double m[3][3];
			ColorMatrix::EyeComfortMatrix(0.0, 0.0, 0.0, m);
			Add(r, "Color Math", "EyeComfortMatrix(0,0,0) is identity", MatrixNear(m, kIdentity));
		}

		// ── Registry integrity - every declared ParamId must actually be
		// registered (RegisterAllParameters ran and covers the whole enum),
		// and every live value must fall within its own declared ParamMeta
		// range - catches both "forgot to register a new ParamId" and
		// "something bypassed the registry's clamp and wrote out-of-range" ──
		{
			ParameterRegistry& reg = ParameterRegistry::Get();
			bool allRegistered = true;
			bool allInRange = true;
			for (int i = 0; i < static_cast<int>(ParamId::_Count); ++i)
			{
				ParamId id = static_cast<ParamId>(i);
				if (!reg.IsRegistered(id)) { allRegistered = false; continue; }
				const ParamMeta& meta = reg.GetMeta(id);
				if (meta.kind == ParamKind::Float)
				{
					float v = reg.GetFloat(id);
					if (meta.maxF > meta.minF && (v < meta.minF || v > meta.maxF)) allInRange = false;
				}
				else if (meta.kind == ParamKind::Int)
				{
					int v = reg.GetInt(id);
					if (meta.maxI > meta.minI && (v < meta.minI || v > meta.maxI)) allInRange = false;
				}
			}
			Add(r, "Registry", "Every declared ParamId is registered", allRegistered);
			Add(r, "Registry", "Every registered parameter's live value is within its declared range", allInRange);
		}

		// ── Settings export/import roundtrip - the exact bug class CLAUDE.md
		// documents for the L10n positional-string arrays (a shifted field
		// silently reads as a different one). Runs on a throwaway Settings
		// instance, never touches CurrentSettings ──────────────────────────
		{
			std::string exported = CurrentSettings.ExportPresetString();
			Settings scratch; // defaults, deliberately not a copy of CurrentSettings
			std::string err;
			bool imported = scratch.ImportPresetString(exported, &err);
			bool roundtripOk = imported
				&& scratch.Type == CurrentSettings.Type
				&& Near(scratch.Severity01, CurrentSettings.Severity01, 0.01)
				&& scratch.Mixed == CurrentSettings.Mixed
				&& Near(scratch.MixedRgSeverity01, CurrentSettings.MixedRgSeverity01, 0.01)
				&& Near(scratch.MixedBySeverity01, CurrentSettings.MixedBySeverity01, 0.01)
				&& Near(scratch.GammaGain, CurrentSettings.GammaGain, 0.01)
				&& scratch.CommanderTagMode == CurrentSettings.CommanderTagMode
				&& scratch.SmartEnhancer == CurrentSettings.SmartEnhancer
				&& scratch.EyeComfortModeEnabled == CurrentSettings.EyeComfortModeEnabled
				&& Near(scratch.BlueFilter01, CurrentSettings.BlueFilter01, 0.01)
				&& Near(scratch.WarmTint01, CurrentSettings.WarmTint01, 0.01)
				&& Near(scratch.SaturationReduction01, CurrentSettings.SaturationReduction01, 0.01);
			Add(r, "Persistence", "Export -> Import preset string roundtrips without field drift", roundtripOk,
				imported ? "" : ("Import failed: " + err));
		}

		// ── Cross-field state consistency - the actual shape of the bugs
		// found and fixed 2026-09-09 ────────────────────────────────────
		Add(r, "State", "Severity01 within documented range [0, 1.25]",
			CurrentSettings.Severity01 >= 0.0f && CurrentSettings.Severity01 <= 1.25f);
		Add(r, "State", "MixedRg/MixedBy Severity within documented range [0, 1.25]",
			CurrentSettings.MixedRgSeverity01 >= 0.0f && CurrentSettings.MixedRgSeverity01 <= 1.25f &&
			CurrentSettings.MixedBySeverity01 >= 0.0f && CurrentSettings.MixedBySeverity01 <= 1.25f);
		Add(r, "State", "GammaGain within documented range [0.70, 1.30]",
			CurrentSettings.GammaGain >= 0.70f && CurrentSettings.GammaGain <= 1.30f);
		Add(r, "State", "Eye-Sensitive Mode sliders within [0, 1]",
			CurrentSettings.BlueFilter01 >= 0.0f && CurrentSettings.BlueFilter01 <= 1.0f &&
			CurrentSettings.WarmTint01 >= 0.0f && CurrentSettings.WarmTint01 <= 1.0f &&
			CurrentSettings.SaturationReduction01 >= 0.0f && CurrentSettings.SaturationReduction01 <= 1.0f);
		Add(r, "State", "ActiveSlotIdx within [0, 2]", s_activeSlotIdx >= 0 && s_activeSlotIdx <= 2);
		Add(r, "State", "AutoStartSlot within [-1, 2]",
			CurrentSettings.AutoStartSlot >= -1 && CurrentSettings.AutoStartSlot <= 2);
		Add(r, "State", "ContrastPairIndex within [0, 4]",
			CurrentSettings.ContrastPairIndex >= 0 && CurrentSettings.ContrastPairIndex <= 4);
		{
			// The exact invariant SyncAutoBrightnessGain() maintains (see
			// UIState.h/ModuleMain.cpp, Gamma/Auto-Brightness dedup) - if this
			// ever fails while Auto-Brightness is on, that refactor broke it.
			bool ok = true;
			if (CurrentSettings.AutoBrightness)
			{
				BrightnessRetentionResult retention = GetBrightnessRetention();
				ok = std::fabs(CurrentSettings.GammaGain - retention.recommendedGain) < 0.02f;
			}
			Add(r, "State", "When Auto-Brightness is on, GammaGain matches the recommended value", ok,
				CurrentSettings.AutoBrightness ? "" : "(Auto-Brightness currently off, trivially true)");
		}
		{
			// A dangling AutoStartSlot pointing at an unused slot would
			// silently load empty defaults on next startup.
			bool ok = (CurrentSettings.AutoStartSlot == -1) ||
				(CurrentSettings.AutoStartSlot >= 0 && CurrentSettings.AutoStartSlot < 3 &&
					CurrentSettings.Slots[CurrentSettings.AutoStartSlot].Used);
			Add(r, "State", "AutoStartSlot points at a used profile slot (or is -1/none)", ok);
		}

		// ── Runtime/platform status - informational, not pass/fail. "OS
		// blocked the DWM call" is a legitimate, expected state under real
		// exclusive fullscreen, not a bug by itself - shown here so it's
		// visible in a copied diagnostic report without reading as a failure.
		{
			bool magInit = GetColorEffectController().IsInitialized();
			AddInfo(r, "Platform", "Magnification session initialized", magInit,
				magInit ? "" : "Not initialized yet - normal before first Enable, or MagInitialize failed and is retrying.");
			AddInfo(r, "Platform", "Last DWM color-effect call accepted by the OS", g_DwmLastCallSuccessful,
				g_DwmLastCallSuccessful ? "" : "OS currently rejecting calls - expected under real exclusive fullscreen.");
		}
		{
			// The HybridScanner worker thread self-heals via the Watchdog and
			// Reset Filter (2026-09-09 fix), but this is a hard failure, not
			// info: if a feature that needs the scanner is on and the thread
			// isn't running right now, highlighting is silently dead until
			// the next Watchdog tick (<=50ms) or a Reset Filter click.
			bool needsScanner = CurrentSettings.CommanderTagMode != 0 || CurrentSettings.LabModeEnabled || CurrentSettings.EnableHybridMode;
			bool ok = !needsScanner || GetHybridScanner().IsRunning();
			Add(r, "Platform", "HybridScanner worker thread is running when a feature needs it", ok,
				ok ? "" : "Thread is not running - Watchdog will restart it within ~50ms, or click Reset Filter now.");
		}

		return r;
	}
}
