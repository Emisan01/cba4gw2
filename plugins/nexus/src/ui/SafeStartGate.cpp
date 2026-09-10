#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SafeStartGate.h"
#include "UIState.h"
#include "Theme.h"
#include "Shared.h"
#include "L10n.h"

#include <imgui.h>
#include <cfloat>

namespace cba
{
	void RenderSafeStartDialog()
	{
		if (!s_safeStartPending.load() || !ImGui::GetCurrentContext()) return;

		// Was a local hack treating System-language (0) as always-German -
		// cba::IsGerman() already resolves System via DetectSystemLanguage()
		// correctly and had zero callers until now (found in the 2026-09-09
		// codebase review). This is the very first screen a user sees after
		// any crash, so getting the language wrong here is worst-case timing.
		bool isDe = cba::IsGerman();
		ImGui::SetNextWindowSize(ImVec2(480.0f, 0.0f), ImGuiCond_Always);
		ImVec2 disp = ImGui::GetIO().DisplaySize;
		ImVec2 center(disp.x * 0.5f, disp.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;
		bool open = true;
		if (ImGui::Begin(isDe ? "cba4gw2 - Sicherheitsstart & Profilauswahl###CBA_SafeStart" : "cba4gw2 - Safe-Start & Profile Gate###CBA_SafeStart", &open, flags))
		{
			if (CurrentSettings.SafeModeTriggered)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Theme::kTextGoldLabel);
				ImGui::TextUnformatted(isDe ? "[!] CBA Safe-Mode aktiv!" : "[!] CBA Safe-Mode Active!");
				ImGui::PopStyleColor();
				ImGui::Spacing();
				ImGui::TextWrapped(isDe
					? "Das Spiel wurde beim letzten Mal unplanmaessig beendet oder es gab einen Absturz. Um Blendungen zu vermeiden, bleibt der Filter vorerst neutral (AUS)."
					: "The game exited unexpectedly or crashed last session. To prevent blinding visual effects, the filter starts disarmed (OFF).");
				ImGui::Spacing();
			}
			else
			{
				ImGui::TextColored(Theme::kTextBlauPeak, "%s", isDe ? "Willkommen bei cba4gw2!" : "Welcome to cba4gw2!");
				ImGui::Spacing();
				ImGui::TextWrapped(isDe
					? "Waehle, wie das Addon fuer diese Sitzung gestartet werden soll:"
					: "Choose how the addon should be initialized for this session:");
				ImGui::Spacing();
			}

			ImGui::Separator();
			ImGui::Spacing();

			// Option 1: Start with saved settings
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnStateActiveIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnStateActiveHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnStateActivePress);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
			if (ImGui::Button(isDe ? "  Gespeicherte Settings aktivieren  " : "  Activate Saved Settings  ", ImVec2(-FLT_MIN, 34.0f)))
			{
				CurrentSettings.Enabled = true;
				s_safeStartPending.store(false);
				CurrentSettings.Save(AddonDir);
				Recompute(/*aForce=*/true);
			}
			ImGui::PopStyleColor(4);

			ImGui::Spacing();

			// The Safe-Mode-only "Activate Anyway" button used to live here,
			// running byte-for-byte identical code to "Activate Saved
			// Settings" above (same Enabled=true/Save/Recompute sequence) -
			// two buttons implying a real choice with no actual behavioral
			// difference, on the one screen where a confused click matters
			// most (found in the 2026-09-09 codebase review). Removed rather
			// than given fake distinct logic - "Activate Saved Settings"
			// already covers exactly this case.

			// Option 2: Open setup with filter OFF
			ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
			ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextBlauPeak);
			if (ImGui::Button(isDe ? "  Setup oeffnen (Filter bleibt AUS)  " : "  Open Setup (Filter remains OFF)  ", ImVec2(-FLT_MIN, 32.0f)))
			{
				CurrentSettings.Enabled = false;
				CurrentSettings.ShowMainWindow = true;
				s_focusMainWindow = true;
				s_safeStartPending.store(false);
				Recompute(/*aForce=*/true);
			}
			ImGui::PopStyleColor(4);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Checkbox(isDe ? "Zukuenftig direkt starten (ohne Gate)" : "Always start directly (skip this gate)", &CurrentSettings.AlwaysDirectStart))
			{
				CurrentSettings.Save(AddonDir);
			}

			ImGui::End();
		}
		if (!open)
		{
			s_safeStartPending.store(false);
		}
	}
}
