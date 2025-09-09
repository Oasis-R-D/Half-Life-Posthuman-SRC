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
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "physical_bullet.h"

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
LINK_ENTITY_TO_CLASS(phys_bullet, CPhysbullet);
void CPhysbullet::BulletCreate(int BLLTamnt, float BLLTDamage, int BLLTSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, float BLLTGravity, int FlareType, edict_t *shooter)
{
	for (int i = 0; i < BLLTamnt; i++) // Allows multishot
	{
		// Create a new entity with CPhysbullet private data
		CPhysbullet* pBullet = GetClassPtr((CPhysbullet*)NULL);
		pBullet->pev->classname = MAKE_STRING("bullet");
		if (g_iSkillLevel != SKILL_HARD)
		{
			pBullet->m_muzzlevelocity = BLLTSpeed;
		}
		else
		{
			pBullet->m_muzzlevelocity = BLLTSpeed * 1.25;
		}
		pBullet->m_BulletDamage = BLLTDamage;
		pBullet->m_SpawnPos = VecSpawnPos;
		pBullet->m_direction = vecDir;
		pBullet->m_Spread = vecSpread;
		pBullet->m_SpreadVert = vecSpreadvert; // Shotgun duckbill choke
		pBullet->m_Gravity = BLLTGravity;
		pBullet->m_Flare = FlareType; // tracer type
		pBullet->pev->owner = shooter;
		pBullet->Spawn();
		
	}
}

void CPhysbullet::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_TOSS; // makes it have gravity
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos + m_direction * 4); //spawn a little bit more forward
	pev->velocity = (m_direction + Vector(RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_SpreadVert, -m_SpreadVert))) * m_muzzlevelocity; // Applies spread and velocity
	pev->speed = m_muzzlevelocity; // I have no fucking clue what the difference between speed and velocity is :3
	pev->gravity = m_Gravity; // sets the gravity (bullet drop)
	pev->angles = m_direction;
	
	if (m_Flare == 556) // probably 556, idk
	{
		SET_MODEL(ENT(pev), "sprites/tracer_556mm.spr");
		//pev->scale = 0.25;
		pev->scale = RANDOM_FLOAT(0.23, 0.27);
	}
	else if (m_Flare == 12) // 12 gauge
	{
		SET_MODEL(ENT(pev), "sprites/tracer_12g.spr");
		//pev->scale = 0.15;
		pev->scale = RANDOM_FLOAT(0.13, 0.17);
	}
	else if (m_Flare == 357)
	{
		SET_MODEL(ENT(pev), "sprites/tracer_357mm.spr");
		//pev->scale = 0.3;
		pev->scale = RANDOM_FLOAT(0.28, 0.32);
	}
	else if (m_Flare == 69) // Training weapons
	{
		SET_MODEL(ENT(pev), "models/rubber_bullet.mdl");
	}
	else if (m_Flare == 420) // HC Deagle
	{
		SET_MODEL(ENT(pev), "sprites/tracer_357mm.spr");
		pev->scale = 2;
	}
	else //	9MM
	{
		SET_MODEL(ENT(pev), "sprites/tracer_9mm.spr");
		//pev->scale = 0.2;
		pev->scale = RANDOM_FLOAT(0.18, 0.22);
	}
	
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	if (m_Flare == 420)
	{

		pev->rendercolor = Vector(255, 70, 170);
		pev->rendermode = kRenderTransAdd;
	}
	else if (m_Flare == 69)
	{
		pev->rendermode = kRenderNormal;
	}
	else
	{
		pev->rendercolor = Vector(255, 255, 255);
		pev->rendermode = kRenderTransAdd;	
	}
	pev->renderamt = 0;
	SetTouch(&CPhysbullet::BoltTouch);
	SetThink(&CPhysbullet::AirThink);
	pev->nextthink = gpGlobals->time;
}


void CPhysbullet::Precache()
{
	PRECACHE_MODEL("models/rubber_bullet.mdl");
	PRECACHE_MODEL("sprites/tracer_9mm.spr");
	PRECACHE_MODEL("sprites/tracer_556mm.spr");
	PRECACHE_MODEL("sprites/tracer_357mm.spr");
	PRECACHE_MODEL("sprites/tracer_12g.spr");
}


int CPhysbullet::Classify()
{
	return CLASS_NONE;
}
void CPhysbullet::Stay() //TO-DO: add imapct sounds
{
	pev->velocity = Vector(0, 0, 0);
	pev->avelocity.z = 0;
	SetThink(&CPhysbullet::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
void CPhysbullet::BoltTouch(CBaseEntity* pOther)
{
	TraceResult tr = UTIL_GetGlobalTrace();
	pev->movetype = MOVETYPE_NONE;
	SetTouch(NULL);
	SetThink(NULL);

	if (0 != pOther->pev->takedamage)
	{
		entvars_t* pevOwner;
		m_Endpos = pev->origin;
		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();
		if (m_Flare != 420)
		{
			pOther->TraceAttack(pevOwner, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
		}
		else
		{
			pOther->TraceAttack(pevOwner, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET);
		}

		ApplyMultiDamage(pev, pevOwner);

		if (pOther->IsBSPModel())
		{
			Stay();
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
			Vector knockbackdir = Vector(pev->angles.x, pev->angles.y, 0);
			if (m_Flare == 12) // makes the shotgun knock dead enemies back #swag
			{
				pOther->pev->velocity = pOther->pev->velocity + (knockbackdir * 10);
				pOther->pev->velocity = pOther->pev->velocity + gpGlobals->v_up * 15;
			}
			else if (m_Flare == 357)
			{
				pOther->pev->velocity = pOther->pev->velocity + (knockbackdir * 125);
				pOther->pev->velocity = pOther->pev->velocity + gpGlobals->v_up * 75;
			}
			else if (m_Flare == 556)
			{
				pOther->pev->velocity = pOther->pev->velocity + (knockbackdir * 30);
				pOther->pev->velocity = pOther->pev->velocity + gpGlobals->v_up * 10;
			}
			else // 9MM, also affects rubber bullets and the HC deagle though
			{
				pOther->pev->velocity = pOther->pev->velocity + (knockbackdir * 20);
				pOther->pev->velocity = pOther->pev->velocity + gpGlobals->v_up * 10;
			}
			UTIL_Remove(this);
		}
	}
	else
	{
		Stay();
	}
	DecalGunshot(&tr, BULLET_PLAYER_9MM);
	TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_PLAYER_9MM);

}

void CPhysbullet::AirThink()
{
	pev->nextthink = gpGlobals->time + 0.075; // was 0.05
	if (pev->renderamt < 225)
	{
		pev->renderamt += 75;
	}
	if (pev->waterlevel == 0)
	return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}
#endif
