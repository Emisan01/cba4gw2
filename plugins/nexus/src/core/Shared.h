#pragma once

// This pulls in the official Nexus addon API definitions (AddonAPI,
// AddonDefinition, ERenderType, ...). Get Nexus.h from:
// https://github.com/RaidcoreGG/RCGG-lib-nexus-api
// and drop it (plus its dependencies) into an include path.
#include "Nexus.h"
#include "Settings.h"
#include <string>
#include <cstdint>
#include <atomic>

namespace cba
{
	struct MumbleContext {
		unsigned char serverAddress[28];
		uint32_t mapId;
		uint32_t mapType;
		uint32_t shardId;
		uint32_t instance;
		uint32_t buildId;
		uint32_t uiState;
		uint16_t compassWidth;
		uint16_t compassHeight;
		float compassRotation;
		float playerX;
		float playerY;
		float mapCenterX;
		float mapCenterY;
		float mapScale;
		uint32_t processId;
		uint8_t mountIndex;
	};

	struct GW2MumbleLink {
		uint32_t uiVersion;
		uint32_t uiTick;
		float fAvatarPosition[3];
		float fAvatarFront[3];
		float fAvatarTop[3];
		wchar_t name[256];
		float fCameraPosition[3];
		float fCameraFront[3];
		float fCameraTop[3];
		wchar_t identity[256];
		uint32_t context_len;
		MumbleContext context;
		wchar_t description[2048];
	};

	struct MumbleGameContext
	{
		uint32_t mapId = 0;
		uint32_t mapType = 0;
		bool isInCombat = false;
		bool isWvW = false;
		bool isInstance = false;
		const char* modeNameEn = "Unknown";
		const char* modeNameDe = "Unbekannt";
	};

	MumbleGameContext GetCurrentGameContext();

	// The real, currently-running AddonDef.Version (Build/Revision carry the
	// hour/minute-second build stamp added 2026-09-09 specifically so a
	// diagnostic report can prove which compile is actually loaded) - added
	// because the "Copy System Diagnostics" report used to hardcode a stale
	// "1.0.2.0 (Build 2)" literal instead of reading this (found in the
	// 2026-09-09 codebase review).
	AddonVersion GetAddonVersion();
}

extern AddonAPI* APIDefs;
extern HMODULE   AddonModuleHandle;
extern cba::Settings CurrentSettings;
extern std::string   AddonDir;
extern NexusLinkData* NexusLink;
extern cba::GW2MumbleLink* MumbleLinkData;
extern bool g_DwmLastCallSuccessful;
// How many times the OS refused to switch the screen-wide colour effect OFF
// since this DLL was loaded (2026-09-12). Counted, not inferred: the Nexus
// log showed only "(Clear) REJECTED" lines and never an Apply rejection,
// which is what pointed at clears running from the wrong thread while GW2 was
// already in the background. A rejected clear is the one failure the user
// actually feels - the correction stays on their whole desktop after they
// alt-tab away - so it gets a number instead of a banner. SelfTest reports it;
// see "Where a fact belongs" in CLAUDE.md.
extern std::atomic<unsigned int> g_DwmClearRejectCount;
