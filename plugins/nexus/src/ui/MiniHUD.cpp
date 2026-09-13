#include "MiniHUD.h"
#include "../core/Shared.h"
#include "../core/Settings.h"
#include "../core/NexusEcosystem.h"
#include "ImGuiSafe.h"
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

		// AlwaysAutoResize used to sit here - HANDOVER.md's own account of the
		// original ImGui crash class names it as a contributor (negative
		// window sizes -> D3D11 vertex-buffer crash on Nexus's ImGui 1.8x). A
		// fixed default size the user can still resize once is the same
		// outcome without the risk.
		ImGui::SetNextWindowSize(ImVec2(160.0f, 40.0f), ImGuiCond_FirstUseEver);

		// Apply custom ArcDPS-style transparency
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, CurrentSettings.MiniHudBgAlpha));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, CurrentSettings.MiniHudBorders ? 1.0f : 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));

		if (ImGui::Begin("CBAMiniHUD", &CurrentSettings.ShowMiniHUD, flags))
		{
			// Status text
			ImVec4 activeColor = CurrentSettings.Enabled ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
			
			std::string statusText = "CBA: ";
			statusText += CurrentSettings.Enabled ? "On" : "Off";
			
			std::string modeText = "";
			if (CurrentSettings.Type == BalanceType::Deutan) modeText = "Deutan";
			else if (CurrentSettings.Type == BalanceType::Protan) modeText = "Protan";
			else if (CurrentSettings.Type == BalanceType::Tritan) modeText = "Tritan";
			else modeText = "Custom";
			
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
