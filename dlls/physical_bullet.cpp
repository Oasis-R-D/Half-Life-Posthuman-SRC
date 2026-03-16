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

/*
* 9MM MV: 6000 // REALISM: 10.0-15.0k
* 556 MV: 7000 // REALISM: 32.4-39.6k
* 357 MV: 7500 // REALISM: 14.4-18.0k
* 12G MV: 5750 // REALISM: 14.4-19.2k
* 44M MV: 6000 // REALISM: 14.4-21.6k
* RUB MV: ???? // REALISM: 4k
* NOTE: The game CAN handle realistic values but they aren't very fun (practically just hitscan)
* NOTE: Realism values calculated by multiplying IRL FeetPS by 12 (IPS) (assuming Inches = HU)
*/

// UNDONE: Save/restore this?

// OVERLOADS SOME ENTVARS:
// speed - the ideal magnitude of my velocity

LINK_ENTITY_TO_CLASS(phys_bullet, CPhysbullet);
void CPhysbullet::BulletCreate(int BLLTamnt, float BLLTDamage, int BLLTSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, float BLLTGravity, int FlareType, edict_t *shooter, bool subsonic, float maxpenoverride)
{
	for (int i = 0; i < BLLTamnt; i++) // Allows multishot
	{
		// Create a new entity with CPhysbullet private data
		CPhysbullet* pBullet = GetClassPtr((CPhysbullet*)NULL);
		pBullet->pev->classname = MAKE_STRING("phys_bullet");
		pBullet->m_muzzlevelocity = (g_iSkillLevel != SKILL_HARD) ? BLLTSpeed : BLLTSpeed * 1.25f;
		pBullet->m_BulletDamage = BLLTDamage;
		pBullet->m_SpawnPos = VecSpawnPos;
		pBullet->m_direction = vecDir;
		pBullet->m_Spread = vecSpread;
		pBullet->m_SpreadVert = vecSpreadvert; // Shotgun duckbill choke
		pBullet->m_Gravity = BLLTGravity;
		pBullet->m_Flare = FlareType; // tracer type
		pBullet->m_bsubsonic = subsonic;
		pBullet->m_fPenoverride = maxpenoverride; // for penetration
		pBullet->Owner = (shooter != nullptr) ? shooter : pBullet->edict();
		pBullet->pev->owner = NULL;

		pBullet->Spawn();
	}
}

const char* CPhysbullet::pNearMissSounds[] =
	{
		"weapons/nearmiss1.wav",
		"weapons/nearmiss2.wav",
		"weapons/nearmiss3.wav",
		"weapons/nearmiss4.wav",
		"weapons/nearmiss5.wav",
		"weapons/nearmiss6.wav",
};

void CPhysbullet::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE; // makes it have gravity

	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos); // TO-DO: now that there's no +4 in direction, will need to delay the trail starting somehow

	float x, y, z;
	do
	{
		x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		z = x * x + y * y;
	} while (z > 1);

	m_direction = m_direction + x * m_Spread * gpGlobals->v_right + y * m_SpreadVert * gpGlobals->v_up;
					
	pev->velocity = m_direction * m_muzzlevelocity; // Applies spread and velocity
	pev->gravity = m_Gravity; // sets the gravity (bullet drop)
	pev->angles = m_direction;

	m_haswizzed = false;

	pev->rendercolor = Vector(255, 255, 255);
	pev->rendermode = kRenderTransAdd;	

	switch(m_Flare)
	{
		case 556:
			SET_MODEL(ENT(pev), "sprites/tracer_556mm.spr");
			pev->scale = RANDOM_FLOAT(0.23f, 0.27f);
			m_distpenetrate = 24;
			break;
		case 762:
			SET_MODEL(ENT(pev), "sprites/tracer_44magnum.spr");
			pev->scale = RANDOM_FLOAT(0.31f, 0.35f);
			m_distpenetrate = 32;
			m_bHeavyDecal = true;
			break;
		case 12:
			SET_MODEL(ENT(pev), "sprites/tracer_12g.spr");
			pev->scale = RANDOM_FLOAT(0.13f, 0.17f);
			m_distpenetrate = 10;
			break;
		case 357:
			SET_MODEL(ENT(pev), "sprites/tracer_357magnum.spr");
			pev->scale = RANDOM_FLOAT(0.28f, 0.32f);
			m_distpenetrate = 18;
			m_bHeavyDecal = true;
			break;
		case 44:
			SET_MODEL(ENT(pev), "sprites/tracer_44magnum.spr");
			pev->scale = RANDOM_FLOAT(0.32f, 0.33f);
			m_distpenetrate = 16;
			m_bHeavyDecal = true;
			break;
		case 69:
			SET_MODEL(ENT(pev), "models/rubber_bullet.mdl");
			m_distpenetrate = 0;
			pev->rendermode = kRenderNormal;
			break;
		case 420:
			SET_MODEL(ENT(pev), "sprites/tracer_357magnum.spr");
			pev->scale = 1.0;
			m_distpenetrate = 128;
			pev->rendercolor = Vector(255, 70, 170);
			pev->rendermode = kRenderTransAdd;
			m_bHeavyDecal = true;
			break;
		default:
		case 9:
			SET_MODEL(ENT(pev), "sprites/tracer_9mm.spr");
			pev->scale = RANDOM_FLOAT(0.18f, 0.22f);
			m_distpenetrate = 16;
			break;
	}

	if (m_bsubsonic)
		m_distpenetrate = round(m_distpenetrate * 0.75f);

	if (m_fPenoverride != NULL)
		m_distpenetrate = m_fPenoverride;

	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	if (owner != nullptr) // shouldn't happen since the spawn nullptr check, here Justin Case.
	{
		if (owner->IsPlayer())
		{
			pev->renderamt = 0;
			if (g_iSkillLevel == SKILL_HARD && m_Flare != 44)
			{
				pev->velocity = pev->velocity + owner->pev->velocity;
			}
		}
	}

	if (m_bsubsonic)
		pev->renderamt = 5;
	else if (pev->renderamt != 0)
		pev->renderamt = 150;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	SetTouch(&CPhysbullet::BoltTouch);
	SetThink(&CPhysbullet::AirThink);
	if (!g_pGameRules->IsMultiplayer() || true)
	{
		// TRAIL START
		pev->nextthink = gpGlobals->time + 0.05f;
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_BEAMFOLLOW);
		WRITE_SHORT(entindex());		 // entity
		WRITE_SHORT(m_iTrail);			 // model
		WRITE_BYTE(RANDOM_LONG(2, 3));	 // life
		WRITE_BYTE(1);					 // width
		WRITE_BYTE(128);				 // r, g, b
		WRITE_BYTE(128);				 // r, g, b
		WRITE_BYTE(128);				 // r, g, b
		WRITE_BYTE(RANDOM_LONG(60, 80)); // brightness
		MESSAGE_END();
	}
}


void CPhysbullet::Precache()
{
	PRECACHE_MODEL("models/rubber_bullet.mdl");
	PRECACHE_MODEL("sprites/tracer_9mm.spr");
	PRECACHE_MODEL("sprites/tracer_556mm.spr");
	PRECACHE_MODEL("sprites/tracer_357magnum.spr");
	PRECACHE_MODEL("sprites/tracer_44magnum.spr");
	PRECACHE_MODEL("sprites/tracer_12g.spr");
	m_iTrail = PRECACHE_MODEL("sprites/RCtrail.spr");
	
	PRECACHE_SOUND_ARRAY(pNearMissSounds);

	PRECACHE_SOUND("bullet/imp_metal01.wav");

	PRECACHE_SOUND("debris/glassshatter1.wav");
	PRECACHE_SOUND("debris/glassshatter2.wav");
	PRECACHE_SOUND("debris/glassshatter3.wav");

	PRECACHE_SOUND("bullet/imp_wood01.wav"); // TO-DO: better sound

	PRECACHE_SOUND("bullet/imp_dirt01.wav");
}


/*int CPhysbullet::Classify() // Why is this here? // could be used to make npcs fear bullets?
{
	return CLASS_NONE;
}*/

void CPhysbullet::BoltTouch(CBaseEntity* pOther)
{	
	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	if (owner == nullptr)
	{
		owner = this; // fixes RC crash
		Owner = this->edict();
	}

	m_Endpos = pev->origin; // where bullet hit
	TraceResult tr = UTIL_GetGlobalTrace();

	if (m_distpenetrate > 0) // penetrate (ask your mother what that means)
	{
		TraceResult beam_tr;
		TraceResult beam_tr2;
		double p;
		int i = 1;

		UTIL_TraceLine(tr.vecEndPos + m_direction * 1, tr.vecEndPos + m_direction * i, dont_ignore_monsters, NULL, &beam_tr2);
		while (1 == beam_tr2.fAllSolid && i <= m_distpenetrate) // Raymarching (works better than the tau cannons trace back method, but still sucks ass)
		{
			i += 1;
			UTIL_TraceLine(tr.vecEndPos + m_direction * 1, tr.vecEndPos + m_direction * i, dont_ignore_monsters, NULL, &beam_tr2);
			if (i > m_distpenetrate)
				break;
		}
		if (0 == beam_tr2.fAllSolid) // Raymarch found da way // BUGBUG!! the while loop just skips and returns 1 if the player is far from the end point
		{
			//ALERT(at_console, "est wall depth %i\n", i);

			UTIL_TraceLine(beam_tr2.vecEndPos, tr.vecEndPos, dont_ignore_monsters, NULL, &beam_tr); // trace backwards to add exit decal
			m_SpawnPos = beam_tr.vecEndPos;															// where bullet comes out of wall

			// Multiply dist by the penetration multiplier and round to the 3rd or 4th decimal (I forget which)
			p = i * TEXTURETYPE_Penetration(&tr, tr.vecEndPos, tr.vecEndPos + m_direction * i); // m_direction seems to be innacurate
			p *= 1000;
			p = round(p);
			p /= 1000;
			
			if (p < m_distpenetrate && m_distpenetrate > 0)
			{
				// Prevent inf penetration
				m_distpenetrate = m_distpenetrate - p;
				if (m_distpenetrate < 0)
					m_distpenetrate = 0;

				//ALERT(at_console, "new dist pen %f\n", m_distpenetrate);
				//ALERT(at_console, "penetrated: %f units + mult\n", p);

				// Damage reduction
				m_BulletDamage -= round(0.125 * p);
				if (m_BulletDamage <= 0)
					m_BulletDamage = 2;

				// Fire penetrated bullet
				Vector spawnpos = tr.vecEndPos + (m_direction * (i+1)); // use beam_tr2?
				CPhysbullet::BulletCreate(1, m_BulletDamage, m_muzzlevelocity, spawnpos, m_direction, 0, 0, m_Gravity, m_Flare, Owner, m_bsubsonic, m_distpenetrate);

				// Damage
				ClearMultiDamage();
				pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
				ApplyMultiDamage(pev, owner->pev);
				
				/*
				if (pOther->BloodColor() != DONT_BLEED && !g_pGameRules->IsMultiplayer())
				{
					int BLDAMNT;
					Vector vecOrigin = beam_tr.vecEndPos - (-m_direction) * 4;
					BLDAMNT = round(m_BulletDamage / 2);
					SpawnBlood(vecOrigin, pOther->BloodColor(), m_BulletDamage/2); // a little surface blood.
					TraceBleed(m_BulletDamage/2, -m_direction, &beam_tr, DMG_BULLET);
				}
				*/

				// VFX
				DecalGunshot(&tr, BULLET_MONSTER_12MM);		 // Entry decal  - 12mm is the heavy decal
				DecalGunshot(&beam_tr, BULLET_MONSTER_12MM); // Exit decal - 12 mm is the heavy decal
				TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_MONSTER_12MM);

				// Remove original bullet
				UTIL_Remove(this);
				return;
			}
			/*else
			{
				ALERT(at_console, "too thick! Size: %f units + mult\n", p);
			}*/
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
	char material = TEXTURETYPE_PlaySound(&tr, tr.vecEndPos, tr.vecEndPos + m_direction * 2, BULLET_PLAYER_9MM);
	if (0 != pOther->pev->takedamage)
	{
		ClearMultiDamage();
		pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, (m_Flare != 420) ? (DMG_BULLET | DMG_NEVERGIB) : DMG_BULLET);
		ApplyMultiDamage(pev, owner->pev);
	}
	else
	{
		if (pOther->IsBSPModel())
		{
			//ALERT(at_console, "char is %c \0", material);

			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
			WRITE_BYTE(TE_IMPACTVFX);
			WRITE_COORD_VECTOR(tr.vecEndPos + (-1 * pev->velocity.Normalize()) * 0.1f); // org
			WRITE_COORD_VECTOR(-1 * pev->velocity.Normalize());							// dir
			WRITE_COORD(0);
			WRITE_COORD(0);
			WRITE_BYTE(material); // (count) (use as mat type?)
			WRITE_BYTE(0); // (bullethole decal texture index)
			MESSAGE_END();
			
			if (pev->waterlevel == 0)
			{
				pev->angles = tr.vecPlaneNormal;
				PLAYBACK_EVENT_FULL(0, Owner, g_sParticleEvent, 0.0, tr.vecEndPos + tr.vecPlaneNormal * 0.5f, tr.vecPlaneNormal, 0.0, 0.0, PE_BLLTIMPACTGLOW, 0, 0, 0);
			}
		}
	}

	DecalGunshot(&tr, BULLET_MONSTER_9MM);
	
	UTIL_Remove(this);
}

void CPhysbullet::AirThink()
{
	pev->angles = m_direction;
	pev->nextthink = gpGlobals->time + 0.1; // was 0.05f
	
	CBaseEntity* m_ent = NULL;
	if (!m_haswizzed && !m_bsubsonic)
	{
		while ((m_ent = UTIL_FindEntityInSphere(m_ent, pev->origin, 128)) != NULL)
		{
			if (m_ent->IsPlayer() && Owner != m_ent->edict())
			{
				EMIT_SOUND_DYN(edict(), CHAN_AUTO, RANDOM_SOUND_ARRAY(pNearMissSounds), 1.0, 1, 0, 100 + RANDOM_LONG(-5, 5));
				m_haswizzed = true; // will not play twice if it passes 2 players, fix?
			}
		}
	}

	if (pev->renderamt < 225 && !m_bsubsonic) // fade in
	{
		pev->renderamt += 75;
	}

	// WIND
	float WINDxvel, flwindmult;

	for (int i = 0; i < 3; i++)
	{
		WINDxvel = 10;

		flwindmult = 8;
		pev->velocity[i] += sin((gpGlobals->time * flwindmult)) * WINDxvel;
	}
	// WIND END

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
				ALERT(at_console, "penetration mult: %f\n", 512);
				return penmodifier = 512;
				
			}
			else if (!strcmp(szbuffer, "mat_flesh"))
			{
				ALERT(at_console, "penetration mult: %f\n", 1.66);
				return penmodifier = 1.75f;
				
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
