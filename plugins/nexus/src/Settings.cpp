#include "Settings.h"
#include <fstream>
#include <sstream>

namespace cba
{
	namespace
	{
		std::string ConfigPath(const std::string& aAddonDir)
		{
			return aAddonDir + "\\settings.cfg";
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

		std::ifstream file(ConfigPath(aAddonDir));
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

			if (key == "Enabled")            s.Enabled = (value == "1");
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
			else if (key == "EnhancerTol")       s.EnhancerTolerance = safeStof(value, 0.12f);
			else if (key == "UiOpacity")         s.UiOpacity = safeStof(value, 1.0f);
			else if (key == "LoadOnStartup")     s.LoadOnStartup = (value == "1");
			else if (key == "DetachedWindow")    s.DetachedWindow = (value == "1");
			else if (key == "SystemWide")        s.SystemWide = (value == "1");
		}

		return s;
	}

	void Settings::Save(const std::string& aAddonDir) const
	{
		std::ofstream file(ConfigPath(aAddonDir), std::ios::trunc);
		if (!file.is_open())
			return;

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
		file << "UiOpacity=" << UiOpacity << "\n";
		file << "LoadOnStartup=" << (LoadOnStartup ? "1" : "0") << "\n";
		file << "DetachedWindow=" << (DetachedWindow ? "1" : "0") << "\n";
		file << "SystemWide=" << (SystemWide ? "1" : "0") << "\n";
	}
}
