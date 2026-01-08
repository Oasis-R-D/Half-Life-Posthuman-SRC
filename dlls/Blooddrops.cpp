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

// UNDONE: Save/restore this?

LINK_ENTITY_TO_CLASS(phys_blood, CPhysblood);
void CPhysblood::BloodCreate(int BLDamnt, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float BLLTGravity, int BloodType, bool isgib, float spread, bool speedRNG)
{
	if (UTIL_ShouldShowBlood(BloodType) == true)
	{
		int i;
		if (isgib == false && BLDamnt > 16)
		{
			BLDamnt = 16;
		}
		if (g_iSkillLevel == SKILL_HARD || g_pGameRules->IsMultiplayer())
			BLDamnt /= 2;
		for (i = 0; i < BLDamnt; i++) // Allows multishot
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
		pev->scale = RANDOM_FLOAT(0.4f, 0.65f);
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
			pev->scale = RANDOM_FLOAT(0.35f, 0.6f); // makes the ones going towards the player smaller
		}
		else
		{
			pev->scale = RANDOM_FLOAT(0.4f, 0.65f);
		}
	}

	if (m_isgib == true)
	{
		pev->scale = RANDOM_FLOAT(0.30f, 0.40f);
	}

	pev->movetype = MOVETYPE_TOSS; // makes it have gravity
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos);
	pev->velocity = ((m_direction + RANDOM_VECTOR(-m_Spread, m_Spread)) * m_BloodDropVel) * m_opposite; // Applies spread and velocity, also applies the chance to have the entry wound droplets
	pev->gravity = m_Gravity;
	pev->owner = NULL;

	if (m_BloodType == BLOOD_COLOR_RED)
	{
		pev->rendercolor = Vector(RANDOM_LONG(102, 200), 0, 0);
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
		pev->rendercolor = Vector(RANDOM_LONG(16, 18), RANDOM_LONG(253, 255), RANDOM_LONG(190, 192));
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

void CPhysblood::BoltTouch(CBaseEntity* pOther)
{
	SetTouch(NULL);
	SetThink(NULL);
	TraceResult tr = UTIL_GetGlobalTrace();
	PLAYBACK_EVENT_FULL(0, edict(), g_sParticleEvent, 0.0, tr.vecEndPos + (gpGlobals->v_up * 2), gpGlobals->v_up, 0.0, 0.0, PE_BLDIMPACTCLUST, m_BloodType, 0, 0);
	if (m_BloodType == BLOOD_COLOR_RED)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_BLOODSPRAY1, DECAL_BLOODSPRAY6), 6);
	}
	else if (m_BloodType == BLOOD_COLOR_YELLOW)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_ABLOODSPRAY1, DECAL_YBLOOD6_2), 6);
	}
	else if (m_BloodType == BLOOD_COLOR_GREEN)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_XBLOODSPRAY1, DECAL_XBLOODSPRAY7), 6);
	}
	else if (m_BloodType == BLOOD_COLOR_CYAN)
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_BBLOODSPRAY1, DECAL_BBLOODSPRAY3), 6);
	}
	else if (m_BloodType == NULL) // add decal?
	{
		//UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_WBLOODSPRAY1, DECAL_WBLOODSPRAY3), 6);
	}
	else
	{
		UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_NBLOODSPRAY1, DECAL_NBLOODSPRAY6), 6);
	}
	char dripsnd[256];
	sprintf(dripsnd, "common/drip_0%d.wav", RANDOM_LONG(1, 7));
	EMIT_SOUND(edict(), CHAN_AUTO, dripsnd, 1, 0.6f);
 	UTIL_remove(this);
}

void CPhysblood::AirThink()
{
	
	pev->nextthink = gpGlobals->time + 0.05f;
	if (m_BloodType == BLOOD_COLOR_CYAN)
	{
		CBaseEntity* pObject = NULL;
		pObject = UTIL_FindEntityInSphere(pObject, pev->origin, 4);
		if (pObject)
		{
			if (!pObject->IsBSPModel() && 0 != pObject->pev->takedamage && m_hashealed != true && !FClassnameIs(pObject->pev, "monster_headcrab_super"))
			{
				ALERT(at_console, "attempt heal\n");
				m_hashealed = true;
				pObject->TakeHealth(2, DMG_GENERIC);
			}
		}
	}

	if (pev->waterlevel == 0)
		return;

	char dripsnd[256];
	sprintf(dripsnd, "common/drip_0%d.wav", RANDOM_LONG(1, 7));
	EMIT_SOUND(edict(), CHAN_AUTO, dripsnd, 1, 0.6f);
	SetThink(&CPhysblood::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

int CPhysblood::ShouldCollide(CBaseEntity* pentTouched)
{
	if (pentTouched->IsBSPModel())
		return 1;
	else
		return 0;
}
#endif