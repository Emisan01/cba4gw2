#include "Settings.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <windows.h>

namespace cba
{
	namespace
	{
		std::string ConfigPath(const std::string& aAddonDir)
		{
			return aAddonDir + "\\settings.ini";
		}

		std::string LegacyConfigPath(const std::string& aAddonDir)
		{
			return aAddonDir + "\\settings.cfg";
		}

		std::string LockFilePath(const std::string& aAddonDir)
		{
			return aAddonDir + "\\cba_session.lock";
		}

		DeficiencyType ParseType(const std::string& aValue)
		{
			if (aValue == "Protan") return DeficiencyType::Protan;
			if (aValue == "Tritan") return DeficiencyType::Tritan;
			return DeficiencyType::Deutan;
		}

		const char* TypeName(DeficiencyType aType)
		{
			switch (aType)
			{
				case DeficiencyType::Protan: return "Protan";
				case DeficiencyType::Tritan: return "Tritan";
				default:                     return "Deutan";
			}
		}
	}

	Settings Settings::Load(const std::string& aAddonDir)
	{
		Settings s{};

		// Crash detection via breadcrumb lock file
		std::string lockPath = LockFilePath(aAddonDir);
		if (!aAddonDir.empty() && std::filesystem::exists(lockPath))
		{
			s.SafeModeTriggered = true;
			s.CleanExit = false;
			s.Enabled = false; // Safe-mode: keep filter disabled on crash recovery
		}

		std::string path = ConfigPath(aAddonDir);
		if (!std::filesystem::exists(path))
		{
			std::string legacy = LegacyConfigPath(aAddonDir);
			if (std::filesystem::exists(legacy))
			{
				path = legacy;
			}
		}

		std::ifstream file(path);
		if (!file.is_open())
			return s; // first run — defaults are fine

		auto safeStoi = [](const std::string& v, int def) -> int {
			try { return std::stoi(v); } catch (...) { return def; }
		};
		auto safeStod = [](const std::string& v, double def) -> double {
			try { return std::stod(v); } catch (...) { return def; }
		};
		auto safeStof = [](const std::string& v, float def) -> float {
			try { return std::stof(v); } catch (...) { return def; }
		};

		std::string line;
		while (std::getline(file, line))
		{
			auto sep = line.find('=');
			if (sep == std::string::npos) continue;

			std::string key = line.substr(0, sep);
			std::string value = line.substr(sep + 1);

			if (key == "Enabled")
			{
				if (!s.SafeModeTriggered)
					s.Enabled = (value == "1");
			}
			else if (key == "Type")          s.Type = ParseType(value);
			else if (key == "Severity")      s.Severity01 = safeStod(value, 0.0);
			else if (key == "MixedRg")       s.MixedRgSeverity01 = safeStod(value, 0.0);
			else if (key == "MixedBy")       s.MixedBySeverity01 = safeStod(value, 0.0);
			else if (key == "Mixed")         s.Mixed = (value == "1");
			else if (key == "ToggleKeybind") s.ToggleKeybind = value;
			else if (key == "Language")
			{
				if (value == "en") s.Language = 1;
				else if (value == "de") s.Language = 2;
				else if (value == "game") s.Language = 3;
				else if (value == "sys") s.Language = 0;
				else s.Language = safeStoi(value, 1);
			}
			else if (key == "DiagnosisHint") s.DiagnosisHint = value;
			else if (key == "EnableHybrid")  s.EnableHybridMode = (value == "1");
			else if (key == "DebugMode")     s.DebugMode = (value == "1");
			else if (key == "CommanderTagMode")  s.CommanderTagMode = safeStoi(value, 0);
			else if (key == "SmartEnhancer")     s.SmartEnhancer = (value == "1");
			else if (key == "EnhancerHue")       s.EnhancerHue = safeStof(value, 60.0f);
			else if (key == "EnhancerTol")       s.EnhancerTolerance = std::clamp(safeStof(value, 0.12f), 0.04f, 0.20f);
			else if (key == "GammaGain")         s.GammaGain = std::clamp(safeStof(value, 1.0f), 0.70f, 1.30f);
			else if (key == "UiOpacity")         s.UiOpacity = std::clamp(safeStof(value, 1.0f), 0.10f, 1.00f);
			else if (key == "GraphMode")         s.GraphMode = std::clamp(safeStoi(value, 0), 0, 2);
			else if (key == "MainGraphMode")     s.MainGraphMode = std::clamp(safeStoi(value, 0), 0, 2);
			else if (key == "AutoBrightness")    s.AutoBrightness = (value == "1");
			else if (key == "AlwaysDirectStart") s.AlwaysDirectStart = (value == "1");
			else if (key == "LoadOnStartup")     s.LoadOnStartup = (value == "1");
			else if (key == "ShowMainWindow")   s.ShowMainWindow = (value == "1");
			else if (key == "ShowGraphWindow")  s.ShowGraphWindow = (value == "1");
			else if (key == "DetachedWindow")   {
				s.DetachedWindow = (value == "1");
				if (s.DetachedWindow && !s.ShowMainWindow && !s.ShowGraphWindow) {
					s.ShowMainWindow = true;
				}
			}
			else if (key == "ShowQuickAccess")   s.ShowQuickAccessIcon = (value == "1");
			else if (key == "SystemWide")        s.SystemWide = (value == "1");
			else if (key.rfind("Preset", 0) == 0 && key.size() >= 10)
			{
				int idx = key[6] - '0';
				if (idx >= 0 && idx < 3 && key[7] == '_')
				{
					std::string prop = key.substr(8);
					if (prop == "Name") s.Presets[idx].Name = value;
					else if (prop == "Type") s.Presets[idx].Type = safeStoi(value, 0);
					else if (prop == "Sev")  s.Presets[idx].Severity = safeStod(value, 1.0);
					else if (prop == "Tol")  s.Presets[idx].Tolerance = safeStof(value, 0.12f);
				}
			}
			else if (key.rfind("Slot", 0) == 0 && key.size() >= 8)
			{
				int idx = key[4] - '0';
				if (idx >= 0 && idx < 3 && key[5] == '_')
				{
					std::string prop = key.substr(6);
					if (prop == "Used") s.Slots[idx].Used = (value == "1");
					else if (prop == "Name") s.Slots[idx].Name = value;
					else if (prop == "Type") s.Slots[idx].Type = ParseType(value);
					else if (prop == "Sev")  s.Slots[idx].Severity01 = safeStod(value, 0.0);
					else if (prop == "Mixed") s.Slots[idx].Mixed = (value == "1");
					else if (prop == "MixedRg") s.Slots[idx].MixedRg01 = safeStod(value, 0.0);
					else if (prop == "MixedBy") s.Slots[idx].MixedBy01 = safeStod(value, 0.0);
					else if (prop == "Gamma") s.Slots[idx].GammaGain = std::clamp(safeStof(value, 1.0f), 0.70f, 1.30f);
				}
			}
		}

		return s;
	}

	void Settings::Save(const std::string& aAddonDir) const
	{
		if (aAddonDir.empty()) return;

		std::error_code ec;
		std::filesystem::create_directories(aAddonDir, ec);

		std::string tmpPath = aAddonDir + "\\settings.ini.tmp";
		std::string targetPath = aAddonDir + "\\settings.ini";

		std::ofstream file(tmpPath, std::ios::trunc);
		if (!file.is_open())
			return;

		file << "[CBA]\n";
		file << "Enabled=" << (Enabled ? "1" : "0") << "\n";
		file << "Type=" << TypeName(Type) << "\n";
		file << "Severity=" << Severity01 << "\n";
		file << "MixedRg=" << MixedRgSeverity01 << "\n";
		file << "MixedBy=" << MixedBySeverity01 << "\n";
		file << "Mixed=" << (Mixed ? "1" : "0") << "\n";
		file << "ToggleKeybind=" << ToggleKeybind << "\n";
		file << "Language=" << Language << "\n";
		file << "DiagnosisHint=" << DiagnosisHint << "\n";
		file << "EnableHybrid=" << (EnableHybridMode ? "1" : "0") << "\n";
		file << "DebugMode=" << (DebugMode ? "1" : "0") << "\n";
		file << "CommanderTagMode=" << CommanderTagMode << "\n";
		file << "SmartEnhancer=" << (SmartEnhancer ? "1" : "0") << "\n";
		file << "EnhancerHue=" << EnhancerHue << "\n";
		file << "EnhancerTol=" << EnhancerTolerance << "\n";
		file << "GammaGain=" << GammaGain << "\n";
		file << "UiOpacity=" << UiOpacity << "\n";
		file << "GraphMode=" << GraphMode << "\n";
		file << "MainGraphMode=" << MainGraphMode << "\n";
		file << "AutoBrightness=" << (AutoBrightness ? "1" : "0") << "\n";
		file << "AlwaysDirectStart=" << (AlwaysDirectStart ? "1" : "0") << "\n";
		file << "LoadOnStartup=" << (LoadOnStartup ? "1" : "0") << "\n";
		file << "ShowMainWindow=" << (ShowMainWindow ? "1" : "0") << "\n";
		file << "ShowGraphWindow=" << (ShowGraphWindow ? "1" : "0") << "\n";
		file << "DetachedWindow=" << (ShowGraphWindow ? "1" : "0") << "\n";
		file << "ShowQuickAccess=" << (ShowQuickAccessIcon ? "1" : "0") << "\n";
		file << "SystemWide=" << (SystemWide ? "1" : "0") << "\n";

		file << "\n[Presets]\n";
		for (int i = 0; i < 3; ++i)
		{
			file << "Preset" << i << "_Name=" << Presets[i].Name << "\n";
			file << "Preset" << i << "_Type=" << Presets[i].Type << "\n";
			file << "Preset" << i << "_Sev=" << Presets[i].Severity << "\n";
			file << "Preset" << i << "_Tol=" << Presets[i].Tolerance << "\n";
		}

		file << "\n[ProfileSlots]\n";
		for (int i = 0; i < 3; ++i)
		{
			file << "Slot" << i << "_Used=" << (Slots[i].Used ? "1" : "0") << "\n";
			file << "Slot" << i << "_Name=" << Slots[i].Name << "\n";
			file << "Slot" << i << "_Type=" << TypeName(Slots[i].Type) << "\n";
			file << "Slot" << i << "_Sev=" << Slots[i].Severity01 << "\n";
			file << "Slot" << i << "_Mixed=" << (Slots[i].Mixed ? "1" : "0") << "\n";
			file << "Slot" << i << "_MixedRg=" << Slots[i].MixedRg01 << "\n";
			file << "Slot" << i << "_MixedBy=" << Slots[i].MixedBy01 << "\n";
			file << "Slot" << i << "_Gamma=" << Slots[i].GammaGain << "\n";
		}

		file.flush();
		file.close();

		// Atomic File Replacement
		MoveFileExA(tmpPath.c_str(), targetPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
	}

	void Settings::MarkRunning(const std::string& aAddonDir)
	{
		if (aAddonDir.empty()) return;
		std::string lockPath = LockFilePath(aAddonDir);
		std::ofstream lockFile(lockPath, std::ios::trunc);
		if (lockFile.is_open())
		{
			lockFile << "running\n";
			lockFile.close();
		}
	}

	void Settings::MarkCleanExit(const std::string& aAddonDir)
	{
		if (aAddonDir.empty()) return;
		std::string lockPath = LockFilePath(aAddonDir);
		std::error_code ec;
		std::filesystem::remove(lockPath, ec);
	}
}
