#include "UIState.h"
#include "Theme.h"
#include "L10n.h"
#include "WindowMode.h"
#include <imgui.h>
#include <cmath>

namespace cba
{
	std::atomic<bool> s_resetMainWindowPos{false};
	std::atomic<bool> s_resetGraphWindowPos{false};
	std::atomic<bool> s_resetLabWindowPos{false};
	std::atomic<bool> s_resetVisionLabWindowPos{false};

	std::atomic<bool> s_focusMainWindow{false};
	std::atomic<bool> s_focusGraphWindow{false};
	std::atomic<bool> s_focusLabWindow{false};
	std::atomic<bool> s_focusVisionLabWindow{false};

	std::atomic<bool> s_safeStartPending{false};
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
	double g_perfFilterLabMs = 0.0;
	double g_perfVisionLabMs = 0.0;
	double g_perfSafeStartMs = 0.0;
	double g_perfTotalImGuiMs = 0.0;

	// ── Live Status Dot & Filter State Indicator ─────────────────────────────
	void DrawFilterStatusIndicator(bool aWithText)
	{
		const L10n& t = Strings();
		bool isDe = cba::IsGerman(); // was a fragile first-letter check - see CLAUDE.md 2026-09-09

		// Determine current filter state
		bool isEnabled = CurrentSettings.Enabled;
		WindowMode mode = DetectWindowMode(APIDefs ? static_cast<IDXGISwapChain*>(APIDefs->SwapChain) : nullptr);
		bool isExclusive = (mode == WindowMode::ExclusiveFullscreen);

		HWND fg = GetForegroundWindow();
		DWORD fgPid = 0;
		if (fg) GetWindowThreadProcessId(fg, &fgPid);
		bool isGw2Foreground = (fg && fgPid == GetCurrentProcessId());
		bool isMinimized = s_gw2Minimized.load() || (s_gw2Hwnd.load() && IsIconic(s_gw2Hwnd.load()));
		bool isPaused = (!isMinimized && !isGw2Foreground && !CurrentSettings.SystemWide);

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
		else if (isExclusive)
		{
			dotColor = Theme::kDotWarnCol;
			statusText = isDe ? "Blockiert (Vollbild)" : "Blocked (Fullscreen)";
			tooltipText = isDe ? "CBA Status: Windows DWM-Farbfilter wird durch exklusives Vollbild blockiert!\nBitte in GW2 Grafikoptionen auf 'Fenster-Vollbild' (Borderless) umschalten."
			                   : "CBA Status: Windows DWM filter blocked by exclusive fullscreen!\nPlease switch GW2 graphics to 'Windowed Fullscreen' (Borderless).";
		}
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
			statusText = isDe ? "Aktiv (DWM)" : "Active (DWM)";
			tooltipText = isDe ? "CBA Status: Farbfilter ist aktiv und an Guild Wars 2 gebunden.\nWindows Magnification DWM-Hardwarebeschleunigung laeuft stabil."
			                   : "CBA Status: Color filter active and bound to Guild Wars 2.\nWindows Magnification DWM hardware acceleration active.";
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
}

