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
/*

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "decals.h"
#include "physical_bullet.h"
#include "player.h"
#include "shake.h"


#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS(hw_beam, CHopWireBeam);
void CHopWireBeam::ShootBeams(CGrenade* ownerOgrenade, Vector direction)
{
	// Create a new entity with CHopWireBeam private data
	CHopWireBeam* pBullet = GetClassPtr((CHopWireBeam*)NULL);
	pBullet->pev->classname = MAKE_STRING("hw_beam");
	pBullet->m_Spread = CONE_20DEGREES;
	pBullet->m_direction = direction;
	pBullet->m_SpreadVect = Vector(RANDOM_FLOAT(pBullet->m_Spread, -pBullet->m_Spread), RANDOM_FLOAT(pBullet->m_Spread, -pBullet->m_Spread), RANDOM_FLOAT(pBullet->m_Spread, -pBullet->m_Spread));
	pBullet->spawner = ownerOgrenade;
	pBullet->Spawn();	
}
void CHopWireBeam::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	SET_MODEL(ENT(pev), "models/w_hopwire.mdl");
	UTIL_SetOrigin(pev, spawner->pev->origin); //spawn a little bit more forward

	pev->velocity = (m_direction + m_SpreadVect) * 600; // Applies spread and velocity
	pev->velocity = pev->velocity + spawner->pev->velocity;
	pev->gravity = 1; // sets the gravity (bullet drop)
	pev->angles = m_direction + m_SpreadVect;

	SetTouch(&CHopWireBeam::BoltTouch);
	
	
}
void CHopWireBeam::BoltTouch(CBaseEntity* pOther)
{
	if (pOther->IsBSPModel() && (!FClassnameIs(pOther->pev, "hw_beam") && !FClassnameIs(pOther->pev, "grenade")))
	{
		if (pev->movetype != MOVETYPE_NONE)
		{
			SetThink(&CHopWireBeam::MakeBeam);
			pev->nextthink = gpGlobals->time;
		}
		pev->movetype = MOVETYPE_NONE;
		pev->velocity = Vector(0, 0, 0);
		pev->avelocity.z = 0;	
	}
}
void CHopWireBeam::MakeBeam()
{
	CBaseEntity* that = this;
	if (spawner->m_bHasExploded == true)
	{
		UTIL_Remove(m_pSprite);
		UTIL_Remove(m_pBeam);
		UTIL_Remove(this);
		
		return;
	}

	//UTIL_Remove(m_pBeam);
	TraceResult tr;
	pev->nextthink = gpGlobals->time + 0.01f;
	// ALERT( at_console, "serverflags %f\n", gpGlobals->serverflags );

	UTIL_TraceLine(pev->origin, spawner->pev->origin, dont_ignore_monsters, ENT(pev), &tr);
	CBaseEntity* Hit = CBaseEntity::Instance(tr.pHit);
	if (Hit == nullptr || FClassnameIs(Hit->pev, "grenade") || Hit->IsBSPModel())
	{
		// TO-DO: remove this and use ! instead
	}
	else
	{
		spawner->CallDetonate();
	}
	
	// VFX START

	// HL2 FILES NEEDED:
	// "sprites/blueflare1.vmt"		  // GLOW FX
	// "sprites/rollermine_shock.vmt" // ELECTRICITY BEAM FX

	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate(g_pModelNameLgtng, 4);
		// Mark as temporary so the beam will be recreated on save game load and level transitions.
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
		m_pBeam->EntsInit(that->entindex(), spawner->entindex());
		m_pBeam->SetColor(255, 225, 0);
		m_pBeam->SetScrollRate(25);
		m_pBeam->SetBrightness(128);
		m_pBeam->SetNoise(0.5f);
	}
	if (!m_pSprite)
	{
		m_pSprite = CSprite::SpriteCreate("sprites/flare3.spr", pev->origin, false);
	
		m_pSprite->SetTransparency(kRenderTransAdd, 255, 255, 0, 128, kRenderFxNone);
	
		m_pSprite->SetScale(0.5f);
	
		m_pSprite->SetAttachment(edict(), 0);
	}
}

int CHopWireBeam::ShouldCollide(CBaseEntity* pentTouched)
{
	if (FClassnameIs(pentTouched->pev, "hw_beam") || FClassnameIs(pentTouched->pev, "grenade"))
		return 0;
	else
		return 1;
}
#endif

//===================grenade

LINK_ENTITY_TO_CLASS(grenade, CGrenade);

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE 0x0001



//
// Grenade Explode
//
void CGrenade::Explode(Vector vecSrc, Vector vecAim)
{ 

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), ignore_monsters, ENT(pev), &tr);
	Explode(&tr, DMG_BLAST);
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	float flRndSound; // sound randomizer

	pev->model = iStringNull; //invisible
	pev->solid = SOLID_NOT;	  // intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	int iContents = UTIL_PointContents(pev->origin);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);	// This makes a dynamic light and the explosion sprites/sound
	WRITE_COORD(pev->origin.x); // Send to PAS because of the sound
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
	WRITE_BYTE((pev->dmg - 50) * .60); // scale * 10
	WRITE_BYTE(15);					   // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
	entvars_t* pevOwner;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	// Counteract the + 1 in RadiusDamage.
	Vector origin = pev->origin;
	origin.z -= 1;

	RadiusDamage(origin, pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType);

	if (RANDOM_FLOAT(0, 1) < 0.5)
	{
		UTIL_DecalTrace(pTrace, DECAL_SCORCH1);
	}
	else
	{
		UTIL_DecalTrace(pTrace, DECAL_SCORCH2);
	}

	flRndSound = RANDOM_FLOAT(0, 1);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);
		break;
	}

	pev->effects |= EF_NODRAW;
	SetThink(&CGrenade::Smoke);
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0, 3);
		for (int i = 0; i < sparkCount; i++)
			Create("spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL);
	}

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_BREAKMODEL);
	// position
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	// size
	WRITE_COORD(8);
	WRITE_COORD(8);
	WRITE_COORD(8);
	// velocity
	WRITE_COORD(pev->velocity.x);
	WRITE_COORD(pev->velocity.y);
	WRITE_COORD(pev->velocity.z);
	WRITE_BYTE(50); // randomization
	// Model
	WRITE_SHORT(g_sModelIndexShrapnel); // model id#
	// # of shards
	WRITE_BYTE(pev->dmg / 10); // let client decide
	// duration
	WRITE_BYTE(30); // 3.0 seconds
	WRITE_BYTE(BREAK_SMOKE); // flags
	MESSAGE_END();
}

void CGrenade::ExplodeFlash(TraceResult* pTrace, int bitsDamageType)
{
	float flRndSound; // sound randomizer

	pev->model = iStringNull; //invisible
	SET_MODEL(ENT(pev), "sprites/flashbangflash.spr");
	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->renderfx = kRenderFxNoDissipation;
	pev->scale = 2.5;
	pev->solid = SOLID_NOT;	  // intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
	entvars_t* pevOwner;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	// Counteract the + 1 in RadiusDamage.
	Vector origin = pev->origin;
	origin.z -= 1;

	UTIL_DecalTrace(pTrace, RANDOM_LONG(DECAL_OFSCORCH1, DECAL_OFSCORCH3));

	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 400, 0.5);
	
	CBaseEntity* pEntity = NULL;
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 400)) != NULL)
	{
		TraceResult sightline;
		if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_PLAYER && !IsMachine(pEntity) && pEntity->IsAlive())
		{
			// stuns the enemy
			CBaseMonster* pMonster = dynamic_cast<CBaseMonster*>(pEntity);
			if (pMonster != nullptr)
			{
				UTIL_TraceLine(origin, pMonster->EyePosition(), ignore_monsters, ignore_glass, NULL, &sightline);
				if (sightline.flFraction == 1.0)
				{
					ALERT(at_console, "attempt stun\n");
					pMonster->TaskComplete();
					pMonster->ClearSchedule();
					pMonster->m_hEnemy = NULL;
					pMonster->m_movementGoal = MOVEGOAL_NONE;
					pMonster->m_Activity = ACT_COWER;
					pMonster->m_IdealMonsterState = MONSTERSTATE_IDLE;
					pMonster->SetActivity(ACT_COWER);
					pMonster->ClearConditions(bits_COND_SEE_ENEMY | bits_COND_CAN_ATTACK);
					pMonster->SetConditions(bits_COND_TASK_FAILED | bits_COND_LIGHT_DAMAGE);
					
					pMonster->TakeDamage(pev, pev, 5, DMG_SONIC);
					pMonster->pev->nextthink = gpGlobals->time;
					if (pMonster->pev->health > 0)
						pMonster->pev->nextthink = gpGlobals->time + 4.5;
				}
				pMonster->ClearConditions(bits_COND_HEAR_SOUND);
				pMonster->Forget(bits_MEMORY_INCOVER);
			}
		}
		if (pEntity->IsPlayer())
		{
			CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(pEntity);
			if (pPlayer != nullptr)
			{
				UTIL_TraceLine(origin, pPlayer->EyePosition(), ignore_monsters, ignore_glass, NULL, &sightline);
				if (sightline.flFraction == 1.0)
				{
					Vector Grennormal = (pev->origin - pPlayer->EyePosition()).Normalize();
					UTIL_MakeVectors(pPlayer->pev->v_angle);
					float dp = DotProduct(Grennormal, -gpGlobals->v_forward);
					Vector Color = (g_iSkillLevel == SKILL_HARD) ? Vector(255, 255, 255) : Vector(128, 128, 128);
					if (dp < 0)	
						UTIL_ScreenFade(pPlayer, Color, 2, 1, 255, FFADE_IN);
					else
						UTIL_ScreenFade(pPlayer, Color, 1, 0, 255, FFADE_IN);
				}
			}
		}
	}
	flRndSound = RANDOM_FLOAT(0, 1);

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/flashbang-1.wav", 1, ATTN_GUN);

	MESSAGE_BEGIN(MSG_PVS, gmsgCreateDLight, pev->origin);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 8);
	WRITE_BYTE(40); // Radius/10
	WRITE_BYTE(255); // R
	WRITE_BYTE(255); // G
	WRITE_BYTE(255); // B
	WRITE_LONG(0.25); // TIME
	WRITE_BYTE(0); // Decay
	MESSAGE_END();

	pev->velocity = g_vecZero;
	int iContents = UTIL_PointContents(pev->origin);
	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = 3;
		for (int i = 0; i < sparkCount; i++)
			Create("spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL);
	}

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_BREAKMODEL);
	// position
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	// size
	WRITE_COORD(8);
	WRITE_COORD(8);
	WRITE_COORD(8);
	// velocity
	WRITE_COORD(pev->velocity.x);
	WRITE_COORD(pev->velocity.y);
	WRITE_COORD(pev->velocity.z);
	WRITE_BYTE(50); // randomization
	// Model
	WRITE_SHORT(g_sModelIndexShrapnel); // model id#
	// # of shards
	WRITE_BYTE(2); // let client decide
	// duration
	WRITE_BYTE(30); // 3.0 seconds
	WRITE_BYTE(BREAK_SMOKE); // flags
	MESSAGE_END();
	SetThink(&CGrenade::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.125;
}

void CGrenade::Smoke()
{
	if (UTIL_PointContents(pev->origin) == CONTENTS_WATER)
	{
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);
	}
	else
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE((pev->dmg - 50) * 0.80); // scale * 10
		WRITE_BYTE(12);						// framerate
		MESSAGE_END();
	}
	UTIL_Remove(this);
}

void CGrenade::Killed(entvars_t* pevAttacker, int iGib)
{
	SetThink(&CGrenade::CallDetonate);
	pev->nextthink = gpGlobals->time;
}

// Timed grenade, this think is called when time runs out.
void CGrenade::DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CGrenade::Detonate);
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate()
{
	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 400, 0.3);

	SetThink(&CGrenade::CallDetonate);
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::Detonate()
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);
	#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_HARD)
	{
		CPhysbullet::BulletCreate(8, 15, 5750, pev->origin + gpGlobals->v_up * 1, gpGlobals->v_up, M_PI, M_PI, 1, 12, edict());
	}
	else
	{
		CPhysbullet::BulletCreate(8, 20, 5750, pev->origin + gpGlobals->v_up * 1, gpGlobals->v_up, M_PI, M_PI, 1, 12, edict());
	}
	#endif
	Explode(&tr, DMG_BLAST);
}

void CGrenade::DetonateFlash()
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);
	ExplodeFlash(&tr, DMG_SONIC);
}

//
// Contact grenade, explode when it touches something
//
void CGrenade::ExplodeTouch(CBaseEntity* pOther)
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr);
	#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_HARD)
	{
		CPhysbullet::BulletCreate(8, 15, 5750, pev->origin, VECTOR_CONE_20DEGREES, M_PI, M_PI, 1, 12, edict());
	}
	else
	{
		CPhysbullet::BulletCreate(8, 20, 5750, pev->origin, VECTOR_CONE_20DEGREES, M_PI, M_PI, 1, 12, edict());
	}
	#endif
	Explode(&tr, DMG_BLAST);
}

void CGrenade::DangerSoundThink()
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length(), 0.2);
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}
}

void CGrenade::ArmHopwire()
{
	pev->health = 5;
	pev->takedamage = DAMAGE_YES;

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
	entvars_t* pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 400, 0.5);

	EMIT_SOUND(ENT(pev), CHAN_AUTO, "weapons/hopwire_fly.wav", 0.8f, ATTN_NORM);

	pev->gravity = 0.25f;
	pev->velocity = gpGlobals->v_up * 200;
	pev->avelocity.x = RANDOM_LONG(-100, -400);
	pev->avelocity.z = RANDOM_LONG(-100, -400);
	pev->avelocity.y = RANDOM_LONG(-100, -400);

	m_pSprite = CSprite::SpriteCreate("sprites/flare3.spr", pev->origin, false);

	m_pSprite->SetTransparency(kRenderTransAdd, 255, 255, 0, 128, kRenderFxNone);

	m_pSprite->SetScale(0.5f);

	m_pSprite->SetAttachment(edict(), 0);

	pev->nextthink = 0.125f;
	SetThink(&CGrenade::HopwireThink);
}

void CGrenade::HopwireThink()
{
	pev->nextthink = 0.25f;
	
	if (pev->health <= 0)
	{
		SetThink(&CGrenade::CallDetonate); // replace with higher radius?
		m_bHasExploded = true;
		pev->nextthink = gpGlobals->time;
	}

	// PHYSICSPHYSICS - Shoot entities out that stick to surfaces + tied to hopwire by rope constraints

	if (pev->velocity.z <= 0) // At apex, set gravity back to normal
	{
		pev->gravity = 0.5f;
		if (wireamnt > 0 && nextwire <= gpGlobals->time)
		{
			Vector RNDDIR = Vector(RANDOM_FLOAT(M_PI, -M_PI), RANDOM_FLOAT(M_PI, -M_PI), RANDOM_FLOAT(M_PI, -M_PI));
			CHopWireBeam::ShootBeams(this, gpGlobals->v_up + RNDDIR);
			pev->velocity.x += -RNDDIR.x * 10;
			pev->velocity.y += -RNDDIR.y * 10;
			pev->velocity.z += -RNDDIR.z * 10;
			wireamnt -= 1;
			nextwire = gpGlobals->time + RANDOM_FLOAT(0.1f, 0.3f);
		}
	}
}

void CGrenade::BounceTouch(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		entvars_t* pevOwner = VARS(pev->owner);
		if (pevOwner)
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB);
			ApplyMultiDamage(pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity.
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity;
	vecTestVelocity.z *= 0.45;

	if (!m_fRegisteredSound && vecTestVelocity.Length() <= 60)
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving.
		// go ahead and emit the danger sound.

		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, pev->dmg / 0.4, 0.3);
		m_fRegisteredSound = true;
	}

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG(1, 1);
		ResetSequenceInfo();
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
	{
		pev->framerate = 0;
		pev->frame = 0;
	}
}

void CGrenade::SlideTouch(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;

		if (pev->velocity.x != 0 || pev->velocity.y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade::BounceSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM);
		break;
	}
}
void CGrenade::CallDetonate()
{
	switch (m_iGrenType)
	{
		case 0:
			SetThink(&CGrenade::Detonate);
			break;
		case 1:
			break;
		case 2:
			SetThink(&CGrenade::DetonateFlash);
			break;
		case 3:
			if (wireamnt == 8)
				SetThink(&CGrenade::ArmHopwire);
			else
			{
				UTIL_Remove(m_pSprite);
				SetThink(&CGrenade::Detonate);
				m_bHasExploded = true;
			}
			break;
	}
	pev->nextthink = gpGlobals->time;
}
void CGrenade::TumbleThink()
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		CallDetonate();
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}

void CGrenade::Spawn()
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING("grenade");

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	if (g_iSkillLevel != SKILL_HARD)
	{
		pev->dmg = 100;
	}
	else
	{
		pev->dmg = 160;
	}

	m_fRegisteredSound = false;
}

CGrenade* CGrenade::ShootContact(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = GetClassPtr((CGrenade*)NULL);
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5; // lower gravity since grenade is aerodynamic and engine doesn't know it. //make it faster :shrug:
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	// make monsters afaid of it while in the air
	pGrenade->SetThink(&CGrenade::DangerSoundThink);
	pGrenade->pev->nextthink = gpGlobals->time;

	// Explode on contact
	pGrenade->SetTouch(&CGrenade::ExplodeTouch);

	if (g_iSkillLevel != SKILL_HARD)
		pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;
	else
		pGrenade->pev->dmg = 160;

	return pGrenade;
}

CGrenade* CGrenade::ShootTimed(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float time)
{
	CGrenade* pGrenade = GetClassPtr((CGrenade*)NULL);
	pGrenade->Spawn();
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->SetTouch(&CGrenade::BounceTouch); // Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed().

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink(&CGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	SET_MODEL(ENT(pGrenade->pev), "models/w_grenade.mdl");
	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1.0;
	pGrenade->ResetSequenceInfo();

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	if (g_iSkillLevel != SKILL_HARD)
	{
		pGrenade->pev->dmg = 100;
	}
	else
	{
		pGrenade->pev->dmg = 160;
	}
	return pGrenade;
}

CGrenade* CGrenade::ShootOffhand(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, int type, float time)
{
	CGrenade* pGrenade = GetClassPtr((CGrenade*)NULL);
	pGrenade->Spawn();
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->SetTouch(&CGrenade::BounceTouch); // Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed().

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	
	if (time < 0.1 && time != -1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	switch (type)
	{
		case 0:
			SET_MODEL(ENT(pGrenade->pev), "models/w_grenade.mdl");
			pGrenade->pev->dmg = (g_iSkillLevel == SKILL_HARD) ? 160 : 100;
			pGrenade->SetThink(&CGrenade::TumbleThink);
			break;
		case 1:
			SET_MODEL(ENT(pGrenade->pev), "models/grenade.mdl");
			pGrenade->pev->dmg = (g_iSkillLevel == SKILL_HARD) ? 160 : 80;
			// Tumble through the air
			pGrenade->pev->avelocity.x = -400;
			break;
		case 2:
			SET_MODEL(ENT(pGrenade->pev), "models/w_fgrenade.mdl");
			pGrenade->pev->dmg = 10;
			pGrenade->SetThink(&CGrenade::TumbleThink);
			break;
		case 3:
			SET_MODEL(ENT(pGrenade->pev), "models/w_hopwire.mdl");
			pGrenade->pev->dmg = (g_iSkillLevel == SKILL_HARD) ? 160 : 80;
			pGrenade->SetThink(&CGrenade::TumbleThink);
			// Tumble through the air
			pGrenade->pev->avelocity.x = RANDOM_LONG(-100, -400);
			pGrenade->pev->avelocity.z = RANDOM_LONG(-100, -400);
			pGrenade->pev->avelocity.y = RANDOM_LONG(-100, -400);
			pGrenade->wireamnt = 8;
			pGrenade->nextwire = gpGlobals->time;
			break;
	}
	pGrenade->m_iGrenType = type;

	if (time == -1)
	{
			// make monsters afaid of it while in the air
			pGrenade->SetThink(&CGrenade::DangerSoundThink);
			pGrenade->pev->nextthink = gpGlobals->time;

			// Explode on contact
			pGrenade->SetTouch(&CGrenade::ExplodeTouch);
	}
	else
	{
		pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	}
	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1.0;
	pGrenade->ResetSequenceInfo();



	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;
	
	return pGrenade;
}

CGrenade* CGrenade::ShootSatchelCharge(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = GetClassPtr((CGrenade*)NULL);
	pGrenade->pev->movetype = MOVETYPE_BOUNCE;
	pGrenade->pev->classname = MAKE_STRING("grenade");

	pGrenade->pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pGrenade->pev), "models/grenade.mdl"); // Change this to satchel charge model

	UTIL_SetSize(pGrenade->pev, Vector(0, 0, 0), Vector(0, 0, 0));

	if (g_iSkillLevel != SKILL_HARD)
	{
		pGrenade->pev->dmg = 200;
	}
	else
	{
		pGrenade->pev->dmg = 260;
	}

	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = g_vecZero;
	pGrenade->pev->owner = ENT(pevOwner);

	// Detonate in "time" seconds
	pGrenade->SetThink(&CGrenade::SUB_DoNothing);
	pGrenade->SetUse(&CGrenade::DetonateUse);
	pGrenade->SetTouch(&CGrenade::SlideTouch);
	pGrenade->pev->spawnflags = SF_DETONATE;

	pGrenade->pev->friction = 0.9;

	return pGrenade;
}

void CGrenade::UseSatchelCharges(entvars_t* pevOwner, SATCHELCODE code)
{
	edict_t* pentFind;
	edict_t* pentOwner;

	if (!pevOwner)
		return;

	CBaseEntity* pOwner = CBaseEntity::Instance(pevOwner);

	pentOwner = pOwner->edict();

	pentFind = FIND_ENTITY_BY_CLASSNAME(NULL, "grenade");
	while (!FNullEnt(pentFind))
	{
		CBaseEntity* pEnt = Instance(pentFind);
		if (pEnt)
		{
			if (FBitSet(pEnt->pev->spawnflags, SF_DETONATE) && pEnt->pev->owner == pentOwner)
			{
				if (code == SATCHEL_DETONATE)
					pEnt->Use(pOwner, pOwner, USE_ON, 0);
				else // SATCHEL_RELEASE
					pEnt->pev->owner = NULL;
			}
		}
		pentFind = FIND_ENTITY_BY_CLASSNAME(pentFind, "grenade");
	}
}

#ifndef CLIENT_DLL
int CGrenade::ShouldCollide(CBaseEntity* pentTouched)
{
	if (FClassnameIs(pentTouched->pev, "hw_beam") || FClassnameIs(pentTouched->pev, "grenade"))
		return 0;
	else
		return 1;
}
#endif
//======================end grenade







// WE'RE DONE WHEN I SAY WE'RE DONE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
class CGrenadePickup : public CBaseButton
{
	int m_iTracerType = 0;
	int m_iAmnt = 3;
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_grenade.mdl");
		ASSERT((3 - m_iAmnt) >= 0);
		SetBodygroup(1, 3 - m_iAmnt);
		SetBodygroup(0, m_iTracerType);
		// Set up BBox and origin
		pev->solid = SOLID_BBOX;
		SetSequenceBox();
		UTIL_SetOrigin(pev, pev->origin);
		pev->movetype = MOVETYPE_NONE;
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_grenade.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool KeyValue(KeyValueData* pkvd) override
	{
		if (FStrEq(pkvd->szKeyName, "grentype"))
		{
			m_iTracerType = atoi(pkvd->szValue); // choices (0 and 1 rn)
			return true;
		}
		else if (FStrEq(pkvd->szKeyName, "amount")) // value between max grenades and 1
		{
			m_iAmnt = (atoi(pkvd->szValue));
			return true;
		}
		return CBaseButton::KeyValue(pkvd);
	}
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override
	{
		if (pActivator->IsPlayer())
		{
			int iPlayerGrenType;
			int iPlayerGrenAmnt;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			CBasePlayer* player = dynamic_cast<CBasePlayer*>(pActivator);
			if (player->m_iGrenadeAmnt == 0) // if player has no grenades to exchange, remove this
			{
				player->m_iGrenadeAmnt = m_iAmnt;
				player->m_iGrenadeType = m_iTracerType;
				MESSAGE_BEGIN(MSG_ONE, gmsgGrenadeHUD, NULL, player->pev);
				WRITE_BYTE(player->m_iGrenadeType);
				WRITE_BYTE(player->m_iGrenadeAmnt);
				MESSAGE_END();
				UTIL_Remove(this);
				return; 
			}
				

			if (m_iTracerType != player->m_iGrenadeType)
			{
				iPlayerGrenType = player->m_iGrenadeType;
				iPlayerGrenAmnt = player->m_iGrenadeAmnt;

				player->m_iGrenadeAmnt = m_iAmnt; // exchanges player grenades with pickups amnt and type
				player->m_iGrenadeType = m_iTracerType;

				m_iTracerType = iPlayerGrenType; // exchanges pickup grenades with players amnt and type
				m_iAmnt = iPlayerGrenAmnt;
				ALERT(at_console, "Player grenades: %i\n", player->m_iGrenadeAmnt);
				ALERT(at_console, "Pickup grenades: %i\n", m_iAmnt);
				ALERT(at_console, "Player type: %i\n", player->m_iGrenadeType);
				ALERT(at_console, "Pickup type: %i\n", m_iTracerType);
			}
			else
			{
				while (m_iAmnt > 0)
				{
					if (player->m_iGrenadeAmnt >= 3)
						break;
					player->m_iGrenadeAmnt += 1;
					m_iAmnt -= 1;
				}
				ALERT(at_console, "Player grenades: %i\n", player->m_iGrenadeAmnt);
				ALERT(at_console, "Pickup grenades: %i\n", m_iAmnt);
			}
			MESSAGE_BEGIN(MSG_ONE, gmsgGrenadeHUD, NULL, player->pev);
			WRITE_BYTE(player->m_iGrenadeType);
			WRITE_BYTE(player->m_iGrenadeAmnt);
			MESSAGE_END();

			ASSERT((3 - m_iAmnt) >= 0);
			SetBodygroup(1, 3 - m_iAmnt);
			SetBodygroup(0, m_iTracerType);
		}
	}
};
LINK_ENTITY_TO_CLASS(ammo_grenade, CGrenadePickup);
