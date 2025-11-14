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

// UNDONE: Save/restore this?
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity

LINK_ENTITY_TO_CLASS(phys_bullet, CPhysbullet);
void CPhysbullet::BulletCreate(int BLLTamnt, float BLLTDamage, int BLLTSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, float BLLTGravity, int FlareType, edict_t *shooter, bool subsonic, float maxpenoverride)
{
	for (int i = 0; i < BLLTamnt; i++) // Allows multishot
	{
		// Create a new entity with CPhysbullet private data
		CPhysbullet* pBullet = GetClassPtr((CPhysbullet*)NULL);
		pBullet->pev->classname = MAKE_STRING("phys_bullet");
		if (g_iSkillLevel != SKILL_HARD)
		{
			pBullet->m_muzzlevelocity = BLLTSpeed;
		}
		else
		{
			pBullet->m_muzzlevelocity = BLLTSpeed * 1.25f;
		}
		pBullet->m_BulletDamage = BLLTDamage;
		pBullet->m_SpawnPos = VecSpawnPos;
		pBullet->m_direction = vecDir;
		pBullet->m_Spread = vecSpread;
		pBullet->m_SpreadVert = vecSpreadvert; // Shotgun duckbill choke
		pBullet->m_Gravity = BLLTGravity;
		pBullet->m_Flare = FlareType; // tracer type
		pBullet->m_bsubsonic = subsonic;
		pBullet->m_SpreadVect = Vector(RANDOM_FLOAT(pBullet->m_Spread, -pBullet->m_Spread), RANDOM_FLOAT(pBullet->m_Spread, -pBullet->m_Spread), RANDOM_FLOAT(pBullet->m_SpreadVert, -pBullet->m_SpreadVert));
		pBullet->m_fPenoverride = maxpenoverride; // for penetration
		if (shooter != nullptr)
			pBullet->Owner = shooter;
		else
			pBullet->Owner = pBullet->edict();
		pBullet->pev->owner = NULL;

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
	if (m_Flare == 556) // probably 556
	{
		SET_MODEL(ENT(pev), "sprites/tracer_556mm.spr");
		pev->scale = RANDOM_FLOAT(0.23f, 0.27f);
		m_distpenetrate = 24;
	}
	if (m_Flare == 762) // probably 762
	{
		SET_MODEL(ENT(pev), "sprites/tracer_44magnum.spr");
		pev->scale = RANDOM_FLOAT(0.31f, 0.35f);
		m_distpenetrate = 32;
		m_bHeavyDecal = true;
	}
	else if (m_Flare == 12) // 12 gauge
	{
		SET_MODEL(ENT(pev), "sprites/tracer_12g.spr");
		pev->scale = RANDOM_FLOAT(0.13f, 0.17f);
		m_distpenetrate = 10;
	}
	else if (m_Flare == 357)
	{
		SET_MODEL(ENT(pev), "sprites/tracer_357magnum.spr");
		pev->scale = RANDOM_FLOAT(0.28f, 0.32f);
		m_distpenetrate = 18;
		m_bHeavyDecal = true;
	}
	else if (m_Flare == 44)
	{
		SET_MODEL(ENT(pev), "sprites/tracer_44magnum.spr");
		pev->scale = RANDOM_FLOAT(0.32f, 0.33f);
		m_distpenetrate = 16;
		m_bHeavyDecal = true;
	}
	else if (m_Flare == 69) // Training weapons
	{
		SET_MODEL(ENT(pev), "models/rubber_bullet.mdl");
		m_distpenetrate = 2;
		pev->rendermode = kRenderNormal;
	}
	else if (m_Flare == 420) // HC Deagle
	{
		SET_MODEL(ENT(pev), "sprites/tracer_357magnum.spr");
		pev->scale = 2;
		m_distpenetrate = 128;
		pev->rendercolor = Vector(255, 70, 170);
		pev->rendermode = kRenderTransAdd;
		m_bHeavyDecal = true;
	}
	else //	9MM
	{
		SET_MODEL(ENT(pev), "sprites/tracer_9mm.spr");
		pev->scale = RANDOM_FLOAT(0.18f, 0.22f);
		m_distpenetrate = 16;
	}
	if (m_bsubsonic)
		m_distpenetrate = round(m_distpenetrate * 0.75f);

	if (m_fPenoverride != NULL)
		m_distpenetrate = m_fPenoverride;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	if (owner != nullptr) // shouldn't happen since the spawn nullptr check, here Justin Case.
	{
		if (owner->IsPlayer())
		{
			pev->renderamt = 0;
			if (g_iSkillLevel == SKILL_HARD)
			{
				pev->velocity = pev->velocity + owner->pev->velocity;
				UTIL_SetOrigin(pev, m_SpawnPos + m_direction * 6 + gpGlobals->v_right * 5 + gpGlobals->v_up * -4); //spawn a little bit more forward
			}
		}
	}
	if (m_bsubsonic)
		pev->renderamt = 5;
	else if (pev->renderamt != 0)
		pev->renderamt = 150;
	SetTouch(&CPhysbullet::BoltTouch);
	SetThink(&CPhysbullet::AirThink);
	pev->nextthink = gpGlobals->time + 0.05f;
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex()); // entity
	WRITE_SHORT(m_iTrail);	 // model
	WRITE_BYTE(RANDOM_LONG(2, 3));	// life
	WRITE_BYTE(1);			 // width
	WRITE_BYTE(128);		 // r, g, b
	WRITE_BYTE(128);		 // r, g, b
	WRITE_BYTE(128);		 // r, g, b
	WRITE_BYTE(RANDOM_LONG(60,80));	 // brightness
	MESSAGE_END();
}


void CPhysbullet::Precache()
{
	PRECACHE_MODEL("models/rubber_bullet.mdl");
	PRECACHE_MODEL("sprites/tracer_9mm.spr");
	PRECACHE_MODEL("sprites/tracer_556mm.spr");
	PRECACHE_MODEL("sprites/tracer_357magnum.spr");
	PRECACHE_MODEL("sprites/tracer_44magnum.spr");
	PRECACHE_MODEL("sprites/tracer_12g.spr");
	PRECACHE_SOUND("weapons/nearmiss1.wav");
	PRECACHE_SOUND("weapons/nearmiss2.wav");
	PRECACHE_SOUND("weapons/nearmiss3.wav");
	PRECACHE_SOUND("weapons/nearmiss4.wav");
	PRECACHE_SOUND("weapons/nearmiss5.wav");
	PRECACHE_SOUND("weapons/nearmiss6.wav");
	m_iTrail = PRECACHE_MODEL("sprites/RCtrail.spr");
	m_ParticleEvent = PRECACHE_EVENT(1, "events/particles.sc");
}


int CPhysbullet::Classify() // Why is this here?
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
	if (pOther->IsBullet())
		return;
	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	m_Endpos = pev->origin; // where bullet hit
	TraceResult tr = UTIL_GetGlobalTrace();
	TraceResult beam_tr;
	TraceResult beam_tr2;
	CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
	
	if (m_distpenetrate > 0) // penetrate (ask your mother what that means)
	{
		double p;
		int i = 1;

		UTIL_TraceLine(tr.vecEndPos + m_direction * 1, tr.vecEndPos + m_direction * i, dont_ignore_monsters, NULL, &beam_tr2);
		while (1 == beam_tr2.fAllSolid && i <= m_distpenetrate) // Raymarching (works better than the tau cannons trace back method)
		{
			i += 1;
			UTIL_TraceLine(tr.vecEndPos + m_direction * 1, tr.vecEndPos + m_direction * i, dont_ignore_monsters, NULL, &beam_tr2);
			if (i > m_distpenetrate)
			{
				ALERT(at_console, "Wall is too thick! Max Size: %i\n", i);
				break;
			}
		}
		if (0 == beam_tr2.fAllSolid) // Raymarch found da way
		{
			ALERT(at_console, "est wall depth %i\n", i);

			UTIL_TraceLine(beam_tr2.vecEndPos, tr.vecEndPos, dont_ignore_monsters, NULL, &beam_tr); // trace backwards to add exit decal
			m_SpawnPos = beam_tr.vecEndPos;															// where bullet comes out of wall

			// Multiply dist by the penetration multiplier and round to the 3rd or 4th decimal (I forget which)
			p = i * TEXTURETYPE_Penetration(&tr, m_Endpos, m_Endpos + m_direction * i);
			p *= 1000;
			p = round(p);
			p /= 1000;
			
			if (p < m_distpenetrate && m_distpenetrate > 0)
			{
				// Prevent inf penetration
				m_distpenetrate = m_distpenetrate - p;
				if (m_distpenetrate < 0)
					m_distpenetrate = 0;

				ALERT(at_console, "new dist pen %f\n", m_distpenetrate);
				ALERT(at_console, "penetrated: %d units + mult\n", p);

				// Damage reduction
				m_BulletDamage -= round(0.125 * p);
				if (m_BulletDamage <= 0)
					m_BulletDamage = 2;

				// Fire penetrated bullet
				Vector spawnpos = tr.vecEndPos + m_direction * i+1;
				CPhysbullet::BulletCreate(1, m_BulletDamage, m_muzzlevelocity, spawnpos, m_direction, 0, 0, m_Gravity, m_Flare, Owner, m_bsubsonic, m_distpenetrate);

				// Damage
				ClearMultiDamage();
				pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
				pOther->TraceAttack(owner->pev, m_BulletDamage/2, pev->velocity.Normalize(), &beam_tr, DMG_BULLET | DMG_NEVERGIB);
				ApplyMultiDamage(pev, owner->pev);

				// VFX
				DecalGunshot(&tr, BULLET_MONSTER_12MM);		 // Entry decal 12 - mm is the heavy decal
				DecalGunshot(&beam_tr, BULLET_MONSTER_12MM); // Exit decal - 12 mm is the heavy decal
				TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_PLAYER_9MM);

				// Remove original bullet
				UTIL_Remove(this);
				return;
			}
			else
			{
				ALERT(at_console, "too thick! Size: %f units + mult\n", p);
			}
		}
	}
	else
	{
		ALERT(at_console, "pen below 0!\n");
		m_distpenetrate = 0;
	}

	// Did not penetrate, normal collision

	pev->movetype = MOVETYPE_NONE;
	SetTouch(NULL);
	SetThink(NULL);

	if (0 != pOther->pev->takedamage)
	{
		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();
		pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, (m_Flare != 420) ? (DMG_BULLET | DMG_NEVERGIB) : DMG_BULLET);
		ApplyMultiDamage(pev, owner->pev);

		if (!pOther->IsBSPModel())
		{
			// play NPC hit sound
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bullet_hit1.wav", 1, ATTN_NORM);
				break;
			case 1:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bullet_hit2.wav", 1, ATTN_NORM);
				break;
			}
			UTIL_Remove(this);
		}
	}
	else
	{
		if (pOther->IsBSPModel())
		{
			pev->angles = tr.vecPlaneNormal;
			PLAYBACK_EVENT_FULL(0, Owner, m_ParticleEvent, 0.0, tr.vecEndPos + tr.vecPlaneNormal * 0.1f, tr.vecPlaneNormal, 0.0, 0.0, PE_BLLTIMPACTGLOW, 0, 0, 0);
		}
	}
	DecalGunshot(&tr, BULLET_MONSTER_9MM);
	TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_PLAYER_9MM);
	Stay();
}

void CPhysbullet::AirThink()
{
	m_direction = UTIL_VecToAngles(pev->velocity);
	pev->angles = m_direction;
	pev->nextthink = gpGlobals->time + 0.1; // was 0.05f
	CBaseEntity* m_ent = NULL;
	if (!m_haswizzed && !m_bsubsonic)
	{
		while ((m_ent = UTIL_FindEntityInSphere(m_ent, pev->origin, 128)) != NULL)
		{
			CBaseEntity* m_pPlyr = m_ent;
			if (m_pPlyr->IsPlayer())
			{
				if (!m_haswizzed && Owner != m_pPlyr->edict() && !m_bsubsonic)
				{
					char dripsnd[256];
					sprintf(dripsnd, "weapons/nearmiss%d.wav", RANDOM_LONG(1, 6));
					EMIT_SOUND(edict(), CHAN_AUTO, dripsnd, 1, 1);
					m_haswizzed = true;
				}
			}
		}
	}
	if (pev->renderamt < 225 && !m_bsubsonic) // fade in
	{
		pev->renderamt += 75;
	}

	if (pev->waterlevel == 0)
		return;
	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1f, pev->origin, 1);
}

int CPhysbullet::ShouldCollide(CBaseEntity* pentTouched)
{
	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	if (pentTouched == owner || pentTouched->IsBullet())
	{
		return 0;
	}

	return 1;
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
	else if (pEntity->IsMachine(pEntity))
	{
		chTextureType = CHAR_TEX_METAL;
	}
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
			if (!strcmp(szbuffer, "mat_impen")) // HACKHACK: can't get the material type to work so just gonna do this instead
			{
				return penmodifier = 512;
				ALERT(at_console, "penetration mult: %f\n", penmodifier);
			}
			else if (!strcmp(szbuffer, "mat_flesh"))
			{
				return penmodifier = 1.66f;
				ALERT(at_console, "penetration mult: %f\n", penmodifier);
			}
			chTextureType = TEXTURETYPE_Find(szbuffer);
		}
	}

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		penmodifier = 1.33f;
		break;
	case CHAR_TEX_METAL:
		penmodifier = 2;
		break;
	case CHAR_TEX_IMPEN:
		penmodifier = 64;
		break;
	case CHAR_TEX_DIRT:
		penmodifier = 2;
		break;
	case CHAR_TEX_VENT:
		penmodifier = 1.5f;
		break;
	case CHAR_TEX_GRATE:
		penmodifier = 1;
		break;
	case CHAR_TEX_TILE:
		penmodifier = 1.1f;
		break;
	case CHAR_TEX_SLOSH:
		penmodifier = 1.125f;
		break;
	case CHAR_TEX_WOOD:
		penmodifier = 1.25f;
		break;
	case CHAR_TEX_GLASS:
		penmodifier = 1;
		break;
	case CHAR_TEX_COMPUTER:
		penmodifier = 1.125f;
		break;
	case CHAR_TEX_FLESH: // less overpenetration
		penmodifier = 1.75f;
		break;
	}
	ALERT(at_console, "penetration mult: %f\n", penmodifier);
	return penmodifier;
}
