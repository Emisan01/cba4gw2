#pragma once
#include <imgui.h>

namespace Theme
{
	// ── Curated TAC-Inspired Palette (Cyan, Blau, Grau, Gold) ───────────────
	// Mittelwert Cyan / Blau (Deep Marine Teal) — Muted, harmonious button states
	const ImVec4 kBtnMittelwertIdle   = ImVec4(0.06f, 0.20f, 0.30f, 0.88f); // #0F334D (Struktur/Tief)
	const ImVec4 kBtnMittelwertHover  = ImVec4(0.10f, 0.28f, 0.40f, 0.95f); // #1A4766
	const ImVec4 kBtnMittelwertActive = ImVec4(0.04f, 0.15f, 0.24f, 1.00f);

	// Aktiver Zustand (Fenster geöffnet oder Filter AN — kein grelles Giftgrün mehr!)
	const ImVec4 kBtnStateActiveIdle  = ImVec4(0.02f, 0.32f, 0.36f, 0.92f); // #05525C (Deep Teal)
	const ImVec4 kBtnStateActiveHover = ImVec4(0.04f, 0.42f, 0.46f, 1.00f); // #0A6B75
	const ImVec4 kBtnStateActivePress = ImVec4(0.01f, 0.24f, 0.28f, 1.00f);

	// Neutral / Struktur (Grau / Slate)
	const ImVec4 kBtnNeutralIdle      = ImVec4(0.14f, 0.17f, 0.21f, 0.85f); // #242B36
	const ImVec4 kBtnNeutralHover     = ImVec4(0.20f, 0.24f, 0.30f, 0.95f); // #333D4D
	const ImVec4 kBtnNeutralPress     = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);

	// Subtle Reset / Danger
	const ImVec4 kBtnDangerSubtleIdle  = ImVec4(0.22f, 0.15f, 0.16f, 0.85f);
	const ImVec4 kBtnDangerSubtleHover = ImVec4(0.32f, 0.20f, 0.22f, 0.95f);
	const ImVec4 kBtnDangerSubtlePress = ImVec4(0.15f, 0.10f, 0.11f, 1.00f);

	// Text Farbstufen
	const ImVec4 kTextPrimary         = ImVec4(0.88f, 0.92f, 0.96f, 1.00f); // #E0EBF5 (hell)
	const ImVec4 kTextSecondary       = ImVec4(0.66f, 0.72f, 0.78f, 0.95f); // #A8B8C7 (inaktiv/sekundär)
	const ImVec4 kTextCyanLicht       = ImVec4(0.55f, 0.92f, 0.90f, 1.00f); // #8CEBE6 (Lichtakzent)
	const ImVec4 kTextBlauPeak        = ImVec4(0.72f, 0.85f, 0.97f, 1.00f); // #B8D8F8 (Blau Peak)
	const ImVec4 kTextGoldLabel       = ImVec4(0.98f, 0.85f, 0.25f, 1.00f); // #FAD840 (Gold Text / Label)
	const ImVec4 kTextDangerSubtle    = ImVec4(0.90f, 0.75f, 0.76f, 0.95f);

	// Signal Dots (Nur kleine Dots / Icons, nie als Fläche)
	const ImU32  kDotReadyCol         = IM_COL32(0, 210, 190, 255);         // Cyan / Emerald Ready
	const ImU32  kDotWarnCol          = IM_COL32(250, 216, 64, 255);        // Gold Warning
	const ImU32  kDotOffCol           = IM_COL32(110, 120, 130, 255);       // Gray Inactive

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
