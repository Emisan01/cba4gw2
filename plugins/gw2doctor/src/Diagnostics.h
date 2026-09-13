#pragma once
#include <string>
#include <vector>

namespace gw2doc
{
	// A DLL loaded in this process that matches a known third-party
	// overlay/injector, plus what it is. Read-only detection only - this
	// module never touches or unloads anything it finds.
	struct ModuleFinding
	{
		std::wstring fileName;   // as loaded, e.g. "RTSSHooks64.dll"
		std::string label;       // "RivaTuner Statistics Server / MSI Afterburner"
		std::string category;    // "Overlay", "Recording", "Storefront Overlay"
	};

	struct GpuInfo
	{
		bool ok = false;
		std::string description;
		unsigned long long dedicatedVideoMemoryMB = 0;
	};

	struct DiskInfo
	{
		bool ok = false;
		std::wstring drive;
		unsigned long long freeMB = 0;
		unsigned long long totalMB = 0;
	};

	// Enumerates every module loaded in THIS process (the GW2 process
	// Nexus and every addon run inside) and returns the subset that
	// matches a known overlay/injector DLL name. This is not hooking or
	// reading anything about those modules beyond their own file name -
	// the same category of introspection cba4gw2's NexusEcosystem.cpp
	// already does for ArcDPS/Fast_Load via GetModuleHandleW.
	std::vector<ModuleFinding> ScanLoadedModules();

	// Primary GPU adapter description + dedicated VRAM, via DXGI adapter
	// enumeration (no game-memory access, just an OS-level D3D query).
	GpuInfo GetPrimaryGpuInfo();

	// Free/total space on the drive GW2's own executable is running from.
	DiskInfo GetGw2DriveInfo();
}
