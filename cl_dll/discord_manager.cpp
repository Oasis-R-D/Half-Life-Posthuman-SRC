// ================================================== \\
// Discord RPC implemenatation
// Based on code from the Valve Dev Community wiki
// 
// Jay - 2022
// ================================================== \\

#include "hud.h"
#include "discord_manager.h"
#include <string.h>
#include "discord\discord_rpc.h"
#include <time.h>

static DiscordRichPresence discordPresence;
extern cl_enginefunc_t gEngfuncs;

// Blank handlers; not required for singleplayer Half-Life
static void HandleDiscordReady(const DiscordUser* connectedUser) {}
static void HandleDiscordDisconnected(int errcode, const char* message) {}
static void HandleDiscordError(int errcode, const char* message) {}
static void HandleDiscordJoin(const char* secret) {}
static void HandleDiscordSpectate(const char* secret) {}
static void HandleDiscordJoinRequest(const DiscordUser* request) {}

// Default logo to use as a fallback
const char* defaultLogo = "ph_logo";

void DiscordMan_Startup(void)
{
	int64_t startTime = time(0);

	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	handlers.ready = HandleDiscordReady;
	handlers.disconnected = HandleDiscordDisconnected;
	handlers.errored = HandleDiscordError;
	handlers.joinGame = HandleDiscordJoin;
	handlers.spectateGame = HandleDiscordSpectate;
	handlers.joinRequest = HandleDiscordJoinRequest;

	Discord_Initialize("1306778334579134534", &handlers, 1, 0);

	memset(&discordPresence, 0, sizeof(discordPresence));

	discordPresence.startTimestamp = startTime;
	discordPresence.largeImageKey = defaultLogo;
	Discord_UpdatePresence(&discordPresence);
}

void DiscordMan_Update(void)
{
	char curArea[64]; // If the CVar is empty, use the map file name
	snprintf(curArea, sizeof(curArea) - 1, strncmp(gEngfuncs.pfnGetCvarString("rpc_chapter"), "", sizeof(curArea)) ? gEngfuncs.pfnGetCvarString("rpc_chapter") : gEngfuncs.pfnGetLevelName());
	char curImage[16]; // If the CVar is empty, use the default logo
	snprintf(curImage, sizeof(curImage) - 1, strncmp(gEngfuncs.pfnGetCvarString("rpc_image"), "", sizeof(curImage)) ? gEngfuncs.pfnGetCvarString("rpc_image") : defaultLogo);

	int skill = int(gEngfuncs.pfnGetCvarFloat("skill"));
	const char* skilllevel;
	const char* map = curArea;
	switch (skill)
	{
	default:
		skilllevel = "In Menus";
	case 1:
		skilllevel = "Easy Mode";
		break;
	case 2:
		skilllevel = "Hard Mode";
		break;
	case 3:
		skilllevel = "Realism Mode";
		break;
	}
	// Menu detections, these don't seem to work very well.
	if (engine_cl->paused)
	{
		skilllevel = "In Menus";
	}
	if (engine_cl->intermission)
	{
		map = 0;
		skilllevel = "Intermission";
	}
	if (engine_cls->state == ca_disconnected)
	{
		map = 0;
		skilllevel = "In Main Menus";
	}

	discordPresence.details = map;	// Chapter name doesn't matter; if it's blank, Discord shows map name
	discordPresence.state = skilllevel;
	discordPresence.largeImageKey = curImage;
    discordPresence.largeImageText = "Post-Human";
    discordPresence.smallImageKey = "ord_logo";
    discordPresence.smallImageText = "Oasis R&D";
	Discord_UpdatePresence(&discordPresence);
}

void DiscordMan_Kill(void)
{
	Discord_Shutdown();
}
