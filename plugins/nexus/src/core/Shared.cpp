#include "Shared.h"

AddonAPI* APIDefs = nullptr;
HMODULE   AddonModuleHandle = nullptr;
cba::Settings CurrentSettings{};
std::string   AddonDir;
NexusLinkData* NexusLink = nullptr;
cba::GW2MumbleLink* MumbleLinkData = nullptr;

namespace cba
{
	MumbleGameContext GetCurrentGameContext()
	{
		MumbleGameContext ctx{};
		if (MumbleLinkData)
		{
			ctx.mapId = MumbleLinkData->context.mapId;
			ctx.mapType = MumbleLinkData->context.mapType;
			ctx.isInCombat = (MumbleLinkData->context.uiState & 0x02) != 0;
			ctx.isWvW = (ctx.mapType == 10 || ctx.mapType == 11);
			ctx.isInstance = (ctx.mapType == 5);

			switch (ctx.mapType)
			{
				case 1:  ctx.modeNameEn = "Character Creation"; ctx.modeNameDe = "Charaktererstellung"; break;
				case 2:  ctx.modeNameEn = "Competitive PvP";     ctx.modeNameDe = "PvP (Gewertet)"; break;
				case 3:  ctx.modeNameEn = "GvG";                 ctx.modeNameDe = "Gildenkampf (GvG)"; break;
				case 5:  ctx.modeNameEn = "Instance (Raid/Frac)";ctx.modeNameDe = "Instanz (Raid/Fraktal)"; break;
				case 6:  ctx.modeNameEn = "Open World";          ctx.modeNameDe = "Offene Welt"; break;
				case 10: ctx.modeNameEn = "World vs World";      ctx.modeNameDe = "Welt gegen Welt (WvW)"; break;
				case 11: ctx.modeNameEn = "WvW Lounge";          ctx.modeNameDe = "WvW Lounge"; break;
				default: ctx.modeNameEn = "GW2 Map";             ctx.modeNameDe = "GW2 Karte"; break;
			}
		}
		return ctx;
	}
}
