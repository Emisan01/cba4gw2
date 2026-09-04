#pragma once
#include <string>
#include "ColorMatrix.h"

namespace cba
{
	// Deliberately a flat key=value text file, not JSON — this addon has
	// six settings total, pulling in a JSON dependency for that would be
	// the kind of "simpler is better" violation Emi flagged earlier.
	struct Settings
	{
		bool Enabled = false;
		DeficiencyType Type = DeficiencyType::Deutan;
		double Severity01 = 0.5;       // used for Protan/Deutan/Tritan
		double MixedRgSeverity01 = 0.5; // used for Mixed
		double MixedBySeverity01 = 0.5; // used for Mixed
		bool Mixed = false;
		std::string ToggleKeybind = "CTRL+ALT+C"; // matches the exe's default
		std::string Language = "de"; // "de" or "en"; keep German as the default base
		std::string DiagnosisHint = ""; // freeform AQ/HRR diagnostic label for presets

		// aAddonDir: the path returned by GetAddonDirectory("cba"), Nexus
		// creates it for us the first time we ask for it.
		static Settings Load(const std::string& aAddonDir);
		void Save(const std::string& aAddonDir) const;
	};
}
