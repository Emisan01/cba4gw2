#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ui/MainWindow.h"
#include "ui/SensorGraphHUD.h"
#include "ui/UIState.h"
#include "ui/Theme.h"
#include "ui/L10n.h"
#include "ui/ImGuiSafe.h"

#include "core/NexusEcosystem.h"
#include <imgui.h>
#include "ColorMatrix.h"
#include "ColorMath.h"
#include "FilterLab.h"
#include "CreditsDialog.h"
#include "HybridScanner.h"
#include "VisionLab.h"
#include "WindowMode.h"
#include "ColorEffectController.h"
#include "ParameterRegistry.h"
#include "FeatureModule.h"
#include "FilterLayers.h"
#include "ShaderColorPipeline.h"
#include "SelfTest.h"
#include "Shared.h"

#include <imgui.h>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cfloat>

#include "MainWindowTabs.h"

namespace cba
{
	void RenderEyeComfortTab(bool isDe, const L10n& t, bool& changed, bool& saveNeeded)
	{
		cba::ScopedChild tileEye("Tile_Eye", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar()) { ImGui::TextColored(Theme::kTextCyanLicht, "Eye Comfort Settings"); ImGui::EndMenuBar(); }

		static int s_lastHdrCheckFrameDet = -1;
		static bool s_cachedHdrDetectedDet = false;
		int curFrame = ImGui::GetFrameCount();
		if (curFrame != s_lastHdrCheckFrameDet + 1)
		{
			IDXGISwapChain* sc = APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr;
			s_cachedHdrDetectedDet = DetectHdrColorSpace(sc);
		}
		s_lastHdrCheckFrameDet = curFrame;

		ImVec2 dotPos = ImGui::GetCursorScreenPos();
		float dotRadius = 4.0f;
		ImU32 dotColor = s_cachedHdrDetectedDet ? Theme::kDotReadyCol : Theme::kDotOffCol;
		ImVec2 dotCenter(dotPos.x + dotRadius + 2.0f, dotPos.y + ImGui::GetTextLineHeight() * 0.5f);
		ImGui::GetWindowDrawList()->AddCircleFilled(dotCenter, dotRadius, dotColor);
		ImGui::Dummy(ImVec2(dotRadius * 2.0f + 4.0f, ImGui::GetTextLineHeight()));
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("HDR: %s\n(%s)", s_cachedHdrDetectedDet ? (isDe ? "Aktiv" : "Active") : (isDe ? "Inaktiv (SDR)" : "Inactive (SDR)"), t.EyeComfortHdrTooltip);
		}
		ImGui::SameLine(0, 6.0f);
		ImGui::TextDisabled("HDR: %s", s_cachedHdrDetectedDet ? (isDe ? "Erkannt" : "Detected") : (isDe ? "Aus (SDR)" : "Off (SDR)"));
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("HDR: %s\n(%s)", s_cachedHdrDetectedDet ? (isDe ? "Aktiv" : "Active") : (isDe ? "Inaktiv (SDR)" : "Inactive (SDR)"), t.EyeComfortHdrTooltip);
		}

		ImGui::Spacing();
		ImGui::TextUnformatted(isDe ? "Helligkeit (Eye Comfort Gamma):" : "Brightness (Eye Comfort Gamma):");
		float availGamma = ImGui::GetContentRegionAvail().x;
		float padXGamma = ImGui::GetStyle().FramePadding.x * 2.0f;
		float btnWGamma = ImGui::CalcTextSize("Reset").x + padXGamma + 8.0f;
		float spGamma = 6.0f;
		float sWGamma = (availGamma > (btnWGamma + spGamma + 60.0f)) ? (availGamma - btnWGamma - spGamma) : 180.0f;

		ImGui::SetNextItemWidth(sWGamma);
		// Registry-backed (CLAUDE.md, Registry/Control Layer step 1) - same
		// pattern as the Tolerance slider pilot: clamp lives in ParamMeta.
		{
			float gain = ParameterRegistry::Get().GetFloat(ParamId::GammaGain);
			if (ImGui::SliderFloat("##EyeComfortGammaSlider", &gain, 0.70f, 1.30f, "%.2fx", ImGuiSliderFlags_AlwaysClamp))
			{
				SetGammaGainManual(gain);
				changed = true;
			}
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			saveNeeded = true;
		}
		ImGui::SameLine(0, spGamma);
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
		if (ImGui::Button("Reset##gamma_main", ImVec2(btnWGamma, 0.0f)))
		{
			SetGammaGainManual(1.0f);
			changed = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		if (ImGui::IsItemHovered()) ImGui::SetTooltip(isDe ? "Helligkeit auf 1.00x zuruecksetzen" : "Reset brightness to 1.00x");

		SyncAutoBrightnessGain(changed, saveNeeded);
		BrightnessRetentionResult retention = GetBrightnessRetention();

		ImGui::Spacing();
		ImGui::Text(t.EyeComfortRetention, retention.retentionRatio * 100.0f, retention.recommendedGain);
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnMittelwertIdle);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnMittelwertHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnMittelwertActive);
		ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextCyanLicht);
		if (ImGui::Button(t.EyeComfortApply, ImVec2(0.0f, 24.0f)))
		{
			ApplyAutoBrightnessGain();
			changed = true;
			saveNeeded = true;
		}
		ImGui::PopStyleColor(4);
		ImGui::SameLine(0, 12.0f);
		if (ImGui::Checkbox(isDe ? "Auto-Helligkeit##main_auto" : "Auto-Brightness##main_auto", &CurrentSettings.AutoBrightness))
		{
			if (CurrentSettings.AutoBrightness)
			{
				ApplyAutoBrightnessGain();
				changed = true;
			}
			saveNeeded = true;
		}

		// Eye-Sensitive Mode (2026-09-09) - its own module within Eye
		// Comfort, independent of the Gamma/Auto-Brightness pair above
		// (which stays untouched, Emi: "funktioniert einwandfrei").
		// Composes with CVD correction in Recompute(), never replaces it.
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		if (ImGui::Checkbox(isDe ? "Eye-Sensitive Mode aktivieren" : "Activate Eye-Sensitive Mode", &CurrentSettings.EyeComfortModeEnabled))
		{
			changed = true;
			saveNeeded = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isDe
				? "Eigenstaendiges Layer fuer Blaufilter, Warmton und Saettigungsreduktion - unabhaengig von der Farbkorrektur, wird zusaetzlich angewendet."
				: "Independent layer for blue-light filter, warm tint and saturation reduction - separate from color correction, applied on top of it.");
		}

		if (CurrentSettings.EyeComfortModeEnabled)
		{
			ImGui::Indent(16.0f);

			// ImGui's SliderFloat format string does not auto-scale - a
			// 0.0-1.0 range with "%.0f%%" just printed the raw fraction
			// with a % sign glued on, so the label only ever read "0%"
			// or "1%" across the whole range (found in the 2026-09-09
			// codebase review). Fix: widget operates in 0-100 display
			// units, converted to/from the registry's 0.0-1.0 storage
			// range right at the boundary - same trick the "Strength"
			// display text a few lines up already uses (manual *100.0).
			// Three near-identical hand-copied blocks until 2026-09-12,
			// which is how they came to differ from the Nexus panel's own
			// copy of the same three sliders (that one had no Reset at
			// all). One lambda now, Reset included - same shape as the
			// panel's embEyeSlider and the Sensor Graph window's
			// severitySlider.
			auto eyeSlider = [&](const char* aLabel, const char* aId, ParamId aParam) {
				ImGui::TextUnformatted(aLabel);

				float avail = ImGui::GetContentRegionAvail().x;
				float padX  = ImGui::GetStyle().FramePadding.x * 2.0f;
				float btnW  = ImGui::CalcTextSize("Reset").x + padX + 8.0f;
				const float sp = 6.0f;
				float sW = (avail > (btnW + sp + 60.0f)) ? (avail - btnW - sp) : 180.0f;

				ImGui::SetNextItemWidth(sW);
				float v = ParameterRegistry::Get().GetFloat(aParam) * 100.0f;
				if (ImGui::SliderFloat(aId, &v, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
				{
					ParameterRegistry::Get().SetFloat(aParam, v / 100.0f);
					changed = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) saveNeeded = true;

				ImGui::SameLine(0, sp);
				char eyeResetId[64];
				std::snprintf(eyeResetId, sizeof(eyeResetId), "Reset%s", aId);
				ImGui::PushStyleColor(ImGuiCol_Button,        Theme::kBtnNeutralIdle);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::kBtnNeutralHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::kBtnNeutralPress);
				ImGui::PushStyleColor(ImGuiCol_Text,          Theme::kTextSecondary);
				if (ImGui::Button(eyeResetId, ImVec2(btnW, 0.0f)))
				{
					ParameterRegistry::Get().SetFloat(aParam, 0.0f);
					changed = true;
					saveNeeded = true;
				}
				ImGui::PopStyleColor(4);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", isDe ? "Wert auf 0% zuruecksetzen" : "Reset value to 0%");
			};

			eyeSlider(isDe ? "Blaufilter:" : "Blue Light Filter:", "##blue_filter_slider", ParamId::BlueFilter01);
			eyeSlider(isDe ? "Warmton:" : "Warm Tint:", "##warm_tint_slider", ParamId::WarmTint01);
			eyeSlider(isDe ? "Saettigungsreduktion:" : "Saturation Reduction:", "##sat_reduction_slider", ParamId::SaturationReduction01);

			ImGui::Unindent(16.0f);
		}

		
	}
}
