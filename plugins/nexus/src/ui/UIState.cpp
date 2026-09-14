#include "UIState.h"
#include "Theme.h"
#include "L10n.h"
#include "WindowMode.h"
#include "ShaderColorPipeline.h"
#include <imgui.h>
#include <cmath>
#include <cstdio>

namespace cba
{
	std::atomic<bool> s_resetMainWindowPos{false};
	std::atomic<bool> s_resetGraphWindowPos{false};
	std::atomic<bool> s_resetLabWindowPos{false};

	std::atomic<bool> s_focusMainWindow{false};
	std::atomic<bool> s_focusGraphWindow{false};
	std::atomic<bool> s_focusLabWindow{false};

	std::atomic<bool> s_showC64Credits{false};
	std::atomic<bool> s_deferredInitDone{false};
	bool s_showGraphOpacityDrawer{false};
	int s_activeSlotIdx{0}; // UI-only render thread, no atomic needed
	// Atomic, unlike s_activeSlotIdx above: written from Nexus's input
	// callback, read by the render thread and by Recompute() on the Watchdog.
	std::atomic<bool> s_compareHoldActive{false};

	std::atomic<HWND> s_gw2Hwnd{nullptr};
	std::atomic<bool> s_gw2Minimized{false};

	std::array<TagConflictState, 9> s_tagConflictStates{};

	double g_perfMainWindowMs = 0.0;
	double g_perfSensorGraphMs = 0.0;
	double g_perfCurvesMs = 0.0;
	double g_perfShaderPassMs = 0.0;
	double g_perfFilterLabMs = 0.0;
	double g_perfTotalImGuiMs = 0.0;

	// ── Live Status Dot & Filter State Indicator ─────────────────────────────
	void DrawFilterStatusIndicator(bool aWithText)
	{
		const L10n& t = Strings();
		bool isDe = cba::IsGerman(); // was a fragile first-letter check - see CLAUDE.md 2026-09-09

		// Determine current filter state
		bool isEnabled = CurrentSettings.Enabled;
		// Was the third hand-written copy of the gate condition (2026-09-12).
		// "Paused" here means specifically "suspended because GW2 is not in
		// front", which is why it still asks about minimized separately - the
		// minimized case has its own status text below.
		bool isMinimized = s_gw2Minimized.load() || (s_gw2Hwnd.load() && IsIconic(s_gw2Hwnd.load()));
		bool isPaused = (isEnabled && !isMinimized && !ShouldScreenEffectBeActive());

		ImU32 dotColor;
		ImU32 glowColor = 0;
		const char* statusText = "";
		const char* tooltipText = "";

		if (!isEnabled)
		{
			dotColor = Theme::kDotOffCol;
			statusText = isDe ? "Inaktiv" : "Inactive";
			tooltipText = isDe ? "CBA Status: Farbfilter ist ausgeschaltet (OFF).\nKlicke auf [ON], um den Filter zu aktivieren." 
			                   : "CBA Status: Color filter is OFF.\nClick [ON] to activate the filter.";
		}
		// The "Blockiert (Vollbild)" state is gone (2026-09-12). It described
		// the DWM backend, where Windows really did refuse over an exclusive
		// fullscreen swapchain. The shader backend writes into GW2's own
		// backbuffer regardless of window mode, so the dot would have been
		// reporting a blockage that was not happening.
		else if (isMinimized || isPaused)
		{
			dotColor = Theme::kDotWarnCol;
			statusText = isDe ? "Pausiert" : "Paused";
			tooltipText = isDe ? "CBA Status: GW2 ist im Hintergrund oder minimiert.\nFilter pausiert automatisch zum Schutz anderer Anwendungen.\n('Im Hintergrund aktiv lassen' fuer Dauerbetrieb)."
			                   : "CBA Status: GW2 is in background or minimized.\nFilter pauses automatically.\n('Keep active in background' to keep active).";
		}
		else
		{
			// Active & running: soft cyan-emerald pulse
			float time = (float)ImGui::GetTime();
			float pulse = 0.70f + 0.30f * std::sin(time * 3.5f);
			dotColor = Theme::kDotReadyCol;
			glowColor = IM_COL32(0, 210, 190, (int)(pulse * 90.0f));

			// Backend suffix used to be hardcoded "(DWM)" regardless of which
			// backend is actually running - wrong for the common case, since
			// Shader is the default (Settings.h, RenderBackend = 1). Same
			// mistake CLAUDE.md's diagnostics-copy fix already caught once
			// (MainWindowSystem.cpp, "Colour path" line) - this call site was
			// still making it. Same condition as the Backend: readout next to
			// the tab bar (MainWindow.cpp), so the two never disagree again.
			bool shaderActive = (CurrentSettings.RenderBackend == 1) && GetShaderColorPipeline().IsReady();
			statusText = shaderActive ? (isDe ? "Aktiv (Shader)" : "Active (Shader)")
			                          : (isDe ? "Aktiv (DWM)" : "Active (DWM)");
			tooltipText = shaderActive
				? (isDe ? "CBA Status: Farbfilter ist aktiv und an Guild Wars 2 gebunden.\nDer Shader-Pass laeuft direkt im Spielbild, ohne Windows-Systemkomponenten."
				        : "CBA Status: Color filter active and bound to Guild Wars 2.\nThe shader pass runs directly in the game frame, no Windows system components involved.")
				: (isDe ? "CBA Status: Farbfilter ist aktiv und an Guild Wars 2 gebunden.\nWindows Magnification DWM-Hardwarebeschleunigung laeuft stabil."
				        : "CBA Status: Color filter active and bound to Guild Wars 2.\nWindows Magnification DWM hardware acceleration active.");
		}

		ImVec2 p = ImGui::GetCursorScreenPos();
		float radius = 5.0f;
		float h = ImGui::GetTextLineHeight();
		ImVec2 center(p.x + radius + 2.0f, p.y + h * 0.5f);
		ImDrawList* dl = ImGui::GetWindowDrawList();

		if (glowColor != 0)
		{
			dl->AddCircleFilled(center, radius + 3.0f, glowColor);
		}
		dl->AddCircleFilled(center, radius, dotColor);
		dl->AddCircle(center, radius, IM_COL32(20, 30, 40, 200), 0, 1.0f);

		float itemW = radius * 2.0f + 4.0f;
		ImGui::Dummy(ImVec2(itemW, h));
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltipText);
		}

		if (aWithText)
		{
			ImGui::SameLine(0, 4.0f);
			ImGui::TextColored(ImColor(dotColor), "%s", statusText);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", tooltipText);
			}
		}
	}

	bool DrawTileHeader(const char* aTitle, bool& aCollapsed)
	{
		bool isDe = cba::IsGerman();
		if (ImGui::BeginMenuBar())
		{
			ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
			if (ImGui::SmallButton(aCollapsed ? "[+]##tile_arrow" : "[-]##tile_arrow"))
			{
				aCollapsed = !aCollapsed;
			}
			ImGui::PopStyleColor(3);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", aCollapsed ? (isDe ? "Aufklappen" : "Expand") : (isDe ? "Einklappen" : "Collapse"));
			}
			ImGui::SameLine();
			ImGui::TextColored(Theme::kTextCyanLicht, "%s", aTitle);
			ImGui::EndMenuBar();
		}
		return !aCollapsed;
	}

	bool DrawSubsectionHeader(const char* aTitle, bool& aCollapsed)
	{
		bool isDe = cba::IsGerman();
		// ID bug found 2026-09-14 (Emi's screenshot: the collapse arrows
		// "don't work as expected"): every call used the literal same ID
		// ("[+]##subsection_arrow"), fine for DrawTileHeader (one call per
		// distinct child window) but this is called three times in the
		// SAME window (the Dashboard's "Filter Control" tile) - all three
		// arrow buttons collided onto one ImGui ID, so clicks landed on
		// whichever one ImGui's ID stack resolved to that frame, not
		// necessarily the one actually clicked. aTitle is unique per call,
		// so folding it into the ID (not the visible label) fixes it.
		char arrowId[160];
		std::snprintf(arrowId, sizeof(arrowId), "%s##sub_arrow_%s", aCollapsed ? "[+]" : "[-]", aTitle);
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(1.0f, 1.0f, 1.0f, 0.08f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
		if (ImGui::SmallButton(arrowId))
		{
			aCollapsed = !aCollapsed;
		}
		ImGui::PopStyleColor(3);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", aCollapsed ? (isDe ? "Aufklappen" : "Expand") : (isDe ? "Einklappen" : "Collapse"));
		}
		ImGui::SameLine();
		ImGui::TextColored(Theme::kTextGoldLabel, "%s", aTitle);
		return !aCollapsed;
	}
}

