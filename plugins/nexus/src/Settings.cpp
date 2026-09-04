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

		std::string line;
		while (std::getline(file, line))
		{
			auto sep = line.find('=');
			if (sep == std::string::npos) continue;

			std::string key = line.substr(0, sep);
			std::string value = line.substr(sep + 1);

			if (key == "Enabled")            s.Enabled = (value == "1");
			else if (key == "Type")          s.Type = ParseType(value);
			else if (key == "Severity")      s.Severity01 = std::stod(value);
			else if (key == "MixedRg")       s.MixedRgSeverity01 = std::stod(value);
			else if (key == "MixedBy")       s.MixedBySeverity01 = std::stod(value);
			else if (key == "Mixed")         s.Mixed = (value == "1");
			else if (key == "ToggleKeybind") s.ToggleKeybind = value;
			else if (key == "Language")      s.Language = value;
			else if (key == "DiagnosisHint") s.DiagnosisHint = value;
		}

		return s;
	}

	void Settings::Save(const std::string& aAddonDir) const
	{
		std::ofstream file(ConfigPath(aAddonDir), std::ios::trunc);
		if (!file.is_open())
			return; // non-fatal — worst case, defaults again on next load

		file << "Enabled=" << (Enabled ? "1" : "0") << "\n";
		file << "Type=" << TypeName(Type) << "\n";
		file << "Severity=" << Severity01 << "\n";
		file << "MixedRg=" << MixedRgSeverity01 << "\n";
		file << "MixedBy=" << MixedBySeverity01 << "\n";
		file << "Mixed=" << (Mixed ? "1" : "0") << "\n";
		file << "ToggleKeybind=" << ToggleKeybind << "\n";
		file << "Language=" << Language << "\n";
		file << "DiagnosisHint=" << DiagnosisHint << "\n";
	}
}
