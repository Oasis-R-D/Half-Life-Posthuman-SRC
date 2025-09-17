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
		m_distpenetrate = 10;
		m_maxricochet = 0;
	}
	else if (m_Flare == 357)
	{
		SET_MODEL(ENT(pev), "sprites/tracer_357mm.spr");
		//pev->scale = 0.3;
		pev->scale = RANDOM_FLOAT(0.28, 0.32);
		m_distpenetrate = 18;
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

	if (m_distpenetrate > 0) // penetrate (ask your mother what that means)
	{
		if (pEntity->ReflectGauss())
		{
			Vector vecDest = pev->origin + m_direction * 8192;
			UTIL_TraceLine(tr.vecEndPos + m_direction * 8, vecDest, dont_ignore_monsters, NULL, &beam_tr);
			if (0 == beam_tr.fAllSolid)
			{
				// trace backwards to find exit point
				UTIL_TraceLine(beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, NULL, &beam_tr);

				float p = (beam_tr.vecEndPos - tr.vecEndPos).Length() * TEXTURETYPE_Penetration(&tr, m_SpawnPos, m_Endpos);

				if (p < m_distpenetrate)
				{
					
					m_distpenetrate -= p;
					if (g_iSkillLevel != SKILL_HARD)
					{
						m_BulletDamage -= round(0.25 * p);
					}
					else
					{
						m_BulletDamage -= round(0.75 * p);
					}
					ALERT(at_console, "punch %f\n", p);
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
	pev->nextthink = gpGlobals->time + 0.1; // was 0.05
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

float TEXTURETYPE_Penetration(TraceResult* ptr, Vector vecSrc, Vector vecEnd)
{
	// hit an object, determine how well the bullet can penetrate your mo-

	char chTextureType;
	char szbuffer[64];
	const char* pTextureName;
	float rgfl1[3];
	float rgfl2[3];
	float penmodifier;

	CBaseEntity* pEntity = CBaseEntity::Instance(ptr->pHit);

	chTextureType = 0;

	if (pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	else
	{
		// hit world

		// find texture under strike, get material type

		// copy trace vector into array for trace_texture

		vecSrc.CopyToArray(rgfl1);
		vecEnd.CopyToArray(rgfl2);

		// get texture from entity or world (world is ent(0))
		if (pEntity)
			pTextureName = TRACE_TEXTURE(ENT(pEntity->pev), rgfl1, rgfl2);
		else
			pTextureName = TRACE_TEXTURE(CWorld::World->edict(), rgfl1, rgfl2);

		if (pTextureName)
		{
			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
				pTextureName += 2;

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
				pTextureName++;
			// '}}'
			strcpy(szbuffer, pTextureName);
			szbuffer[CBTEXTURENAMEMAX - 1] = 0;

			//ALERT ( at_console, "texture hit: %s\n", szbuffer);

			chTextureType = TEXTURETYPE_Find(szbuffer);
		}
	}

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		penmodifier = 1.25;
		break;
	case CHAR_TEX_METAL:
		penmodifier = 1.5;
		break;
	case CHAR_TEX_DIRT:
		penmodifier = 2;
		break;
	case CHAR_TEX_VENT:
		penmodifier = 1;
		break;
	case CHAR_TEX_GRATE:
		penmodifier = 1;
		break;
	case CHAR_TEX_TILE:
		penmodifier = 1;
		break;
	case CHAR_TEX_SLOSH:
		penmodifier = 1.125;
		break;
	case CHAR_TEX_WOOD:
		penmodifier = 1.125;
		break;
	case CHAR_TEX_GLASS:
		penmodifier = 0.75;
		break;
	case CHAR_TEX_COMPUTER:
		penmodifier = 1.125;
		break;
	case CHAR_TEX_FLESH: // less overpenetration
		penmodifier = 1.5;
		break;
	}
	ALERT(at_console, "penetration mult: %f\n", penmodifier);
	return penmodifier;
}
