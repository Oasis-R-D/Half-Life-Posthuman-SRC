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
#define BOLT_AIR_VELOCITY 6000
#define BOLT_WATER_VELOCITY 5000 //replace with a speed change perhaps?

/*
* 9MM MV: 6000
* 556 MV: 7000
* 357 MV: 7500
* 12G MV: 5750
* 
*/

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
LINK_ENTITY_TO_CLASS(phys_blood, CPhysblood);
void CPhysblood::BloodCreate(int BLDamnt, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float BLLTGravity, int BloodType)
{
	if (UTIL_ShouldShowBlood(BloodType) == true)
	{
		if (BLDamnt > 16)
		{
			BLDamnt = 16;
		}
		for (int i = 0; i < BLDamnt; i++) // Allows multishot
		{
			// Create a new entity with CPhysblood private data
			CPhysblood* pBlood = GetClassPtr((CPhysblood*)NULL);
			pBlood->pev->classname = MAKE_STRING("blooddrop");
			pBlood->m_BloodDropVel = BLDSpeed;
			pBlood->m_SpawnPos = VecSpawnPos;
			pBlood->m_direction = vecDir;
			pBlood->m_Spread = vecSpread;
			pBlood->m_Gravity = BLLTGravity;
			pBlood->m_BloodType = BloodType;

			pBlood->Spawn();
		}
	}
}

void CPhysblood::Spawn()
{
	Precache();
	switch (RANDOM_LONG(1, 3))
		{
			case 1 || 3:
			{
				m_opposite = 1;
				break;
			}
			case 2:
			{
				m_opposite = -1;
				break;
			}
		}
	SET_MODEL(ENT(pev), "sprites/blood.spr");
	if (m_opposite == 1)
	{
		pev->scale = RANDOM_FLOAT(0.4, 0.65);
		m_BloodDropVel -= RANDOM_LONG(200, -200);

	}
	else
	{
		m_BloodDropVel -= RANDOM_LONG(0, 350);
		pev->scale = RANDOM_FLOAT(0.35, 0.6); // makes the ones going towards the player smaller
	}
	pev->movetype = MOVETYPE_TOSS; // makes it have gravity
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos + (m_direction * 48) * m_opposite); //spawn a little bit more forward
	pev->velocity = ((m_direction + Vector(RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread))) * m_BloodDropVel) * m_opposite; // Applies spread and velocity, also applies the chance to have the outwards droplets
	pev->speed = m_BloodDropVel; // I have no fucking clue what the difference between speed and velocity is :3
	pev->gravity = m_Gravity; // sets the gravity (bullet drop)
	pev->angles = m_direction;
	
	
	
	pev->renderamt = 225;
	if (m_BloodType == BLOOD_COLOR_RED)
	{
		pev->rendercolor = Vector(RANDOM_LONG(100, 120), 0, 0);
	}
	else if (m_BloodType == BLOOD_COLOR_GREEN || BLOOD_COLOR_YELLOW)
	{
		pev->rendercolor = Vector(RANDOM_LONG(205, 255), RANDOM_LONG(185, 235), RANDOM_LONG(82, 160));
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

	if (0 != pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		Stay();

	}
	else 
	{
		Stay();
	}
	TraceResult tr = UTIL_GetGlobalTrace();
	if (m_BloodType == BLOOD_COLOR_RED)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(45, 52));
	}
	else if (m_BloodType == BLOOD_COLOR_GREEN || BLOOD_COLOR_YELLOW)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(53, 58));
	}
	char dripsnd[256];
	sprintf(dripsnd, "common/drip_0%d.wav", RANDOM_LONG(1, 3));
	EMIT_SOUND(edict(), CHAN_AUTO, dripsnd, 1, 0.6);
	Stay();

}

void CPhysblood::AirThink()
{
	pev->nextthink = gpGlobals->time + 0.25;
	if (pev->waterlevel == 0)
	return;
	
	SetThink(&CPhysblood::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
#endif
