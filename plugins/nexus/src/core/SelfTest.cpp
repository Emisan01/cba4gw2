#include "SelfTest.h"
#include "ColorMatrix.h"
#include "ParameterRegistry.h"
#include "Settings.h"
#include "ColorEffectController.h"
#include "FilterLayers.h"
#include "Shared.h"
#include "../platform/HybridScanner.h"
#include "../platform/ShaderColorPipeline.h"
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
				// SmartEnhancer was dropped from the exported string
				// 2026-09-11 (dead field, no behavioural reader left), so it
				// is deliberately no longer round-tripped. Import still
				// accepts it for codes from older builds.
				&& scratch.CommanderTagLayerPriority == CurrentSettings.CommanderTagLayerPriority
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

		// ── Pipeline composition (2026-09-11). These guard the exact failure
		// that caused the Commander Tag "looks off with the base correction"
		// bug: the DWM stage's composition existed only as inline code inside
		// Recompute(), so the enhancer and Auto-Brightness silently modelled a
		// pipeline missing a stage. core/FilterLayers.cpp now owns that
		// composition; these assert its two defining properties, so a future
		// refactor cannot quietly pull them apart again.
		// Full derivation: COLOR_MATH.md section 8.
		{
			double disp[3][3];
			EffectiveDisplayMatrix(disp);
			bool ok = CurrentSettings.Enabled || MatrixNear(disp, kIdentity);
			Add(r, "Pipeline", "EffectiveDisplayMatrix is identity while the master filter is off", ok,
				CurrentSettings.Enabled
					? "(filter currently on, trivially true)"
					: (ok ? "" : "A non-identity matrix here means the tag enhancer is compensating for a transform that is not being applied."));
		}
		{
			// Eye-Sensitive off must leave the colour stack exactly equal to
			// the CVD correction - i.e. the extra layer really is a no-op when
			// disabled, rather than subtly rescaling everything.
			double stack[3][3], corr[3][3];
			ColorStackMatrix(stack);
			ActiveCorrectionMatrix(corr);
			bool ok = CurrentSettings.EyeComfortModeEnabled || MatrixNear(stack, corr);
			Add(r, "Pipeline", "Colour stack equals the plain CVD correction while Eye-Sensitive is off", ok,
				CurrentSettings.EyeComfortModeEnabled ? "(Eye-Sensitive currently on, trivially true)" : "");
		}
		{
			// Clamped at its use site in FilterLab, but a stored out-of-range
			// value still means something wrote past the vector - and the
			// clamp there is std::clamp(idx, 0, count-1), which would be UB if
			// the vector were ever empty (see CLAUDE.md, Trinity sweep).
			int n = (int)CurrentSettings.LabFilters.size();
			bool ok = n > 0
				&& CurrentSettings.SelectedLabFilterIndex >= 0
				&& CurrentSettings.SelectedLabFilterIndex < n;
			Add(r, "Pipeline", "SelectedLabFilterIndex points inside LabFilters", ok,
				ok ? "" : (n == 0 ? "LabFilters is EMPTY - the non-empty invariant several call sites rely on is broken."
				                  : "Index is outside the vector."));
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

			// The screen-effect gate (2026-09-12). Two separate questions:
			// whether the effect is in the state the gate asks for *right now*,
			// and whether switching it off has ever been refused since load.
			// The second is the one that matters, because the failure happens
			// while the user is in another application and cannot see it.
			const bool wantActive = ShouldScreenEffectBeActive();
			const bool isApplied  = IsScreenEffectApplied();
			Add(r, "Platform", "Screen effect matches the gate", wantActive || !isApplied,
				(wantActive || !isApplied) ? ""
					: "Effect is still installed although the gate says it should be off - it is tinting the whole desktop right now.");

			// Which path is painting, and whether it is actually able to.
			// A backend selected but not ready is the one state that looks
			// exactly like "the filter is broken" from inside the game.
			const bool shaderBackend = (CurrentSettings.RenderBackend == 1);
			AddInfo(r, "Platform", "Render backend", true,
				shaderBackend ? "Shader - GW2's own frame only" : "DWM - screen-wide");
			if (shaderBackend)
			{
				const bool ready = GetShaderColorPipeline().IsReady();
				const char* err = GetShaderColorPipeline().LastError();
				Add(r, "Platform", "Shader pipeline is ready", ready,
					ready ? "" : ((err && *err) ? err : "Not initialized yet - builds on the next rendered frame."));

				const DXGI_FORMAT fmt = GetShaderColorPipeline().SourceFormat();
				const char* fmtName =
					(fmt == DXGI_FORMAT_R8G8B8A8_UNORM)       ? "R8G8B8A8_UNORM (8-bit, normalised)" :
					(fmt == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)  ? "R8G8B8A8_UNORM_SRGB (8-bit, normalised)" :
					(fmt == DXGI_FORMAT_B8G8R8A8_UNORM)       ? "B8G8R8A8_UNORM (8-bit, normalised)" :
					(fmt == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)  ? "B8G8R8A8_UNORM_SRGB (8-bit, normalised)" :
					(fmt == DXGI_FORMAT_R10G10B10A2_UNORM)    ? "R10G10B10A2_UNORM (10-bit, normalised)" :
					(fmt == DXGI_FORMAT_R16G16B16A16_FLOAT)   ? "R16G16B16A16_FLOAT (scRGB - values may exceed 1.0)" :
					(fmt == DXGI_FORMAT_UNKNOWN)              ? "not sampled yet" : "other";
				// A float backbuffer is the one case where the maths would be
				// wrong rather than merely unusual - saturate() would clip
				// highlights the display can actually show.
				const bool normalised = (fmt != DXGI_FORMAT_R16G16B16A16_FLOAT);
				AddInfo(r, "Platform", "Backbuffer format", normalised, fmtName);
			}

			const unsigned int rejects = g_DwmClearRejectCount.load();
			AddInfo(r, "Platform", "Clears the OS refused since load", rejects == 0,
				rejects == 0 ? "Every attempt to switch the screen-wide effect off went through."
				             : ("" + std::to_string(rejects) + " rejected clear(s). Each one is a window in which the correction stayed on the desktop after alt-tab or minimize."));
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
		{
			// Hold-to-compare suppresses BOTH filter stages while its key is
			// physically down. Info rather than a failure because holding the
			// bind while clicking this button is legitimate, if unusual - but
			// a stuck flag would look exactly like "the filter stopped
			// working", the same shape as the scanner-death bug above, so it
			// belongs in a diagnostic report either way. The Watchdog clears
			// it on focus loss; see ProcessKeybind / WatchdogLoop.
			bool held = s_compareHoldActive.load();
			AddInfo(r, "Platform", "Hold-to-compare is not suppressing the filter", !held,
				held ? "Compare-hold is ACTIVE - both filter stages are suspended right now. Expected only while Ctrl+Shift+V is held down."
				     : "");
		}

		return r;
	}
}
