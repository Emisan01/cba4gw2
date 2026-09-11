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

	// ── Pipeline composition (2026-09-11) ────────────────────────────────
	// GetFilterLayerOrder() above answers "WHICH layers are active"; the two
	// functions below answer "HOW do they compose mathematically". Same
	// module on purpose - the pipeline is one concept, and keeping the
	// displayed order and the actual math in one place is precisely what
	// stops the two drifting apart (they already had: the tag enhancer used
	// to model a pipeline that omitted the DWM stage entirely).

	// The CVD correction matrix for the currently selected profile. This is
	// the exact "if (Mixed) MixedCorrectionMatrix else CorrectionMatrix"
	// pattern that used to be copy-pasted at 6 call sites - and had already
	// drifted into a real transcription bug at one of them (FilterLab.cpp
	// used corrMat[1][0] twice in the green row instead of [1][1]).
	void ActiveCorrectionMatrix(double aOut3x3[3][3]);

	// The COLOUR transform stack only: CVD correction, with Eye-Sensitive
	// Mode composed on top of it - deliberately WITHOUT the GammaGain
	// brightness scalar.
	//
	// Kept separable because Auto-Brightness solves FOR that gain by
	// measuring how much relative luminance this stack costs
	// (GetBrightnessRetention). Feeding it a matrix that already contains
	// GammaGain would close a feedback loop: gain up -> measured retention
	// up -> recommended gain down -> oscillation.
	void ColorStackMatrix(double aOut3x3[3][3]);

	// The full matrix the DWM actually applies to EVERY pixel on screen, in
	// Recompute()'s own composition order: GammaGain * (EyeComfort x CVD).
	// Recompute() itself calls this, so there is one source of truth rather
	// than a second hand-maintained copy of the same composition.
	//
	// Returns identity when the master filter is off. That is not a
	// defensive default - it is a real reachable state: the tag highlighter
	// is NOT gated on Settings.Enabled (see AddonRender, which draws it for
	// EnableHybridMode || CommanderTagMode), so "highlighter drawing while
	// no DWM transform is applied" genuinely happens. Deliberately NOT
	// modelling Recompute()'s transient foreground/minimized gate: tag
	// colours would then churn on every alt-tab.
	void EffectiveDisplayMatrix(double aOut3x3[3][3]);
}
