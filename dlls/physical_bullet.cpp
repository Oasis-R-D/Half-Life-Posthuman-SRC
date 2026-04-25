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

#include "physical_bullet.h"

#pragma region Penetration Tables

// PENETRATION VALUES BASED ON CONCRETE (hence why concrete is always 1.0)
static const std::unordered_map<int, float> map_556 =
{//	   MAT		MULT
	{P_CRETE,	1.0f},	// 6″
	{P_DIRT,	0.6f},	// 10″
	{P_METAL,	12.0f},	// 1/4″
	{P_GLASS,	2.0f},	// 3″ (fiberglass)
	{P_WOOD,	.35f},	// ~16″
	{P_FLESH,	0.4f},	// 15″
};

static const std::unordered_map<int, float> map_9 =
{//	   MAT		MULT	
	{P_CRETE,	1.0f},	// 3″
	{P_DIRT,	.33f},	// 9″
	{P_METAL,	16.0f},	// 3/16″
	{P_GLASS,	2.0f},	// 1.5″
	{P_WOOD,	0.5f},	// 6″
	{P_FLESH,	.25f},	// 12″
};

static std::unordered_map<int, std::unordered_map<int, float>> penetrationMaps =
{
	{9,		  map_9},
	{12,	  map_9}, // TODO
	{556,	map_556},
	{762,	map_556}, // TODO
	{357,	map_556}, // TODO
	{44,	map_556}, // TODO
};

#pragma endregion

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

void CPhysbullet::BulletCreate(unsigned int BLLTamnt, unsigned int BLLTDamage, unsigned int BLLTSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, float BLLTGravity, int FlareType, edict_t *shooter, bool subsonic, float maxpenoverride, CBaseEntity* pIgnore)
{
	for (unsigned int i = 0; i < BLLTamnt; i++) // Allows multishot
	{
		// Create a new entity with CPhysbullet private data
		CPhysbullet* pBullet = GetClassPtr((CPhysbullet*)NULL);
		pBullet->pev->classname = MAKE_STRING("phys_bullet");
		pBullet->m_muzzlevelocity = BLLTSpeed;
		pBullet->m_BulletDamage = BLLTDamage;
		pBullet->m_SpawnPos = VecSpawnPos;
		pBullet->m_direction = vecDir;
		pBullet->m_Spread = vecSpread;
		pBullet->m_SpreadVert = vecSpreadvert; // Shotgun duckbill choke
		pBullet->m_Gravity = BLLTGravity;
		pBullet->m_iCaliber = FlareType; // tracer type
		pBullet->m_bsubsonic = subsonic;
		pBullet->m_fPenoverride = maxpenoverride; // for penetration
		pBullet->Owner = (shooter != nullptr) ? shooter : pBullet->edict();
		pBullet->m_pIgnore = (pIgnore != nullptr) ? pIgnore : pBullet;
		pBullet->pev->owner = NULL;

		pBullet->Spawn();
	}
}

static const char* pNearMissSounds[] =
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

	switch(m_iCaliber)
	{
		case 556:
			SET_MODEL(ENT(pev), "sprites/tracer_556mm.spr");
			pev->scale = RANDOM_FLOAT(0.23f, 0.27f);
			m_distpenetrate = 6;
			break;
		case 762:
			SET_MODEL(ENT(pev), "sprites/tracer_44magnum.spr");
			pev->scale = RANDOM_FLOAT(0.31f, 0.35f);
			m_distpenetrate = 10;
			break;
		case 12:
			SET_MODEL(ENT(pev), "sprites/tracer_12g.spr");
			pev->scale = RANDOM_FLOAT(0.13f, 0.17f);
			m_distpenetrate = 3;
			break;
		case 357:
			SET_MODEL(ENT(pev), "sprites/tracer_357magnum.spr");
			pev->scale = RANDOM_FLOAT(0.28f, 0.32f);
			m_distpenetrate = 6;
			break;
		case 44:
			SET_MODEL(ENT(pev), "sprites/tracer_44magnum.spr");
			pev->scale = RANDOM_FLOAT(0.32f, 0.33f);
			m_distpenetrate = 5;
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
			break;
		default:
		case 9:
			SET_MODEL(ENT(pev), "sprites/tracer_9mm.spr");
			pev->scale = RANDOM_FLOAT(0.18f, 0.22f);
			m_distpenetrate = 3;
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
		}
	}

	if (m_bsubsonic)
		pev->renderamt = 5;
	else if (pev->renderamt != 0)
		pev->renderamt = 150;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	SetTouch(&CPhysbullet::BulletImpact);
	SetThink(&CPhysbullet::AirThink);

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

void CPhysbullet::BulletImpact(CBaseEntity* pOther)
{	
	TraceResult tr = UTIL_GetGlobalTrace();
	if (UTIL_PointContents(tr.vecEndPos) == CONTENTS_SKY)
	{
		UTIL_Remove(this);
		return;
	}

	m_Endpos = pev->origin; // where bullet hit

	CBaseEntity* owner = CBaseEntity::Instance(Owner);
	if (owner == nullptr)
	{
		owner = this; // fixes RC crash
		Owner = this->edict();
	}

	if (m_distpenetrate > 0) // penetrate (ask your mother what that means)
	{
		TraceResult beam_tr;
		TraceResult beam_tr2;
		double p;
		int i = 1;

		UTIL_TraceLine(tr.vecEndPos + m_direction * 1, tr.vecEndPos + m_direction * i, dont_ignore_monsters, NULL, &beam_tr2);
		while (1 == beam_tr2.fAllSolid && i <= m_distpenetrate) // Raymarching (works better than the tau cannons trace back method)
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

				m_BulletDamage -= round(0.125 * p);
				if (m_BulletDamage <= 0)
					m_BulletDamage = 2;

				m_muzzlevelocity -= 10 * p;

				// Fire penetrated bullet
				Vector spawnpos = tr.vecEndPos + (m_direction * (i+1));
				CPhysbullet::BulletCreate(1, m_BulletDamage, m_muzzlevelocity, spawnpos, m_direction, 0, 0, m_Gravity, m_iCaliber, Owner, m_bsubsonic, m_distpenetrate, pOther->pev->takedamage ? pOther : nullptr);

				// Damage
				ClearMultiDamage();
				pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, (m_iCaliber != 420) ? (DMG_BULLET | DMG_NEVERGIB) : DMG_BULLET);
				ApplyMultiDamage(owner->pev, owner->pev);
				
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

				if (pOther->IsBSPModel())
				{
					char material = TEXTURETYPE_PlaySound(&tr, m_SpawnPos, m_Endpos, BULLET_MONSTER_12MM);

					// ALERT(at_console, "char is %c \0", material);

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

					// exit (disabled for being unperformant)
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

	// ricochet
	if (pOther->IsBSPModel())
	{
		// if what we hit is static architecture, can stay around for a while.
		Vector vecDir = m_direction;

		// See if we should reflect off this surface
		float hitDot = -DotProduct(tr.vecPlaneNormal, vecDir);
			
		if ((hitDot < 0.0871) && (m_muzzlevelocity > 2500)) // 85 degrees
		{
			Vector vReflection = (2.0f * tr.vecPlaneNormal * hitDot) + vecDir;
			
			m_distpenetrate *= 10 * hitDot;
			m_BulletDamage -= round(50 * hitDot);

			CPhysbullet::BulletCreate(1, m_BulletDamage, m_muzzlevelocity * 0.75f, tr.vecEndPos + vReflection * 8, vReflection, CONE_2DEGREES, CONE_2DEGREES, 1.0 /* fall more */, m_iCaliber, Owner, m_bsubsonic, m_distpenetrate, pOther->pev->takedamage ? pOther : nullptr);

			// Damage
			ClearMultiDamage();
			pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET);
			ApplyMultiDamage(owner->pev, owner->pev);

			// Remove original bullet
			UTIL_Remove(this);
			return;
		}
	}

	// Did not penetrate or bounce, normal collision

	pev->movetype = MOVETYPE_NONE;

	SetTouch(NULL);
	SetThink(NULL);

	if (0 != pOther->pev->takedamage)
	{
		ClearMultiDamage();
		pOther->TraceAttack(owner->pev, m_BulletDamage, pev->velocity.Normalize(), &tr, DMG_BULLET);
		ApplyMultiDamage(owner->pev, owner->pev);
	}
	else
	{
		if (pOther->IsBSPModel())
		{
			char material = TEXTURETYPE_PlaySound(&tr, tr.vecEndPos, tr.vecEndPos + m_direction * 2, BULLET_PLAYER_9MM);
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
	float flWindVel, flwindmult;

	for (int i = 0; i < 3; i++)
	{
		flWindVel = 8;

		flwindmult = 0.25;

		double calculatedWind = sin(gpGlobals->time * flwindmult) * flWindVel; // only calculate this once

		pev->velocity = pev->velocity + (gpGlobals->v_up * calculatedWind);
		pev->velocity = pev->velocity + (gpGlobals->v_right * calculatedWind);
	}
	// WIND END

	m_distpenetrate -= 0.5;

	if (pev->waterlevel != 0)
	{
		m_distpenetrate -= 0.5; // loses even more penetration when under the water

		UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1f, pev->origin, 1);
	}

	if (m_distpenetrate <= 0)
	{
		m_distpenetrate = 0;
		return;
	}

	m_distpenetrate -= 0.5;
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

const float CPhysbullet::TEXTURETYPE_Penetration(TraceResult* ptr, Vector vecSrc, Vector vecEnd)
{
	// hit an object, determine how well the bullet can penetrate your mo-
	if (m_iCaliber == 420)
		return 1.0f;
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

	std::unordered_map<int, float> map = penetrationMaps[m_iCaliber];
	
	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		penmodifier = map[P_CRETE];
		break;
	case CHAR_TEX_METAL:
	case CHAR_TEX_VENT:
	case CHAR_TEX_GRATE:
		penmodifier = map[P_METAL];
		break;
	case CHAR_TEX_IMPEN:
		penmodifier = 64;
		break;
	case CHAR_TEX_DIRT:
	case CHAR_TEX_SLOSH:
		penmodifier = map[P_DIRT];
		break;
	case CHAR_TEX_WOOD:
		penmodifier = map[P_WOOD];
		break;
	case CHAR_TEX_TILE:
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		penmodifier = map[P_GLASS];
		break;
	case CHAR_TEX_FLESH: // less overpenetration
		penmodifier = map[P_FLESH];
		break;
	}
	ALERT(at_console, "penetration mult: %f\n", penmodifier);
	return penmodifier;
}