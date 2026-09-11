#include "Settings.h"
#include "ParameterRegistry.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <windows.h>
#include <cstdio>

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

		BalanceType ParseType(const std::string& aValue)
		{
			if (aValue == "Protan") return BalanceType::Protan;
			if (aValue == "Tritan") return BalanceType::Tritan;
			return BalanceType::Deutan;
		}

		const char* TypeName(BalanceType aType)
		{
			switch (aType)
			{
				case BalanceType::Protan: return "Protan";
				case BalanceType::Tritan: return "Tritan";
				default:                  return "Deutan";
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
			else if (key == "Severity")      s.Severity01 = safeStof(value, 0.0f);
			else if (key == "MixedRg")       s.MixedRgSeverity01 = safeStof(value, 0.0f);
			else if (key == "MixedBy")       s.MixedBySeverity01 = safeStof(value, 0.0f);
			else if (key == "Mixed")         s.Mixed = (value == "1");
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
			else if (key == "UiTheme")       s.UiTheme = std::clamp(safeStoi(value, 0), 0, 1);
			else if (key == "AdvancedModeUnlocked") s.AdvancedModeUnlocked = (value == "1");
			else if (key == "EyeComfortModeEnabled") s.EyeComfortModeEnabled = (value == "1");
			else if (key == "BlueFilter01")      s.BlueFilter01 = std::clamp(safeStof(value, 0.0f), 0.0f, 1.0f);
			else if (key == "WarmTint01")        s.WarmTint01 = std::clamp(safeStof(value, 0.0f), 0.0f, 1.0f);
			else if (key == "SaturationReduction01") s.SaturationReduction01 = std::clamp(safeStof(value, 0.0f), 0.0f, 1.0f);
			else if (key == "CommanderTagMode")  s.CommanderTagMode = safeStoi(value, 0);
			else if (key == "SmartEnhancer")     s.SmartEnhancer = (value == "1");
			else if (key == "EnhancerTol")       s.EnhancerTolerance = std::clamp(safeStof(value, 0.12f), 0.04f, 0.20f);
			else if (key == "CmdrLayerPriority") s.CommanderTagLayerPriority = safeStoi(value, 0);
			else if (key == "GammaGain")         s.GammaGain = std::clamp(safeStof(value, 1.0f), 0.70f, 1.30f);
			else if (key == "UiOpacity")         s.UiOpacity = std::clamp(safeStof(value, 1.0f), 0.00f, 1.00f);
			else if (key == "GraphMode")         s.GraphMode = std::clamp(safeStoi(value, 0), 0, 2);
			else if (key == "MainGraphMode")     s.MainGraphMode = std::clamp(safeStoi(value, 0), 0, 2);
			else if (key == "AutoBrightness")    s.AutoBrightness = (value == "1");
			else if (key == "AlwaysDirectStart") s.AlwaysDirectStart = (value == "1");
			else if (key == "ShowMainWindow")   s.ShowMainWindow = (value == "1");
			else if (key == "ShowGraphWindow")  s.ShowGraphWindow = (value == "1");
			else if (key == "ShowLabWindow")    s.ShowLabWindow = (value == "1");
			else if (key == "ShowVisionLab")    s.ShowVisionLabWindow = (value == "1");
			else if (key == "ShowQuickAccess")   s.ShowQuickAccessIcon = (value == "1");
			else if (key == "MovableToolbarIcon") s.MovableToolbarIcon = (value == "1");
			else if (key == "ToolbarIconPosX")     s.ToolbarIconPosX = safeStof(value, 405.0f);
			else if (key == "ToolbarIconPosY")     s.ToolbarIconPosY = safeStof(value, 8.0f);
			else if (key == "SystemWide")        s.SystemWide = (value == "1");
			else if (key == "ContrastPairIdx")   s.ContrastPairIndex = std::clamp(safeStoi(value, 0), 0, 4);
			else if (key == "GlassContrastCards") s.GlassContrastCards = (value == "1");
			else if (key == "AutoStartSlot")     s.AutoStartSlot = std::clamp(safeStoi(value, -1), -1, 2);
			else if (key == "LabModeEnabled")    s.LabModeEnabled = (value == "1");
			else if (key == "SelectedLabFilter") s.SelectedLabFilterIndex = safeStoi(value, 0);
			else if (key.rfind("LabFilter_", 0) == 0 && key.size() >= 12)
			{
				size_t underPos = key.find('_', 10);
				if (underPos != std::string::npos)
				{
					int fIdx = safeStoi(key.substr(10, underPos - 10), -1);
					if (fIdx >= 0)
					{
						if (fIdx >= (int)s.LabFilters.size())
							s.LabFilters.resize(fIdx + 1);

						std::string prop = key.substr(underPos + 1);
						if (prop == "Enabled")       s.LabFilters[fIdx].Enabled = (value == "1");
						else if (prop == "Name")     s.LabFilters[fIdx].Name = value;
						else if (prop == "TargetR")  s.LabFilters[fIdx].TargetRgb[0] = safeStof(value, 0.25f);
						else if (prop == "TargetG")  s.LabFilters[fIdx].TargetRgb[1] = safeStof(value, 0.62f);
						else if (prop == "TargetB")  s.LabFilters[fIdx].TargetRgb[2] = safeStof(value, 0.30f);
						else if (prop == "ReplaceR") s.LabFilters[fIdx].ReplaceRgb[0] = safeStof(value, 0.85f);
						else if (prop == "ReplaceG") s.LabFilters[fIdx].ReplaceRgb[1] = safeStof(value, 0.28f);
						else if (prop == "ReplaceB") s.LabFilters[fIdx].ReplaceRgb[2] = safeStof(value, 0.24f);
						else if (prop == "TolTones") s.LabFilters[fIdx].ToleranceTones = std::clamp(safeStoi(value, 3), 1, 32);
						else if (prop == "Diffusion")s.LabFilters[fIdx].Diffusion = std::clamp(safeStof(value, 0.35f), 0.0f, 1.0f);
						else if (prop == "Action")   s.LabFilters[fIdx].ActionType = std::clamp(safeStoi(value, 0), 0, 3);
					else if (prop == "LayerPriority") s.LabFilters[fIdx].LayerPriority = safeStoi(value, (int)fIdx + 1);
					}
				}
			}
			// The [Presets] block that used to parse here was write-only:
			// CurrentSettings.Presets[3] was populated by MainWindow.cpp's
			// Com-Tag Save button but never read back anywhere in the UI or
			// logic layer (found in the 2026-09-09 codebase review). Removed
			// along with the field, the writer, and the Save() lines below -
			// still parses (and ignores) any [Presets] section in an old
			// settings.ini via the loop's default "unknown key" fallthrough.
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

		if (s.LabFilters.empty())
		{
			LabFilter f1;
			f1.Enabled = true;
			f1.Name = "Gruen zu Signal-Rot";
			f1.TargetRgb[0] = 0.25f; f1.TargetRgb[1] = 0.62f; f1.TargetRgb[2] = 0.30f; // #3f9d4d
			f1.ReplaceRgb[0] = 0.85f; f1.ReplaceRgb[1] = 0.28f; f1.ReplaceRgb[2] = 0.24f; // #d9463c
			f1.ToleranceTones = 3;
			f1.Diffusion = 0.35f;
			f1.ActionType = 0;
			f1.LayerPriority = 1;
			s.LabFilters.push_back(f1);

			LabFilter f2;
			f2.Enabled = false;
			f2.Name = "Blau zu Cyan-Kontrast";
			f2.TargetRgb[0] = 0.21f; f2.TargetRgb[1] = 0.44f; f2.TargetRgb[2] = 0.80f; // #3670cc
			f2.ReplaceRgb[0] = 0.15f; f2.ReplaceRgb[1] = 0.68f; f2.ReplaceRgb[2] = 0.74f; // #26aebd
			f2.ToleranceTones = 3;
			f2.Diffusion = 0.40f;
			f2.ActionType = 1;
			f2.LayerPriority = 2;
			s.LabFilters.push_back(f2);
		}

		// All windows start closed by default - user opens them via Nexus icon or hotkey
		s.ShowMainWindow = false;
		s.ShowGraphWindow = false;
		s.ShowLabWindow = false;
		s.ShowVisionLabWindow = false;

		// The filter ALWAYS starts disarmed, every launch, regardless of what
		// was saved - this used to be conditional on a "LoadOnStartup" opt-in
		// (removed 2026-09-09 as dead code once nothing read it anymore), but
		// removing that also silently removed the only thing forcing Enabled
		// back to false on a normal clean-shutdown restart. Net effect Emi hit:
		// the filter would auto-reapply at character select with zero user
		// action this session - exactly the "unexpected color shift on launch"
		// the whole Safe-Start Gate concept exists to prevent. This is the
		// unconditional version of that same guarantee: no opt-in, no
		// exceptions, every single launch starts neutral.
		s.Enabled = false;

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
		file << "Language=" << Language << "\n";
		file << "DiagnosisHint=" << DiagnosisHint << "\n";
		file << "EnableHybrid=" << (EnableHybridMode ? "1" : "0") << "\n";
		file << "DebugMode=" << (DebugMode ? "1" : "0") << "\n";
		file << "UiTheme=" << UiTheme << "\n";
		file << "AdvancedModeUnlocked=" << (AdvancedModeUnlocked ? "1" : "0") << "\n";
		file << "EyeComfortModeEnabled=" << (EyeComfortModeEnabled ? "1" : "0") << "\n";
		file << "BlueFilter01=" << BlueFilter01 << "\n";
		file << "WarmTint01=" << WarmTint01 << "\n";
		file << "SaturationReduction01=" << SaturationReduction01 << "\n";
		file << "CommanderTagMode=" << CommanderTagMode << "\n";
		file << "SmartEnhancer=" << (SmartEnhancer ? "1" : "0") << "\n";
		file << "EnhancerTol=" << EnhancerTolerance << "\n";
		file << "CmdrLayerPriority=" << CommanderTagLayerPriority << "\n";
		file << "GammaGain=" << GammaGain << "\n";
		file << "UiOpacity=" << UiOpacity << "\n";
		file << "GraphMode=" << GraphMode << "\n";
		file << "MainGraphMode=" << MainGraphMode << "\n";
		file << "AutoBrightness=" << (AutoBrightness ? "1" : "0") << "\n";
		file << "AlwaysDirectStart=" << (AlwaysDirectStart ? "1" : "0") << "\n";
		file << "ShowMainWindow=" << (ShowMainWindow ? "1" : "0") << "\n";
		file << "ShowGraphWindow=" << (ShowGraphWindow ? "1" : "0") << "\n";
		file << "ShowLabWindow=" << (ShowLabWindow ? "1" : "0") << "\n";
		file << "ShowVisionLab=" << (ShowVisionLabWindow ? "1" : "0") << "\n";
		file << "ShowQuickAccess=" << (ShowQuickAccessIcon ? "1" : "0") << "\n";
		file << "MovableToolbarIcon=" << (MovableToolbarIcon ? "1" : "0") << "\n";
		file << "ToolbarIconPosX=" << ToolbarIconPosX << "\n";
		file << "ToolbarIconPosY=" << ToolbarIconPosY << "\n";
		file << "SystemWide=" << (SystemWide ? "1" : "0") << "\n";
		file << "ContrastPairIdx=" << ContrastPairIndex << "\n";
		file << "GlassContrastCards=" << (GlassContrastCards ? "1" : "0") << "\n";
		file << "AutoStartSlot=" << AutoStartSlot << "\n";
		file << "LabModeEnabled=" << (LabModeEnabled ? "1" : "0") << "\n";
		file << "SelectedLabFilter=" << SelectedLabFilterIndex << "\n";

		file << "\n[LabFilters]\n";
		for (size_t i = 0; i < LabFilters.size(); ++i)
		{
			file << "LabFilter_" << i << "_Enabled=" << (LabFilters[i].Enabled ? "1" : "0") << "\n";
			file << "LabFilter_" << i << "_Name=" << LabFilters[i].Name << "\n";
			file << "LabFilter_" << i << "_TargetR=" << LabFilters[i].TargetRgb[0] << "\n";
			file << "LabFilter_" << i << "_TargetG=" << LabFilters[i].TargetRgb[1] << "\n";
			file << "LabFilter_" << i << "_TargetB=" << LabFilters[i].TargetRgb[2] << "\n";
			file << "LabFilter_" << i << "_ReplaceR=" << LabFilters[i].ReplaceRgb[0] << "\n";
			file << "LabFilter_" << i << "_ReplaceG=" << LabFilters[i].ReplaceRgb[1] << "\n";
			file << "LabFilter_" << i << "_ReplaceB=" << LabFilters[i].ReplaceRgb[2] << "\n";
			file << "LabFilter_" << i << "_TolTones=" << LabFilters[i].ToleranceTones << "\n";
			file << "LabFilter_" << i << "_Diffusion=" << LabFilters[i].Diffusion << "\n";
			file << "LabFilter_" << i << "_Action=" << LabFilters[i].ActionType << "\n";
			file << "LabFilter_" << i << "_LayerPriority=" << LabFilters[i].LayerPriority << "\n";
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

	std::string Settings::ExportPresetString() const
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "CBA1:T=%d;S=%.2f;M=%d;RG=%.2f;BY=%.2f;G=%.2f;CT=%d;SE=%d;ET=%.2f;EM=%d;BF=%.2f;WT=%.2f;SR=%.2f",
			static_cast<int>(Type),
			static_cast<float>(Severity01),
			Mixed ? 1 : 0,
			static_cast<float>(MixedRgSeverity01),
			static_cast<float>(MixedBySeverity01),
			GammaGain,
			CommanderTagMode,
			SmartEnhancer ? 1 : 0,
			EnhancerTolerance,
			EyeComfortModeEnabled ? 1 : 0,
			BlueFilter01,
			WarmTint01,
			SaturationReduction01);
		std::string out = buf;
		if (!DiagnosisHint.empty())
		{
			std::string safeHint = DiagnosisHint;
			for (char& c : safeHint) {
				if (c == ';' || c == ':' || c == '\n' || c == '\r') c = ' ';
			}
			out += ";H=" + safeHint;
		}
		return out;
	}

	bool Settings::ImportPresetString(const std::string& aPresetStr, std::string* aOutError)
	{
		size_t start = aPresetStr.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
		{
			if (aOutError) *aOutError = "Empty string";
			return false;
		}
		std::string s = aPresetStr.substr(start);
		size_t end = s.find_last_not_of(" \t\r\n");
		if (end != std::string::npos) s = s.substr(0, end + 1);

		if (s.rfind("CBA1:", 0) != 0)
		{
			if (aOutError) *aOutError = "Invalid header (must start with CBA1:)";
			return false;
		}

		std::string payload = s.substr(5);
		std::stringstream ss(payload);
		// Clamp ranges below now read from ParameterRegistry's ParamMeta
		// instead of re-typed literals - GammaGain/EnhancerTolerance/
		// Severity01/MixedRg/MixedBy had drifted from the registry's
		// authoritative ranges (e.g. GammaGain clamped to [0.50,2.00] here
		// vs. [0.70,1.30] everywhere else; Severity01/RG/BY clamped to
		// [0.0,1.0] here vs. the registry's [0.0,1.25]) - a pasted preset
		// above 1.0 used to silently truncate on import even though the
		// same value was perfectly valid everywhere else in the app (found
		// in the 2026-09-09 codebase review). One clamp source now.
		ParameterRegistry& reg = ParameterRegistry::Get();
		int appliedCount = 0;
		int failedCount = 0;
		std::string item;
		while (std::getline(ss, item, ';'))
		{
			size_t eq = item.find('=');
			if (eq == std::string::npos) continue;
			std::string k = item.substr(0, eq);
			std::string v = item.substr(eq + 1);

			try
			{
				if (k == "T") {
					int t = std::stoi(v);
					if (t >= 0 && t <= 2) { Type = static_cast<BalanceType>(t); ++appliedCount; }
					else ++failedCount;
				} else if (k == "S") {
					const ParamMeta& m = reg.GetMeta(ParamId::Severity01);
					Severity01 = std::clamp(std::stof(v), m.minF, m.maxF);
					++appliedCount;
				} else if (k == "M") {
					Mixed = (std::stoi(v) != 0);
					++appliedCount;
				} else if (k == "RG") {
					const ParamMeta& m = reg.GetMeta(ParamId::MixedRgSeverity01);
					MixedRgSeverity01 = std::clamp(std::stof(v), m.minF, m.maxF);
					++appliedCount;
				} else if (k == "BY") {
					const ParamMeta& m = reg.GetMeta(ParamId::MixedBySeverity01);
					MixedBySeverity01 = std::clamp(std::stof(v), m.minF, m.maxF);
					++appliedCount;
				} else if (k == "G") {
					const ParamMeta& m = reg.GetMeta(ParamId::GammaGain);
					GammaGain = std::clamp(std::stof(v), m.minF, m.maxF);
					++appliedCount;
				} else if (k == "CT") {
					CommanderTagMode = (std::stoi(v) != 0) ? 1 : 0;
					++appliedCount;
				} else if (k == "SE") {
					SmartEnhancer = (std::stoi(v) != 0);
					++appliedCount;
				} else if (k == "ET") {
					const ParamMeta& m = reg.GetMeta(ParamId::EnhancerTolerance);
					EnhancerTolerance = std::clamp(std::stof(v), m.minF, m.maxF);
					++appliedCount;
				} else if (k == "H") {
					DiagnosisHint = v;
					++appliedCount;
				} else if (k == "EM") {
					EyeComfortModeEnabled = (std::stoi(v) != 0);
					++appliedCount;
				} else if (k == "BF") {
					const ParamMeta& m = reg.GetMeta(ParamId::BlueFilter01);
					BlueFilter01 = std::clamp(std::stof(v), m.minF, m.maxF);
					++appliedCount;
				} else if (k == "WT") {
					const ParamMeta& m = reg.GetMeta(ParamId::WarmTint01);
					WarmTint01 = std::clamp(std::stof(v), m.minF, m.maxF);
					++appliedCount;
				} else if (k == "SR") {
					const ParamMeta& m = reg.GetMeta(ParamId::SaturationReduction01);
					SaturationReduction01 = std::clamp(std::stof(v), m.minF, m.maxF);
					++appliedCount;
				}
				// Unrecognized keys are silently skipped for forward-compat
				// (a newer CBA's preset string pasted into an older build) -
				// not counted as a failure.
			}
			catch (...)
			{
				++failedCount;
			}
		}

		// Used to unconditionally return true as long as the CBA1: header
		// was present, even if every single field failed to parse (found in
		// the 2026-09-09 codebase review) - a corrupted clipboard paste
		// silently "succeeded" while changing nothing. Now fails outright if
		// nothing at all applied, and reports partial failures either way.
		if (appliedCount == 0)
		{
			if (aOutError) *aOutError = "No valid fields could be parsed from this preset string";
			return false;
		}
		if (failedCount > 0 && aOutError)
		{
			*aOutError = std::to_string(failedCount) + " field(s) failed to parse and were skipped";
		}
		return true;
	}
}
