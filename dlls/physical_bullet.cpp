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

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
LINK_ENTITY_TO_CLASS(phys_bullet, CPhysbullet);
void CPhysbullet::BulletCreate(int BLLTamnt, float BLLTDamage, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, int FlareType, edict_t *shooter)
{
	for (int i = 0; i < BLLTamnt; i++)
	{
		// Create a new entity with CPhysbullet private data
		CPhysbullet* pBullet = GetClassPtr((CPhysbullet*)NULL);
		pBullet->pev->classname = MAKE_STRING("bullet");
		// BLLTamnt not turned into a variable since it's only used here
		pBullet->m_BulletAmount = BLLTamnt;
		pBullet->m_BulletDamage = BLLTDamage;
		pBullet->m_SpawnPos = VecSpawnPos;
		pBullet->m_direction = vecDir;
		pBullet->m_Spread = vecSpread;
		pBullet->m_SpreadVert = vecSpreadvert;
		pBullet->m_Flare = FlareType; // tracer type
		pBullet->Spawn();
		pBullet->pev->owner = shooter;
	}
}

void CPhysbullet::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos + m_direction * 4); //spawn a little bit more forward
	pev->velocity = (m_direction + Vector(RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_Spread, -m_Spread), RANDOM_FLOAT(m_SpreadVert, -m_SpreadVert))) * BOLT_AIR_VELOCITY;
	pev->speed = BOLT_AIR_VELOCITY;
	pev->gravity = 0.66;
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
	else if (m_Flare == 69)
	{
		//rubber bullet
	}
	else //	9MM
	{
		SET_MODEL(ENT(pev), "sprites/tracer_9mm.spr");
		//pev->scale = 0.2;
		pev->scale = RANDOM_FLOAT(0.18, 0.22);
	}
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	pev->rendercolor = Vector(255, 255, 255);
	pev->renderamt = 255;
	pev->rendermode = kRenderTransAdd;
	SetTouch(&CPhysbullet::BoltTouch);
	SetThink(&CPhysbullet::AirThink);
	pev->nextthink = gpGlobals->time + 0.2;
}


void CPhysbullet::Precache()
{
	PRECACHE_MODEL("sprites/tracer_9mm.spr");
	PRECACHE_MODEL("sprites/tracer_556mm.spr");
	PRECACHE_MODEL("sprites/tracer_12g.spr");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("fvox/beep.wav"); // why is this here
}


int CPhysbullet::Classify()
{
	return CLASS_NONE;
}
void CPhysbullet::Stay() //TO-DO: add imapct sounds
{
	Vector vecDir = pev->velocity.Normalize();

	pev->angles = UTIL_VecToAngles(vecDir);
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	pev->velocity = Vector(0, 0, 0);
	pev->avelocity.z = 0;
	pev->angles.z = RANDOM_LONG(0, 360);
	
}
void CPhysbullet::BoltTouch(CBaseEntity* pOther)
{
	SetTouch(NULL);
	SetThink(NULL);

	if (0 != pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t* pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		pOther->TraceAttack(pevOwner, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
	

		ApplyMultiDamage(pev, pevOwner);

		pev->velocity = Vector(0, 0, 0);
		if (pOther->IsBSPModel())
			Stay();
		else
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
		SetThink(&CPhysbullet::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
	else 
	{
		SetThink(&CPhysbullet::SUB_Remove);
		pev->nextthink = gpGlobals->time;
		if (FClassnameIs(pOther->pev, "worldspawn"))
			Stay();
	}
	TraceResult tr = UTIL_GetGlobalTrace();
	DecalGunshot(&tr, BULLET_PLAYER_9MM);
	TEXTURETYPE_PlaySound(&tr, m_SpawnPos, pev->origin, BULLET_PLAYER_9MM);

}

void CPhysbullet::AirThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
	return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}
#endif
