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
		DeficiencyType Type = DeficiencyType::Protan;
		double Severity01 = 0.0;
		double MixedRgSeverity01 = 0.0;
		double MixedBySeverity01 = 0.0;
		bool Mixed = false;
		std::string ToggleKeybind = "CTRL+ALT+C"; // matches the exe's default
		int Language = 0; // 0 = Auto (System), 1 = English, 2 = German
		std::string DiagnosisHint = ""; // freeform AQ/HRR diagnostic label for presets
		bool EnableHybridMode = false; // toggles the experimental DXGI CPU readback layer
		bool DebugMode = false; // toggles the developer metrics UI

		// Commander Tag Enhancer
		int CommanderTagMode = 0; // 0=Off, 1=On (Smart Enhancer)
		float EnhancerHue = 60.0f; // 0-360 degrees
		float EnhancerTolerance = 0.12f; // 0.04 - 0.20

		float UiOpacity = 1.0f;
		bool LoadOnStartup = false;
		bool DetachedWindow = false;
		bool SystemWide = false; // false = strictly GW2 window focus only (default), true = optionally extended to system on Alt-Tab

		// aAddonDir: the path returned by GetAddonDirectory("cba"), Nexus
		// creates this automatically before AddonLoad.
		static Settings Load(const std::string& aAddonDir);
		void Save(const std::string& aAddonDir) const;
	};
}
