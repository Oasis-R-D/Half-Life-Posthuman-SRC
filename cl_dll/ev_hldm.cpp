/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "hud.h"
#include "cl_util.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

#include "com_weapons.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"

#include "engine_particles.h"
#include "../renderer/bsprenderer.h"
#include "../renderer/particle_engine.h"
extern engine_studio_api_t IEngineStudio;
extern char * UTIL_VarArgs_client(const char* format, ...);

static int tracerCount[MAX_PLAYERS];

#include "pm_shared.h"

void V_PunchAxis(int axis, float punch);
void VectorAngles(const float* forward, float* angles);

extern cvar_t* cl_lw;
extern cvar_t* r_decals;

static inline bool EV_HLDM_IsBSPModel(physent_t* pe)
{
	return pe != nullptr && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP);
}
// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound(int idx, pmtrace_t* ptr, float* vecSrc, float* vecEnd, int iBulletType)
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	cl_entity_t* cl_entity = NULL;
	float fvol;
	float fvolbar;
	const char* rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char* pTextureName;
	char texname[64];
	char szbuffer[64];

	entity = EV_IndexFromTrace(ptr);

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;

	// Player
	if (entity == 0)
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char*)EV_TraceTexture(ptr->ent, vecSrc, vecEnd);

		if (pTextureName)
		{
			strcpy(texname, pTextureName);
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}

			// '}}'
			strcpy(szbuffer, pTextureName);
			szbuffer[CBTEXTURENAMEMAX - 1] = 0;

			// get texture type
			chTextureType = PM_FindTextureType(szbuffer);
		}
	}
	else
	{
		// JoshA: Look up the entity and find the EFLAG_FLESH_SOUND flag.
		// This broke at some point then TF:C added prediction.
		//
		// It used to use Classify of pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE
		// to determine what sound to play, but that's server side and isn't available on the client
		// and got lost in the translation to that.
		// Now the server will replicate that state via an eflag.
		cl_entity = gEngfuncs.GetEntityByIndex(entity);

		if (cl_entity && !!(cl_entity->curstate.eflags & EFLAG_FLESH_SOUND))
			chTextureType = CHAR_TEX_FLESH;
	}
	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		fvol = 0.9;
		fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL:
		fvol = 0.9;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:
		fvol = 0.9;
		fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:
		fvol = 0.5;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE:
		fvol = 0.9;
		fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH:
		fvol = 0.9;
		fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD:
		fvol = 0.9;
		fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if (iBulletType == BULLET_PLAYER_CROWBAR)
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;
		fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	case CHAR_TEX_IMPEN:
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0, cnt - 1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0, 0xf));
	return fvolbar;
}

char* EV_HLDM_DamageDecal(physent_t* pe)
{
	static char decalname[32];
	int idx;
	if (pe->rendermode == kRenderTransAlpha)
		return nullptr;

	if (pe->classnumber == 1)
	{
		idx = gEngfuncs.pfnRandomLong(0, 2);
		sprintf(decalname, "{break%i", idx + 1);
	}
	else if (pe->rendermode != kRenderNormal)
	{
		sprintf(decalname, "{bproof1");
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong(0, 4);
		sprintf(decalname, "{shot%i", idx + 1);
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace(pmtrace_t* pTrace, char* decalName)
{
	int iRand;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles(pTrace->endpos);

	iRand = gEngfuncs.pfnRandomLong(0, 0x7FFF);
	if (iRand < (0x7fff / 2)) // not every bullet makes a sound.
	{
		switch (iRand % 5)
		{
		case 0:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 3:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 4:
			gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		}
	}

	// Only decal brush models such as the world etc.
	if (decalName && '\0' != decalName[0] && r_decals->value > 0)
	{
		gEngfuncs.pEfxAPI->R_DecalShoot(
		gEngfuncs.pEfxAPI->Draw_DecalIndex(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalName)),
		EV_IndexFromTrace(pTrace), 0, pTrace->endpos, 0);
	}
}

void EV_HLDM_DecalGunshot(pmtrace_t* pTrace, int iBulletType)
{
	physent_t* pe;

	pe = EV_GetPhysent(pTrace->ent);

	if (EV_HLDM_IsBSPModel(pe))
	{
		switch (iBulletType)
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			EV_HLDM_GunshotDecalTrace(pTrace, EV_HLDM_DamageDecal(pe));
			break;
		}
	}
}

void EV_HLDM_CheckTracer(int idx, float* vecSrc, float* end, float* forward, float* right, int iBulletType, int iTracerFreq, int* tracerCount)
{
	int i;
	bool player = idx >= 1 && idx <= engine_cl->maxclients;

	if (iTracerFreq != 0 && ((*tracerCount)++ % iTracerFreq) == 0)
	{
		Vector vecTracerSrc;

		if (player)
		{
			Vector offset(0, 0, -4);

			// adjust tracer position for player
			for (i = 0; i < 3; i++)
			{
				vecTracerSrc[i] = vecSrc[i] + offset[i] + right[i] * 2 + forward[i] * 16;
			}
		}
		else
		{
			VectorCopy(vecSrc, vecTracerSrc);
		}

		switch (iBulletType)
		{
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_MONSTER_9MM:
		case BULLET_MONSTER_12MM:
		default:
			EV_CreateTracer(vecTracerSrc, end);
			break;
		}
	}
}


/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets(int idx, float* forward, float* right, float* up, int cShots, float* vecSrc, float* vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int* tracerCount, float flSpreadX, float flSpreadY)
{
	int i;
	pmtrace_t tr;
	int iShot;

	for (iShot = 1; iShot <= cShots; iShot++)
	{
		Vector vecDir, vecEnd;

		float x, y, z;
		//We randomize for the Shotgun.
		if (iBulletType == BULLET_PLAYER_BUCKSHOT)
		{
			do
			{
				x = gEngfuncs.pfnRandomFloat(-0.5, 0.5) + gEngfuncs.pfnRandomFloat(-0.5, 0.5);
				y = gEngfuncs.pfnRandomFloat(-0.5, 0.5) + gEngfuncs.pfnRandomFloat(-0.5, 0.5);
				z = x * x + y * y;
			} while (z > 1);

			for (i = 0; i < 3; i++)
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[i] + y * flSpreadY * up[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		} //But other guns already have their spread randomized in the synched spread.
		else
		{

			for (i = 0; i < 3; i++)
			{
				vecDir[i] = vecDirShooting[i] + flSpreadX * right[i] + flSpreadY * up[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

		// Store off the old count
		EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

		EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

		EV_HLDM_CheckTracer(idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount);

		// do damage, paint decals
		if (tr.fraction != 1.0)
		{
			switch (iBulletType)
			{
			default:
			case BULLET_PLAYER_9MM:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);

				break;
			case BULLET_PLAYER_MP5:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);
				break;
			case BULLET_PLAYER_BUCKSHOT:

				EV_HLDM_DecalGunshot(&tr, iBulletType);

				break;
			case BULLET_PLAYER_357:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);

				break;
			}
		}

		EV_PopPMStates();
	}
}

//======================
//	    GLOCK START
//======================
void EV_FireGlock1(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	bool empty;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	empty = 0 != args->bparam1;
	AngleVectors(angles, &forward, &right, &up);

	shell = EV_FindModelIndex("models/shell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		EV_MuzzleFlash();
		//EV_WeaponAnimation(empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, args->bparam2);

		V_PunchAxis(0, -2.0);
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);
	const char* sound = args->bparam2 ? "weapons/pl_gun1.wav" : "weapons/pl_gun3.wav";
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, sound, gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));
	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);
	//EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx - 1], args->fparam1, args->fparam2);
}

void EV_FireGlock2(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	bool empty;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	empty = 0 != args->bparam1;
	AngleVectors(angles, &forward, &right, &up);

	shell = EV_FindModelIndex("models/shell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		EV_WeaponAnimation(empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 0, false);

		V_PunchAxis(0, -2.0);
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	EV_GetGunPosition(args, vecSrc, origin);

	VectorCopy(forward, vecAiming);

	//EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx - 1], args->fparam1, args->fparam2);
}
//======================
//	   GLOCK END
//======================

//======================
//	   ELITE START
//	 ( 10mmhandgun )
//======================
void EV_FireElite(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, &forward, &right, &up);

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		EV_WeaponAnimation(PYTHON_FIRE1, 0, false);
	}

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/elite_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/elite_shot2.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	}
}
//======================
//	    ELITE END
//	  ( 10mmhandgun )
//======================

//======================
//	  SHOTGUN START
//======================
void EV_FireShotGunDouble(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, &forward, &right, &up);

	shell = EV_FindModelIndex("models/shotgunshell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		V_PunchAxis(0, -10.0);
	}

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/dbarrel1.wav", gEngfuncs.pfnRandomFloat(0.98, 1.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong(0, 0x1f));

	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);

	if (engine_cl->maxclients > 1)
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 8, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx - 1], 0.17365, 0.04362);
	}
	else
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 12, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx - 1], 0.08716, 0.08716);
	}
}

void EV_FireShotGunSingle(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, &forward, &right, &up);

	shell = EV_FindModelIndex("models/shotgunshell.mdl"); // brass shell

	if (args->bparam2 > 0)
	{
		EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6);
		EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHOTSHELL);
		if (args->bparam2 > 1)
		{
			EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6);
			EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHOTSHELL);
		}
		return;
	}

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		V_PunchAxis(0, -5.0);
	}

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0x1f));

	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);
	
	EV_HLDM_FireBullets(idx, forward, right, up, 4, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx - 1], 0.08716, 0.04362);

	if (args->iparam2 == 1)
	{
		EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6);
		EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHOTSHELL);
	}
}
//======================
//	   SHOTGUN END
//======================

//======================
//	    MP5 START
//======================
void EV_FireMP5(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, &forward, &right, &up);

	shell = EV_FindModelIndex("models/shell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		//EV_WeaponAnimation(MP5_FIRE1 + gEngfuncs.pfnRandomLong(0, 2), 0);

		V_PunchAxis(0, gEngfuncs.pfnRandomFloat(-2, 2));
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	}

	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx - 1], args->fparam1, args->fparam2);
}

// We only predict the animation and sound
// The grenade is still launched from the server.
void EV_FireMP52(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	if (EV_IsLocal(idx))
	{
		//EV_WeaponAnimation(MP5_LAUNCH, 0);
		V_PunchAxis(0, -10);
	}

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/glauncher.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/glauncher2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	}
}
//======================
//		 MP5 END
//======================

//======================
//	   PHYTON START
//	     ( .357 )
//======================
void EV_FirePython(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, &forward, &right, &up);

	if (EV_IsLocal(idx))
	{
		// Python uses different body in multiplayer versus single player
		bool multiplayer = engine_cl->maxclients != 1;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		EV_WeaponAnimation(PYTHON_FIRE1, multiplayer ? 1 : 0, false);

		//V_PunchAxis(0, -10.0);
	}

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	}
}
//======================
//	    PHYTON END
//	     ( .357 )
//======================

//======================
//	    M29 START
//	    
//======================
void EV_FireM29(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	int M29numb = args->iparam1; // 0 for R || 1 for L
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, &forward, &right, &up);

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash((bool)M29numb);
		EV_WeaponAnimation(PYTHON_FIRE1, 0, (bool)M29numb);

		event_args_t* acousticArgs = args;
		acousticArgs->fparam1 = AC_LOUD;
		acousticArgs->bparam1 = false;
		EV_Acoustic(args);
	}

	switch (M29numb)
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_AUTO, "weapons/m29_fire1.wav", 1.0f, ATTN_NORM, 0, 90);
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_AUTO, "weapons/m29_fire2.wav", 1.0f, ATTN_NORM, 0, 110);
		break;
	}

	EV_GetGunPosition(args, vecSrc, origin);

	VectorCopy(forward, vecAiming);
}
//======================
//		 M29 END
//
//======================

//======================
//	   CROWBAR START
//======================

// unused now
void EV_Crowbar(event_args_t* args)
{

}
//======================
//	   CROWBAR END
//======================
//======================
//	   MELEE START
//======================
int g_iSwing;

//Only predicts the miss sounds, hit sounds are still played
//server side, so players don't get the wrong idea.
void EV_Melee(event_args_t* args)
{
	const int idx = args->entindex;
	Vector origin = args->origin;
	int iBigSwing = args->iparam2;
	const bool hitSomething = 0 != args->bparam2;

	if (!EV_IsLocal(idx))
	{
		return;
	}

	//Play Swing sound
	if (iBigSwing == 1)
	{
		if (hitSomething)
		{
			EV_WeaponAnimation(MELEE_CHARGEHIT, 0, false);
		}
		else
		{
			EV_WeaponAnimation(MELEE_CHARGEMISS, 0, false);
		}

		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pwrench_big_miss.wav", 1, ATTN_NORM, 0, PITCH_NORM);
	}
	else if (iBigSwing == 0)
	{
		if (hitSomething)
		{
			switch (g_iSwing % 3)
			{
			case 0:
				EV_WeaponAnimation(MELEE_ATTACK1HIT, 0, false);
				break;
			case 1:
				EV_WeaponAnimation(MELEE_ATTACK2HIT, 0, false);
				break;
			case 2:
				EV_WeaponAnimation(MELEE_ATTACK3HIT, 0, false);
				break;
			}
		}
		else
		{
			switch (g_iSwing % 3)
			{
			case 0:
				EV_WeaponAnimation(MELEE_ATTACK1MISS, 0, false);
				break;
			case 1:
				EV_WeaponAnimation(MELEE_ATTACK2MISS, 0, false);
				break;
			case 2:
				EV_WeaponAnimation(MELEE_ATTACK3MISS, 0, false);
				break;
			}

			//Play Swing sound
			gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM);\
		}

		++g_iSwing;
	}
	else
	{
		if (hitSomething)
		{
			switch (g_iSwing % 3)
			{
			case 0:
				EV_WeaponAnimation(MELEE_HEAVYHIT1, 0, false);
				break;
			case 1:
				EV_WeaponAnimation(MELEE_HEAVYHIT2, 0, false);
				break;
			case 2:
				EV_WeaponAnimation(MELEE_HEAVYHIT3, 0, false);
				break;
			}
		}
		else
		{
			switch (g_iSwing % 3)
			{
			case 0:
				EV_WeaponAnimation(MELEE_HEAVYMISS1, 0, false);
				break;
			case 1:
				EV_WeaponAnimation(MELEE_HEAVYMISS2, 0, false);
				break;
			case 2:
				EV_WeaponAnimation(MELEE_HEAVYMISS3, 0, false);
				break;
			}

			//Play Swing sound
			gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM);\
		}

		++g_iSwing;
	}
}
//======================
//	   MELEE END
//======================

//======================
//	  CROSSBOW START
//======================
//=====================
// EV_BoltCallback
// This function is used to correct the origin and angles
// of the bolt, so it looks like it's stuck on the wall.
//=====================
void EV_BoltCallback(struct tempent_s* ent, float frametime, float currenttime)
{
	ent->entity.origin = ent->entity.baseline.vuser1;
	ent->entity.angles = ent->entity.baseline.vuser2;
}

void EV_FireCrossbow2(event_args_t* args)
{
	Vector vecSrc, vecEnd;
	Vector up, right, forward;
	pmtrace_t tr;

	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);

	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, &forward, &right, &up);

	EV_GetGunPosition(args, vecSrc, origin);

	VectorMA(vecSrc, 8192, forward, vecEnd);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));

	if (EV_IsLocal(idx))
	{
		if (0 != args->iparam1)
			EV_WeaponAnimation(CROSSBOW_FIRE1, 0, false);
		else
			EV_WeaponAnimation(CROSSBOW_FIRE3, 0, false);
	}

	// Store off the old count
	EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

	//We hit something
	if (tr.fraction < 1.0)
	{
		physent_t* pe = EV_GetPhysent(tr.ent);

		//Not the world, let's assume we hit something organic ( dog, cat, uncle joe, etc ).
		if (!EV_HLDM_IsBSPModel(pe))
		{
			switch (gEngfuncs.pfnRandomLong(0, 1))
			{
			case 0:
				gEngfuncs.pEventAPI->EV_PlaySound(idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_PlaySound(idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			}
		}
		//Stick to world but don't stick to glass, it might break and leave the bolt floating. It can still stick to other non-transparent breakables though.
		else if (pe->rendermode == kRenderNormal)
		{
			gEngfuncs.pEventAPI->EV_PlaySound(0, tr.endpos, CHAN_BODY, "weapons/xbow_hit1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, PITCH_NORM);

			//Not underwater, do some sparks...
			if (gEngfuncs.PM_PointContents(tr.endpos, NULL) != CONTENTS_WATER)
				gEngfuncs.pEfxAPI->R_SparkShower(tr.endpos);

			Vector vBoltAngles;
			int iModelIndex = EV_FindModelIndex("models/crossbow_bolt.mdl");

			VectorAngles(forward, vBoltAngles);

			TEMPENTITY* bolt = gEngfuncs.pEfxAPI->R_TempModel(tr.endpos - forward * 10, Vector(0, 0, 0), vBoltAngles, 5, iModelIndex, TE_BOUNCE_NULL);

			if (bolt)
			{
				bolt->flags |= (FTENT_CLIENTCUSTOM);					 //So it calls the callback function.
				bolt->entity.baseline.vuser1 = tr.endpos - forward * 10; // Pull out a little bit
				bolt->entity.baseline.vuser2 = vBoltAngles;				 //Look forward!
				bolt->callback = EV_BoltCallback;						 //So we can set the angles and origin back. (Stick the bolt to the wall)
			}
		}
	}

	EV_PopPMStates();
}

//TODO: Fully predict the flying bolt.
void EV_FireCrossbow(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));

	//Only play the weapon anims if I shot it.
	if (EV_IsLocal(idx))
	{
		if (0 != args->iparam1)
			EV_WeaponAnimation(CROSSBOW_FIRE1, 0, false);
		else
			EV_WeaponAnimation(CROSSBOW_FIRE3, 0, false);
	}
}
//======================
//	   CROSSBOW END
//======================

//======================
//	   SQUEAK START
//======================
void EV_SnarkFire(event_args_t* args)
{
	int idx;
	Vector vecSrc, angles, forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy(args->origin, vecSrc);
	VectorCopy(args->angles, angles);

	AngleVectors(angles, &forward, NULL, NULL);

	if (!EV_IsLocal(idx))
		return;

	if (0 != args->ducking)
		vecSrc = vecSrc - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);

	// Store off the old count
	EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc + forward * 20, vecSrc + forward * 64, PM_NORMAL, -1, &tr);

	//Find space to drop the thing.
	if (tr.allsolid == 0 && tr.startsolid == 0 && tr.fraction > 0.25)
		EV_WeaponAnimation(SQUEAK_THROW, 0, false);

	EV_PopPMStates();
}
//======================
//	   SQUEAK END
//======================

//======================
//	   STAIN START
//======================
void EV_VMstain(event_args_t* args)
{
	int idx;
	idx = args->entindex;
	if (EV_IsLocal(idx))
	{
		if (engine_cl->viewent.model != nullptr)
		{
			engine_cl->viewent.curstate.skin = args->iparam1;
		}
	}
}
//======================
//	   STAIN END
//======================

//======================
//	   SILENCE START
//======================
void EV_VMsilence(event_args_t* args)
{
	int idx;
	idx = args->entindex;
	if (EV_IsLocal(idx) && engine_cl->viewent.model != nullptr)
	{
		engine_cl->viewent.curstate.body = args->iparam1;
	}
}
//======================
//	   SILENCE END
//======================

void EV_TrainPitchAdjust(event_args_t* args)
{
	int idx;
	Vector origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	bool stop;

	char sz[256];

	idx = args->entindex;

	VectorCopy(args->origin, origin);

	us_params = (unsigned short)args->iparam1;
	stop = 0 != args->bparam1;

	m_flVolume = (float)(us_params & 0x003f) / 40.0;
	noise = (int)(((us_params) >> 12) & 0x0007);
	pitch = (int)(10.0 * (float)((us_params >> 6) & 0x003f));

	switch (noise)
	{
	case 1:
		strcpy(sz, "plats/ttrain1.wav");
		break;
	case 2:
		strcpy(sz, "plats/ttrain2.wav");
		break;
	case 3:
		strcpy(sz, "plats/ttrain3.wav");
		break;
	case 4:
		strcpy(sz, "plats/ttrain4.wav");
		break;
	case 5:
		strcpy(sz, "plats/ttrain6.wav");
		break;
	case 6:
		strcpy(sz, "plats/ttrain7.wav");
		break;
	default:
		// no sound
		strcpy(sz, "");
		return;
	}

	if (stop)
	{
		gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, sz);
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch);
	}
}
void EV_VehiclePitchAdjust(event_args_t* args)
{
	int idx;
	Vector origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	bool stop;

	char sz[256];

	idx = args->entindex;

	VectorCopy(args->origin, origin);

	us_params = (unsigned short)args->iparam1;
	stop = 0 != args->bparam1;

	m_flVolume = (float)(us_params & 0x003f) / 40.0;
	noise = (int)(((us_params) >> 12) & 0x0007);
	pitch = (int)(10.0 * (float)((us_params >> 6) & 0x003f));

	switch (noise)
	{
	case 1:
		strcpy(sz, "plats/vehicle1.wav");
		break;
	case 2:
		strcpy(sz, "plats/vehicle2.wav");
		break;
	case 3:
		strcpy(sz, "plats/vehicle3.wav");
		break;
	case 4:
		strcpy(sz, "plats/vehicle4.wav");
		break;
	case 5:
		strcpy(sz, "plats/vehicle6.wav");
		break;
	case 6:
		strcpy(sz, "plats/vehicle7.wav");
		break;
	default:
		// no sound
		strcpy(sz, "");
		return;
	}

	if (stop)
	{
		gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, sz);
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch);
	}
}

bool EV_TFC_IsAllyTeam(int iTeam1, int iTeam2)
{
	return false;
}

static std::string GetAcoustic(int numb)
{
	using namespace std::string_literals;

	switch(numb)
	{
		default:
		case AC_NORM:	return "357"s;		break;
		case AC_LOUD:	return "generic"s;	break;
	}
}

void EV_Acoustic(event_args_t* args)
{
	if (args->fparam1 == AC_NONE) // no sound
		return;

	Vector Origin;
	Vector Dir;
	
	std::string sound = GetAcoustic(args->fparam1);
	std::string soundPath = std::string("weapons/acoustic/").append(sound);

	pmtrace_t ForTr, UpTr;

	if (args->bparam1 != 1) // not func_tank
	{
		Origin = engine_cl->viewent.attachment[0];
		Dir = args->origin;
	}
	else // func_tank
	{
		Origin = args->origin;
		Dir = args->angles;
	}

	EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(Origin, Origin+Dir*768, PM_WORLD_ONLY, -1, &ForTr);

	EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(Origin, Origin + Vector(0, 0, 768), PM_WORLD_ONLY, -1, &UpTr);

	// check to see if it's in the sky, bias if so
	if (gEngfuncs.PM_PointContents(UpTr.endpos, nullptr) == CONTENTS_SKY)
		UpTr.fraction = 4;
	if (gEngfuncs.PM_PointContents(ForTr.endpos, nullptr) == CONTENTS_SKY)
		ForTr.fraction = 4;

	// more biases
	if (UpTr.fraction == 1)
		ForTr.fraction = 1.25;

	if (ForTr.fraction == 1)
		ForTr.fraction = 1.25;

	float dist = ((768 * ForTr.fraction) + (768 * UpTr.fraction)) / 2;

	if (dist >= 768)
	{	// large area
		soundPath.append("_big.wav");
	}
	else if (dist <= 256)
	{	// small area
		soundPath.append("_sml.wav");
	}
	else	 
	{	// medium area
		soundPath.append("_med.wav");
	}

	// CHAN_BOT is unused, perfect for this
	gEngfuncs.pEventAPI->EV_PlaySound(args->entindex, Origin, CHAN_BOT, soundPath.c_str(), 1, ATTN_ACOUSTIC, 0, 98 + gEngfuncs.pfnRandomLong(0, 4));
}

void EV_Particles(event_args_t* args)
{
	Vector Origin;
	Vector Dir;
	int R = 0, G = 0, B = 0;
	int idx;
	idx = args->entindex;
	const char* constchar;
	const char* constchar2;
	const char* constchar3;
	int BLDAMNT;
	int skill = int(gEngfuncs.pfnGetCvarFloat("skill"));

	BLDAMNT = args->fparam1 / ((skill != 3) ? 2.0 : 4);
	if (BLDAMNT > 24)
		BLDAMNT = 24;

	switch (args->iparam1) // particle type
	{
		case PE_MUZZLESMK:
		{
			EV_Acoustic(args);
			if (args->bparam1 != 1) // not func_tank
			{
				Origin = engine_cl->viewent.attachment[0];
				Dir = args->origin;
			}
			else // func_tank
			{
				Origin = args->origin;
				Dir = args->angles;
			}
			switch (args->iparam2)
			{
			default:
			case 0: // Def muzzle smoke
				gParticleEngine.CreateSystem("engine_muzzle_smoke.txt", Origin, Dir, 0);
				// gParticleEngine.CreateSystem("engine_muzzleflash_flames.txt", Origin, Dir, 0);
				break;
			case 1: // shotgun?
				// reserved for sg if we do change it
				break;
			case 2: // Railcannon
				gParticleEngine.CreateCluster("railcannon_muzzle_cluster.txt", Origin, Dir, 0);
				break;
			}
		}
		break;
		case PE_MUZZLESMKSG: // Should this be a param2 thing for the def muzzle flash? // Should this be a cluster?
		{
			Origin = engine_cl->viewent.attachment[0];
			EV_Acoustic(args);
			gParticleEngine.CreateSystem("engine_muzzle_smoke.txt", Origin, args->origin, 0);
			if (args->bparam1 != 1)
			{
				gParticleEngine.CreateSystem("engine_shotgun_puff.txt", Origin, args->origin, 0);
			}
			else
			{
				gParticleEngine.CreateSystem("engine_shotgun_puff2.txt", Origin, args->origin, 0);
				gParticleEngine.CreateSystem("engine_muzzle_smoke.txt", Origin, args->origin, 0); // Double the smoke
			}
			break;
		}
		case PE_EXPLOSIONCLUST:
			switch (args->iparam2)
			{
				default:
				case 0:
					gParticleEngine.CreateCluster("expl_fireball_cluster.txt", args->origin, args->origin, 0);
					break;
				case 1:
					gParticleEngine.CreateCluster("expl_he_cluster.txt", args->origin, args->origin, 0);
					break;
				case 2:
					gParticleEngine.CreateCluster("expl_flash_cluster.txt", args->origin, args->origin, 0);
					break;
			}
			break;
		case PE_NPCIMPACTCLUST:
			switch (args->iparam2)
			{
				case BLOOD_COLOR_RED:
					R = 160;
					constchar = "bloodspot";
					constchar2 = "engine_blood_impact.txt";
					constchar3 = "blood_effects_cluster.txt";
					break;
				case BLOOD_COLOR_YELLOW:
					R = 199;
					G = 195;
					B = 55;
					constchar = "abloodspot";
					constchar2 = "engine_blood_impact_alien.txt";
					constchar3 = "blood_effects_cluster_alien.txt";
					break;
				case BLOOD_COLOR_GREEN:
					R = 185;
					G = 235;
					B = 85;
					constchar = "xbloodspot";
					constchar2 = "engine_blood_impact_rx.txt";
					constchar3 = "blood_effects_cluster_rx.txt";
					break;
				case BLOOD_COLOR_INFECTION:
					R = 128;
					G = 54;
					B = 32;
					constchar = "Bbloodspot";
					constchar2 = "engine_blood_impact_fung.txt";
					constchar3 = "blood_effects_cluster_fung.txt";
					break;
				default:
					return;
					break;
			}

			if (!args->bparam1)
				gParticleEngine.CreateCluster(constchar3, args->origin, args->angles, 0);

			if (args->fparam2 > 0)
			{
				gParticleEngine.CreateSystem_File(UTIL_VarArgs_client(bonefragment1, (int)args->fparam2), args->origin, args->angles, 0);
				gParticleEngine.CreateSystem_File(UTIL_VarArgs_client(bonefragment2, (int)args->fparam2), args->origin, args->angles, 0);
			}

			if (args->bparam1) // bleeding
				BLDAMNT = gEngfuncs.pfnRandomLong(2, 3);

			for (int i = 0; i < BLDAMNT; i++)
			{
				switch (gEngfuncs.pfnRandomLong(1, 3))
				{
				case 1:
				case 2: idx = -1; break;
				case 3: idx = 1; break;
				}

				if (args->bparam1) // bleeding
					idx = 1;
				// TO-DO: randomly goes wrong way at light speed sometimes
				gParticleEngine.CreateSystem_File(UTIL_VarArgs_client(bloodspray, args->bparam2, gEngfuncs.pfnRandomLong(0, 1), constchar, constchar2, R, G, B), args->origin, args->angles * idx, 0);
			}

			break;
		case PE_BLOATERGASEXPL: // neurotoxin expl
			gParticleEngine.CreateSystem("bloaterexpl.txt", args->origin, args->origin, 0);
			break;
		case PE_BLDIMPACTCLUST: // phys blood hit the ground
			switch (args->iparam2)
			{
				case BLOOD_COLOR_RED:
					gParticleEngine.CreateSystem("engine_blood_impact.txt", args->origin, gpGlobals->v_up, 0);
					break;
				case BLOOD_COLOR_YELLOW:
					gParticleEngine.CreateSystem("engine_blood_impact_alien.txt", args->origin, gpGlobals->v_up, 0);
					break;
				case BLOOD_COLOR_GREEN:
					gParticleEngine.CreateSystem("engine_blood_impact_rx.txt", args->origin, gpGlobals->v_up, 0);
					break;
				case BLOOD_COLOR_INFECTION:
					gParticleEngine.CreateSystem("engine_blood_impact_fung.txt", args->origin, gpGlobals->v_up, 0);
					break;
			}
			break;
		case PE_BLLTIMPACTGLOW: // glowing bullet impact 'crater'
			if (args->bparam2 != 1)
				gParticleEngine.CreateSystem_File(bulletholeglow, args->origin, args->angles, 0);
			else if (EV_IsLocal(idx))
				gParticleEngine.CreateSystem_File(innacuracydebug, args->origin, args->angles, 0);
			break;
		case PE_BLDGIBCLOUD: // enemy gib cloud
			switch (args->iparam2)
			{
				case BLOOD_COLOR_RED:
					R = 160;
					constchar = "bloodspot";
					constchar2 = "engine_blood_impact.txt";
					break;
				case BLOOD_COLOR_YELLOW:
					R = 199;
					G = 195;
					B = 55;
					constchar = "abloodspot";
					constchar2 = "engine_blood_impact_alien.txt";
					break;
				case BLOOD_COLOR_GREEN:
					R = 185;
					G = 235;
					B = 85;
					constchar = "xbloodspot";
					constchar2 = "engine_blood_impact_rx.txt";
					break;
				case BLOOD_COLOR_INFECTION:
					R = 128;
					G = 54;
					B = 32;
					constchar = "Bbloodspot";
					constchar2 = "engine_blood_impact_fung.txt";
					break;
				default:
					return;
					break;
			}
			gParticleEngine.CreateSystem_File(UTIL_VarArgs_client(bloodgibcloud, R, G, B), args->origin, g_vecZero, 0);
			for (int i = 0; i < 20; i++)
			{
				gParticleEngine.CreateSystem_File(UTIL_VarArgs_client(bloodspray, 1, gEngfuncs.pfnRandomLong(0, 1), constchar, constchar2, R, G, B), args->origin, g_vecZero, 0);
			}
			break;
		case PE_SMOKECLOUD: // smoke gren expl
			gParticleEngine.CreateSystem("engine_smokegren.txt", args->origin, gpGlobals->v_up, 0);
			break;
		case PE_FIRE: // fire
			if (args->bparam2 != 1)
				gParticleEngine.CreateSystem("flames.txt", args->origin, gpGlobals->v_up, 0);
			else
				gParticleEngine.CreateSystem("blueflames.txt", args->origin, gpGlobals->v_up, 0);
			break;
		case PE_BILLOWSMOKE: // grenade/flying vehicle smoke
			gParticleEngine.CreateSystem("engine_smoke.txt", args->origin, gpGlobals->v_up, 0);
			break;
	}
}