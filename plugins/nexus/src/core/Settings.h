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
		// SmartEnhancer removed 2026-09-11. It used to switch between a static
		// per-CVD-type conflict table and the real severity-aware computation;
		// the selectivity fix made both paths use the real one, leaving the
		// field with no behavioural reader at all. It was kept as inert legacy
		// storage for one day because removing it also meant editing L10n.h's
		// positional language blocks - that edit is now verified mechanically
		// by the audit ("L10n struct fields and de/en blocks stay positionally
		// aligned"), so the reason to keep it went away.
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
		// Which delivery path paints the correction (v2-shader-core branch,
		// 2026-09-12). 0 = DWM/Magnification, screen-wide, what has shipped
		// so far. 1 = a pixel shader on GW2's own backbuffer, which touches
		// nothing outside the game. Both are built; this picks one, so the
		// two can be compared live instead of argued about.
		int RenderBackend = 1;
		// Where Auto-Brightness gets its answer from (2026-09-12).
		// 0 = the matrix prediction from nine reference tag colours, which
		// is all that was ever possible under DWM. 1 = the sensor, i.e. the
		// luminance actually measured before and after the correction on the
		// real frame. Default stays 0: the prediction works everywhere, the
		// measurement needs the shader backend and a valid reading.
		// 0 = matrix prediction (works everywhere, the only option under DWM)
		// 1 = sensor, filter-neutral: hold the CORRECTION to zero brightness
		//     cost. Scene-independent, because it steers on a ratio.
		// 2 = sensor, hold a level: steer the measured brightness towards a
		//     remembered target. This is auto-exposure, and it WILL fight the
		//     game's own lighting - a cave and a desert are supposed to differ.
		//     Bounded only by GammaGain's own 0.70-1.30 clamp.
		int AutoBrightnessSource = 0;
		// The level mode 2 steers towards, captured from a measurement.
		// 0 means "not captured yet" - mode 2 does nothing until it is.
		float SensorBrightnessTarget = 0.0f;
		bool SystemWide = false; // false = strictly GW2 window focus only (default), true = optionally extended to system on Alt-Tab

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
			int LayerPriority = 1;                        // Filter Layer Matrix rank - see CommanderTagLayerPriority above
		};

		bool LabModeEnabled = false;
		int SelectedLabFilterIndex = 0;
		std::vector<LabFilter> LabFilters;

		// 3 User-Saved Color Correction Profiles
		// ── A saved filter state, complete ──────────────────────────────
		//
		// Emi's definition (2026-09-12): "alle Filterzustaende wie sie eben
		// gerade sind, reiner IST-Zustand, unabhaengig von aller anderer
		// Logik" - everything that changes the picture, nothing that
		// describes the window it is shown in.
		//
		// It held six fields until then, while the shareable preset code
		// carried thirteen. Saving a profile locally silently dropped Eye
		// Comfort and Commander Tag settings that sharing the same profile
		// as a code preserved. Three different writers each wrote the subset
		// they happened to care about; one of them (the Com-Tag quick-save)
		// wrote Mixed = false and never wrote the mixed severities at all,
		// because in ITS worldview a Com-Tag profile is always one pure
		// type - deliberate there, data loss here, because both wrote into
		// this same struct.
		//
		// What is deliberately NOT here, and why, is in
		// docs/PROFILE_SLOT_SPEC.md. The short version: a slot stores what
		// the filter LOOKS LIKE, never whether it is running (Enabled has
		// exactly one authority and the neutral start is audit-pinned),
		// never how it is painted (RenderBackend is a per-machine choice),
		// never where it applies (SystemWide could tint a browser), and not
		// yet the Filter Lab stack (its data model is being rebuilt - see
		// ROADMAP phase 4 - and persisting today's shape would create the
		// migration this rewrite exists to avoid).
		struct ProfileSlot
		{
			bool Used = false;
			std::string Name = "";

			// Base correction
			BalanceType Type = BalanceType::Protan;
			float Severity01 = 0.0f;
			bool Mixed = false;
			float MixedRg01 = 0.0f;
			float MixedBy01 = 0.0f;

			// Brightness. The gain is stored; AutoBrightness deliberately is
			// not - SyncAutoBrightnessGain overwrites the gain from the
			// recommendation within a frame, so a slot storing both would
			// advertise a value it never applies. Loading sets the gain and
			// turns the automatic off, which is what a manual act does
			// everywhere else in this codebase.
			float GammaGain = 1.0f;

			// Eye-Sensitive layer
			bool EyeComfortModeEnabled = false;
			float BlueFilter01 = 0.0f;
			float WarmTint01 = 0.0f;
			float SaturationReduction01 = 0.0f;

			// Commander Tag contrast
			int CommanderTagMode = 0;
			float EnhancerTolerance = 0.12f;

			// The overlay layer gate. Non-obvious but load-bearing: the
			// scanner and its overlay are gated on
			// (EnableHybridMode || CommanderTagMode != 0), so with Com-Tag
			// off this flag is the only thing that lets target replacement
			// reach the screen at all.
			bool EnableHybridMode = false;
		};
		ProfileSlot Slots[3]{ {}, {}, {} };
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
