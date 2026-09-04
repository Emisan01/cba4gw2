#include "Shared.h"
#include "ColorMatrix.h"
#include "ColorEffectController.h"
#include "WindowMode.h"
#include "Settings.h"

#include <imgui.h>
#include <array>
#include <cstdio>
#include <string>

using namespace cba;

namespace
{
	AddonDefinition AddonDef{};
	Settings        CurrentSettings{};
	std::string     AddonDir;

	struct L10n
	{
		const char* Enabled;
		const char* CorrectionProfile;
		const char* Protan;
		const char* Deutan;
		const char* Tritan;
		const char* Mixed;
		const char* Strength;
		const char* RgStrength;
		const char* ByStrength;
		const char* WindowMode;
		const char* ScreenshotNote;
		const char* Language;
		const char* Diagnosis;
		const char* DiagnosisHelp;
		const char* BeamPreview;
		const char* CommanderView;
		const char* ContrastHint;
	};

	const L10n& Strings()
	{
		static const L10n de{
			"Aktiv",
			"Korrekturprofil",
			"Protan",
			"Deutan",
			"Tritan",
			"Gemischt",
			"Stärke",
			"Rot-Grün Stärke",
			"Blau-Gelb Stärke",
			"Fenstermodus",
			"Screenshots: GW2-intern wirkt vor dem Filter. PrintScreen / Win+PrintScreen und die meisten Display-Captures sehen den Filter.",
			"Sprache",
			"AQ/HRR Diagnose",
			"Freitext für Diagnose-Presets oder eine genauere Zuordnung.",
			"Farbstrahlen",
			"Commander-Symbole",
			"Hoher Kontrast ist besser lesbar; die Ansicht zeigt die Symbolfarben statisch an.",
		};

		static const L10n en{
			"Enabled",
			"Correction profile",
			"Protan",
			"Deutan",
			"Tritan",
			"Mixed",
			"Strength",
			"Red-Green strength",
			"Blue-Yellow strength",
			"Window mode",
			"Screenshots: GW2's internal capture sees the scene before the filter. PrintScreen / Win+PrintScreen and most display captures see the filter.",
			"Language",
			"AQ/HRR diagnosis",
			"Free text for diagnosis presets or more precise mapping.",
			"Color beams",
			"Commander symbols",
			"Higher contrast is easier to read; this view keeps the symbol colors static.",
		};

		return (CurrentSettings.Language == "en") ? en : de;
	}

	// Rebuilds the MAGCOLOREFFECT from CurrentSettings and either applies or
	// clears it, depending on Enabled + window mode. Called any time a
	// setting changes and once after load.
	void Recompute()
	{
		auto& controller = GetColorEffectController();

		WindowMode mode = DetectWindowMode(APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);

		if (!CurrentSettings.Enabled || mode == WindowMode::ExclusiveFullscreen)
		{
			controller.Clear();
			return;
		}

		double m3x3[3][3];
		if (CurrentSettings.Mixed)
		{
			ColorMatrix::MixedCorrectionMatrix(
				CurrentSettings.MixedRgSeverity01,
				CurrentSettings.MixedBySeverity01,
				m3x3);
		}
		else
		{
			ColorMatrix::CorrectionMatrix(CurrentSettings.Type, CurrentSettings.Severity01, m3x3);
		}

		controller.Apply(ColorMatrix::ToMagColorEffect(m3x3));
	}

	void ProcessKeybind(const char* aIdentifier, bool aIsRelease)
	{
		if (aIsRelease) return;

		if (strcmp(aIdentifier, "KB_CBA_TOGGLE") == 0)
		{
			CurrentSettings.Enabled = !CurrentSettings.Enabled;
			CurrentSettings.Save(AddonDir);
			Recompute();
		}
	}

	// Registered as ERenderType::OptionsRender — appended into Nexus's own
	// options window under this addon's name, no separate window needed.
	void AddonOptions()
	{
		const L10n& t = Strings();
		bool changed = false;

		changed |= ImGui::Checkbox(t.Enabled, &CurrentSettings.Enabled);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(140.0f);
		if (ImGui::BeginCombo(t.Language, CurrentSettings.Language == "en" ? "English" : "Deutsch"))
		{
			if (ImGui::Selectable("Deutsch", CurrentSettings.Language == "de"))
			{
				CurrentSettings.Language = "de";
				changed = true;
			}
			if (ImGui::Selectable("English", CurrentSettings.Language == "en"))
			{
				CurrentSettings.Language = "en";
				changed = true;
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();
		ImGui::TextUnformatted(t.CorrectionProfile);

		if (ImGui::RadioButton(t.Protan, !CurrentSettings.Mixed && CurrentSettings.Type == DeficiencyType::Protan))
		{
			CurrentSettings.Mixed = false;
			CurrentSettings.Type = DeficiencyType::Protan;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton(t.Deutan, !CurrentSettings.Mixed && CurrentSettings.Type == DeficiencyType::Deutan))
		{
			CurrentSettings.Mixed = false;
			CurrentSettings.Type = DeficiencyType::Deutan;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton(t.Tritan, !CurrentSettings.Mixed && CurrentSettings.Type == DeficiencyType::Tritan))
		{
			CurrentSettings.Mixed = false;
			CurrentSettings.Type = DeficiencyType::Tritan;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton(t.Mixed, CurrentSettings.Mixed))
		{
			CurrentSettings.Mixed = true;
			changed = true;
		}

		if (CurrentSettings.Mixed)
		{
			float rg = (float)CurrentSettings.MixedRgSeverity01;
			float by = (float)CurrentSettings.MixedBySeverity01;
			if (ImGui::SliderFloat(t.RgStrength, &rg, 0.0f, 1.0f))
			{
				CurrentSettings.MixedRgSeverity01 = rg;
				changed = true;
			}
			if (ImGui::SliderFloat(t.ByStrength, &by, 0.0f, 1.0f))
			{
				CurrentSettings.MixedBySeverity01 = by;
				changed = true;
			}
		}
		else
		{
			float severity = (float)CurrentSettings.Severity01;
			if (ImGui::SliderFloat(t.Strength, &severity, 0.0f, 1.0f))
			{
				CurrentSettings.Severity01 = severity;
				changed = true;
			}
		}

		ImGui::Separator();

		ImGui::TextUnformatted(t.Diagnosis);
		char diagnosisBuf[128]{};
		std::snprintf(diagnosisBuf, sizeof(diagnosisBuf), "%s", CurrentSettings.DiagnosisHint.c_str());
		if (ImGui::InputText("##diagnosis", diagnosisBuf, sizeof(diagnosisBuf)))
		{
			CurrentSettings.DiagnosisHint = diagnosisBuf;
			changed = true;
		}
		ImGui::TextWrapped("%s", t.DiagnosisHelp);

		ImGui::Separator();
		ImGui::TextUnformatted(t.BeamPreview);
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImVec2 size(260.0f, 110.0f);
		ImU32 bg = ImGui::GetColorU32(ImVec4(0.11f, 0.11f, 0.13f, 1.0f));
		ImU32 border = ImGui::GetColorU32(ImVec4(0.35f, 0.35f, 0.42f, 1.0f));
		draw->AddRectFilled(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y), bg, 8.0f);
		draw->AddRect(cursor, ImVec2(cursor.x + size.x, cursor.y + size.y), border, 8.0f, 0, 1.0f);
		draw->AddRectFilledMultiColor(ImVec2(cursor.x + 20, cursor.y + 20), ImVec2(cursor.x + 65, cursor.y + 95),
			ImGui::GetColorU32(ImVec4(1.0f, 0.15f, 0.15f, 0.10f)),
			ImGui::GetColorU32(ImVec4(1.0f, 0.15f, 0.15f, 0.85f)),
			ImGui::GetColorU32(ImVec4(1.0f, 0.15f, 0.15f, 0.85f)),
			ImGui::GetColorU32(ImVec4(1.0f, 0.15f, 0.15f, 0.10f)));
		draw->AddRectFilledMultiColor(ImVec2(cursor.x + 105, cursor.y + 20), ImVec2(cursor.x + 150, cursor.y + 95),
			ImGui::GetColorU32(ImVec4(0.15f, 1.0f, 0.3f, 0.10f)),
			ImGui::GetColorU32(ImVec4(0.15f, 1.0f, 0.3f, 0.85f)),
			ImGui::GetColorU32(ImVec4(0.15f, 1.0f, 0.3f, 0.85f)),
			ImGui::GetColorU32(ImVec4(0.15f, 1.0f, 0.3f, 0.10f)));
		draw->AddRectFilledMultiColor(ImVec2(cursor.x + 190, cursor.y + 20), ImVec2(cursor.x + 235, cursor.y + 95),
			ImGui::GetColorU32(ImVec4(0.25f, 0.55f, 1.0f, 0.10f)),
			ImGui::GetColorU32(ImVec4(0.25f, 0.55f, 1.0f, 0.85f)),
			ImGui::GetColorU32(ImVec4(0.25f, 0.55f, 1.0f, 0.85f)),
			ImGui::GetColorU32(ImVec4(0.25f, 0.55f, 1.0f, 0.10f)));
		ImGui::InvisibleButton("##beam_preview", size);

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		ImGui::TextUnformatted(t.CommanderView);
		ImGui::TextWrapped("%s", t.ContrastHint);

		static const std::array<const char*, 8> markers = { "1", "2", "3", "4", "5", "6", "7", "8" };
		static const std::array<ImVec4, 8> markerColors = {
			ImVec4(0.96f, 0.53f, 0.12f, 1.0f),
			ImVec4(0.95f, 0.20f, 0.20f, 1.0f),
			ImVec4(0.93f, 0.86f, 0.12f, 1.0f),
			ImVec4(0.22f, 0.75f, 0.34f, 1.0f),
			ImVec4(0.28f, 0.60f, 0.98f, 1.0f),
			ImVec4(0.70f, 0.34f, 0.94f, 1.0f),
			ImVec4(0.96f, 0.36f, 0.68f, 1.0f),
			ImVec4(0.70f, 0.70f, 0.74f, 1.0f)
		};

		for (size_t i = 0; i < markers.size(); ++i)
		{
			if (i > 0) ImGui::SameLine();
			ImGui::BeginGroup();
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImVec2 box(32.0f, 32.0f);
			draw->AddRectFilled(pos, ImVec2(pos.x + box.x, pos.y + box.y), ImGui::GetColorU32(ImVec4(0.18f, 0.18f, 0.20f, 1.0f)), 6.0f);
			draw->AddRect(pos, ImVec2(pos.x + box.x, pos.y + box.y), ImGui::GetColorU32(ImVec4(0.35f, 0.35f, 0.4f, 1.0f)), 6.0f);
			draw->AddText(ImVec2(pos.x + 9.0f, pos.y + 7.0f), ImGui::GetColorU32(markerColors[i]), markers[i]);
			std::string markerId = "##marker_" + std::to_string(i);
			ImGui::InvisibleButton(markerId.c_str(), box);
			ImGui::EndGroup();
		}

		// Window mode status — this is the "kleiner aber großer Haken":
		// the filter is a DWM effect, so it silently does nothing in
		// exclusive fullscreen. Make that visible instead of letting
		// people wonder why nothing happens.
		WindowMode mode = DetectWindowMode(APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);
		if (mode == WindowMode::ExclusiveFullscreen)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.2f, 1.0f), "%s: %s", t.WindowMode, ToDisplayString(mode));
			ImGui::TextWrapped(
				"Switch Guild Wars 2 to Windowed or Windowed Fullscreen "
				"(Borderless) in Graphics Options to use the filter.");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.0f), "%s: %s", t.WindowMode, ToDisplayString(mode));
		}

		ImGui::Separator();
		ImGui::TextWrapped("%s", t.ScreenshotNote);
		ImGui::TextWrapped("Toggle keybind: %s", CurrentSettings.ToggleKeybind.c_str());

		if (changed)
		{
			CurrentSettings.Save(AddonDir);
			Recompute();
		}
	}

	void AddonLoad(AddonAPI* aApi)
	{
		APIDefs = aApi;

		ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
		ImGui::SetAllocatorFunctions(
			(void* (*)(size_t, void*))APIDefs->ImguiMalloc,
			(void(*)(void*, void*))APIDefs->ImguiFree);

		AddonDir = APIDefs->Paths.GetAddonDirectory("cba");
		CurrentSettings = Settings::Load(AddonDir);

		GetColorEffectController().Initialize();
		InstallCrashGuard();

		APIDefs->InputBinds.RegisterWithString("KB_CBA_TOGGLE", ProcessKeybind, CurrentSettings.ToggleKeybind.c_str());
		APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);

		Recompute();
	}

	void AddonUnload()
	{
		APIDefs->Renderer.Deregister(AddonOptions);
		APIDefs->InputBinds.Deregister("KB_CBA_TOGGLE");

		RemoveCrashGuard();
		GetColorEffectController().Shutdown(); // clears the effect before unload
	}
}

BOOL APIENTRY DllMain(HMODULE aModule, DWORD aReasonForCall, LPVOID)
{
	if (aReasonForCall == DLL_PROCESS_ATTACH)
		AddonModuleHandle = aModule;

	return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef.Signature = -78341; // arbitrary negative ID — not on Raidcore (yet)
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "Colorblind Assist";
	AddonDef.Version.Major = 1;
	AddonDef.Version.Minor = 0;
	AddonDef.Version.Build = 0;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "Emisan01";
	AddonDef.Description =
		"Adjustable color-vision-deficiency correction filter with diagnosis hint, language toggle, "
		"beam preview, and commander contrast view. Applied via the Windows Magnification API.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	AddonDef.Provider = EUpdateProvider_GitHub;
	AddonDef.UpdateLink = "https://github.com/Emisan01/ColorblindAssist";

	return &AddonDef;
}
