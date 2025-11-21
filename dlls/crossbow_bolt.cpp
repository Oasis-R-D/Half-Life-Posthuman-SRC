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
#include "railcannon_bolt.h"

// UNDONE: Save/restore this?  Don't forget to set classname

// OVERLOADS SOME ENTVARS:
// speed - the ideal magnitude of my velocity

LINK_ENTITY_TO_CLASS(crossbow_bolt, CCrossbowBolt);

CCrossbowBolt* CCrossbowBolt::BoltCreate(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner)
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt* pBolt = GetClassPtr((CCrossbowBolt*)NULL);
	pBolt->pev->classname = MAKE_STRING("bolt");
	pBolt->pev->origin = vecOrigin;
	pBolt->pev->angles = vecAngles;
	pBolt->pev->owner = pOwner->edict();
	pBolt->Spawn();

	return pBolt;
}

void CCrossbowBolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CCrossbowBolt::BoltTouch);
	SetThink(&CCrossbowBolt::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;

	// trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex()); // entity
	WRITE_SHORT(m_iTrail);	 // model
	WRITE_BYTE(10);			 // life
	WRITE_BYTE(2);			 // width
	WRITE_BYTE(128);		 // r, g, b
	WRITE_BYTE(128);		 // r, g, b
	WRITE_BYTE(128);		 // r, g, b
	WRITE_BYTE(128);		 // brightness
	MESSAGE_END();
}


void CCrossbowBolt::Precache()
{
	PRECACHE_MODEL("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	PRECACHE_SOUND("weapons/bolt_impact.wav");
	PRECACHE_SOUND("weapons/bolt_impact_con.wav");
	m_iTrail = PRECACHE_MODEL("sprites/RCtrail.spr");
}

int CCrossbowBolt::Classify()
{
	return CLASS_NONE;
}

void CCrossbowBolt::Stay()
{
	//EMIT_SOUND_DYN(ENT(pev), CHAN_AUTO, "weapons/bolt_impact_con.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));
	Vector vecDir = pev->velocity.Normalize();
	UTIL_SetOrigin(pev, pev->origin - vecDir * 12);

	pev->angles = UTIL_VecToAngles(vecDir);
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	pev->velocity = Vector(0, 0, 0);
	pev->avelocity.z = 0;
	pev->angles.z = RANDOM_LONG(0, 360);
	pev->nextthink = gpGlobals->time + 1.5;
	//pev->renderfx = kRenderFxGlowShell;
	//pev->rendercolor = Vector(0, 128, 128);

	SetThink(&CCrossbowBolt::ExplodeThink);

	TraceResult tr = UTIL_GetGlobalTrace();
	UTIL_DecalTrace(&tr, RANDOM_LONG(28, 32));
	UTIL_BloodPuff(tr, DONT_BLEED);
}

void CCrossbowBolt::BoltTouch(CBaseEntity* pOther)
{
	SetTouch(NULL);
	SetThink(NULL);
	EMIT_SOUND(edict(), CHAN_AUTO, "weapons/bolt_impact.wav", 1, ATTN_NORM);
	if (pOther->pev->takedamage != DAMAGE_NO)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t* pevOwner = VARS(pev->owner);
		if ((pOther->pev->flags & FL_MONSTER) != 0)
		{
			auto monster = pOther->MyMonsterPointer();
			if (monster->pev->deadflag == DEAD_DEAD)
			{
				monster->pev->nextthink = gpGlobals->time + 0.1; // keep monster thinking.
				monster->SetThink(&CBaseMonster::DeadMonsterThink);
			}
			monster->m_bRailed = true;
			monster->m_flRailChargeTime = gpGlobals->time + 1.5;
		}
		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, 30, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
		ApplyMultiDamage(pev, pevOwner);
		if (pOther->IsBSPModel())
			Stay();
		else
		{
			UTIL_Remove(this);
		}
	}
	else
	{
		SetThink(&CCrossbowBolt::SUB_Remove);
		pev->nextthink = gpGlobals->time; // this will get changed below if the bolt is allowed to stick in what it hit.
		if (FClassnameIs(pOther->pev, "worldspawn"))
			Stay();
	}
	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		UTIL_Sparks(pev->origin);
}

void CCrossbowBolt::BubbleThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CCrossbowBolt::ExplodeThink()
{
	int iContents = UTIL_PointContents(pev->origin);
	int iScale;

	pev->dmg = 30;
	iScale = 10;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15);		// framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	entvars_t* pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, pev->dmg, 96, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB); // TO-DO: make full radius do full damage instead of falling off

	UTIL_Remove(this);
}
