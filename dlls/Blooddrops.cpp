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

#ifndef CLIENT_DLL

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()

LINK_ENTITY_TO_CLASS(phys_blood, CPhysblood);
void CPhysblood::BloodCreate(int BLDamnt, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float BLLTGravity, int BloodType, bool isgib, float spread)
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
		if (m_BloodDropVel > 0)
		{
			m_BloodDropVel -= RANDOM_LONG(0, 400);
		}

	}
	else
	{
		if (m_BloodDropVel > 0)
		{
			m_BloodDropVel -= RANDOM_LONG(0, 300);
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
	UTIL_SetOrigin(pev, m_SpawnPos + (m_direction * 48) * m_opposite); //spawn a little bit more forward
	pev->velocity = ((m_direction + Vector(RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread))) * m_BloodDropVel) * m_opposite; // Applies spread and velocity, also applies the chance to have the outwards droplets
	pev->gravity = m_Gravity; // sets the gravity (bullet drop)
	pev->owner = NULL;
	pev->renderamt = 225;
	if (m_BloodType == BLOOD_COLOR_RED)
	{
		pev->rendercolor = Vector(RANDOM_LONG(100, 120), 0, 0);
	}
	else if (m_BloodType == BLOOD_COLOR_YELLOW)
	{
		pev->rendercolor = Vector(RANDOM_LONG(205, 255), RANDOM_LONG(185, 235), RANDOM_LONG(82, 160));
	}
	else if (m_BloodType == BLOOD_COLOR_GREEN)
	{
		pev->rendercolor = Vector(185, 235, 85);
	}	
	else if (m_BloodType == BLOOD_COLOR_CYAN)
	{
		pev->rendercolor = Vector(RANDOM_LONG(25, 50), RANDOM_LONG(150, 200), RANDOM_LONG(225, 255));
	}
	else
	{
		pev->rendercolor = Vector(RANDOM_LONG(0, 255), RANDOM_LONG(0, 255), RANDOM_LONG(0, 255));
	}
	pev->rendermode = kRenderTransAlpha;
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
	m_stain = PRECACHE_EVENT(1, "events/bloodspray.sc");
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
		UTIL_DecalTrace(&tr, RANDOM_LONG(45, 52));
	}
	else if (m_BloodType == BLOOD_COLOR_YELLOW)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(53, 59));
	}
	else if (m_BloodType == BLOOD_COLOR_GREEN)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(60, 66));
	}
	else if (m_BloodType == BLOOD_COLOR_CYAN)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(72, 74));
	}
	else
	{
		switch(RANDOM_LONG(0, 2))
		{
		case 0:
		case 1:
			UTIL_DecalTrace(&tr, RANDOM_LONG(78, 83));
			break;
		case 2:
			UTIL_DecalTrace(&tr, 78); // purple is very common in the noise textures
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
	pev->nextthink = gpGlobals->time + 0.5;
	if (pev->waterlevel == 0)
	return;

	SetThink(&CPhysblood::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
#endif
int CPhysblood::ShouldCollide(CBaseEntity* pentTouched)
{
	if (pentTouched->IsBSPModel())
		return 1;
	else
	{
		if (0 != pentTouched->pev->takedamage && m_hashealed != true)
		{
			m_hashealed = true;
			if (m_BloodType == BLOOD_COLOR_CYAN)
			{
				pentTouched->TakeHealth(2, DMG_GENERIC);
				
			}
			
		}
		if (pentTouched->IsPlayer() && m_hasstained != true)
		{
			int flags;
#if defined(CLIENT_WEAPONS)
			flags = FEV_NOTHOST;
#else
			flags = 0;
#endif
			CBasePlayer* player = dynamic_cast<CBasePlayer*>(pentTouched);
			//CBasePlayerWeapon* weapon = player->m_pActiveItem->GetWeaponPtr();
			m_hasstained = true;
			if (m_BloodType == BLOOD_COLOR_RED)
			{
				PLAYBACK_EVENT_FULL(flags, player->edict(), m_stain, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 1, 0, 0, 0);
				ALERT(at_console, "red gun\n");
			}
			else if (m_BloodType == BLOOD_COLOR_YELLOW)
			{
				PLAYBACK_EVENT_FULL(flags, player->edict(), m_stain, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 2, 0, 0, 0);
				ALERT(at_console, "yellow gun\n");		
			}
			else if (m_BloodType == BLOOD_COLOR_GREEN)
			{
				PLAYBACK_EVENT_FULL(flags, player->edict(), m_stain, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 3, 0, 0, 0);
				ALERT(at_console, "green gun\n");		
			}
			else if (m_BloodType == BLOOD_COLOR_CYAN)
			{
				PLAYBACK_EVENT_FULL(flags, player->edict(), m_stain, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 4, 0, 0, 0);
				ALERT(at_console, "cyan gun\n");
			}
		}
		return 0;
	}
}