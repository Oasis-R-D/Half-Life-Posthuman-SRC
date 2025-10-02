/****
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
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "Blooddrops.h"
#include "decals.h"

#ifndef CLIENT_DLL

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()

LINK_ENTITY_TO_CLASS(phys_blood, CPhysblood);
void CPhysblood::BloodCreate(int BLDamnt, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float BLLTGravity, int BloodType, bool isgib, float spread, bool speedRNG)
{
	if (UTIL_ShouldShowBlood(BloodType) == true)
	{
		if (isgib == false && BLDamnt > 16)
		{
			BLDamnt = 16;
		}
		if (g_pGameRules->IsMultiplayer())
			BLDamnt /= 2;
		for (int i = 0; i < BLDamnt; i++) // Allows multishot
		{
			// Create a new entity with CPhysblood private data
			CPhysblood* pBlood = GetClassPtr((CPhysblood*)NULL);
			pBlood->pev->classname = MAKE_STRING("blooddrop");
			pBlood->m_BloodDropVel = BLDSpeed;
			pBlood->m_SpawnPos = VecSpawnPos;
			pBlood->m_direction = vecDir;
			pBlood->m_Spread = spread;
			pBlood->m_Gravity = BLLTGravity;
			pBlood->m_BloodType = BloodType;
			pBlood->m_isgib = isgib;
			pBlood->m_randomspeed = speedRNG;
			pBlood->Spawn();
		}
	}
}

void CPhysblood::Spawn()
{
	Precache();
	
	switch (RANDOM_LONG(1, 3))
	{
	case 1:
	case 2:
		m_opposite = 1;
		break;
	case 3:
		if (m_isgib != true)
			m_opposite = -1;
		else
			m_opposite = 1;
		break;
	}
	SET_MODEL(ENT(pev), "sprites/blood.spr");
	if (m_opposite == 1)
	{
		pev->scale = RANDOM_FLOAT(0.4, 0.65);
		if (m_BloodDropVel > 0 && m_randomspeed == true)
		{
			m_BloodDropVel -= RANDOM_LONG(0, 375);
		}

	}
	else
	{
		if (m_BloodDropVel > 0)
		{
			if (m_randomspeed == true)
				m_BloodDropVel -= RANDOM_LONG(0, 275);
			pev->scale = RANDOM_FLOAT(0.35, 0.6); // makes the ones going towards the player smaller
		}
		else
		{
			pev->scale = RANDOM_FLOAT(0.4, 0.65);
		}
	}
	if (m_isgib == true)
	{
		pev->scale = RANDOM_FLOAT(0.30, 0.40);
	}
	pev->movetype = MOVETYPE_TOSS; // makes it have gravity
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos + (m_direction * 24) * m_opposite); //spawn a little bit more forward
	pev->velocity = ((m_direction + Vector(RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread))) * m_BloodDropVel) * m_opposite; // Applies spread and velocity, also applies the chance to have the outwards droplets
	pev->gravity = m_Gravity; // sets the gravity (bullet drop)
	pev->owner = NULL;

	if (m_BloodType == BLOOD_COLOR_RED)
	{
		pev->rendercolor = Vector(RANDOM_LONG(100, 120), 0, 0);
	}
	else if (m_BloodType == BLOOD_COLOR_YELLOW)
	{
		pev->rendercolor = Vector(199, 195, 55);
	}
	else if (m_BloodType == BLOOD_COLOR_GREEN)
	{
		pev->rendercolor = Vector(185, 235, 85);
	}	
	else if (m_BloodType == BLOOD_COLOR_CYAN)
	{
		pev->rendercolor = Vector(RANDOM_LONG(25, 50), RANDOM_LONG(150, 200), RANDOM_LONG(225, 255));
	}
	else if (m_BloodType == NULL) // water drop
	{
		pev->rendercolor = Vector(115, 205, 255);
	}
	else // corruption
	{
		pev->rendercolor = Vector(RANDOM_LONG(0, 255), RANDOM_LONG(0, 255), RANDOM_LONG(0, 255));
	}
	
	if (m_BloodType == NULL) // water drop
	{
		pev->renderamt = 125;
		pev->rendermode = kRenderTransAdd;
	}
	else
	{
		pev->rendermode = kRenderTransAlpha;
		pev->renderamt = 225;
	}
	pev->frame = RANDOM_LONG(0, 8);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	SetTouch(&CPhysblood::BoltTouch);
	SetThink(&CPhysblood::AirThink);
	pev->nextthink = gpGlobals->time;
}


void CPhysblood::Precache()
{
	PRECACHE_MODEL("sprites/blood.spr");
	PRECACHE_SOUND("common/drip_01.wav");
	PRECACHE_SOUND("common/drip_02.wav");
	PRECACHE_SOUND("common/drip_03.wav");
	PRECACHE_SOUND("common/drip_04.wav");
	PRECACHE_SOUND("common/drip_05.wav");
	PRECACHE_SOUND("common/drip_06.wav");
	PRECACHE_SOUND("common/drip_07.wav");
}


int CPhysblood::Classify()
{
	return CLASS_NONE;
}
void CPhysblood::Stay()
{
	SetThink(&CPhysblood::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
void CPhysblood::BoltTouch(CBaseEntity* pOther)
{
	SetTouch(NULL);
	SetThink(NULL);

	Stay();
	TraceResult tr = UTIL_GetGlobalTrace();
	if (m_BloodType == BLOOD_COLOR_RED)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_BLOODSPRAY1, DECAL_BLOODSPRAY6));
	}
	else if (m_BloodType == BLOOD_COLOR_YELLOW)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_ABLOODSPRAY1, DECAL_YBLOOD6_2));
	}
	else if (m_BloodType == BLOOD_COLOR_GREEN)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_XBLOODSPRAY1, DECAL_XBLOODSPRAY7));
	}
	else if (m_BloodType == BLOOD_COLOR_CYAN)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_BBLOODSPRAY1, DECAL_BBLOODSPRAY3));
	}
	else if (m_BloodType == NULL) // water drop TO-DO: see if we should do a puddle decal for this
	{
		//UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_WBLOODSPRAY1, DECAL_WBLOODSPRAY3));
	}
	else
	{
		switch(RANDOM_LONG(0, 2))
		{
		case 0:
		case 1:
			UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_NBLOODSPRAY1, DECAL_NBLOODSPRAY6));
			break;
		case 2:
			UTIL_DecalTrace(&tr, DECAL_NBLOODSPRAY1); // purple is very common in the noise textures
			break;
		}
	}
	char dripsnd[256];
	sprintf(dripsnd, "common/drip_0%d.wav", RANDOM_LONG(1, 7));
	EMIT_SOUND(edict(), CHAN_AUTO, dripsnd, 1, 0.6);
	Stay();

}

void CPhysblood::AirThink()
{
	CBaseEntity* pObject = NULL;
	pev->nextthink = gpGlobals->time + 0.05;
	pObject = UTIL_FindEntityInSphere(pObject, pev->origin, 4);
	if (pObject)
	{
		if (pObject->IsPlayer())
			ALERT(at_console, "pObject is player\n");
		if (pObject->IsPlayer() && 0 != pObject->pev->takedamage && m_hashealed != true)
		{
			ALERT(at_console, "attempt heal\n");
			m_hashealed = true;
			if (m_BloodType == BLOOD_COLOR_CYAN)
			{
				pObject->TakeHealth(2, DMG_GENERIC);
			}
		}
		if (pObject->IsPlayer() && m_hasstained != true)
		{
			ALERT(at_console, "attempt stain\n");
			int flags;
			flags = 0;

			CBasePlayer* player = dynamic_cast<CBasePlayer*>(pObject);
			CBasePlayerWeapon* weapon = player->m_pActiveItem->GetWeaponPtr();
			m_hasstained = true;
			if (m_BloodType == BLOOD_COLOR_RED)
			{
				//PLAYBACK_EVENT_FULL(flags, player->edict(), m_stain, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 1, 0, 0, 0);
				weapon->m_stain = 1;
				ALERT(at_console, "red gun\n");
			}
			else if (m_BloodType == BLOOD_COLOR_YELLOW)
			{
				weapon->m_stain = 2;
				ALERT(at_console, "yellow gun\n");
			}
			else if (m_BloodType == BLOOD_COLOR_GREEN)
			{
				weapon->m_stain = 3;
				ALERT(at_console, "green gun\n");
			}
			else if (m_BloodType == BLOOD_COLOR_CYAN)
			{
				weapon->m_stain = 4;
				ALERT(at_console, "cyan gun\n");
			}
			else if (m_BloodType == NULL) // water
			{
				weapon->m_stain = 0;
				ALERT(at_console, "clean gun\n");
			}
			else // corruption
			{
				// does nothing currently
			}
			char dripsnd[256];
			sprintf(dripsnd, "common/drip_0%d.wav", RANDOM_LONG(1, 7));
			EMIT_SOUND(player->edict(), CHAN_AUTO, dripsnd, 1, 0.6);
		}
	}
	if (pev->waterlevel == 0)
	return;

	SetThink(&CPhysblood::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

int CPhysblood::ShouldCollide(CBaseEntity* pentTouched)
{
	if (pentTouched->IsBSPModel())
		return 1;
	else
	{
		return 0;
	}
}
#endif