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
		BalanceType Type = BalanceType::Deutan;
		// float, not double: these are UI slider values in [0, 1.25] shown at
		// 1-3 decimal places, nothing here needs double precision. Matches
		// EnhancerTolerance/GammaGain below and lets the Registry (which only
		// speaks Float/Bool/Int) point at them directly instead of needing a
		// fourth Double kind bolted on just for these three fields.
		float Severity01 = 0.0f;
		float MixedRgSeverity01 = 0.0f;
		float MixedBySeverity01 = 0.0f;
		bool Mixed = false;
		int Language = 1; // 1 = English (default), 0 = System (Windows), 2 = German, 3 = Game (GW2)
		std::string DiagnosisHint = ""; // freeform AQ/HRR diagnostic label for presets
		bool EnableHybridMode = false; // toggles the experimental DXGI CPU readback layer
		bool DebugMode = false; // toggles the developer metrics UI

		// Advanced/experimental visual theme - 0 = Classic (the only look
		// this addon has ever had, stays the default so existing users see
		// zero change), 1 = Symbiont (bio-clinical HUD palette, opt-in only -
		// see Theme.h). Deliberately palette-only, not fonts or panel shapes -
		// Emi's call 2026-09-09 ("zuviel UI gedoens koennte nach hinten
		// losgehen"): keep the experimental look low-risk and fully reversible.
		int UiTheme = 0;

		// Advanced Mode gate (2026-09-09, Emi: "machen wir die Nexus main zu
		// unserer Basis wieder, und erst wenn dort Advanced Mode aktiviert
		// wird gibt es das ganze Spektrum frei"). false = the Nexus-embedded
		// panel IS the app: Master toggle, Profiles, Commander Tag, swatches -
		// the "Open Studio" entry point is locked. true = unlocks it, giving
		// access to the Main Window and its satellite windows (Sensor Graph,
		// Filter Lab, Vision Lab). Defaults false for new installs; existing
		// settings.ini files without this key also load false (safeStoi
		// default) - anyone with Studio windows already open right now just
		// needs to flip this once to keep using them next launch.
		bool AdvancedModeUnlocked = false;

		// Eye-Sensitive Mode (2026-09-09, Emi: "ein vollstaendiges logisches
		// Layer, ein eigenes Modul das nur den Augenschon-Modus im Fokus
		// hat"). Independent of CVD correction entirely - composes with it
		// (see ColorMatrix::EyeComfortMatrix, applied in Recompute()) rather
		// than replacing anything. Deliberately NOT touching the existing
		// GammaGain/AutoBrightness pair (Emi: that one "funktioniert
		// einwandfrei," don't rebuild it) or Hybrid Mode's Kinematic Fader
		// (Emi: separate, neglected area, revisit later "wenn wir einmal
		// durchs ganze Tool durch sind").
		bool EyeComfortModeEnabled = false;
		float BlueFilter01 = 0.0f;          // 0 = off, 1 = max blue reduction
		float WarmTint01 = 0.0f;            // 0 = off, 1 = max warm shift
		float SaturationReduction01 = 0.0f; // 0 = off, 1 = full greyscale

		// Commander Tag Enhancer
		int CommanderTagMode = 0; // 0=Off, 1=On
		bool SmartEnhancer = true; // true = auto conflict resolution based on CVD profile
		float EnhancerTolerance = 0.12f; // 0.04 - 0.20
		// Filter Layer Matrix (2026-09-11) - explicit, user-visible priority
		// instead of hidden "automation overrides manual" logic. Lower value
		// = higher priority = wins first for a pixel that multiple layers'
		// targets could match. Default 0 puts Commander Tag Auto-Contrast
		// ahead of Filter Lab instances (which default to their own vector
		// index, see LabFilter::LayerPriority below) unless reordered.
		int CommanderTagLayerPriority = 0;
		float GammaGain = 1.0f; // Eye comfort brightness scaling (0.70 - 1.30)

		float UiOpacity = 1.0f;
		int GraphMode = 0; // 0 = Polygonal (PWL), 1 = Harmonisch (Gauss/LMS), 2 = Strahlen (Ray Scope) - HUD / Detached
		int MainGraphMode = 2; // Independent Graph Mode for Main Window (0=Polygonal, 1=Harmonic, 2=Rays - default per Emi)
		bool AutoBrightness = false; // When true, GammaGain follows recommended retention dynamically
		bool AlwaysDirectStart = true; // When true, skips Safe-Start gate
		bool CleanExit = true; // Set to 0 at runtime, set to 1 on graceful exit
		bool SafeModeTriggered = false; // Runtime flag: true if previous run crashed
		bool ShowMainWindow = false;
		bool ShowGraphWindow = false;
		bool ShowLabWindow = false;
		bool ShowVisionLabWindow = false;
		bool ShowQuickAccessIcon = true;
		bool MovableToolbarIcon = true;
		float ToolbarIconPosX = 405.0f;
		float ToolbarIconPosY = 8.0f;
		bool SystemWide = false; // false = strictly GW2 window focus only (default), true = optionally extended to system on Alt-Tab

		int ContrastPairIndex = 0; // 0=Blau/Grün, 1=Rot/Grün, 2=Gelb/Blau, 3=Cyan/Blau, 4=Orange/Rot
		// Optional stylized "glass" look for the Contrast Test Swatches
		// cards (2026-09-11, Emi's ask) - translucency + a soft top-edge
		// highlight, NOT a real backdrop blur (CBA doesn't hook D3D11 to
		// sample/blur what's behind the panel, by design - AGENTS.md §4).
		bool GlassContrastCards = false;

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
			int LayerPriority = 1;                        // Filter Layer Matrix rank - see CommanderTagLayerPriority above
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
		ProfileSlot Slots[3]{
			{ false, "", BalanceType::Deutan, 0.0, false, 0.0, 0.0, 1.0f },
			{ false, "", BalanceType::Deutan, 0.0, false, 0.0, 0.0, 1.0f },
			{ false, "", BalanceType::Deutan, 0.0, false, 0.0, 0.0, 1.0f }
		};
		// Which Slots[] index to auto-load and auto-enable at startup, -1 = none
		// (the default: always start neutral/off, see AddonLoad in ModuleMain.cpp).
		// Replaces the old "LoadOnStartup" bool, which just remembered whatever
		// Enabled happened to be at last save with no way to pick a specific
		// profile - this ties startup persistence to an actual named profile
		// instead, and is skipped entirely on crash recovery (SafeModeTriggered).
		int AutoStartSlot = -1;

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
