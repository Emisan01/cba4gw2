#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Nexus.h"
#include "Diagnostics.h"

#include <imgui.h>
#include <windows.h>
#include <cstdio>
#include <vector>

namespace
{
	AddonDefinition AddonDef{};
	AddonAPI* APIDefs = nullptr;

	// Findings are gathered once when the panel is first opened, not every
	// frame - module enumeration and a DXGI adapter query are cheap, but
	// there is no reason to repeat them 60 times a second for a static
	// report the user opens deliberately.
	bool s_scanned = false;
	std::vector<gw2doc::ModuleFinding> s_modules;
	gw2doc::GpuInfo s_gpu;
	gw2doc::DiskInfo s_disk;

	void RunScanIfNeeded()
	{
		if (s_scanned) return;
		s_modules = gw2doc::ScanLoadedModules();
		s_gpu = gw2doc::GetPrimaryGpuInfo();
		s_disk = gw2doc::GetGw2DriveInfo();
		s_scanned = true;
	}

	// Nexus-embedded options panel (shown under Configure in Nexus's addon
	// list) - the same home cba4gw2's own RenderEmbeddedOptions uses. This
	// is read-only reporting only, on purpose: no "first aid" actions yet
	// (2026-09-13 design note, Emi's own scoping - detection first, fixes
	// only once the detection layer is trusted).
	void AddonOptions()
	{
		if (!ImGui::GetCurrentContext()) return;

		ImGui::TextDisabled("GW2 Doctor: Startup & Stability Diagnostics");
		ImGui::Spacing();

		if (ImGui::Button("Rescan"))
		{
			s_scanned = false;
		}
		RunScanIfNeeded();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "GPU");
		if (s_gpu.ok)
		{
			ImGui::Text("%s", s_gpu.description.c_str());
			ImGui::Text("Dedicated VRAM: %llu MB", s_gpu.dedicatedVideoMemoryMB);
		}
		else
		{
			ImGui::TextDisabled("Could not query the primary adapter.");
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Disk (GW2's own drive)");
		if (s_disk.ok)
		{
			double freeGB = s_disk.freeMB / 1024.0;
			double totalGB = s_disk.totalMB / 1024.0;
			ImGui::Text("%.1f GB free of %.1f GB", freeGB, totalGB);
			if (freeGB < 5.0)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "Low free space - under 5 GB can cause failed writes on launch.");
			}
		}
		else
		{
			ImGui::TextDisabled("Could not query free disk space.");
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Known overlay / injector modules loaded in this process");
		if (s_modules.empty())
		{
			ImGui::TextDisabled("None of the known overlay/injector DLLs were found.");
		}
		else
		{
			for (const auto& m : s_modules)
			{
				ImGui::BulletText("%s (%s)", m.label.c_str(), m.category.c_str());
			}
			ImGui::TextDisabled("These are commonly-loaded overlay tools, not automatically the cause of a crash -");
			ImGui::TextDisabled("useful to know about when comparing notes on a startup crash.");
		}
	}

	void AddonLoad(AddonAPI* aApi)
	{
		APIDefs = aApi;
		if (APIDefs && APIDefs->Renderer.Register)
		{
			APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);
		}
	}

	void AddonUnload()
	{
		if (APIDefs && APIDefs->Renderer.Deregister)
		{
			APIDefs->Renderer.Deregister(AddonOptions);
		}
		s_scanned = false;
		APIDefs = nullptr;
	}
}

BOOL APIENTRY DllMain(HMODULE, DWORD aReason, LPVOID)
{
	switch (aReason)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef.Signature = -78342; // distinct from cba4gw2's -78341
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "gw2doctor";
	AddonDef.Version.Major = 0;
	AddonDef.Version.Minor = 1;
	AddonDef.Version.Build = 1;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "Emisan01";
	AddonDef.Description =
		"Startup & stability diagnostics for Guild Wars 2 - detects known overlay/injector modules, "
		"GPU and free-disk-space headroom. Read-only reporting, no automatic fixes.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;
	AddonDef.Provider = EUpdateProvider_None;
	AddonDef.UpdateLink = nullptr;

	return &AddonDef;
}
