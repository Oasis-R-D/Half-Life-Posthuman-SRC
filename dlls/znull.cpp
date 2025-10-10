/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Corruption zombie guy thing
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "weapons.h"
#include "UserMessages.h"
#include "Blooddrops.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ZOMBIE_AE_ATTACK_RIGHT 0x01
#define ZOMBIE_AE_ATTACK_LEFT 0x02
#define ZOMBIE_AE_ATTACK_BOTH 0x03

#define ZOMBIE_FLINCH_DELAY 10 // at most one flinch every n secs

class CCorrupted : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int IgnoreConditions() override;

	float m_flNextFlinch;
	int m_nextchange;
	int m_lastwasdiff;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void AttackSound();
	void MonsterThink() override;

	static const char* pAttackSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pDeathSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];

	// No range attacks
	bool CheckRangeAttack1(float flDot, float flDist) override { return false; }
	bool CheckRangeAttack2(float flDot, float flDist) override { return false; }
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
};

LINK_ENTITY_TO_CLASS(monster_znull, CCorrupted);

const char* CCorrupted::pAttackHitSounds[] =
	{
		"debris/beamstart4.wav",
};

const char* CCorrupted::pAttackMissSounds[] =
	{
		"zombie/claw_miss1.wav",
		"zombie/claw_miss2.wav",
};

const char* CCorrupted::pAttackSounds[] =
	{
		"corruption/attack.wav",
};

const char* CCorrupted::pIdleSounds[] =
	{
		"corruption/idle1.wav",
		"corruption/idle2.wav",
		"corruption/idle3.wav",
		"corruption/idle4.wav",
		"corruption/idle5.wav",
};

const char* CCorrupted::pAlertSounds[] =
	{
		"corruption/alert1.wav",
		"corruption/alert2.wav",
		"corruption/alert3.wav",
		"corruption/alert4.wav",
		"corruption/alert5.wav",
		"corruption/alert6.wav",
		"corruption/alert6.wav",
};

const char* CCorrupted::pPainSounds[] =
	{
		"corruption/pain1.wav",
		"corruption/pain4.wav",
		"corruption/pain6.wav",
};

const char* CCorrupted::pDeathSounds[] =
	{
		"corruption/death.wav",
};
//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CCorrupted::Classify()
{
	return CLASS_HALLUCINATION;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCorrupted::SetYawSpeed()
{
	pev->yaw_speed = 360;
}

bool CCorrupted::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	flDamage = 5;
	m_bloodColor = (byte)RANDOM_LONG(0, 255);

	// HACK HACK -- until we fix this.
	if (IsAlive())
		PainSound();
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, 5, bitsDamageType);
}
//=========================================================
// TraceAttack
//=========================================================
void CCorrupted::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	ptr->iHitgroup = HITGROUP_GENERIC;
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;
	int BLDAMNT;

	BLDAMNT = round(flDamage / 2);
	SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage); // a little surface blood.
	TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
#ifndef CLIENT_DLL
	CPhysblood::BloodCreate(BLDAMNT, 350, vecOrigin, vecDir, 0.5, m_bloodColor);
#endif
	flDamage = 5;
	AddMultiDamage(pevAttacker, this, 5, bitsDamageType);
}

void CCorrupted::Killed(entvars_t* pevAttacker, int iGib)
{
	CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
	if (pOwner)
	{
		pOwner->DeathNotice(pev);
	}
	int pitch = 95 + RANDOM_LONG(0, 9);
	EMIT_SOUND_DYN(ENT(pev), CHAN_AUTO, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, pitch);
	for (int i = 0; i < 15; i++)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-32, 32));
		WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-32, 32));
		WRITE_COORD(pev->origin.z + 36 + RANDOM_FLOAT(-32, 32));
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(RANDOM_LONG(8, 11));	 // scale * 10
		WRITE_BYTE(RANDOM_LONG(10, 15)); // framerate
		// WRITE_BYTE(RANDOM_LONG(0, 255));
		// WRITE_BYTE(RANDOM_LONG(0, 255)); // colors
		// WRITE_BYTE(RANDOM_LONG(0, 255));
		MESSAGE_END();
	}
#ifndef CLIENT_DLL
	CPhysblood::BloodCreate(16, 350, pev->origin + gpGlobals->v_up * 32, gpGlobals->v_up, 0.5, m_bloodColor);
	CPhysblood::BloodCreate(16, 350, pev->origin + gpGlobals->v_up * 32, gpGlobals->v_up, 0.5, m_bloodColor);
#endif
	UTIL_Remove(this);
}
void CCorrupted::PainSound()
{
	m_bloodColor = (byte)RANDOM_LONG(0, 255);
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 1)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CCorrupted::AlertSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CCorrupted::IdleSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	// Play a random idle sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CCorrupted::AttackSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM, 0, pitch);
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCorrupted::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
		AttackSound();
		// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, 25, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			}
			// Play a random attack hit sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else // Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));


		Killed(NULL, 2);
	}
	break;

	case ZOMBIE_AE_ATTACK_LEFT:
	{
		AttackSound();
		// do stuff for this event.
		//ALERT( at_console, "Slash left!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, 25, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = 18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));


		Killed(NULL, 2);
	}
	break;

	case ZOMBIE_AE_ATTACK_BOTH:
	{
		AttackSound();
		// do stuff for this event.
		CBaseEntity* pHurt = CheckTraceHullAttack(70, 25, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));


		Killed(NULL, 2);
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CCorrupted::Spawn()
{
	Precache();
	m_nextchange = gpGlobals->time;
	SET_MODEL(ENT(pev), "models/lucigast.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = (byte)RANDOM_LONG(0, 255);
	pev->health = 25;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

void CCorrupted::MonsterThink()
{
	CBaseMonster::MonsterThink();
	if (gpGlobals->time = m_nextchange)
	{
		switch (RANDOM_LONG(0, 10-m_lastwasdiff))
		{
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				m_lastwasdiff = 0;
				++pev->skin;
				if (pev->skin > 5)
					pev->skin = 0;
				break;
			case 10:
				m_lastwasdiff = 1;
				pev->skin = RANDOM_LONG(6, 10);
		}
		m_nextchange = gpGlobals->time + RANDOM_FLOAT(0.10, 0.40);
	}
	
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCorrupted::Precache()
{
	PRECACHE_MODEL("models/lucigast.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CCorrupted::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity == ACT_MELEE_ATTACK1)
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
}
