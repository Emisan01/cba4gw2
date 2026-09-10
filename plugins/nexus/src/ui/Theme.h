#pragma once
#include <imgui.h>

namespace Theme
{
	// ── Runtime-switchable palette (2026-09-09) ─────────────────────────────
	// These used to be compile-time `const ImVec4`, one value forever. Now
	// mutable globals (defined once in Theme.cpp, declared `extern` here so
	// every .cpp that includes this header shares the same live values) -
	// ApplyTheme() overwrites them all at once when CurrentSettings.UiTheme
	// changes. Every one of the ~200 existing `Theme::kXxx` call sites across
	// ui/*.cpp keeps working completely unchanged: reading a mutable global
	// looks identical to reading a const one from the call site's point of
	// view, so no widget code needed to change for this feature to exist.
	extern ImVec4 kBtnMittelwertIdle;
	extern ImVec4 kBtnMittelwertHover;
	extern ImVec4 kBtnMittelwertActive;

	extern ImVec4 kBtnStateActiveIdle;
	extern ImVec4 kBtnStateActiveHover;
	extern ImVec4 kBtnStateActivePress;

	extern ImVec4 kBtnNeutralIdle;
	extern ImVec4 kBtnNeutralHover;
	extern ImVec4 kBtnNeutralPress;

	extern ImVec4 kBtnDangerSubtleIdle;
	extern ImVec4 kBtnDangerSubtleHover;
	extern ImVec4 kBtnDangerSubtlePress;

	extern ImVec4 kTextPrimary;
	extern ImVec4 kTextSecondary;
	extern ImVec4 kTextCyanLicht;
	extern ImVec4 kTextBlauPeak;
	extern ImVec4 kTextGoldLabel;
	extern ImVec4 kTextDangerSubtle;

	extern ImU32 kDotReadyCol;
	extern ImU32 kDotWarnCol;
	extern ImU32 kDotOffCol;

	// HUD status-pill accent colors for FeatureModuleRegistry modules
	// (2026-09-09) - only kTextGoldLabel (used by the Commander Tag module)
	// was ever theme-driven; Hybrid Mode/Filter Lab/Eye-Sensitive's colors
	// were hardcoded ImVec4 literals passed straight to FeatureModuleDef,
	// invisible only because Symbiont is currently aliased to Classic
	// (found in the 2026-09-09 codebase review). Same values as before, now
	// switchable.
	extern ImVec4 kHudHybridMode;
	extern ImVec4 kHudFilterLab;
	extern ImVec4 kHudEyeComfort;

	// 0 = Classic (this addon's only look until now), 1 = Symbiont
	// (experimental bio-clinical HUD palette - see CLAUDE.md 2026-09-09,
	// "Ocular Symbiont Console" concept). Palette-only: no font or panel-shape
	// changes, deliberately - Emi's call, keep the experimental theme low-risk
	// and easy to fall back from. Call once after Settings::Load and again
	// whenever the user changes the Advanced UI Theme setting.
	void ApplyTheme(int aThemeId);

	// WCAG Relative Luminance helper: guarantees readable contrast for any button background
	inline ImVec4 GetContrastTextColor(const ImVec4& bgCol)
	{
		float lum = 0.2126f * bgCol.x + 0.7152f * bgCol.y + 0.0722f * bgCol.z;
		return (lum > 0.48f) ? ImVec4(0.08f, 0.10f, 0.14f, 1.00f) : ImVec4(0.92f, 0.96f, 1.00f, 1.00f);
	}
}

namespace cba
{
	static const float kPanelItemWidth = 240.0f;
	inline void PanelHeader(const char* title) {
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::kTextBlauPeak);
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextUnformatted(title);
		ImGui::Spacing();
		ImGui::PopStyleColor();
	}
	inline void PanelSpacing() { ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); }
}
