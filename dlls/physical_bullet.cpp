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
void CPhysbullet::BulletCreate(unsigned int BLLTamnt, unsigned int BLLTdamage, unsigned int BLLTspeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, float BLLTGravity, int BLLTtype, edict_t *shooter, bool subsonic, float maxpenoverride, CBaseEntity* pIgnore)
{
	for (unsigned int i = 0; i < BLLTamnt; i++) // Allows multishot
	{
		// Create a new entity with CPhysbullet private data
		CPhysbullet* pBullet = GetClassPtr((CPhysbullet*)NULL);
		pBullet->pev->classname = MAKE_STRING("phys_bullet");
		pBullet->m_iMuzzleVel = BLLTspeed;
		pBullet->m_BulletDamage = BLLTdamage;
		pBullet->m_SpawnPos = VecSpawnPos;
		pBullet->m_vecDir = vecDir;
		pBullet->m_Spread = vecSpread;
		pBullet->m_SpreadVert = vecSpreadvert; // Shotgun duckbill choke
		pBullet->m_Gravity = BLLTGravity;
		pBullet->m_Flare = BLLTtype; // tracer type
		pBullet->m_bsubsonic = subsonic;
		pBullet->m_fPenoverride = maxpenoverride; // for penetration
		pBullet->Owner = (shooter != nullptr) ? shooter : pBullet->edict();
		pBullet->m_pIgnore = (pIgnore != nullptr) ? pIgnore : pBullet;
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

	pev->movetype = MOVETYPE_TOSS; // makes it have gravity

	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, m_SpawnPos);

	float x, y, z;
	do
	{
		x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		z = x * x + y * y;
	} while (z > 1);

	m_vecDir = m_vecDir + x * m_Spread * gpGlobals->v_right + y * m_SpreadVert * gpGlobals->v_up;
					
	pev->velocity = m_vecDir * m_iMuzzleVel; // Applies spread and velocity
	pev->gravity = m_Gravity; // sets the gravity (bullet drop)
	pev->angles = UTIL_VecToAngles(m_vecDir);

	pev->rendercolor = Vector(255, 255, 255);
	pev->rendermode = kRenderTransAdd;	

	switch(m_Flare)
	{
		case 556:
			if (CVAR_GET_FLOAT("sv_classictracers") >= 1)
			{
				SET_MODEL(ENT(pev), "sprites/tracer_classic.spr");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/tracer_556mm.spr");
				pev->scale = RANDOM_FLOAT(0.23f, 0.27f);
			}
			
			m_flPenetrationPow = 24;
			break;
		case 762:
			if (CVAR_GET_FLOAT("sv_classictracers") >= 1)
			{
				SET_MODEL(ENT(pev), "sprites/tracer_classic.spr");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/tracer_44magnum.spr");
				pev->scale = RANDOM_FLOAT(0.31f, 0.35f);
			}

			m_flPenetrationPow = 32;
			break;
		case 12:
			if (CVAR_GET_FLOAT("sv_classictracers") >= 1)
			{
				SET_MODEL(ENT(pev), "sprites/tracer_classic.spr");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/tracer_12g.spr");
				pev->scale = RANDOM_FLOAT(0.13f, 0.17f);
			}

			m_flPenetrationPow = 10;
			break;
		case 357:
			if (CVAR_GET_FLOAT("sv_classictracers") >= 1)
			{
				SET_MODEL(ENT(pev), "sprites/tracer_classic.spr");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/tracer_357magnum.spr");
				pev->scale = RANDOM_FLOAT(0.28f, 0.32f);
			}

			m_flPenetrationPow = 18;
			break;
		case 44:
			if (CVAR_GET_FLOAT("sv_classictracers") >= 1)
			{
				SET_MODEL(ENT(pev), "sprites/tracer_classic.spr");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/tracer_44magnum.spr");
				pev->scale = RANDOM_FLOAT(0.32f, 0.33f);
			}

			m_flPenetrationPow = 16;
			break;
		case 69:
			SET_MODEL(ENT(pev), "models/rubber_bullet.mdl");
			m_flPenetrationPow = 0;
			pev->rendermode = kRenderNormal;
			break;
		case 420:
			if (CVAR_GET_FLOAT("sv_classictracers") >= 1)
			{
				SET_MODEL(ENT(pev), "sprites/tracer_classic.spr");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/tracer_357magnum.spr");
				pev->scale = 0.5;
			}

			m_flPenetrationPow = 128;
			pev->rendercolor = Vector(255, 70, 170);
			break;
		case 10:
			if (CVAR_GET_FLOAT("sv_classictracers") >= 1)
			{
				SET_MODEL(ENT(pev), "sprites/tracer_classic.spr");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/tracer_9mm.spr");
				pev->scale = RANDOM_FLOAT(0.20f, 0.22f);
			}

			m_flPenetrationPow = 18;
			break;
		default:
		case 9:
			if (CVAR_GET_FLOAT("sv_classictracers") >= 1)
			{
				SET_MODEL(ENT(pev), "sprites/tracer_classic.spr");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/tracer_9mm.spr");
				pev->scale = RANDOM_FLOAT(0.18f, 0.22f);
			}

			m_flPenetrationPow = 16;
			break;
	}

	if (m_bsubsonic)
		m_flPenetrationPow = round(m_flPenetrationPow * 0.75f);

	if (m_fPenoverride != NULL)
		m_flPenetrationPow = m_fPenoverride;

	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	if (owner != nullptr && owner->IsPlayer()) // shouldn't happen since the spawn nullptr check, here Justin Case.
	{
		pev->renderamt = 0;
	}

	if (m_bsubsonic)
		pev->renderamt = 5;
	else if (pev->renderamt != 0)
		pev->renderamt = 150;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	SetTouch(&CPhysbullet::BulletImpact);
	SetThink(&CPhysbullet::AirThink);

	pev->nextthink = gpGlobals->time + 0.05f;

	// TRAIL START
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

void CPhysbullet::Precache()
{
	PRECACHE_MODEL("models/rubber_bullet.mdl");
	PRECACHE_MODEL("sprites/tracer_9mm.spr");
	PRECACHE_MODEL("sprites/tracer_556mm.spr");
	PRECACHE_MODEL("sprites/tracer_357magnum.spr");
	PRECACHE_MODEL("sprites/tracer_44magnum.spr");
	PRECACHE_MODEL("sprites/tracer_12g.spr");
	PRECACHE_MODEL("sprites/tracer_classic.spr"); // TFC laser pistol projectile
	m_iTrail = PRECACHE_MODEL("sprites/RCtrail.spr");
	
	PRECACHE_SOUND_ARRAY(pNearMissSounds);

	PRECACHE_SOUND("bullet/imp_metal01.wav");

	PRECACHE_SOUND("debris/glassshatter1.wav");
	PRECACHE_SOUND("debris/glassshatter2.wav");
	PRECACHE_SOUND("debris/glassshatter3.wav");

	PRECACHE_SOUND("bullet/imp_wood01.wav"); // TO-DO: better sound

	PRECACHE_SOUND("bullet/imp_dirt01.wav");
}

void CPhysbullet::BulletImpact(CBaseEntity* pOther)
{	
	TraceResult tr = UTIL_GetGlobalTrace();

	if (UTIL_PointContents(tr.vecEndPos) == CONTENTS_SKY)
	{
		UTIL_Remove(this);
		return;
	}

	m_Endpos = pev->origin; // where bullet hit

	// Add water hit VFX
	if (pev->waterlevel != 0 || UTIL_PointContents(m_Endpos - m_vecDir * 1) == CONTENTS_WATER)
		FindWaterSurface();

	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	if (owner == nullptr)
	{
		owner = this; // fixes RC crash
		Owner = this->edict();
	}

	// penetrate
	if (m_flPenetrationPow > 0) 
	{
		TraceResult beam_tr;
		TraceResult beam_tr2;
		double p;
		int i = 0;

		do // Raymarching (works better than the tau cannons trace back method)
		{
			i += 1;
			UTIL_TraceLine(tr.vecEndPos + m_vecDir * 1, tr.vecEndPos + m_vecDir * i, dont_ignore_monsters, dont_ignore_glass, NULL, &beam_tr2);
		}
		while (1 == beam_tr2.fAllSolid && i <= m_flPenetrationPow);

		if (0 == beam_tr2.fAllSolid) // Raymarch found da way
		{
			//ALERT(at_console, "\nest wall depth %i\n", i);

			UTIL_TraceLine(beam_tr2.vecEndPos, tr.vecEndPos, dont_ignore_monsters, NULL, &beam_tr); // trace backwards to add exit decal
			m_SpawnPos = beam_tr.vecEndPos;															// where bullet comes out of wall

			// Multiply dist by the penetration multiplier
			float mat_mult = TEXTURETYPE_Penetration(&tr, tr.vecEndPos, tr.vecEndPos + m_vecDir * i);
			p = i * mat_mult;
			
			if (p < m_flPenetrationPow && m_flPenetrationPow > 0)
			{
				// Prevent inf penetration
				m_flPenetrationPow = V_max(m_flPenetrationPow - p, 0);
				m_BulletDamage = V_max(m_BulletDamage - round(0.125 * p), 2);
				m_iMuzzleVel = V_max(m_iMuzzleVel - 10 * p, 1000);

				if (pev->gravity < 75)
					pev->gravity *= 3 * mat_mult;

				//CPhysbullet::BulletCreate(1, m_BulletDamage, m_iMuzzleVel, spawnpos, m_vecDir, 0, 0, m_Gravity < 75 ? m_Gravity * 3 * mat_mult : m_Gravity, m_Flare, Owner, m_bsubsonic, m_flPenetrationPow, pOther->pev->takedamage == DAMAGE_YES ? pOther : nullptr);

				// Damage
				if (DAMAGE_NO != pOther->pev->takedamage)
				{
					m_pIgnore = pOther;

					ClearMultiDamage();
					pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
					ApplyMultiDamage(owner->pev, owner->pev);
				}

				// VFX
				if (pOther->IsBSPModel())
				{

					char material = TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_MONSTER_12MM);

					DecalGunshot(&tr, BULLET_MONSTER_12MM);		 // Entry decal  - 12mm is the heavy decal
					DecalGunshot(&beam_tr, BULLET_MONSTER_12MM); // Exit decal - 12 mm is the heavy decal

					// entry
					MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
						WRITE_BYTE(TE_IMPACTVFX);
						WRITE_COORD_VECTOR(tr.vecEndPos + (-1 * pev->velocity.Normalize()) * 0.1f); // org
						WRITE_COORD_VECTOR(-1 * pev->velocity.Normalize());							// dir
						WRITE_COORD(0);
						WRITE_COORD(0);
						WRITE_BYTE(material); // (count) (use as mat type?)
						WRITE_BYTE(0);		  // (bullethole decal texture index)
					MESSAGE_END();

					// exit
					/*
					MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
					WRITE_BYTE(TE_IMPACTVFX);
					WRITE_COORD_VECTOR(beam_tr.vecEndPos + (pev->velocity.Normalize()) * 0.1f); // org
					WRITE_COORD_VECTOR(pev->velocity.Normalize());							// dir
					WRITE_COORD(0);
					WRITE_COORD(0);
					WRITE_BYTE(material); // (count) (use as mat type?)
					WRITE_BYTE(0);		  // (bullethole decal texture index)
					MESSAGE_END(); */
				}

				pev->movetype = MOVETYPE_NONE;
				SetTouch(NULL);

				m_SpawnPos = tr.vecEndPos + (m_vecDir * (i+1));
				pev->origin = m_SpawnPos;
				m_bTryPen = true;

				return;
			}
		}
	}
	else
	{
		ALERT(at_console, "pen below 0!\n");
		m_flPenetrationPow = 0;
	}

	DecalGunshot(&tr, BULLET_MONSTER_9MM);

	// ricochet (disabled due to issues)
#if 0
	if (pOther->IsBSPModel() && pev->gravity < 10)
	{
		// if what we hit is static architecture, can stay around for a while.
		Vector vecDir = m_vecDir;

		// See if we should reflect off this surface
		float hitDot = -DotProduct(tr.vecPlaneNormal, vecDir);
			
		if ((hitDot < 0.0871) && (m_iMuzzleVel > 2500)) // 85 degrees
		{
			Vector vReflection = (2.0f * tr.vecPlaneNormal * hitDot) + vecDir;

			//CPhysbullet::BulletCreate(1, m_BulletDamage/3, m_iMuzzleVel * 0.75f, tr.vecEndPos + vReflection * 8, vReflection, CONE_2DEGREES, CONE_2DEGREES, V_max(1.0, pev->gravity) /* fall more */, m_Flare, Owner, m_bsubsonic, m_flPenetrationPow, pOther->pev->takedamage ? pOther : nullptr);

			// Damage
			if (DAMAGE_NO != pOther->pev->takedamage)
			{
				ClearMultiDamage();
				pOther->TraceAttack(owner->pev, m_BulletDamage - round(50 * hitDot), pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
				ApplyMultiDamage(owner->pev, owner->pev);
			}

			m_vecDir = vReflection;
			m_iMuzzleVel *= 0.75;
			m_flPenetrationPow *= 10 * hitDot;
			m_BulletDamage /= 1.5;
			m_bTryRefl = true;
			return;
		}
	}
#endif

	// Did not penetrate or bounce, normal collision

	pev->movetype = MOVETYPE_NONE;

	SetTouch(NULL);
	SetThink(NULL);

	if (DAMAGE_NO != pOther->pev->takedamage)
	{
		ClearMultiDamage();
		pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
		ApplyMultiDamage(owner->pev, owner->pev);
	}
	else if (pOther->IsBSPModel())
	{
		char material = TEXTURETYPE_PlaySound(&tr, tr.vecEndPos, tr.vecEndPos + m_vecDir * 2, BULLET_PLAYER_9MM);
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
			PLAYBACK_EVENT_FULL(0, edict(), g_sParticleEvent, 0.0, tr.vecEndPos + tr.vecPlaneNormal * 0.5f, tr.vecPlaneNormal, 0.0, 0.0, PE_BULLET_HITGLOW, 0, 0, 0);
		}
	}

	UTIL_Remove(this);
}

void CPhysbullet::AirThink()
{
	pev->angles = UTIL_VecToAngles(m_vecDir);
	pev->nextthink = gpGlobals->time + 0.01;

	/* // TO-DO: optimize!!
	if (!m_haswizzed && !m_bsubsonic)
	{
		CBaseEntity* m_ent = NULL;
		while ((m_ent = UTIL_FindEntityInSphere(m_ent, pev->origin, 128)) != NULL)
		{
			if (m_ent->IsPlayer() && Owner != m_ent->edict())
			{
				EMIT_SOUND_DYN(edict(), CHAN_AUTO, RANDOM_SOUND_ARRAY(pNearMissSounds), 1.0, 1, 0, 100 + RANDOM_LONG(-5, 5));
				m_haswizzed = true; // will not play twice if it passes 2 players, fix?
			}
		}
	}
	*/

	// fade in
	if (pev->renderamt < 225 && !m_bsubsonic) 
		pev->renderamt += 22.5;

	// WIND
	float flWindVel = 6;
	float flwindmult = 0.25;

	double calculatedWind = sin(gpGlobals->time * flwindmult) * flWindVel; // only calculate this once

	pev->velocity = pev->velocity + (gpGlobals->v_up * calculatedWind);
	pev->velocity = pev->velocity + (gpGlobals->v_right * calculatedWind);
	// WIND END

	// Update pos/vel for penetrations and ricochet
	if (m_bTryRefl)
	{
		pev->velocity = m_vecDir * m_iMuzzleVel;
		pev->angles = UTIL_VecToAngles( pev->velocity );
		m_bTryRefl = false;
	}
	else if (m_bTryPen)
	{
		pev->movetype = MOVETYPE_TOSS;
		SetTouch(&CPhysbullet::BulletImpact);

		UTIL_SetOrigin(pev, m_SpawnPos);
		pev->origin = m_SpawnPos;
		pev->velocity = m_vecDir * m_iMuzzleVel;
		pev->angles = UTIL_VecToAngles( pev->velocity );
		m_bTryPen = false;
	}

	m_flPenetrationPow -= 0.1;
	if (pev->waterlevel != 0)
		UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1f, pev->origin, 1);

	if (m_flPenetrationPow <= 0)
		UTIL_Remove(this);
}

int CPhysbullet::ShouldCollide(CBaseEntity* pentTouched)
{
	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	if (pentTouched->IsBullet() || pentTouched == owner)
	{
		return 0;
	}

	if (m_pIgnore)
	{
		if (pentTouched == m_pIgnore)
		{
			return 0;
		}
	}

	return 1;
}

// "Raymarch" to find the hit point of the water surface. Doesn't work too well for physical bullets but oh well.
void CPhysbullet::FindWaterSurface()
{
	int tries = 0;

	int bound = ceil((m_SpawnPos - m_Endpos).Length()) + 16;

	Vector dir = (m_Endpos - m_SpawnPos).Normalize();

	while (true)
	{
		tries++;

		int contents = UTIL_PointContents(m_Endpos - dir * tries);

		if (contents == CONTENTS_EMPTY)
		{
			PLAYBACK_EVENT_FULL(0, edict(), g_sParticleEvent, 0.0, m_Endpos - (dir * tries) + Vector(0, 0, 2), gpGlobals->v_up, 0.0, 0.0, PE_BULLET_WATERHIT, 0, 0, 0);
			return;
		}
		else if ((contents != CONTENTS_WATER || tries >= bound) && tries > 4) // make it run at least 4 times in case it starts in a wall
		{
			return;
		}
	}
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
	{
		// hit body
		if (pEntity->Classify() == CLASS_FUNGAL)
			return 1.33f; // soft flesh

		return 1.75f; // flesh
	}
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