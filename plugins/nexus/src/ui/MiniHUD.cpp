#include "MiniHUD.h"
#include "../core/Shared.h"
#include "../core/Settings.h"
#include "../core/NexusEcosystem.h"
#include "ImGuiSafe.h"
#include "L10n.h"
#include <string>

namespace cba
{
	MiniHUD& MiniHUD::Get()
	{
		static MiniHUD instance;
		return instance;
	}

	void MiniHUD::Render()
	{
		if (!CurrentSettings.ShowMiniHUD)
			return;

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse;
		if (!CurrentSettings.MiniHudTitleBar)
			flags |= ImGuiWindowFlags_NoTitleBar;

		// AlwaysAutoResize used to sit here - a contributor to a crash class
		// where a negative window size reached Nexus's ImGui 1.8x as a
		// D3D11 vertex-buffer size. A fixed default size the user can still
		// resize once is the same outcome without the risk.
		ImGui::SetNextWindowSize(ImVec2(160.0f, 40.0f), ImGuiCond_FirstUseEver);

		// Apply custom ArcDPS-style transparency
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, CurrentSettings.MiniHudBgAlpha));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, CurrentSettings.MiniHudBorders ? 1.0f : 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));

		if (ImGui::Begin("CBAMiniHUD", &CurrentSettings.ShowMiniHUD, flags))
		{
			// Status text
			bool isDe = cba::IsGerman();
			ImVec4 activeColor = CurrentSettings.Enabled ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

			std::string statusText = "CBA: ";
			statusText += CurrentSettings.Enabled ? (isDe ? "An" : "On") : (isDe ? "Aus" : "Off");

			// Mixed was missing here - unlike every other readout of this same
			// state (Dashboard, Sensor Graph HUD, System diagnostics), this one
			// checked Type alone, so a Mixed-mode profile showed the last
			// single Type it had before switching to Mixed - stale, not the
			// current mode (2026-09-14 consistency pass).
			std::string modeText;
			if (CurrentSettings.Mixed) modeText = isDe ? "Gemischt" : "Mixed";
			else if (CurrentSettings.Type == BalanceType::Deutan) modeText = "Deutan";
			else if (CurrentSettings.Type == BalanceType::Protan) modeText = "Protan";
			else if (CurrentSettings.Type == BalanceType::Tritan) modeText = "Tritan";
			else modeText = isDe ? "Benutzerdef." : "Custom";

			ImGui::TextColored(activeColor, "%s", statusText.c_str());
			ImGui::SameLine();
			ImGui::TextUnformatted("|");
			ImGui::SameLine();
			ImGui::TextUnformatted(modeText.c_str());
		}
		ImGui::End();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor();
	}
}
