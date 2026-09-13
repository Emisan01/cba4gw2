#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Diagnostics.h"

#include <windows.h>
#include <psapi.h>
#include <dxgi1_2.h>
#include <algorithm>
#include <cctype>

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
}
