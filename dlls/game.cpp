/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "client.h"
#include "game.h"
#include "filesystem_utils.h"

cvar_t displaysoundlist = {"displaysoundlist", "0"};

// multiplayer server rules
cvar_t fragsleft = {"mp_fragsleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED}; // Don't spam console/log files/users with this changing
cvar_t timeleft = {"mp_timeleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED};	 // "      "

// multiplayer server rules
cvar_t teamplay = {"mp_teamplay", "0", FCVAR_SERVER};
cvar_t fraglimit = {"mp_fraglimit", "0", FCVAR_SERVER};
cvar_t timelimit = {"mp_timelimit", "0", FCVAR_SERVER};
cvar_t friendlyfire = {"mp_friendlyfire", "0", FCVAR_SERVER};
cvar_t falldamage = {"mp_falldamage", "0", FCVAR_SERVER};
cvar_t weaponstay = {"mp_weaponstay", "0", FCVAR_SERVER};
cvar_t forcerespawn = {"mp_forcerespawn", "1", FCVAR_SERVER};
cvar_t flashlight = {"mp_flashlight", "0", FCVAR_SERVER};
cvar_t aimcrosshair = {"mp_autocrosshair", "1", FCVAR_SERVER};
cvar_t decalfrequency = {"decalfrequency", "30", FCVAR_SERVER};
cvar_t teamlist = {"mp_teamlist", "hgrunt;scientist", FCVAR_SERVER};
cvar_t teamoverride = {"mp_teamoverride", "1"};
cvar_t defaultteam = {"mp_defaultteam", "0"};
cvar_t allowmonsters = {"mp_allowmonsters", "0", FCVAR_SERVER};

cvar_t allow_spectators = {"allow_spectators", "0.0", FCVAR_SERVER}; // 0 prevents players from being spectators

cvar_t mp_chattime = {"mp_chattime", "10", FCVAR_SERVER};

cvar_t sv_allowbunnyhopping = {"sv_allowbunnyhopping", "0", FCVAR_SERVER};

cvar_t sv_firesize = {"sv_firesize", "32", FCVAR_SERVER};

//Macros to make skill cvars easier to define
#define DECLARE_SKILL_CVARS(name)                 \
	cvar_t sk_##name##1 = {"sk_" #name "1", "0"}; \
	cvar_t sk_##name##2 = {"sk_" #name "2", "0"}; \
	cvar_t sk_##name##3 = {"sk_" #name "3", "0"}

#define REGISTER_SKILL_CVARS(name) \
	CVAR_REGISTER(&sk_##name##1);  \
	CVAR_REGISTER(&sk_##name##2);  \
	CVAR_REGISTER(&sk_##name##3)

//CVARS FOR SKILL LEVEL SETTINGS
// Agrunt
DECLARE_SKILL_CVARS(agrunt_health);
DECLARE_SKILL_CVARS(agrunt_dmg_punch);

// Apache
DECLARE_SKILL_CVARS(apache_health);

// Archer
DECLARE_SKILL_CVARS(archer_health);
DECLARE_SKILL_CVARS(archer_shake);

// Barney
DECLARE_SKILL_CVARS(barney_health);

// Bullsquid
DECLARE_SKILL_CVARS(bullsquid_health);
DECLARE_SKILL_CVARS(bullsquid_dmg_bite);
DECLARE_SKILL_CVARS(bullsquid_dmg_whip);
DECLARE_SKILL_CVARS(bullsquid_dmg_spit);

// Big Momma
DECLARE_SKILL_CVARS(bigmomma_health_factor);
DECLARE_SKILL_CVARS(bigmomma_dmg_slash);
DECLARE_SKILL_CVARS(bigmomma_dmg_blast);
DECLARE_SKILL_CVARS(bigmomma_radius_blast);

// Gargantua
DECLARE_SKILL_CVARS(gargantua_health);
DECLARE_SKILL_CVARS(gargantua_dmg_slash);
DECLARE_SKILL_CVARS(gargantua_dmg_fire);
DECLARE_SKILL_CVARS(gargantua_dmg_stomp);

// Hassassin
DECLARE_SKILL_CVARS(hassassin_health);

// Headcrab
DECLARE_SKILL_CVARS(headcrab_health);
DECLARE_SKILL_CVARS(headcrab_dmg_bite);

// Hgrunt
DECLARE_SKILL_CVARS(hgrunt_health);
DECLARE_SKILL_CVARS(hgrunt_kick);
DECLARE_SKILL_CVARS(hgrunt_pellets);
DECLARE_SKILL_CVARS(hgrunt_gspeed);

// Massn
DECLARE_SKILL_CVARS(massassin_health);
DECLARE_SKILL_CVARS(massassin_kick);
DECLARE_SKILL_CVARS(massassin_gspeed);
DECLARE_SKILL_CVARS(plr_556_bullet);

// Houndeye
DECLARE_SKILL_CVARS(houndeye_health);
DECLARE_SKILL_CVARS(houndeye_dmg_blast);

// ISlave
DECLARE_SKILL_CVARS(islave_health);
DECLARE_SKILL_CVARS(islave_dmg_claw);
DECLARE_SKILL_CVARS(islave_dmg_clawrake);
DECLARE_SKILL_CVARS(islave_dmg_zap);

// Icthyosaur
DECLARE_SKILL_CVARS(ichthyosaur_health);
DECLARE_SKILL_CVARS(ichthyosaur_shake);

// Leech
DECLARE_SKILL_CVARS(leech_health);
DECLARE_SKILL_CVARS(leech_dmg_bite);

// Controller
DECLARE_SKILL_CVARS(controller_health);
DECLARE_SKILL_CVARS(controller_dmgzap);
DECLARE_SKILL_CVARS(controller_speedball);
DECLARE_SKILL_CVARS(controller_dmgball);

// Nihilanth
DECLARE_SKILL_CVARS(nihilanth_health);
DECLARE_SKILL_CVARS(nihilanth_zap);

// Scientist
DECLARE_SKILL_CVARS(scientist_health);

// Snark
DECLARE_SKILL_CVARS(snark_health);
DECLARE_SKILL_CVARS(snark_dmg_bite);
DECLARE_SKILL_CVARS(snark_dmg_pop);

// Zombie
DECLARE_SKILL_CVARS(zombie_health);
DECLARE_SKILL_CVARS(zombie_dmg_one_slash);
DECLARE_SKILL_CVARS(zombie_dmg_both_slash);

// Turret
DECLARE_SKILL_CVARS(turret_health);

// MiniTurret
DECLARE_SKILL_CVARS(miniturret_health);

// Sentry Turret
DECLARE_SKILL_CVARS(sentry_health);

// PitDrone
DECLARE_SKILL_CVARS(pitdrone_health);
DECLARE_SKILL_CVARS(pitdrone_dmg_bite);
DECLARE_SKILL_CVARS(pitdrone_dmg_spit);
DECLARE_SKILL_CVARS(pitdrone_dmg_whip);

// Shock Roach
DECLARE_SKILL_CVARS(shockroach_health);
DECLARE_SKILL_CVARS(shockroach_dmg_bite);
DECLARE_SKILL_CVARS(shockroach_lifespan);

// Shock Trooper
DECLARE_SKILL_CVARS(shocktrooper_health);
DECLARE_SKILL_CVARS(shocktrooper_kick);
DECLARE_SKILL_CVARS(shocktrooper_gspeed);
DECLARE_SKILL_CVARS(shocktrooper_maxcharge);
DECLARE_SKILL_CVARS(shocktrooper_rchgspeed);

// Shock Roach
DECLARE_SKILL_CVARS(plr_shockroachs);

// Spore Launcher
DECLARE_SKILL_CVARS(plr_spore);

// Voltigore
DECLARE_SKILL_CVARS(voltigore_health);
DECLARE_SKILL_CVARS(voltigore_dmg_punch);
DECLARE_SKILL_CVARS(voltigore_dmg_beam);

// Baby Voltigore
DECLARE_SKILL_CVARS(babyvoltigore_health);
DECLARE_SKILL_CVARS(babyvoltigore_dmg_punch);

// Funghoul
DECLARE_SKILL_CVARS(funghoul_health);
DECLARE_SKILL_CVARS(funghoul_dmg_slash);
DECLARE_SKILL_CVARS(funghoul_dmg_bite);

// Panthereye
DECLARE_SKILL_CVARS(panthereye_health);
DECLARE_SKILL_CVARS(panthereye_dmg_claw);
DECLARE_SKILL_CVARS(panthereye_dmg_leap);

// PLAYER WEAPONS

// M727
DECLARE_SKILL_CVARS(plr_m727);

// Crowbar whack
DECLARE_SKILL_CVARS(plr_crowbar);

// Sledge
DECLARE_SKILL_CVARS(plr_sledge);

// Glock Round
DECLARE_SKILL_CVARS(plr_9mm_bullet);

// Elite Round
DECLARE_SKILL_CVARS(plr_10mm_bullet);

// 357 Round
DECLARE_SKILL_CVARS(plr_357_bullet);

// MP5 Round
DECLARE_SKILL_CVARS(plr_9mmAR_bullet);

// M203 grenade
DECLARE_SKILL_CVARS(plr_9mmAR_grenade);

// Shotgun buckshot
DECLARE_SKILL_CVARS(plr_buckshot);

// Crossbow
DECLARE_SKILL_CVARS(plr_xbow_bolt_client);
DECLARE_SKILL_CVARS(plr_xbow_bolt_monster);

// RPG
DECLARE_SKILL_CVARS(plr_rpg);

// Zero Point Generator
DECLARE_SKILL_CVARS(plr_gauss);

// Tau Cannon
DECLARE_SKILL_CVARS(plr_egon_narrow);
DECLARE_SKILL_CVARS(plr_egon_wide);

// Hand Grenade
DECLARE_SKILL_CVARS(plr_hand_grenade);

// Satchel Charge
DECLARE_SKILL_CVARS(plr_satchel);

// Tripmine
DECLARE_SKILL_CVARS(plr_tripmine);

// HORNET
DECLARE_SKILL_CVARS(plr_hornet_dmg);

// WORLD WEAPONS
DECLARE_SKILL_CVARS(12mm_bullet);
DECLARE_SKILL_CVARS(9mmAR_bullet);
DECLARE_SKILL_CVARS(9mm_bullet);
DECLARE_SKILL_CVARS(M727_bullet);

// HORNET
DECLARE_SKILL_CVARS(hornet_dmg);

// HEALTH/CHARGE
DECLARE_SKILL_CVARS(suitcharger);
DECLARE_SKILL_CVARS(battery);
DECLARE_SKILL_CVARS(healthcharger);
DECLARE_SKILL_CVARS(healthkit);
DECLARE_SKILL_CVARS(scientist_heal);

// MONSTER DAMAGE ADJUSTERS
DECLARE_SKILL_CVARS(monster_head);
DECLARE_SKILL_CVARS(monster_chest);
DECLARE_SKILL_CVARS(monster_stomach);
DECLARE_SKILL_CVARS(monster_arm);
DECLARE_SKILL_CVARS(monster_leg);

// PLAYER DAMAGE ADJUSTERS
DECLARE_SKILL_CVARS(player_head);
DECLARE_SKILL_CVARS(player_chest);
DECLARE_SKILL_CVARS(player_stomach);
DECLARE_SKILL_CVARS(player_arm);
DECLARE_SKILL_CVARS(player_leg);

DECLARE_SKILL_CVARS(player_fire);

// END Cvars for Skill Level settings

cvar_t sv_pushable_fixed_tick_fudge = {"sv_pushable_fixed_tick_fudge", "15"};

static bool SV_InitServer()
{
	if (!FileSystem_LoadFileSystem())
	{
		return false;
	}

	if (UTIL_IsValveGameDirectory())
	{
		g_engfuncs.pfnServerPrint("This mod has detected that it is being run from a Valve game directory which is not supported\n"
			"Run this mod from its intended location\n\nThe game will now shut down\n");
		return false;
	}

	return true;
}

extern char savefile[128];

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit()
{
	char* cmd = strdup(CMD_ARGV(0));
	strcpy(savefile, CMD_ARGV(1));
	if (!strstr(savefile, ".sav"))
		strcat(savefile, ".sav");

	// Register cvars here:

	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	g_psv_aim = CVAR_GET_POINTER("sv_aim");
	g_psv_allow_autoaim = CVAR_GET_POINTER("sv_allow_autoaim");
	g_footsteps = CVAR_GET_POINTER("mp_footsteps");
	g_psv_cheats = CVAR_GET_POINTER("sv_cheats");

	if (!SV_InitServer())
	{
		g_engfuncs.pfnServerPrint("Error initializing server\n");
		//Shut the game down as soon as possible.
		SERVER_COMMAND("quit\n");
		return;
	}

	CVAR_REGISTER(&displaysoundlist);
	CVAR_REGISTER(&allow_spectators);

	CVAR_REGISTER(&teamplay);
	CVAR_REGISTER(&fraglimit);
	CVAR_REGISTER(&timelimit);

	CVAR_REGISTER(&fragsleft);
	CVAR_REGISTER(&timeleft);

	CVAR_REGISTER(&friendlyfire);
	CVAR_REGISTER(&falldamage);
	CVAR_REGISTER(&weaponstay);
	CVAR_REGISTER(&forcerespawn);
	CVAR_REGISTER(&flashlight);
	CVAR_REGISTER(&aimcrosshair);
	CVAR_REGISTER(&decalfrequency);
	CVAR_REGISTER(&teamlist);
	CVAR_REGISTER(&teamoverride);
	CVAR_REGISTER(&defaultteam);
	CVAR_REGISTER(&allowmonsters);

	CVAR_REGISTER(&mp_chattime);

	CVAR_REGISTER(&sv_allowbunnyhopping);

	CVAR_REGISTER(&sv_firesize);
	
	// REGISTER CVARS FOR SKILL LEVEL STUFF
	// Agrunt
	REGISTER_SKILL_CVARS(agrunt_health);
	REGISTER_SKILL_CVARS(agrunt_dmg_punch);

	// Apache
	REGISTER_SKILL_CVARS(apache_health);

	// Archer
	REGISTER_SKILL_CVARS(archer_health);
	REGISTER_SKILL_CVARS(archer_shake);

	// Barney
	REGISTER_SKILL_CVARS(barney_health);

	// Bullsquid
	REGISTER_SKILL_CVARS(bullsquid_health);
	REGISTER_SKILL_CVARS(bullsquid_dmg_bite);
	REGISTER_SKILL_CVARS(bullsquid_dmg_whip);
	REGISTER_SKILL_CVARS(bullsquid_dmg_spit);

	// Bigmomma
	REGISTER_SKILL_CVARS(bigmomma_health_factor);
	REGISTER_SKILL_CVARS(bigmomma_dmg_slash);
	REGISTER_SKILL_CVARS(bigmomma_dmg_blast);
	REGISTER_SKILL_CVARS(bigmomma_radius_blast);

	// Gargantua
	REGISTER_SKILL_CVARS(gargantua_health);
	REGISTER_SKILL_CVARS(gargantua_dmg_slash);
	REGISTER_SKILL_CVARS(gargantua_dmg_fire);
	REGISTER_SKILL_CVARS(gargantua_dmg_stomp);

	// Hassassin
	REGISTER_SKILL_CVARS(hassassin_health);

	// Headcrab
	REGISTER_SKILL_CVARS(headcrab_health);
	REGISTER_SKILL_CVARS(headcrab_dmg_bite);

	// Hgrunt
	REGISTER_SKILL_CVARS(hgrunt_health);
	REGISTER_SKILL_CVARS(hgrunt_kick);
	REGISTER_SKILL_CVARS(hgrunt_pellets);
	REGISTER_SKILL_CVARS(hgrunt_gspeed);

	// Massassin
	REGISTER_SKILL_CVARS(massassin_health);
	REGISTER_SKILL_CVARS(massassin_kick);
	REGISTER_SKILL_CVARS(massassin_gspeed);

	// Houndeye
	REGISTER_SKILL_CVARS(houndeye_health);
	REGISTER_SKILL_CVARS(houndeye_dmg_blast);

	// ISlave
	REGISTER_SKILL_CVARS(islave_health);
	REGISTER_SKILL_CVARS(islave_dmg_claw);
	REGISTER_SKILL_CVARS(islave_dmg_clawrake);
	REGISTER_SKILL_CVARS(islave_dmg_zap);

	// Ichthyosaur
	REGISTER_SKILL_CVARS(ichthyosaur_health);
	REGISTER_SKILL_CVARS(ichthyosaur_shake);

	// Leech
	REGISTER_SKILL_CVARS(leech_health);
	REGISTER_SKILL_CVARS(leech_dmg_bite);

	// Controller
	REGISTER_SKILL_CVARS(controller_health);
	REGISTER_SKILL_CVARS(controller_dmgzap);
	REGISTER_SKILL_CVARS(controller_speedball);
	REGISTER_SKILL_CVARS(controller_dmgball);

	// Nihilanth
	REGISTER_SKILL_CVARS(nihilanth_health);
	REGISTER_SKILL_CVARS(nihilanth_zap);

	// Scientist
	REGISTER_SKILL_CVARS(scientist_health);

	// Snark
	REGISTER_SKILL_CVARS(snark_health);
	REGISTER_SKILL_CVARS(snark_dmg_bite);
	REGISTER_SKILL_CVARS(snark_dmg_pop);

	// Zombie
	REGISTER_SKILL_CVARS(zombie_health);
	REGISTER_SKILL_CVARS(zombie_dmg_one_slash);
	REGISTER_SKILL_CVARS(zombie_dmg_both_slash);

	// Turret
	REGISTER_SKILL_CVARS(turret_health);

	// MiniTurret
	REGISTER_SKILL_CVARS(miniturret_health);

	// Sentry Turret
	REGISTER_SKILL_CVARS(sentry_health);

	// PitDrone
	REGISTER_SKILL_CVARS(pitdrone_health);
	REGISTER_SKILL_CVARS(pitdrone_dmg_bite);
	REGISTER_SKILL_CVARS(pitdrone_dmg_spit);
	REGISTER_SKILL_CVARS(pitdrone_dmg_whip);

	// M727
	REGISTER_SKILL_CVARS(plr_m727);

	// Crowbar
	REGISTER_SKILL_CVARS(plr_crowbar);
	
	// Sledgehammer
	REGISTER_SKILL_CVARS(plr_sledge);

	// Glock Round
	REGISTER_SKILL_CVARS(plr_9mm_bullet);

	// Elite Round
	REGISTER_SKILL_CVARS(plr_10mm_bullet);

	// 357 Round
	REGISTER_SKILL_CVARS(plr_357_bullet);

	// MP5 Round
	REGISTER_SKILL_CVARS(plr_9mmAR_bullet);

	// M203 Grenade
	REGISTER_SKILL_CVARS(plr_9mmAR_grenade);

	// Shotgun Buckshot
	REGISTER_SKILL_CVARS(plr_buckshot);

	// Crossbow
	REGISTER_SKILL_CVARS(plr_xbow_bolt_monster);
	REGISTER_SKILL_CVARS(plr_xbow_bolt_client);

	// RPG
	REGISTER_SKILL_CVARS(plr_rpg);

	// Hand Grenade
	REGISTER_SKILL_CVARS(plr_hand_grenade);

	// Tripmine
	REGISTER_SKILL_CVARS(plr_tripmine);

	// World weapons
	REGISTER_SKILL_CVARS(12mm_bullet);
	REGISTER_SKILL_CVARS(9mmAR_bullet);
	REGISTER_SKILL_CVARS(9mm_bullet);
	REGISTER_SKILL_CVARS(M727_bullet);

	// Hornet
	REGISTER_SKILL_CVARS(hornet_dmg);

	REGISTER_SKILL_CVARS(suitcharger);
	REGISTER_SKILL_CVARS(battery);
	REGISTER_SKILL_CVARS(healthcharger);
	REGISTER_SKILL_CVARS(healthkit);
	REGISTER_SKILL_CVARS(scientist_heal);

	// Monster damage adjusters
	REGISTER_SKILL_CVARS(monster_head);
	REGISTER_SKILL_CVARS(monster_chest);
	REGISTER_SKILL_CVARS(monster_stomach);
	REGISTER_SKILL_CVARS(monster_arm);
	REGISTER_SKILL_CVARS(monster_leg);

	// Player damage adjusters
	REGISTER_SKILL_CVARS(player_head);
	REGISTER_SKILL_CVARS(player_chest);
	REGISTER_SKILL_CVARS(player_stomach);
	REGISTER_SKILL_CVARS(player_arm);
	REGISTER_SKILL_CVARS(player_leg);
	REGISTER_SKILL_CVARS(player_fire);
	// END REGISTER CVARS FOR SKILL LEVEL STUFF
	
	CVAR_REGISTER(&sv_pushable_fixed_tick_fudge);

	InitMapLoadingUtils();

	SERVER_COMMAND("exec skill.cfg\n");
}

void GameDLLShutdown()
{
	FileSystem_FreeFileSystem();
}
