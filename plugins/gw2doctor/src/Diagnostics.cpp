#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Diagnostics.h"

#include <windows.h>
#include <shlobj.h>
#include <psapi.h>
#include <dxgi1_4.h>
#include <powrprof.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace gw2doc
{
	namespace
	{
		struct KnownModule
		{
			const wchar_t* fileName;
			const char* label;
			const char* category;
		};

		// Known third-party DLLs that inject into (or are loaded by) games
		// for overlay/recording/storefront purposes - not exhaustive, just
		// the common ones. Matched by exact file name, case-insensitive.
		// Deliberately excludes core GPU driver DLLs (nvoglv64.dll etc.) -
		// those are always present and not a meaningful finding on their
		// own.
		const KnownModule kKnownModules[] = {
			{ L"RTSSHooks64.dll",                    "RivaTuner Statistics Server / MSI Afterburner", "Overlay" },
			{ L"RTSSHooks.dll",                       "RivaTuner Statistics Server / MSI Afterburner", "Overlay" },
			{ L"nvspcap64.dll",                       "NVIDIA ShadowPlay / GeForce Experience Overlay", "Overlay" },
			{ L"NahimicOSD.dll",                      "Nahimic Audio Overlay",                          "Overlay" },
			{ L"GameOverlayRenderer64.dll",            "Steam Overlay",                                  "Storefront Overlay" },
			{ L"GameOverlayRenderer.dll",               "Steam Overlay",                                  "Storefront Overlay" },
			{ L"discordhook64.dll",                    "Discord Overlay",                                "Overlay" },
			{ L"EOSOverlayRenderer-Win64-Shipping.dll", "Epic Online Services Overlay",                   "Storefront Overlay" },
			{ L"GalaxyOverlay64.dll",                  "GOG Galaxy Overlay",                             "Storefront Overlay" },
			{ L"uplay_r1_loader64.dll",                 "Ubisoft Connect Overlay",                        "Storefront Overlay" },
			{ L"EAOverlayRenderer64.dll",               "EA App / Origin Overlay",                        "Storefront Overlay" },
			{ L"XSplitGameSource64.dll",                "XSplit Game Capture",                            "Recording" },
			{ L"obs-vulkan64.dll",                     "OBS Studio Game Capture",                        "Recording" },
		};

		bool IEquals(const std::wstring& a, const wchar_t* b)
		{
			size_t lb = 0;
			while (b[lb]) ++lb;
			if (a.size() != lb) return false;
			for (size_t i = 0; i < lb; ++i)
			{
				if (towlower(a[i]) != towlower(b[i])) return false;
			}
			return true;
		}

		std::wstring KnownFolder(REFKNOWNFOLDERID aId)
		{
			std::wstring result;
			PWSTR raw = nullptr;
			if (SUCCEEDED(SHGetKnownFolderPath(aId, 0, nullptr, &raw)) && raw)
			{
				result = raw;
				CoTaskMemFree(raw);
			}
			return result;
		}

		// Pulls Name="..." out of one XML line - the settings file is
		// regular enough (one attribute per quoted value, no escaped
		// quotes inside them) that a tiny string search is enough; not
		// worth a real XML parser dependency for a single known-shape file.
		std::string ExtractAttr(const std::string& aLine, const char* aAttr)
		{
			std::string needle = std::string(aAttr) + "=\"";
			size_t pos = aLine.find(needle);
			if (pos == std::string::npos) return {};
			pos += needle.size();
			size_t end = aLine.find('"', pos);
			if (end == std::string::npos) return {};
			return aLine.substr(pos, end - pos);
		}

		// Known-expensive GW2 graphics options - a note on WHY each one
		// costs what it costs, not a claim about this specific system's
		// framerate (we have no way to measure that from here).
		struct ExpensiveSetting
		{
			const char* name;
			const char* value; // matches this specific value; nullptr = any non-"off"/non-lowest value
			const char* note;
		};

		// Pressure-status thresholds and CPU frequency query, adapted from
		// Emi's Tyrian Art Companion (src/tac/hardware_monitor.cpp), which
		// already had this working there.
		const char* RatioPressureStatus(float ratio)
		{
			if (ratio > 0.90f) return "Critical";
			if (ratio >= 0.75f) return "Warning";
			return "OK";
		}

		const char* MemoryPressureStatus(unsigned int loadPercent)
		{
			if (loadPercent > 90u) return "Critical";
			if (loadPercent >= 75u) return "Warning";
			return "OK";
		}

		const char* CpuClockReserveStatus(float ratio)
		{
			if (ratio <= 0.0f) return "Unknown";
			if (ratio >= 0.80f) return "Clocks active / near max";
			if (ratio >= 0.40f) return "Partial clock reserve";
			return "Power-saving / reserve likely";
		}

		// PROCESSOR_POWER_INFORMATION isn't declared in the public Windows
		// SDK headers even though CallNtPowerInformation is - same layout
		// TAC's own hardware_monitor.cpp uses.
		struct TacProcessorPowerInformation
		{
			ULONG Number = 0;
			ULONG MaxMhz = 0;
			ULONG CurrentMhz = 0;
			ULONG MhzLimit = 0;
			ULONG MaxIdleState = 0;
			ULONG CurrentIdleState = 0;
		};

		// MumbleLink's layout - the public de facto standard every GW2
		// companion tool reads (originally the Mumble voice-chat plugin
		// struct; GW2 fills the trailing "context" bytes with its own
		// GW2Context below). Not declared anywhere we can #include - this
		// is the well-known public shape, not a guess.
		#pragma pack(push, 1)
		struct MumbleLinkedMem
		{
			UINT32 uiVersion;
			DWORD uiTick;
			float fAvatarPosition[3];
			float fAvatarFront[3];
			float fAvatarTop[3];
			wchar_t name[256];
			float fCameraPosition[3];
			float fCameraFront[3];
			float fCameraTop[3];
			wchar_t identity[256];
			UINT32 context_len;
			unsigned char context[256];
			wchar_t description[2048];
		};

		// GW2's own extension living inside MumbleLinkedMem::context.
		// uiState bit values are ArenaNet's own documented Mumble Link
		// flags (api.guildwars2.com wiki).
		struct Gw2MumbleContext
		{
			unsigned char serverAddress[28];
			uint32_t mapId;
			uint32_t mapType;
			uint32_t shardId;
			uint32_t instance;
			uint32_t buildId;
			uint32_t uiState;
			uint16_t compassWidth;
			uint16_t compassHeight;
			float compassRotation;
			float playerX, playerY;
			float mapCenterX, mapCenterY;
			float mapScale;
			uint32_t processId;
			uint8_t mountIndex;
		};
		#pragma pack(pop)

		constexpr uint32_t kGw2UiState_IsMapOpen = 0x01;
		constexpr uint32_t kGw2UiState_GameHasFocus = 0x08;
		constexpr uint32_t kGw2UiState_IsInCombat = 0x40;

		const ExpensiveSetting kExpensiveSettings[] = {
			{ "reflections", "all", "Renders the whole scene a second time for water-plane reflections - one of the single most expensive settings in the game." },
			{ "sampling", "supersample", "Renders at a higher internal resolution then downsamples - a direct GPU/VRAM multiplier." },
			{ "lodDistance", "ultra", "Keeps full-detail models and terrain visible much further away - more geometry drawn every frame." },
			{ "charModelLimit", "highest", "No cap on nearby character model detail - costly in crowded areas (world bosses, cities, zergs)." },
			{ "charModelQuality", "highest", "Highest per-character model detail for everyone nearby, same crowded-area cost as charModelLimit." },
			{ "screenspaceShadows", "true", "An additional per-pixel shadow pass layered on top of the regular shadow maps." },
			{ "shadowsResolution", "2048", "High-resolution shadow maps - cost scales with the number of shadow-casting lights on screen." },
			{ "shadowsCascadeCount", "3", "More shadow cascades - more shadow map splits rendered per light per frame." },
		};
	}

	std::vector<ModuleFinding> ScanLoadedModules()
	{
		std::vector<ModuleFinding> out;

		HANDLE process = GetCurrentProcess();
		HMODULE modules[1024];
		DWORD needed = 0;

		if (!EnumProcessModules(process, modules, sizeof(modules), &needed))
		{
			return out;
		}

		size_t count = std::min<size_t>(needed / sizeof(HMODULE), 1024);
		for (size_t i = 0; i < count; ++i)
		{
			wchar_t pathBuf[MAX_PATH]{};
			if (!GetModuleFileNameExW(process, modules[i], pathBuf, MAX_PATH))
			{
				continue;
			}

			std::wstring full(pathBuf);
			size_t slash = full.find_last_of(L"\\/");
			std::wstring fileName = (slash == std::wstring::npos) ? full : full.substr(slash + 1);

			for (const auto& known : kKnownModules)
			{
				if (IEquals(fileName, known.fileName))
				{
					out.push_back({ fileName, known.label, known.category });
					break;
				}
			}
		}

		return out;
	}

	GpuInfo GetPrimaryGpuInfo()
	{
		GpuInfo info{};

		IDXGIFactory1* factory = nullptr;
		if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)) || !factory)
		{
			return info;
		}

		IDXGIAdapter1* adapter = nullptr;
		if (SUCCEEDED(factory->EnumAdapters1(0, &adapter)) && adapter)
		{
			DXGI_ADAPTER_DESC1 desc{};
			if (SUCCEEDED(adapter->GetDesc1(&desc)))
			{
				int len = WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, nullptr, 0, nullptr, nullptr);
				if (len > 0)
				{
					std::string narrow(len - 1, '\0');
					WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, narrow.data(), len, nullptr, nullptr);
					info.description = narrow;
				}
				info.dedicatedVideoMemoryMB = static_cast<unsigned long long>(desc.DedicatedVideoMemory) / (1024ull * 1024ull);
				info.ok = true;
			}

			// Live VRAM budget/usage (adapted from Emi's Tyrian Art Companion,
			// src/tac/hardware_monitor.cpp) - static card size doesn't say
			// whether the GPU is under pressure right now, this does.
			IDXGIAdapter3* adapter3 = nullptr;
			if (SUCCEEDED(adapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&adapter3)) && adapter3)
			{
				DXGI_QUERY_VIDEO_MEMORY_INFO memInfo{};
				if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo)) && memInfo.Budget > 0)
				{
					info.budgetAvailable = true;
					info.localBudgetMB = memInfo.Budget / (1024ull * 1024ull);
					info.localUsageMB = memInfo.CurrentUsage / (1024ull * 1024ull);
					info.usageToBudgetRatio = static_cast<float>(memInfo.CurrentUsage) / static_cast<float>(memInfo.Budget);
					info.pressureStatus = RatioPressureStatus(info.usageToBudgetRatio);
				}
				adapter3->Release();
			}

			adapter->Release();
		}

		factory->Release();
		return info;
	}

	DiskInfo GetGw2DriveInfo()
	{
		DiskInfo info{};

		wchar_t exePath[MAX_PATH]{};
		if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0)
		{
			return info;
		}

		wchar_t rootPath[4]{};
		if (!GetVolumePathNameW(exePath, rootPath, 4))
		{
			return info;
		}
		info.drive = rootPath;

		ULARGE_INTEGER freeBytes{}, totalBytes{};
		if (GetDiskFreeSpaceExW(rootPath, &freeBytes, &totalBytes, nullptr))
		{
			info.freeMB = freeBytes.QuadPart / (1024ull * 1024ull);
			info.totalMB = totalBytes.QuadPart / (1024ull * 1024ull);
			info.ok = true;
		}

		return info;
	}

	CpuInfo GetCpuInfo()
	{
		CpuInfo info{};

		SYSTEM_INFO sysInfo{};
		GetNativeSystemInfo(&sysInfo);
		info.logicalProcessorCount = sysInfo.dwNumberOfProcessors;

		DWORD length = 0;
		if (GetLogicalProcessorInformationEx(RelationAll, nullptr, &length) || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			auto* buffer = static_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(std::malloc(length));
			if (buffer)
			{
				if (GetLogicalProcessorInformationEx(RelationAll, buffer, &length))
				{
					DWORD offset = 0;
					while (offset < length)
					{
						auto* entry = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(reinterpret_cast<unsigned char*>(buffer) + offset);
						if (entry->Relationship == RelationProcessorCore)
						{
							++info.physicalCoreCount;
						}
						if (entry->Size == 0) break;
						offset += entry->Size;
					}
				}
				std::free(buffer);
			}
		}
		info.topologyAvailable = info.logicalProcessorCount > 0;

		// Clock-speed reserve, same technique as GpuInfo's live budget:
		// adapted from Emi's Tyrian Art Companion (hardware_monitor.cpp).
		if (info.logicalProcessorCount > 0)
		{
			std::array<TacProcessorPowerInformation, 128> power{};
			unsigned int count = std::min<unsigned int>(info.logicalProcessorCount, (unsigned int)power.size());
			ULONG bytes = (ULONG)(sizeof(TacProcessorPowerInformation) * count);

			if (CallNtPowerInformation(ProcessorInformation, nullptr, 0, power.data(), bytes) == ERROR_SUCCESS)
			{
				unsigned int sum = 0;
				unsigned int reportedMax = 0;
				for (unsigned int i = 0; i < count; ++i)
				{
					sum += (unsigned int)power[i].CurrentMhz;
					reportedMax = std::max(reportedMax, (unsigned int)power[i].MaxMhz);
				}
				if (count > 0 && reportedMax > 0)
				{
					info.frequencyAvailable = true;
					info.currentMhzAverage = sum / count;
					info.reportedMaxMhz = reportedMax;
					info.currentToMaxRatio = (float)info.currentMhzAverage / (float)reportedMax;
					info.clockReserveStatus = CpuClockReserveStatus(info.currentToMaxRatio);
				}
			}
		}

		// System-wide CPU usage needs two samples with time between them -
		// not implemented here (a single on-demand "Rescan" click has no
		// natural second sample to diff against without blocking on a
		// sleep). Left unavailable rather than faked.
		info.usageAvailable = false;

		return info;
	}

	MemoryInfo GetMemoryInfo()
	{
		MemoryInfo info{};

		MEMORYSTATUSEX status{};
		status.dwLength = sizeof(status);
		if (!GlobalMemoryStatusEx(&status)) return info;

		info.available = true;
		info.totalPhysicalMB = status.ullTotalPhys / (1024ull * 1024ull);
		info.availablePhysicalMB = status.ullAvailPhys / (1024ull * 1024ull);
		info.memoryLoadPercent = status.dwMemoryLoad;
		info.pressureStatus = MemoryPressureStatus(info.memoryLoadPercent);

		return info;
	}

	MumbleLinkInfo GetMumbleLinkInfo()
	{
		MumbleLinkInfo info{};

		// Opened by name, not created - GW2 itself owns and writes this
		// section; we only ever read. FILE_MAP_READ is the narrowest
		// access that works, on purpose.
		HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, L"MumbleLink");
		if (!mapping) return info;
		info.available = true;

		void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(MumbleLinkedMem));
		if (view)
		{
			const auto* mem = static_cast<const MumbleLinkedMem*>(view);
			if (mem->uiVersion != 0)
			{
				info.populated = true;
				if (mem->context_len >= sizeof(Gw2MumbleContext))
				{
					const auto* ctx = reinterpret_cast<const Gw2MumbleContext*>(mem->context);
					info.buildId = ctx->buildId;
					info.mapId = ctx->mapId;
					info.mapType = ctx->mapType;
					info.processId = ctx->processId;
					info.gameHasFocus = (ctx->uiState & kGw2UiState_GameHasFocus) != 0;
					info.isInCombat = (ctx->uiState & kGw2UiState_IsInCombat) != 0;
					info.isMapOpen = (ctx->uiState & kGw2UiState_IsMapOpen) != 0;
				}
			}
			UnmapViewOfFile(view);
		}

		CloseHandle(mapping);
		return info;
	}

	GfxSettingsInfo GetGfxSettingsInfo()
	{
		GfxSettingsInfo info{};

		std::wstring roaming = KnownFolder(FOLDERID_RoamingAppData);
		if (roaming.empty()) return info;
		info.path = roaming + L"\\Guild Wars 2\\GFXSettings.Gw2-64.exe.xml";

		std::ifstream file(info.path);
		if (!file.is_open()) return info;

		std::string line;
		while (std::getline(file, line))
		{
			if (line.find("<RESOLUTION") != std::string::npos)
			{
				std::string w = ExtractAttr(line, "Width");
				std::string h = ExtractAttr(line, "Height");
				if (!w.empty()) info.resolutionWidth = std::atoi(w.c_str());
				if (!h.empty()) info.resolutionHeight = std::atoi(h.c_str());
				continue;
			}

			if (line.find("<OPTION") == std::string::npos) continue;

			std::string name = ExtractAttr(line, "Name");
			std::string value = ExtractAttr(line, "Value");
			if (name.empty()) continue;

			info.allOptions.push_back({ name, value });

			for (const auto& exp : kExpensiveSettings)
			{
				if (name == exp.name && value == exp.value)
				{
					info.flagged.push_back({ name, value, exp.note });
					break;
				}
			}
		}

		info.ok = true;
		return info;
	}

	std::vector<CacheInfo> GetClearableCaches()
	{
		std::vector<CacheInfo> out;

		std::wstring localAppData = KnownFolder(FOLDERID_LocalAppData);
		if (localAppData.empty()) return out;

		struct Candidate { std::wstring path; const char* label; const char* scope; };
		const Candidate candidates[] = {
			{ localAppData + L"\\D3DSCache",
			  "Windows DirectX Shader Cache",
			  "OS-managed, shared by every DirectX application on this PC - not GW2-exclusive. Regenerates automatically." },
			{ localAppData + L"\\NVIDIA\\DXCache",
			  "NVIDIA Shader Cache (DXCache)",
			  "Driver-managed, shared by every DirectX/OpenGL application using this GPU - not GW2-exclusive. Regenerates automatically; the next shader compile after clearing is briefly slower." },
			{ localAppData + L"\\NVIDIA\\GLCache",
			  "NVIDIA OpenGL Shader Cache (GLCache)",
			  "Driver-managed, shared by every OpenGL application using this GPU - not GW2-exclusive. Regenerates automatically." },
		};

		for (const auto& c : candidates)
		{
			std::error_code ec;
			if (!fs::is_directory(c.path, ec)) continue;

			unsigned long long totalBytes = 0;
			for (const auto& entry : fs::recursive_directory_iterator(c.path, fs::directory_options::skip_permission_denied, ec))
			{
				if (ec) break;
				std::error_code sizeEc;
				if (entry.is_regular_file(sizeEc))
				{
					auto sz = entry.file_size(sizeEc);
					if (!sizeEc) totalBytes += sz;
				}
			}

			CacheInfo info{};
			info.exists = true;
			info.path = c.path;
			info.sizeMB = totalBytes / (1024ull * 1024ull);
			info.label = c.label;
			info.scopeNote = c.scope;
			out.push_back(info);
		}

		return out;
	}

	bool ClearCacheDirectory(const std::wstring& aPath, unsigned long long& outFreedMB, unsigned int& outSkippedFiles)
	{
		outFreedMB = 0;
		outSkippedFiles = 0;

		std::error_code ec;
		if (!fs::is_directory(aPath, ec)) return false;

		unsigned long long freedBytes = 0;
		std::vector<fs::path> directories;

		for (const auto& entry : fs::recursive_directory_iterator(aPath, fs::directory_options::skip_permission_denied, ec))
		{
			if (ec) break;

			std::error_code entryEc;
			if (entry.is_directory(entryEc))
			{
				directories.push_back(entry.path());
				continue;
			}

			std::error_code sizeEc;
			auto sz = entry.file_size(sizeEc);

			std::error_code removeEc;
			if (fs::remove(entry.path(), removeEc))
			{
				if (!sizeEc) freedBytes += sz;
			}
			else
			{
				// Locked by whatever is currently using it (this GW2
				// session's own live shader cache entries, most likely) -
				// expected, not an error worth surfacing per-file.
				++outSkippedFiles;
			}
		}

		// Deepest-first so a now-empty child directory is gone before its
		// parent is attempted - recursive_directory_iterator visits parents
		// before children, so walking the collected list in reverse gives
		// deepest-first. fs::remove only succeeds on an empty directory, so
		// one still holding a skipped (locked) file is simply left in place.
		for (auto it = directories.rbegin(); it != directories.rend(); ++it)
		{
			std::error_code removeEc;
			fs::remove(*it, removeEc);
		}

		outFreedMB = freedBytes / (1024ull * 1024ull);
		return true;
	}

	std::vector<AddonFolderFinding> ScanAddonsFolder()
	{
		std::vector<AddonFolderFinding> out;

		wchar_t exePath[MAX_PATH]{};
		if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) return out;

		fs::path addonsDir = fs::path(exePath).parent_path() / L"addons";
		std::error_code ec;
		if (!fs::is_directory(addonsDir, ec)) return out;

		for (const auto& entry : fs::directory_iterator(addonsDir, fs::directory_options::skip_permission_denied, ec))
		{
			if (ec) break;
			std::error_code fileEc;
			if (!entry.is_regular_file(fileEc)) continue;

			std::wstring name = entry.path().filename().wstring();
			std::wstring lower = name;
			std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

			if (lower.size() > 8 && lower.compare(lower.size() - 8, 8, L".dll.old") == 0)
			{
				out.push_back({ name, "Leftover from a hot-reload rename - Nexus renames a still-loaded DLL aside "
					"when it picks up a changed file mid-session. Harmless; cleared automatically the next time "
					"that addon deploys successfully.", 0 });
				continue;
			}

			if (lower.size() > 4 && lower.compare(lower.size() - 4, 4, L".dll") == 0)
			{
				std::error_code sizeEc;
				auto sz = entry.file_size(sizeEc);
				if (!sizeEc && sz < 4096)
				{
					out.push_back({ name, "Implausibly small for an addon DLL - likely an update or download that "
						"was interrupted mid-write. This addon may fail to load, or worse, crash the game while "
						"trying to. Re-download or reinstall it.", sz });
				}
			}
		}

		return out;
	}
}
