#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CreditsDialog.h"
#include "UIState.h"
#include "L10n.h"

#include <imgui.h>
#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace cba
{
	static std::atomic<bool> s_c64SoundEnabled{true};
	static std::atomic<bool> s_c64AudioRunning{false};

	void StartC64Audio()
	{
		if (s_c64AudioRunning.load()) return;
		s_c64AudioRunning.store(true);
		std::thread th([]() {
			// Classic 8-bit chiptune melody (C major / G / Am / F arpeggios)
			const struct Note { DWORD freq; DWORD dur; } kTrack[] = {
				{ 523, 85 }, { 659, 85 }, { 784, 85 }, { 1046, 110 }, { 784, 80 }, { 659, 80 },
				{ 587, 85 }, { 698, 85 }, { 880, 85 }, { 1175, 110 }, { 880, 80 }, { 698, 80 },
				{ 440, 85 }, { 523, 85 }, { 659, 85 }, { 880,  110 }, { 659, 80 }, { 523, 80 },
				{ 349, 85 }, { 440, 85 }, { 523, 85 }, { 698,  110 }, { 523, 80 }, { 440, 80 },
				{ 392, 95 }, { 494, 95 }, { 587, 95 }, { 784,  140 }, { 587, 80 }, { 494, 80 }
			};
			const size_t kTrackLen = sizeof(kTrack) / sizeof(kTrack[0]);
			size_t noteIdx = 0;
			while (s_c64AudioRunning.load() && s_showC64Credits.load())
			{
				if (s_c64SoundEnabled.load())
				{
					Beep(kTrack[noteIdx].freq, kTrack[noteIdx].dur);
					noteIdx = (noteIdx + 1) % kTrackLen;
					std::this_thread::sleep_for(std::chrono::milliseconds(15));
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}
			s_c64AudioRunning.store(false);
		});
		th.detach();
	}

	void StopC64Audio()
	{
		s_c64AudioRunning.store(false);
	}

	void RenderC64CreditsOverlay()
	{
		if (!s_showC64Credits.load() || !ImGui::GetCurrentContext()) return;

		ImGuiIO& io = ImGui::GetIO();
		ImVec2 disp = io.DisplaySize;
		float winW = std::clamp(disp.x * 0.75f, 480.0f, 680.0f);
		float winH = std::clamp(disp.y * 0.80f, 420.0f, 540.0f);
		float posX = (disp.x - winW) * 0.5f;
		float posY = (disp.y - winH) * 0.5f;

		ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

		// Commodore 64 Palette
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.24f, 0.98f));
		ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(0.40f, 0.35f, 0.85f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_Text,     ImVec4(0.65f, 0.62f, 1.00f, 1.00f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

		bool open = true;
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		if (ImGui::Begin("**** COMMODORE 64 BASIC V2 - CBA4GW2 CREDITS ****", &open, flags))
		{
			if (!open || ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				s_showC64Credits.store(false);
				StopC64Audio();
			}

			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImVec2 p0 = ImGui::GetWindowPos();
			ImVec2 p1(p0.x + winW, p0.y + winH);

			// Animated C64 Copper Raster Bars effect across top and bottom
			static float s_time = 0.0f;
			s_time += io.DeltaTime;
			for (int r = 0; r < 4; ++r)
			{
				float tOffset = s_time * 2.5f + r * 0.4f;
				float colR = 0.5f + 0.45f * std::sin(tOffset);
				float colG = 0.5f + 0.45f * std::sin(tOffset + 2.094f);
				float colB = 0.5f + 0.45f * std::sin(tOffset + 4.188f);
				ImU32 barCol = IM_COL32((int)(colR * 255), (int)(colG * 255), (int)(colB * 255), 180);
				float barYTop = p0.y + 26.0f + r * 3.0f;
				dl->AddLine(ImVec2(p0.x + 4.0f, barYTop), ImVec2(p1.x - 4.0f, barYTop), barCol, 2.0f);
				float barYBot = p1.y - 42.0f + r * 3.0f;
				dl->AddLine(ImVec2(p0.x + 4.0f, barYBot), ImVec2(p1.x - 4.0f, barYBot), barCol, 2.0f);
			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "    **** COMMODORE 64 BASIC V2 ****");
			ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), " 64K RAM SYSTEM  38911 BASIC BYTES FREE");
			ImGui::Spacing();
			ImGui::Text("READY.");
			ImGui::Text("LOAD \"CBA4GW2\",8,1");
			ImGui::Text("SEARCHING FOR CBA4GW2... FOUND.");
			ImGui::Text("LOADING... READY.");
			ImGui::Text("RUN");
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Endless Auto-Scrolling Credits Container
			float scrollAreaH = winH - 240.0f;
			if (ImGui::BeginChild("##c64_scroller", ImVec2(-1.0f, scrollAreaH), true, ImGuiWindowFlags_NoScrollbar))
			{
				static float s_scrollPos = 0.0f;
				s_scrollPos += io.DeltaTime * 32.0f; // Scroll speed (32 px/sec)

				// Loop seamlessly
				float maxScroll = 680.0f;
				if (s_scrollPos > maxScroll)
				{
					s_scrollPos = 0.0f;
				}

				ImGui::SetScrollY(s_scrollPos);

				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "========================================================");
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "        COLOR BALANCE ASSIST FOR GUILD WARS 2           ");
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "                    CBA4GW2 v1.0.2                      ");
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "========================================================");
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(0.35f, 0.95f, 0.75f, 1.0f), "     \"FIGHT THE ELDER DRAGONS, NOT YOUR MONITOR!\"     ");
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::TextColored(ImVec4(1.0f, 0.60f, 0.80f, 1.0f), "[ CONCEPT, VISION & SYSTEM ]");
				ImGui::Text("  Emisan01 & the Guild Wars 2 Accessibility Initiative");
				ImGui::Spacing();

				ImGui::TextColored(ImVec4(1.0f, 0.60f, 0.80f, 1.0f), "[ MATHEMATICAL COLOR MODELS & RESEARCH ]");
				ImGui::Text("  - LMS-Dichromacy & Daltonization: Fidaner et al. (2005)");
				ImGui::Text("  - Computerized Dichromat Simulation: Vienot, Brettel & Mollon (1999)");
				ImGui::Text("  - Hunt-Pointer-Estevez (HPE) Cone Transformation");
				ImGui::Text("  - W3C Web Content Accessibility Guidelines (WCAG 2.1)");
				ImGui::Spacing();

				ImGui::TextColored(ImVec4(1.0f, 0.60f, 0.80f, 1.0f), "[ PLATFORM, ENGINES & COMMUNITY HEROES ]");
				ImGui::Text("  - ArenaNet: For creating Guild Wars 2 and 12+ years of Tyrian joy!");
				ImGui::Text("  - Nightmoore & Raidcore: For the incredible Nexus Addon Engine");
				ImGui::Text("  - DeltaConnected & ArcDPS Team: For pioneering GW2 modding");
				ImGui::Text("  - Omar Cornut: For the Dear ImGui interface library");
				ImGui::Spacing();

				ImGui::TextColored(ImVec4(1.0f, 0.60f, 0.80f, 1.0f), "[ GREETINGS & DANKSAGUNG TO ALL PLAYERS ]");
				ImGui::Text("  * To all Commanders who lead epic Zergs through WvW!");
				ImGui::Text("  * To all Raiders who master Dhuum, Qadim, Samarog & Cerus!");
				ImGui::Text("  * To all Fractal runners and Strike Mission squads!");
				ImGui::Text("  * And to every player who values clear contrast and balance:");
				ImGui::TextColored(ImVec4(0.40f, 1.00f, 0.50f, 1.0f), "    May your greens and reds never blend,");
				ImGui::TextColored(ImVec4(0.40f, 1.00f, 0.50f, 1.0f), "    may commander tags shine bright in every zerg,");
				ImGui::TextColored(ImVec4(0.40f, 1.00f, 0.50f, 1.0f), "    and may your drops forever be Precursor Gold!");
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "+++ ENDLESS RETRO CREDITS LOOPING... THANK YOU ALL! +++");
				ImGui::Spacing();
				ImGui::Spacing();
			}
			ImGui::EndChild();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Bottom Buttons (Sound Toggle & Close)
			bool isDe = (Strings().Enabled[0] == 'A');
			bool sound = s_c64SoundEnabled.load();
			if (sound)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.52f, 0.28f, 0.90f));
				if (ImGui::Button(isDe ? "[ 8-BIT AUDIO: AN ]" : "[ 8-BIT AUDIO: ON ]", ImVec2(0.0f, 26.0f)))
				{
					s_c64SoundEnabled.store(false);
				}
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.38f, 0.90f));
				if (ImGui::Button(isDe ? "[ 8-BIT AUDIO: AUS ]" : "[ 8-BIT AUDIO: OFF ]", ImVec2(0.0f, 26.0f)))
				{
					s_c64SoundEnabled.store(true);
					StartC64Audio();
				}
				ImGui::PopStyleColor();
			}

			ImGui::SameLine(0, 16.0f);
			if (ImGui::Button(isDe ? "Zurueck zum Spiel (ESC)" : "Return to Game (ESC)", ImVec2(0.0f, 26.0f)))
			{
				s_showC64Credits.store(false);
				StopC64Audio();
			}
		}
		ImGui::End();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
	}
}
