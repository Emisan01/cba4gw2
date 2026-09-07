#pragma once
#include <string>
#include <vector>
#include "ColorMatrix.h"

namespace cba
{
	// Deliberately a flat key=value text file, not JSON — this addon has
	// six settings total, pulling in a JSON dependency for that would be
	// the kind of "simpler is better" violation Emi flagged earlier.
	struct Settings
	{
		bool Enabled = false;
		BalanceType Type = BalanceType::Protan;
		double Severity01 = 0.0;
		double MixedRgSeverity01 = 0.0;
		double MixedBySeverity01 = 0.0;
		bool Mixed = false;
		std::string ToggleKeybind = "CTRL+ALT+C"; // matches the exe's default
		int Language = 1; // 1 = English (default), 0 = System (Windows), 2 = German, 3 = Game (GW2)
		std::string DiagnosisHint = ""; // freeform AQ/HRR diagnostic label for presets
		bool EnableHybridMode = false; // toggles the experimental DXGI CPU readback layer
		bool DebugMode = false; // toggles the developer metrics UI

		// Commander Tag Enhancer
		int CommanderTagMode = 0; // 0=Off, 1=On
		bool SmartEnhancer = true; // true = auto conflict resolution based on CVD profile
		float EnhancerHue = 60.0f; // 0-360 degrees
		float EnhancerTolerance = 0.12f; // 0.04 - 0.20
		float GammaGain = 1.0f; // Eye comfort brightness scaling (0.70 - 1.30)

		struct EnhancerPreset
		{
			std::string Name = "";
			int Type = 0; // 0=Protan, 1=Deutan, 2=Tritan, 3=Mixed
			double Severity = 1.0;
			float Tolerance = 0.12f;
		};
		EnhancerPreset Presets[3]{
			{ "Com-Tag Profile 1", 0, 1.0, 0.12f },
			{ "Com-Tag Profile 2", 1, 0.8, 0.10f },
			{ "Com-Tag Profile 3", 2, 1.0, 0.14f }
		};

		float UiOpacity = 1.0f;
		int GraphMode = 0; // 0 = Polygonal (PWL), 1 = Harmonisch (Gauss/LMS), 2 = Strahlen (Ray Scope) - HUD / Detached
		int MainGraphMode = 0; // Independent Graph Mode for Main Window
		bool AutoBrightness = false; // When true, GammaGain follows recommended retention dynamically
		bool AlwaysDirectStart = true; // When true, skips Safe-Start gate unless an actual crash occurred
		bool CleanExit = true; // Set to 0 at runtime, set to 1 on graceful exit
		bool SafeModeTriggered = false; // Runtime flag: true if previous run crashed
		bool LoadOnStartup = false;
		bool ShowMainWindow = false;
		bool ShowGraphWindow = false;
		bool ShowLabWindow = false;
		bool ShowVisionLabWindow = false;
		bool DetachedWindow = false; // backward compatibility
		bool ShowQuickAccessIcon = true;
		bool SystemWide = false; // false = strictly GW2 window focus only (default), true = optionally extended to system on Alt-Tab

		// Free-Filter-Design (Selective Color Isolation & Shift with HSV Color Wheel / Artistic Color Selector, inspired by Krita-style wheel)
		bool FreeFilterEnabled = false;
		float FreeFilterTargetRgb[3] = { 0.25f, 0.62f, 0.30f };  // Default Ziel: GW2 Grün #3f9d4d
		float FreeFilterReplaceRgb[3] = { 0.85f, 0.28f, 0.24f }; // Default Ersatz: GW2 Rot #d9463c
		float FreeFilterToleranceTones = 3.0f; // ±3 Töne / Farbstufen (Default Schwellenwert)
		int ContrastPairIndex = 0; // 0=Blau/Grün, 1=Rot/Grün, 2=Gelb/Blau, 3=Cyan/Blau, 4=Orange/Rot

		// Filter-Labor & Experimentierfeld (Stackable custom filter instances with precision radius & diffusion)
		struct LabFilter
		{
			bool Enabled = true;
			std::string Name = "Filter 1";
			float TargetRgb[3] = { 0.25f, 0.62f, 0.30f };  // Ziel-Farbe
			float ReplaceRgb[3] = { 0.85f, 0.28f, 0.24f }; // Signal-/Ersatzfarbe
			int ToleranceTones = 3;                       // Begrenzungsradius (±1 bis ±32 Töne)
			float Diffusion = 0.35f;                      // Leichte Diffusion / Feathering (0.0 bis 1.0)
			int ActionType = 0;                           // 0=Signal-Farbe, 1=Auto-Komplementär, 2=Invertieren, 3=Luminanz-Boost
		};

		bool LabModeEnabled = false;
		int SelectedLabFilterIndex = 0;
		std::vector<LabFilter> LabFilters;

		// 3 User-Saved Color Correction Profiles
		struct ProfileSlot
		{
			bool Used = false;
			std::string Name = "";
			BalanceType Type = BalanceType::Protan;
			double Severity01 = 0.0;
			bool Mixed = false;
			double MixedRg01 = 0.0;
			double MixedBy01 = 0.0;
			float GammaGain = 1.0f;
		};
		ProfileSlot Slots[3]{};

		// aAddonDir: the path returned by GetAddonDirectory("cba"), Nexus
		// creates this automatically before AddonLoad.
		static Settings Load(const std::string& aAddonDir);
		void Save(const std::string& aAddonDir) const;
		static void MarkRunning(const std::string& aAddonDir);
		static void MarkCleanExit(const std::string& aAddonDir);

		// Preset export / import via compact ASCII string (Clipboard exchange)
		std::string ExportPresetString() const;
		bool ImportPresetString(const std::string& aPresetStr, std::string* aOutError = nullptr);
	};
}
