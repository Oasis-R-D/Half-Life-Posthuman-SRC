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
void CPhysblood::BloodCreate(int BLDamnt, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float BLLTGravity)
{
	for (int i = 0; i < BLDamnt; i++) // Allows multishot
	{
		// Create a new entity with CPhysblood private data
		CPhysblood* pBlood = GetClassPtr((CPhysblood*)NULL);
		pBlood->pev->classname = MAKE_STRING("blooddrop");
		pBlood->m_BloodDropVel = BLDSpeed + RANDOM_LONG(-150, 150);
		pBlood->m_SpawnPos = VecSpawnPos;
		pBlood->m_direction = vecDir;
		pBlood->m_Spread = vecSpread;
		pBlood->m_Gravity = BLLTGravity;
		pBlood->Spawn();
	}
}

void CPhysblood::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_TOSS; // makes it have gravity
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos + m_direction * 24); //spawn a little bit more forward
	pev->velocity = (m_direction + Vector(RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread))) * m_BloodDropVel; // Applies spread and velocity
	pev->speed = m_BloodDropVel; // I have no fucking clue what the difference between speed and velocity is :3
	pev->gravity = m_Gravity; // sets the gravity (bullet drop)
	pev->angles = m_direction;
	
	
	SET_MODEL(ENT(pev), "sprites/blood.spr");
	pev->scale = RANDOM_FLOAT(0.40, 0.60);
	pev->renderamt = 225;
	pev->rendercolor = Vector(RANDOM_LONG(100, 120), 0, 0);
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
}


int CPhysblood::Classify()
{
	return CLASS_NONE;
}
void CPhysblood::Stay()
{
	Vector vecDir = pev->velocity.Normalize();

	pev->angles = UTIL_VecToAngles(vecDir);
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	pev->velocity = Vector(0, 0, 0);
	pev->avelocity.z = 0;
	
}
void CPhysblood::BoltTouch(CBaseEntity* pOther)
{
	SetTouch(NULL);
	SetThink(NULL);

	if (0 != pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();

		if (pOther->IsBSPModel())
		{
			Stay();
		}
		else
		{
			SetThink(&CPhysblood::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
	}
	else 
	{
		SetThink(&CPhysblood::SUB_Remove);
		pev->nextthink = gpGlobals->time;
		if (FClassnameIs(pOther->pev, "worldspawn"))
			Stay();
	}
	TraceResult tr = UTIL_GetGlobalTrace();
	UTIL_DecalTrace(&tr, RANDOM_LONG(45, 50));
	SetThink(&CPhysblood::SUB_Remove);
	pev->nextthink = gpGlobals->time;

}

void CPhysblood::AirThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if (pev->waterlevel == 0)
	return;
	
	pev->renderamt -= 75;
	pev->scale += RANDOM_FLOAT(0.125, 0.025);
}
#endif
