#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Nexus.h"
#include "Diagnostics.h"

#include <imgui.h>
#include <windows.h>
#include <cstdio>
#include <vector>
#include <iterator>

namespace
{
	AddonDefinition AddonDef{};
	AddonAPI* APIDefs = nullptr;

	// In-memory only for now, not persisted across restarts - gw2doctor has
	// no settings file yet (2026-09-13 scope note: add one if this needs to
	// remember open/closed across sessions).
	bool s_showMainWindow = false;

	// Findings are gathered once when the window is first opened, not every
	// frame - module enumeration, a DXGI adapter query and walking the
	// cache directories are cheap but pointless to repeat 60 times a
	// second for a static report the user opens deliberately.
	bool s_scanned = false;
	std::vector<gw2doc::ModuleFinding> s_modules;
	gw2doc::GpuInfo s_gpu;
	gw2doc::CpuInfo s_cpu;
	gw2doc::MemoryInfo s_memory;
	gw2doc::DiskInfo s_disk;
	gw2doc::MumbleLinkInfo s_mumble;
	gw2doc::GfxSettingsInfo s_gfx;
	std::vector<gw2doc::CacheInfo> s_caches;
	std::vector<gw2doc::AddonFolderFinding> s_addonFolder;
	bool s_showAllGfxOptions = false;

	// A cache clear is two clicks, not one - "Clear" arms it and shows
	// exactly what will happen, "Confirm" (or anything else in the panel)
	// is the only way it actually runs. Only one cache armed at a time.
	int s_armedCache = -1;

	// Result of the last clear, shown until the next rescan or another
	// clear replaces it - so the user sees what actually happened, not
	// just that a button was clicked.
	bool s_hasClearResult = false;
	std::string s_clearResultText;

	ImVec4 PressureColor(const std::string& status)
	{
		if (status == "Critical") return ImVec4(1.0f, 0.4f, 0.35f, 1.0f);
		if (status == "Warning") return ImVec4(1.0f, 0.75f, 0.3f, 1.0f);
		return ImVec4(0.4f, 0.9f, 0.5f, 1.0f);
	}

	void RunScanIfNeeded()
	{
		if (s_scanned) return;
		s_modules = gw2doc::ScanLoadedModules();
		s_gpu = gw2doc::GetPrimaryGpuInfo();
		s_cpu = gw2doc::GetCpuInfo();
		s_memory = gw2doc::GetMemoryInfo();
		s_disk = gw2doc::GetGw2DriveInfo();
		s_mumble = gw2doc::GetMumbleLinkInfo();
		s_gfx = gw2doc::GetGfxSettingsInfo();
		s_caches = gw2doc::GetClearableCaches();
		s_addonFolder = gw2doc::ScanAddonsFolder();
		s_scanned = true;
		s_armedCache = -1;
	}

	// All of the actual diagnostics UI - lives in its own function so both
	// the real window and (if ever useful again) an embedded panel can
	// call the same thing. One implementation, not two copies to keep in
	// sync.
	void RenderDiagnosticsContent()
	{
		if (ImGui::Button("Rescan"))
		{
			s_scanned = false;
			s_hasClearResult = false;
		}
		RunScanIfNeeded();

		ImGui::Spacing();

		// Snapshot context via GW2's own MumbleLink shared memory (the same
		// public interface BlishHUD/TacO/etc. read - GW2 writes it, we only
		// open and read, no hooking or process-memory access). buildId is
		// the most useful single field this module has for correlating a
		// startup crash with a specific GW2 patch.
		if (!s_mumble.available)
		{
			ImGui::TextDisabled("MumbleLink not available (unusual - most GW2 sessions have this).");
		}
		else if (!s_mumble.populated)
		{
			ImGui::TextDisabled("MumbleLink present but not yet written to by GW2 (very early in loading).");
		}
		else
		{
			ImGui::TextDisabled("Snapshot: GW2 build %u, map %u, %s, %s",
				s_mumble.buildId, s_mumble.mapId,
				s_mumble.gameHasFocus ? "focused" : "unfocused",
				s_mumble.isInCombat ? "in combat" : "out of combat");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Which parts of Nexus's own API surface are actually wired up for
		// us - idea adapted from Emi's Tyrian Art Companion
		// (src/tac/nexus_bridge.h's NexusApiSnapshot). A null function
		// pointer here means an older/newer Nexus build than this addon
		// expects, not a bug in gw2doctor - still worth surfacing, since
		// "an addon's expected API call silently no-ops" is exactly the
		// kind of thing that looks like a random crash from the outside.
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Nexus API surface");
		if (!APIDefs)
		{
			ImGui::TextDisabled("Not loaded (unexpected - this window wouldn't be open otherwise).");
		}
		else
		{
			struct ApiCheck { const char* name; bool present; };
			const ApiCheck checks[] = {
				{ "Renderer.Register/Deregister", APIDefs->Renderer.Register && APIDefs->Renderer.Deregister },
				{ "Log",                          APIDefs->Log != nullptr },
				{ "UI.RegisterCloseOnEscape",      APIDefs->UI.RegisterCloseOnEscape != nullptr },
				{ "Paths.GetGameDirectory",        APIDefs->Paths.GetGameDirectory != nullptr },
				{ "Paths.GetAddonDirectory",       APIDefs->Paths.GetAddonDirectory != nullptr },
				{ "DataLink.Get",                  APIDefs->DataLink.Get != nullptr },
				{ "Textures.GetOrCreateFromMemory", APIDefs->Textures.GetOrCreateFromMemory != nullptr },
				{ "QuickAccess.Add",               APIDefs->QuickAccess.Add != nullptr },
				{ "InputBinds.RegisterWithString",  APIDefs->InputBinds.RegisterWithString != nullptr },
				{ "Localization.Translate",        APIDefs->Localization.Translate != nullptr },
			};

			int missing = 0;
			for (const auto& c : checks) { if (!c.present) ++missing; }

			if (missing == 0)
			{
				ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "All %zu checked API entry points are present.", std::size(checks));
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%d of %zu checked API entry points are missing:", missing, std::size(checks));
				for (const auto& c : checks)
				{
					if (!c.present) ImGui::BulletText("%s", c.name);
				}
			}

			if (APIDefs->Paths.GetGameDirectory)
			{
				const char* gameDir = APIDefs->Paths.GetGameDirectory();
				if (gameDir) ImGui::TextDisabled("Game directory: %s", gameDir);
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "GPU");
		if (s_gpu.ok)
		{
			ImGui::Text("%s", s_gpu.description.c_str());
			ImGui::Text("Dedicated VRAM: %llu MB", s_gpu.dedicatedVideoMemoryMB);
			if (s_gpu.budgetAvailable)
			{
				ImGui::Text("VRAM in use right now: %llu / %llu MB", s_gpu.localUsageMB, s_gpu.localBudgetMB);
				ImGui::SameLine();
				ImGui::TextColored(PressureColor(s_gpu.pressureStatus), "(%s)", s_gpu.pressureStatus.c_str());
			}
		}
		else
		{
			ImGui::TextDisabled("Could not query the primary adapter.");
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "CPU");
		if (s_cpu.topologyAvailable)
		{
			ImGui::Text("%u logical processors, %u physical cores", s_cpu.logicalProcessorCount, s_cpu.physicalCoreCount);
			if (s_cpu.frequencyAvailable)
			{
				ImGui::Text("Clock: %u / %u MHz (%.0f%% of max) -", s_cpu.currentMhzAverage, s_cpu.reportedMaxMhz, s_cpu.currentToMaxRatio * 100.0f);
				ImGui::SameLine();
				ImGui::TextDisabled("%s", s_cpu.clockReserveStatus.c_str());
			}
		}
		else
		{
			ImGui::TextDisabled("Could not query CPU topology.");
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "System Memory");
		if (s_memory.available)
		{
			double totalGB = s_memory.totalPhysicalMB / 1024.0;
			double availGB = s_memory.availablePhysicalMB / 1024.0;
			ImGui::Text("%.1f GB available of %.1f GB (%u%% used)", availGB, totalGB, s_memory.memoryLoadPercent);
			ImGui::SameLine();
			ImGui::TextColored(PressureColor(s_memory.pressureStatus), "(%s)", s_memory.pressureStatus.c_str());
		}
		else
		{
			ImGui::TextDisabled("Could not query system memory.");
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

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "addons/ folder health");
		if (s_addonFolder.empty())
		{
			ImGui::TextDisabled("No leftover or implausibly small addon DLLs found.");
		}
		else
		{
			for (const auto& f : s_addonFolder)
			{
				char narrow[MAX_PATH]{};
				WideCharToMultiByte(CP_UTF8, 0, f.fileName.c_str(), -1, narrow, sizeof(narrow), nullptr, nullptr);
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "%s", narrow);
				ImGui::TextWrapped("  %s", f.issue.c_str());
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "GW2 graphics settings worth knowing about");
		if (!s_gfx.ok)
		{
			ImGui::TextDisabled("Could not read GFXSettings.Gw2-64.exe.xml.");
		}
		else
		{
			ImGui::Text("Resolution: %d x %d", s_gfx.resolutionWidth, s_gfx.resolutionHeight);
			if (s_gfx.flagged.empty())
			{
				ImGui::TextDisabled("None of the settings known to be especially expensive are turned up.");
			}
			else
			{
				for (const auto& f : s_gfx.flagged)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "%s = %s", f.name.c_str(), f.value.c_str());
					ImGui::TextWrapped("  %s", f.note.c_str());
				}
				ImGui::TextDisabled("These are expensive by design, not a claim about your framerate specifically -");
				ImGui::TextDisabled("worth knowing about if you're chasing performance or stability.");
			}

			ImGui::Spacing();
			ImGui::Checkbox("Show every setting", &s_showAllGfxOptions);
			if (s_showAllGfxOptions)
			{
				for (const auto& opt : s_gfx.allOptions)
				{
					ImGui::BulletText("%s = %s", opt.name.c_str(), opt.value.c_str());
				}
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Clearable shader caches");
		ImGui::TextDisabled("Safe to clear - regenerable, OS/driver-managed. Shared system-wide, not GW2-exclusive.");
		ImGui::Spacing();

		if (s_caches.empty())
		{
			ImGui::TextDisabled("None of the known cache locations were found on this system.");
		}
		else
		{
			for (size_t i = 0; i < s_caches.size(); ++i)
			{
				const auto& c = s_caches[i];
				ImGui::PushID((int)i);

				ImGui::Text("%s - %llu MB", c.label.c_str(), c.sizeMB);
				ImGui::TextWrapped("%s", c.scopeNote.c_str());

				if ((int)i == s_armedCache)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.25f, 0.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.32f, 0.24f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.2f, 0.16f, 1.0f));
					char confirmLabel[128];
					std::snprintf(confirmLabel, sizeof(confirmLabel), "Confirm - delete %llu MB##confirm", c.sizeMB);
					if (ImGui::Button(confirmLabel))
					{
						unsigned long long freedMB = 0;
						unsigned int skipped = 0;
						bool ran = gw2doc::ClearCacheDirectory(c.path, freedMB, skipped);
						char buf[192];
						if (ran)
						{
							std::snprintf(buf, sizeof(buf), "%s: freed %llu MB (%u file(s) still in use, skipped).",
								c.label.c_str(), freedMB, skipped);
						}
						else
						{
							std::snprintf(buf, sizeof(buf), "%s: could not be accessed.", c.label.c_str());
						}
						s_clearResultText = buf;
						s_hasClearResult = true;
						s_armedCache = -1;
						s_scanned = false; // sizes are now stale - rescan on next draw
					}
					ImGui::PopStyleColor(3);
					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						s_armedCache = -1;
					}
				}
				else
				{
					if (ImGui::Button("Clear"))
					{
						s_armedCache = (int)i;
					}
				}

				ImGui::Spacing();
				ImGui::PopID();
			}
		}

		if (s_hasClearResult)
		{
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "%s", s_clearResultText.c_str());
		}
	}

	// The real window - registered under ERenderType_Render (drawn every
	// frame, gated by s_showMainWindow), not ERenderType_OptionsRender
	// (which only ever draws inside Nexus's own Configure dropdown).
	void RenderMainWindow()
	{
		if (!ImGui::GetCurrentContext()) return;
		if (!s_showMainWindow) return;

		ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("gw2doctor - Startup & Stability Diagnostics", &s_showMainWindow))
		{
			RenderDiagnosticsContent();
		}
		ImGui::End();
	}

	// Nexus-embedded options panel (shown under Configure in Nexus's addon
	// list) - deliberately thin: a summary line and a button to open the
	// real window, same pattern as cba4gw2's RenderEmbeddedOptions ->
	// RenderMainWindow.
	void AddonOptions()
	{
		if (!ImGui::GetCurrentContext()) return;

		ImGui::TextDisabled("GW2 Doctor: Startup & Stability Diagnostics");
		ImGui::Spacing();

		if (ImGui::Button("Open GW2 Doctor"))
		{
			s_showMainWindow = true;
		}
	}

	void AddonLoad(AddonAPI* aApi)
	{
		APIDefs = aApi;
		if (APIDefs && APIDefs->Renderer.Register)
		{
			APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);
			APIDefs->Renderer.Register(ERenderType_Render, RenderMainWindow);
		}
	}

	void AddonUnload()
	{
		if (APIDefs && APIDefs->Renderer.Deregister)
		{
			APIDefs->Renderer.Deregister(AddonOptions);
			APIDefs->Renderer.Deregister(RenderMainWindow);
		}
		s_scanned = false;
		s_showMainWindow = false;
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
	AddonDef.Version.Minor = 4;
	AddonDef.Version.Build = 1;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "Emisan01";
	AddonDef.Description =
		"Startup & stability diagnostics for Guild Wars 2 - overlay/injector modules, live CPU/GPU/RAM "
		"headroom, GW2 graphics settings worth knowing about, addons/ folder health, and safe shader-cache "
		"cleanup. Read-only reporting except the cache-clear buttons, which need explicit confirmation.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;
	AddonDef.Provider = EUpdateProvider_None;
	AddonDef.UpdateLink = nullptr;

	return &AddonDef;
}
