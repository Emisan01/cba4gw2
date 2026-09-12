#include "NexusEcosystem.h"
#include "Shared.h"
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

namespace cba
{
	NexusEcosystem::NexusEcosystem()
	{
		mWorkerRunning.store(true);
		mIniSyncThread = std::thread(&NexusEcosystem::IniSyncWorker, this);
	}

	NexusEcosystem::~NexusEcosystem()
	{
		Shutdown();
	}

	NexusEcosystem& NexusEcosystem::Get()
	{
		static NexusEcosystem instance;
		return instance;
	}

	void NexusEcosystem::Initialize()
	{
		Update();
	}

	void NexusEcosystem::Update()
	{
		auto now = std::chrono::steady_clock::now();
		// Throttle checks to once every 2 seconds
		if (std::chrono::duration_cast<std::chrono::seconds>(now - mLastCheckTime).count() < 2)
			return;
			
		mLastCheckTime = now;

		mIsArcDPSLoaded.store(GetModuleHandleW(L"d3d11.dll") != nullptr || GetModuleHandleW(L"ArcDPS.dll") != nullptr);
		mIsFastLoadLoaded.store(GetModuleHandleW(L"Fast_Load.dll") != nullptr);
	}

	void NexusEcosystem::Shutdown()
	{
		if (!mShuttingDown.exchange(true))
		{
			if (mIniSyncThread.joinable())
				mIniSyncThread.join();
		}
	}

	void NexusEcosystem::IniSyncWorker()
	{
		while (mWorkerRunning.load())
		{
			if (mShuttingDown.load())
			{
				if (mIsArcDPSLoaded.load())
				{
					// Wait ~200ms to ensure ArcDPS has closed its handles and written its INI files
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					
					if (::CurrentSettings.ShowMiniHUD)
						InjectArcDPSIni();
						
					if (::CurrentSettings.SyncArcDpsTheme)
						SyncArcDpsColors();
				}
				break; // End the worker thread
			}

			// Poll every 100ms
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	void NexusEcosystem::InjectArcDPSIni()
	{
		// Attempt to read and inject our CBA MiniHUD window state into ArcDPS's ImGui INI
		// so that the shared ImGui context automatically restores our layout next time.
		std::string iniPath = "addons\\arcdps\\arcdps_imgui.ini";
		
		std::ifstream inFile(iniPath);
		if (!inFile.is_open()) return;

		std::vector<std::string> lines;
		std::string line;
		bool hasOurBlock = false;

		while (std::getline(inFile, line))
		{
			lines.push_back(line);
			if (line == "[Window][CBAMiniHUD]")
				hasOurBlock = true;
		}
		inFile.close();

		if (!hasOurBlock)
		{
			// Inject our block at the end
			std::ofstream outFile(iniPath, std::ios::app);
			if (outFile.is_open())
			{
				outFile << "\n[Window][CBAMiniHUD]\n";
				outFile << "Pos=0,0\n";
				outFile << "Size=150,24\n";
				outFile << "Collapsed=0\n";
				outFile.close();
			}
		}
	}

	void NexusEcosystem::SyncArcDpsColors()
	{
		std::string iniPath = "addons\\arcdps\\arcdps.ini";
		std::ifstream inFile(iniPath);
		if (!inFile.is_open()) return;

		std::vector<std::string> lines;
		std::string line;
		bool inColors = false;
		bool injectedColors = false;

		while (std::getline(inFile, line))
		{
			// Strip whitespace for comparison
			std::string tLine = line;
			tLine.erase(tLine.find_last_not_of(" \n\r\t") + 1);

			if (tLine.empty()) {
				lines.push_back(line);
				continue;
			}

			if (tLine[0] == '[')
			{
				if (inColors && !injectedColors)
				{
					// We are leaving the [colors] block and haven't written our colors yet.
					// Write them at the end of the block.
					// 2 = ImGuiCol_WindowBg, 11 = ImGuiCol_TitleBg
					lines.push_back("2=0.08,0.08,0.08,0.90");
					lines.push_back("11=0.20,0.30,0.50,1.00");
					injectedColors = true;
				}
				inColors = (tLine == "[colors]");
			}
			else if (inColors)
			{
				// If we find existing keys, skip adding them to lines so we overwrite them
				if (tLine.find("2=") == 0 || tLine.find("11=") == 0)
					continue;
			}
			
			lines.push_back(line);
		}
		inFile.close();

		if (!injectedColors)
		{
			// [colors] section was not found or was the last section
			if (!inColors)
				lines.push_back("\n[colors]");
			lines.push_back("2=0.08,0.08,0.08,0.90");
			lines.push_back("11=0.20,0.30,0.50,1.00");
		}

		// Atomic write
		std::string tmpPath = iniPath + ".tmp";
		std::ofstream outFile(tmpPath, std::ios::trunc);
		if (outFile.is_open())
		{
			for (const auto& l : lines)
				outFile << l << "\n";
			outFile.close();
			std::error_code ec;
			std::filesystem::rename(tmpPath, iniPath, ec);
		}
	}
}
