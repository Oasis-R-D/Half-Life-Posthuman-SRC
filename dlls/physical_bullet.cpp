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
#include "soundent.h"

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
		pBullet->m_SpreadVect = Vector(RANDOM_FLOAT(pBullet->m_Spread, -pBullet->m_Spread), RANDOM_FLOAT(pBullet->m_Spread, -pBullet->m_Spread), RANDOM_FLOAT(pBullet->m_SpreadVert, -pBullet->m_SpreadVert));
		pBullet->pev->owner = shooter;
		pBullet->Spawn();
		
	}
}

void CPhysbullet::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE; // makes it have gravity
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos + m_direction * 4); //spawn a little bit more forward
	pev->velocity = (m_direction + m_SpreadVect) * m_muzzlevelocity; // Applies spread and velocity
	pev->speed = m_muzzlevelocity; // I have no fucking clue what the difference between speed and velocity is :3
	pev->gravity = m_Gravity; // sets the gravity (bullet drop)
	pev->angles = m_direction + m_SpreadVect;
	m_haswizzed = false;
	pev->rendercolor = Vector(255, 255, 255);
	pev->rendermode = kRenderTransAdd;	
	if (m_Flare == 556) // probably 556, idk
	{
		SET_MODEL(ENT(pev), "sprites/tracer_556mm.spr");
		//pev->scale = 0.25;
		pev->scale = RANDOM_FLOAT(0.23, 0.27);
		m_distpenetrate = 24;
		m_maxricochet = 2;
	}
	else if (m_Flare == 12) // 12 gauge
	{
		SET_MODEL(ENT(pev), "sprites/tracer_12g.spr");
		//pev->scale = 0.15;
		pev->scale = RANDOM_FLOAT(0.13, 0.17);
		m_distpenetrate = 8;
		m_maxricochet = 0;
	}
	else if (m_Flare == 357)
	{
		SET_MODEL(ENT(pev), "sprites/tracer_357mm.spr");
		//pev->scale = 0.3;
		pev->scale = RANDOM_FLOAT(0.28, 0.32);
		m_distpenetrate = 16;
		m_maxricochet = 3;
	}
	else if (m_Flare == 69) // Training weapons
	{
		SET_MODEL(ENT(pev), "models/rubber_bullet.mdl");
		m_distpenetrate = 1;
		m_maxricochet = 3;
		pev->rendermode = kRenderNormal;
	}
	else if (m_Flare == 420) // HC Deagle
	{
		SET_MODEL(ENT(pev), "sprites/tracer_357mm.spr");
		pev->scale = 2;
		m_distpenetrate = 128;
		m_maxricochet = 5;
		pev->rendercolor = Vector(255, 70, 170);
		pev->rendermode = kRenderTransAdd;
	}
	else //	9MM
	{
		SET_MODEL(ENT(pev), "sprites/tracer_9mm.spr");
		//pev->scale = 0.2;
		pev->scale = RANDOM_FLOAT(0.18, 0.22);
		m_distpenetrate = 16;
		m_maxricochet = 1;
	}
	
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
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
	PRECACHE_SOUND("weapons/nearmiss1.wav");
	PRECACHE_SOUND("weapons/nearmiss2.wav");
	PRECACHE_SOUND("weapons/nearmiss3.wav");
	PRECACHE_SOUND("weapons/nearmiss4.wav");
	PRECACHE_SOUND("weapons/nearmiss5.wav");
	PRECACHE_SOUND("weapons/nearmiss6.wav");
}


int CPhysbullet::Classify()
{
	return CLASS_NONE;
}
void CPhysbullet::Stay()
{
	pev->velocity = Vector(0, 0, 0);
	pev->avelocity.z = 0;
	SetThink(&CPhysbullet::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
void CPhysbullet::BoltTouch(CBaseEntity* pOther)
{	
	entvars_t* pevOwner;
	pevOwner = VARS(pev->owner);
	TraceResult tr = UTIL_GetGlobalTrace();
	TraceResult beam_tr;
	CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

	float n;
	n = -DotProduct(tr.vecPlaneNormal, m_direction);

	if (n < 0.25 && m_maxricochet > 0 && RANDOM_LONG(0, 2) == 2) // not 60 degrees
	{
		if (pEntity->IsBSPModel())
		{
			m_maxricochet--;
			ALERT( at_console, "reflect %f\n", n );
			// reflect
			Vector r;

			r = 2.0 * tr.vecPlaneNormal * n + m_direction;
			m_direction = r;
			pev->origin = tr.vecEndPos + m_direction * 8;

			// explode a bit
			RadiusDamage(tr.vecEndPos, pev, pevOwner, 10, 16, CLASS_NONE, DMG_BLAST);

			// lose energy
			if (n == 0)
				n = 0.1;
			m_BulletDamage *= (1 - n);
			ClearMultiDamage();
			pOther->TraceAttack(pevOwner, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
			ApplyMultiDamage(pev, pevOwner);
			DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);
			TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_PLAYER_9MM);
			pev->velocity = pev->velocity * RANDOM_FLOAT(0.65, 0.85);
			return;
		}
	}
	else if (m_distpenetrate > 0)// penetrate (ask your mother what that means)
	{
		Vector vecDest = pev->origin + m_direction * 8192;
		UTIL_TraceLine(tr.vecEndPos + m_direction * 8, vecDest, dont_ignore_monsters, NULL, &beam_tr);
		if (0 == beam_tr.fAllSolid)
		{
			// trace backwards to find exit point
			UTIL_TraceLine(beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, NULL, &beam_tr);

			float p = (beam_tr.vecEndPos - tr.vecEndPos).Length();

			if (p < m_distpenetrate)
			{
				if (p == 0)
					p = 1;
				m_BulletDamage /= 1.5;
				m_distpenetrate -= p;
				ALERT( at_console, "punch %f\n", p );
				CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
				pev->origin = beam_tr.vecEndPos;
				ClearMultiDamage();
				pOther->TraceAttack(pevOwner, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
				ApplyMultiDamage(pev, pevOwner);
				DecalGunshot(&tr, BULLET_PLAYER_9MM);
				TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_PLAYER_9MM);
				return;
			}
		}
	}
	pev->movetype = MOVETYPE_NONE;
	SetTouch(NULL);
	SetThink(NULL);

	if (0 != pOther->pev->takedamage)
	{
		
		m_Endpos = pev->origin;
		

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
	CBaseEntity* m_ent = NULL;
	while ((m_ent = UTIL_FindEntityInSphere(m_ent, pev->origin, 128)) != NULL)
	{
		CBaseEntity* m_pPlyr = m_ent;
		if (m_pPlyr->IsPlayer())
		{
			if (m_haswizzed != true && pev->owner != m_pPlyr->edict())
			{
				char dripsnd[256];
				sprintf(dripsnd, "weapons/nearmiss%d.wav", RANDOM_LONG(1, 6));
				EMIT_SOUND(edict(), CHAN_AUTO, dripsnd, 1, 1);
				m_haswizzed = true;
			}
		}
	}
	
	if (pev->renderamt < 225)
	{
		pev->renderamt += 75;
	}
	if (pev->waterlevel == 0)
	return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}
#endif
