#include "Theme.h"

namespace Theme
{
	ImVec4 kBtnMittelwertIdle;
	ImVec4 kBtnMittelwertHover;
	ImVec4 kBtnMittelwertActive;

	ImVec4 kBtnStateActiveIdle;
	ImVec4 kBtnStateActiveHover;
	ImVec4 kBtnStateActivePress;

	ImVec4 kBtnNeutralIdle;
	ImVec4 kBtnNeutralHover;
	ImVec4 kBtnNeutralPress;

	ImVec4 kBtnDangerSubtleIdle;
	ImVec4 kBtnDangerSubtleHover;
	ImVec4 kBtnDangerSubtlePress;

	ImVec4 kTextPrimary;
	ImVec4 kTextSecondary;
	ImVec4 kTextCyanLicht;
	ImVec4 kTextBlauPeak;
	ImVec4 kTextGoldLabel;
	ImVec4 kTextDangerSubtle;

	ImU32 kDotReadyCol;
	ImU32 kDotWarnCol;
	ImU32 kDotOffCol;

	ImVec4 kHudHybridMode;
	ImVec4 kHudFilterLab;
	ImVec4 kHudEyeComfort;

	namespace
	{
		struct Palette
		{
			ImVec4 btnMittelwertIdle, btnMittelwertHover, btnMittelwertActive;
			ImVec4 btnStateActiveIdle, btnStateActiveHover, btnStateActivePress;
			ImVec4 btnNeutralIdle, btnNeutralHover, btnNeutralPress;
			ImVec4 btnDangerSubtleIdle, btnDangerSubtleHover, btnDangerSubtlePress;
			ImVec4 textPrimary, textSecondary, textCyanLicht, textBlauPeak, textGoldLabel, textDangerSubtle;
			ImU32 dotReady, dotWarn, dotOff;
			ImVec4 hudHybridMode, hudFilterLab, hudEyeComfort;
		};

		// ── Palette 0: Classic ──────────────────────────────────────────────
		// 2026-09-09: refined against Emi's shared cross-project token system
		// (the same Cyan/Blau/Gold/Grau/Signal reference used across
		// TAC/Refractor/CBA) - "nicht zu stark verbessern," so the role
		// structure below is untouched (same slots, same pairing pattern
		// each call site already uses), only the actual RGB values changed to
		// the reference's considered stops. Two roles the reference doesn't
		// define (Danger, KERN-brightness exact hexes) were either left as
		// they were or interpolated conservatively rather than guessed wild.
		//   Cyan:  bg #001A18 / struktur #003D3B / tief #006B68 / aufgehellt #5EEAE4 / hell #B0F5F2
		//   Blau:  bg #051520 / struktur #0D2B45 / tief #163D5E / kurve #70B0EE / peak #B8D8F8
		//   Gold:  min.akzent #C09600 / text-label #FAD840 (unchanged - was already this value)
		//   Grau:  bg #111 / tile #1E1E1E / border #2C2C2C / inaktiv #6B6B6B / sekundaer #AAA / primaer #DDD
		//   Signal: ready/green, warn/gold, off/grey - dots and icons only, never a fill.
		const Palette kClassic = {
			ImVec4(0.051f, 0.169f, 0.271f, 0.88f), ImVec4(0.086f, 0.239f, 0.369f, 0.95f), ImVec4(0.020f, 0.082f, 0.125f, 1.00f),
			ImVec4(0.000f, 0.420f, 0.408f, 0.92f), ImVec4(0.000f, 0.600f, 0.520f, 1.00f), ImVec4(0.000f, 0.239f, 0.231f, 1.00f),
			ImVec4(0.173f, 0.173f, 0.173f, 0.85f), ImVec4(0.227f, 0.227f, 0.227f, 0.95f), ImVec4(0.067f, 0.067f, 0.067f, 1.00f),
			ImVec4(0.22f, 0.15f, 0.16f, 0.85f), ImVec4(0.32f, 0.20f, 0.22f, 0.95f), ImVec4(0.15f, 0.10f, 0.11f, 1.00f),
			ImVec4(0.867f, 0.867f, 0.867f, 1.00f), ImVec4(0.667f, 0.667f, 0.667f, 0.95f), ImVec4(0.369f, 0.918f, 0.894f, 1.00f),
			ImVec4(0.722f, 0.847f, 0.973f, 1.00f), ImVec4(0.980f, 0.847f, 0.251f, 1.00f), ImVec4(0.90f, 0.75f, 0.76f, 0.95f),
			IM_COL32(62, 207, 110, 255), IM_COL32(250, 216, 64, 255), IM_COL32(138, 138, 138, 255),
			ImVec4(0.40f, 0.90f, 0.70f, 1.0f), ImVec4(0.85f, 0.50f, 0.95f, 1.0f), ImVec4(0.55f, 0.80f, 0.95f, 1.0f)
		};

		// ── Palette 1: "Symbiont" (experimental, in progress) ───────────────
		// 2026-09-09: Emi liked the idea of an optional advanced UI theme
		// (see the "Ocular Symbiont Console" artifact concept) but explicitly
		// rejected the specific green/cyan bio-HUD palette that concept used
		// ("gruenes Theme nein, optionaler UI Modus ja") - the MECHANISM was
		// the yes, not that color direction. Rather than guess a replacement
		// palette Emi hasn't actually chosen, this intentionally aliases to
		// Classic for now: the switch is fully wired and safe to select, it
		// just looks identical until real colors are picked. Swap this block
		// for real values once Emi gives a color direction - nothing else
		// needs to change, every call site already reads through ApplyTheme().
		const Palette kSymbiont = kClassic;

		const Palette& PaletteFor(int aThemeId)
		{
			return (aThemeId == 1) ? kSymbiont : kClassic;
		}
	}

	void ApplyTheme(int aThemeId)
	{
		const Palette& p = PaletteFor(aThemeId);
		kBtnMittelwertIdle = p.btnMittelwertIdle;
		kBtnMittelwertHover = p.btnMittelwertHover;
		kBtnMittelwertActive = p.btnMittelwertActive;
		kBtnStateActiveIdle = p.btnStateActiveIdle;
		kBtnStateActiveHover = p.btnStateActiveHover;
		kBtnStateActivePress = p.btnStateActivePress;
		kBtnNeutralIdle = p.btnNeutralIdle;
		kBtnNeutralHover = p.btnNeutralHover;
		kBtnNeutralPress = p.btnNeutralPress;
		kBtnDangerSubtleIdle = p.btnDangerSubtleIdle;
		kBtnDangerSubtleHover = p.btnDangerSubtleHover;
		kBtnDangerSubtlePress = p.btnDangerSubtlePress;
		kTextPrimary = p.textPrimary;
		kTextSecondary = p.textSecondary;
		kTextCyanLicht = p.textCyanLicht;
		kTextBlauPeak = p.textBlauPeak;
		kTextGoldLabel = p.textGoldLabel;
		kTextDangerSubtle = p.textDangerSubtle;
		kDotReadyCol = p.dotReady;
		kDotWarnCol = p.dotWarn;
		kDotOffCol = p.dotOff;
		kHudHybridMode = p.hudHybridMode;
		kHudFilterLab = p.hudFilterLab;
		kHudEyeComfort = p.hudEyeComfort;
	}
}
