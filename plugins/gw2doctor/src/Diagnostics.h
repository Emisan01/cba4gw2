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

	// GPU description plus a LIVE VRAM budget/usage reading (DXGI's
	// IDXGIAdapter3::QueryVideoMemoryInfo) - adapted from Emi's own Tyrian
	// Art Companion project (src/tac/hardware_monitor.cpp), which already
	// had this working. Static VRAM size alone doesn't say whether the
	// card is under pressure right now; budget-vs-usage does.
	struct GpuInfo
	{
		bool ok = false;
		std::string description;
		unsigned long long dedicatedVideoMemoryMB = 0;

		bool budgetAvailable = false;
		unsigned long long localBudgetMB = 0;
		unsigned long long localUsageMB = 0;
		float usageToBudgetRatio = 0.0f;
		std::string pressureStatus; // "OK" / "Warning" / "Critical"
	};

	// CPU clock-speed reserve and system/process load, same source as
	// GpuInfo's budget fields (TAC's hardware_monitor.cpp) - tells you
	// whether the CPU is actually running near its max clock or sitting in
	// a power-saving state, which raw core-count never does.
	struct CpuInfo
	{
		bool topologyAvailable = false;
		unsigned int logicalProcessorCount = 0;
		unsigned int physicalCoreCount = 0;

		bool frequencyAvailable = false;
		unsigned int currentMhzAverage = 0;
		unsigned int reportedMaxMhz = 0;
		float currentToMaxRatio = 0.0f;
		std::string clockReserveStatus;

		bool usageAvailable = false;
		float systemUsagePercent = 0.0f;
	};

	// System RAM pressure (physical memory load %) - same source as above.
	struct MemoryInfo
	{
		bool available = false;
		unsigned long long totalPhysicalMB = 0;
		unsigned long long availablePhysicalMB = 0;
		unsigned int memoryLoadPercent = 0;
		std::string pressureStatus;
	};

	struct DiskInfo
	{
		bool ok = false;
		std::wstring drive;
		unsigned long long freeMB = 0;
		unsigned long long totalMB = 0;
	};

	// One GW2 graphics option read from GFXSettings.Gw2-64.exe.xml.
	struct GfxSetting
	{
		std::string name;
		std::string value;
	};

	// A setting flagged as a known, significant performance cost, with a
	// one-line reason - never a claim about THIS system's actual framerate
	// (we have no way to measure that from here), just "this option is
	// expensive by design."
	struct GfxFinding
	{
		std::string name;
		std::string value;
		std::string note;
	};

	struct GfxSettingsInfo
	{
		bool ok = false;
		std::wstring path;
		int resolutionWidth = 0;
		int resolutionHeight = 0;
		std::vector<GfxSetting> allOptions;
		std::vector<GfxFinding> flagged;
	};

	// A cache directory this module knows how to safely clear. Every one of
	// these is a regenerable, OS/driver-managed cache - never game saves,
	// never account data, never anything under Gw2.dat or the addons
	// folder. Explicitly NOT GW2-exclusive where that's true (scopeNote
	// says so) - these are shared caches other DirectX/OpenGL applications
	// on the same system also use and will simply repopulate.
	struct CacheInfo
	{
		bool exists = false;
		std::wstring path;
		unsigned long long sizeMB = 0;
		std::string label;
		std::string scopeNote;
	};

	// A problem spotted in the addons/ folder itself - not a loaded module,
	// a file on disk. Built directly from this session's own debugging:
	// Nexus renames a still-loaded DLL aside to "*.dll.old" when it picks
	// up a changed file while the game is running (harmless, but litters
	// the folder if never cleaned up), and a 0-byte or otherwise
	// implausibly small .dll is the signature of an update that got
	// interrupted mid-write - exactly the "angerissene Addon-DLL" startup-
	// crash scenario this module exists to catch.
	struct AddonFolderFinding
	{
		std::wstring fileName;
		std::string issue;
		unsigned long long sizeBytes = 0;
	};

	// GW2's own MumbleLink data - a named shared-memory block the game
	// itself writes every frame, originally built for the Mumble voice
	// chat app's positional audio, now a de facto public interface every
	// mainstream GW2 companion tool (BlishHUD, TacO, ...) reads the same
	// way. Reading it is opening a mailbox GW2 chose to publish, not
	// touching its private process memory - no hooking, no injection (see
	// CLAUDE.md's account-safety rules, which this respects). Most useful
	// for us: buildId lets a startup-crash report be correlated against a
	// specific GW2 patch (see the "Nexus/ArcDPS out of date after a patch"
	// scenario), and map/focus/combat give a snapshot some context.
	struct MumbleLinkInfo
	{
		bool available = false;  // the shared memory section itself exists
		bool populated = false;  // and GW2 has actually written a frame into it
		unsigned int buildId = 0;
		unsigned int mapId = 0;
		unsigned int mapType = 0;
		unsigned int processId = 0;
		bool gameHasFocus = false;
		bool isInCombat = false;
		bool isMapOpen = false;
	};

	std::vector<ModuleFinding> ScanLoadedModules();
	GpuInfo GetPrimaryGpuInfo();
	DiskInfo GetGw2DriveInfo();
	GfxSettingsInfo GetGfxSettingsInfo();
	CpuInfo GetCpuInfo();
	MemoryInfo GetMemoryInfo();
	MumbleLinkInfo GetMumbleLinkInfo();

	// Looks in <GW2 install dir>/addons/ (derived from this process's own
	// executable path, not assumed) for the two file-level problems noted
	// above. Read-only - lists them, does not delete or rename anything;
	// a stray .old file is odd but never unsafe to leave alone, and a
	// corrupt DLL should be re-downloaded, not silently removed here.
	std::vector<AddonFolderFinding> ScanAddonsFolder();

	// Sizes are computed fresh each call (walks the directory) - deliberately
	// not cached, since the whole point is showing an accurate "how much
	// would this free" number right before the user decides whether to
	// clear it.
	std::vector<CacheInfo> GetClearableCaches();

	// Recursively deletes every FILE under aPath (these caches nest one
	// level, e.g. D3DSCache/<hash>/<guid>.idx), then removes any
	// subdirectories left empty afterward - but never aPath itself, so the
	// driver/OS finds the root it expects still there next time it wants to
	// write a new entry. Files currently locked by a running process
	// (including this GW2 session's own in-use shader cache entries) are
	// silently skipped, not treated as an error.
	// Returns true if the operation ran (even if some files were skipped);
	// false only if aPath itself could not be accessed at all.
	bool ClearCacheDirectory(const std::wstring& aPath, unsigned long long& outFreedMB, unsigned int& outSkippedFiles);
}
