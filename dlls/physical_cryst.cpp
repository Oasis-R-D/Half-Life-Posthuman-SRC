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

// So basically just a crystal that does crystal things :3

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "physical_cryst.h"

#ifndef CLIENT_DLL

#define Orange 2
#define Green 1
#define Purple 0
/*
* PURP MV: 
* GREEN MV: 
* ORNG MV: 
*/

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
LINK_ENTITY_TO_CLASS(phys_cryst, CPhyscryst);
void CPhyscryst::CrystalCreate(int BLLTamnt, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, int CrystType, edict_t *shooter, int OG)
{
	for (int i = 0; i < BLLTamnt; i++) // Allows multishot
	{
		// Create a new entity with CPhyscryst private data
		CPhyscryst* pBullet = GetClassPtr((CPhyscryst*)NULL);
		pBullet->pev->classname = MAKE_STRING("phys_cryst");
		pBullet->m_SpawnPos = VecSpawnPos;
		pBullet->m_direction = vecDir;
		pBullet->m_Spread = vecSpread;
		pBullet->m_SpreadVert = vecSpreadvert; // Shotgun duckbill choke
		pBullet->m_Crysttype = CrystType; // Crystal type
		pBullet->m_original = OG;
		pBullet->Spawn();
		pBullet->pev->owner = shooter;
	}
}

void CPhyscryst::Spawn()
{
	Precache();
	if (m_Crysttype == Purple)
	{
		pev->movetype = MOVETYPE_BOUNCE;
		m_muzzlevelocity = 2000;
		m_bounceamnt = 0;
	}
	else if (m_Crysttype == Green)
	{
		pev->movetype = MOVETYPE_FLY;
		m_muzzlevelocity = 4000;
		m_bounceamnt = 9;
	}
	else if (m_Crysttype == Orange)
	{
		pev->movetype = MOVETYPE_FLY;
		m_muzzlevelocity = 4000;
		m_bounceamnt = 9;
	}
	m_muzzlevelocity += RANDOM_LONG(-250, 250);
	pev->gravity = 0.01;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos + m_direction * 16); //spawn a little bit more forward
	pev->velocity = (m_direction + Vector(RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_SpreadVert, -m_SpreadVert))) * m_muzzlevelocity; // Applies spread and velocity
	pev->speed = m_muzzlevelocity;	// I have no fucking clue what the difference between speed and velocity is :3
	pev->angles = Vector(RANDOM_LONG(360, -360), RANDOM_LONG(360, -360), RANDOM_LONG(360, -360));
	pev->avelocity = Vector(RANDOM_LONG(360, -360), RANDOM_LONG(360, -360), RANDOM_LONG(360, -360));
	SET_MODEL(ENT(pev), "models/crystalshot.mdl");
	pev->skin = m_Crysttype + 1;
	pev->body = RANDOM_LONG(0, 2);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	
	SetThink(&CPhyscryst::AirThink);
	pev->nextthink = gpGlobals->time;
}


void CPhyscryst::Precache()
{
	PRECACHE_MODEL("models/crystalshot.mdl");
}


int CPhyscryst::Classify()
{
	return CLASS_NONE;
}
void CPhyscryst::Stay() //TO-DO: add imapct sounds
{
	SetTouch(NULL);
	SetThink(NULL);
	pev->velocity = Vector(0, 0, 0);
	pev->avelocity.z = 0;
	SetThink(&CPhyscryst::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
void CPhyscryst::BoltTouch(CBaseEntity* pOther)
{
	m_Endpos = pev->origin;
	TraceResult tr = UTIL_GetGlobalTrace();
	if (m_bounceamnt >= 4 || m_original == 0)
	{
		SetTouch(NULL);
		SetThink(NULL);
	}
	if (0 != pOther->pev->takedamage)
	{
		entvars_t* pevOwner;
		pevOwner = VARS(pev->owner);

		ClearMultiDamage();

		
		if (m_Crysttype == Purple)
		{
		pOther->TraceAttack(pevOwner, 2, pev->velocity.Normalize(), &tr, DMG_BULLET || DMG_NEVERGIB);
		}
		else if (m_Crysttype == Green)
		{
		pOther->TraceAttack(pevOwner, 10, pev->velocity.Normalize(), &tr, DMG_BLAST);
		}
		else if (m_Crysttype == Orange)
		{
		pOther->TraceAttack(pevOwner, 5, pev->velocity.Normalize(), &tr, DMG_BULLET || DMG_NEVERGIB);
		}
		ApplyMultiDamage(pev, pevOwner);

		if (pOther->IsBSPModel())
		{
			if (m_bounceamnt >= 4)
			{
				Stay();
			}
		}
		else
		{
			// play NPC hit sound (this is here because stay() isn't called when hitting an npc so the sounds there don't apply to npcs)
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bullet_hit1.wav", 1, ATTN_NORM);
				break;
			case 1:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bullet_hit2.wav", 1, ATTN_NORM);
				break;
			}
			if (m_bounceamnt >= 4)
			{
				Stay();
			}
		}
	}
	else
	{
		if (m_bounceamnt >= 4)
		{
			Stay();
		}
	}
	if (m_Crysttype != Purple)
	{
		DecalGunshot(&tr, BULLET_PLAYER_9MM);
	}
	else
	{
		DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);
	}
	if (RANDOM_LONG(0, 1) == 1)
	{
		TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_PLAYER_9MM);
	}
	m_bounceamnt += 1;
}

void CPhyscryst::AirThink()
{
	pev->nextthink = gpGlobals->time + 0.05;
	SetTouch(&CPhyscryst::BoltTouch);

	if (pev->waterlevel == 0)
	return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}
#endif
