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
// headcrab.cpp - tiny, jumpy alien parasite
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "game.h"
#include "player.h"
#include "UserMessages.h"
#include "decals.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define HC_AE_JUMPATTACK (2)

Task_t tlHCRangeAttack1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_WAIT_RANDOM, (float)0.5},
};

Schedule_t slHCRangeAttack1[] =
	{
		{tlHCRangeAttack1,
			ARRAYSIZE(tlHCRangeAttack1),
			bits_COND_ENEMY_OCCLUDED |
				bits_COND_NO_AMMO_LOADED,
			0,
			"HCRangeAttack1"},
};

Task_t tlHCRangeAttack1Fast[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
};

Schedule_t slHCRangeAttack1Fast[] =
	{
		{tlHCRangeAttack1Fast,
			ARRAYSIZE(tlHCRangeAttack1Fast),
			bits_COND_ENEMY_OCCLUDED |
				bits_COND_NO_AMMO_LOADED,
			0,
			"HCRAFast"},
};

class CHeadCrab : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void RunTask(Task_t* pTask) override;
	void StartTask(Task_t* pTask) override;
	void SetYawSpeed() override;
	void EXPORT LeapTouch(CBaseEntity* pOther);
	Vector Center() override;
	Vector BodyTarget(const Vector& posSrc) override;
	void PainSound() override;
	void DeathSound() override;
	void IdleSound() override;
	void AlertSound() override;
	void PrescheduleThink() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	bool m_bPrehuman;
	void Touch(CBaseEntity* pOther)
	{
		if (m_bPrehuman == 0)
		{
		if (pOther->IsPlayer() && pev->deadflag == DEAD_NO)
		{
			if (FClassnameIs(pev, "monster_headcrab"))
			{
				auto player = (CBasePlayer*)pOther;
				if (player->HasNamedPlayerItem("weapon_headcrab"))
				{
					if (player->GiveAmmo(1, "headcrab", 5) != -1)
					{
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
						UTIL_Remove(this);
					}
				}
				else
				{
					player->GiveNamedItem("weapon_headcrab");
					UTIL_Remove(this);
				}
			}
			if (FClassnameIs(pev, "monster_headcrab_fast"))
			{
				auto player = (CBasePlayer*)pOther;
				if (player->HasNamedPlayerItem("weapon_headcrab_fast"))
				{
					if (player->GiveAmmo(1, "headcrab_fast", 5) != -1)
					{
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
						UTIL_Remove(this);
					}
				}
				else
				{
					player->GiveNamedItem("weapon_headcrab_fast");
					UTIL_Remove(this);
				}
			}
			if (FClassnameIs(pev, "monster_headcrab_poison"))
			{
				auto player = (CBasePlayer*)pOther;
				if (player->HasNamedPlayerItem("weapon_headcrab_poison"))
				{
					if (player->GiveAmmo(1, "headcrab_poison", 5) != -1)
					{
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
						UTIL_Remove(this);
					}
				}
				else
				{
					player->GiveNamedItem("weapon_headcrab_poison");
					UTIL_Remove(this);
				}
			}
		}
		}
		if (pev->owner)
			pev->owner = NULL;
		CBaseMonster::Touch(pOther);
	}

	virtual float GetDamageAmount()
	{
		return gSkillData.headcrabDmgBite;
	}
	virtual int GetVoicePitch() { return 100; }
	virtual float GetSoundVolue() { return 1.0; }
	Schedule_t* GetScheduleOfType(int Type) override;

	CUSTOM_SCHEDULES;

	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackSounds[];
	static const char* pDeathSounds[];
	static const char* pBiteSounds[];
};
LINK_ENTITY_TO_CLASS(monster_headcrab, CHeadCrab);
LINK_ENTITY_TO_CLASS(monster_headcrab_fast, CHeadCrab);
LINK_ENTITY_TO_CLASS(monster_headcrab_poison, CHeadCrab);

DEFINE_CUSTOM_SCHEDULES(CHeadCrab){
	slHCRangeAttack1,
	slHCRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES(CHeadCrab, CBaseMonster);

const char* CHeadCrab::pIdleSounds[] =
	{
		"headcrab/hc_idle1.wav",
		"headcrab/hc_idle2.wav",
		"headcrab/hc_idle3.wav",
};
const char* CHeadCrab::pAlertSounds[] =
	{
		"headcrab/hc_alert1.wav",
};
const char* CHeadCrab::pPainSounds[] =
	{
		"headcrab/hc_pain1.wav",
		"headcrab/hc_pain2.wav",
		"headcrab/hc_pain3.wav",
};
const char* CHeadCrab::pAttackSounds[] =
	{
		"headcrab/hc_attack1.wav",
		"headcrab/hc_attack2.wav",
		"headcrab/hc_attack3.wav",
};

const char* CHeadCrab::pDeathSounds[] =
	{
		"headcrab/hc_die1.wav",
		"headcrab/hc_die2.wav",
};

const char* CHeadCrab::pBiteSounds[] =
	{
		"headcrab/hc_headbite.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CHeadCrab::Classify()
{
	if (m_bPrehuman == 0)
	{
		return CLASS_PLAYER_ALLY;
	}
	else
	{
		return CLASS_ALIEN_PREY;
	}
}

//=========================================================
// Center - returns the real center of the headcrab.  The
// bounding box is much larger than the actual creature so
// this is needed for targeting
//=========================================================
Vector CHeadCrab::Center()
{
	return Vector(pev->origin.x, pev->origin.y, pev->origin.z + 6);
}


Vector CHeadCrab::BodyTarget(const Vector& posSrc)
{
	return Center();
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHeadCrab::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 30;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 20;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 60;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 30;
		break;
	default:
		ys = 30;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHeadCrab::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case HC_AE_JUMPATTACK:
	{
		ClearBits(pev->flags, FL_ONGROUND);

		UTIL_SetOrigin(pev, pev->origin + Vector(0, 0, 1)); // take him off ground so engine doesn't instantly reset onground
		UTIL_MakeVectors(pev->angles);

		Vector vecJumpDir;
		if (m_hEnemy != NULL)
		{
			float gravity = g_psv_gravity->value;
			if (gravity <= 1)
				gravity = 1;

			// How fast does the headcrab need to travel to reach that height given gravity?
			float height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
			if (height < 16)
				height = 16;
			float speed = sqrt(2 * gravity * height);
			float time = speed / gravity;

			// Scale the sideways velocity to get there at the right time
			vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
			vecJumpDir = vecJumpDir * (1.0 / time);

			// Speed to offset gravity at the desired height
			vecJumpDir.z = speed;

			// Don't jump too far/fast
			float distance = vecJumpDir.Length();

			if (distance > 650)
			{
				vecJumpDir = vecJumpDir * (650.0 / distance);
			}
		}
		else
		{
			// jump hop, don't care where
			vecJumpDir = Vector(gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z) * 350;
		}

		if (FClassnameIs(pev, "monster_headcrab_poison"))
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, UTIL_VarArgs("headcrab_poison/jump%d.wav", RANDOM_LONG(1, 3)), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
		else
		{
			int iSound = RANDOM_LONG(0, 2);
			if (iSound != 0)
				EMIT_SOUND_DYN(edict(), CHAN_VOICE, pAttackSounds[iSound], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
		}

		pev->velocity = vecJumpDir;
		m_flNextAttack = gpGlobals->time + 2;
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
void CHeadCrab::Spawn()
{
	Precache();
	if (FBitSet(pev->spawnflags, SF_PREHUMAN))
	{
		m_bPrehuman = 1;
	}
	else if (pev->armorvalue == 1)
	{
		m_bPrehuman = 1;
	}
	else if (FBitSet(pev->spawnflags, SF_NOTINHARD))
	{
		if (g_iSkillLevel == SKILL_HARD)
		{
			SetThink(&CHeadCrab::SUB_Remove);
			pev->nextthink = gpGlobals->time;
			return;
		}
	}
	else if (FBitSet(pev->spawnflags, SF_ONLYINHARD))
	{
		if (g_iSkillLevel != SKILL_HARD)
		{
			SetThink(&CHeadCrab::SUB_Remove);
			pev->nextthink = gpGlobals->time;
			return;
		}
	}
	if (FClassnameIs(pev, "monster_headcrab_fast"))
		SET_MODEL(ENT(pev), "models/headcrab_fast.mdl");
	else if (FClassnameIs(pev, "monster_headcrab_poison"))
		SET_MODEL(ENT(pev), "models/headcrab_poison.mdl");
	else
		SET_MODEL(ENT(pev), "models/headcrab.mdl");

	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 18));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = gSkillData.headcrabHealth;
	pev->effects = 0;
	pev->view_ofs = Vector(0, 0, 20); // position of the eyes relative to monster's origin.
	pev->yaw_speed = 5;				  //!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	
	m_bloodColor = BLOOD_COLOR_GREEN;
	m_flFieldOfView = 0.5;			  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHeadCrab::Precache()
{
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);

	if (FClassnameIs(pev, "monster_headcrab_fast"))
		PRECACHE_MODEL("models/headcrab_fast.mdl");
	else if (FClassnameIs(pev, "monster_headcrab_poison"))
	{
		PRECACHE_MODEL("models/headcrab_poison.mdl");
		PRECACHE_SOUND("headcrab_poison/idle1.wav");
		PRECACHE_SOUND("headcrab_poison/idle2.wav");
		PRECACHE_SOUND("headcrab_poison/idle3.wav");
		PRECACHE_SOUND("headcrab_poison/jump1.wav");
		PRECACHE_SOUND("headcrab_poison/jump2.wav");
		PRECACHE_SOUND("headcrab_poison/jump3.wav");
		PRECACHE_SOUND("headcrab_poison/pain1.wav");
		PRECACHE_SOUND("headcrab_poison/pain2.wav");
		PRECACHE_SOUND("headcrab_poison/pain3.wav");
		PRECACHE_SOUND("headcrab_poison/poisonbite1.wav");
		PRECACHE_SOUND("headcrab_poison/poisonbite2.wav");
		PRECACHE_SOUND("headcrab_poison/poisonbite3.wav");
		PRECACHE_SOUND("headcrab_poison/warning1.wav");
		PRECACHE_SOUND("headcrab_poison/warning2.wav");
		PRECACHE_SOUND("headcrab_poison/warning3.wav");
	}
	else
		PRECACHE_MODEL("models/headcrab.mdl");
}


//=========================================================
// RunTask
//=========================================================
void CHeadCrab::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
	{
		if (m_fSequenceFinished)
		{
			TaskComplete();
			SetTouch(NULL);
			m_IdealActivity = ACT_IDLE;
		}
		break;
	}
	default:
	{
		CBaseMonster::RunTask(pTask);
	}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CHeadCrab::LeapTouch(CBaseEntity* pOther)
{
	if (0 == pOther->pev->takedamage || pOther->Classify() == Classify())
		return;

	// Don't hit if back on ground
	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		if (FClassnameIs(pev, "monster_headcrab_poison"))
		{
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, UTIL_VarArgs("headcrab_poison/poisonbite%d.wav", RANDOM_LONG(1, 3)), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
			pOther->TakeDamage(pev, pev, GetDamageAmount(), DMG_POISON);
		}
		else
		{
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
			pOther->TakeDamage(pev, pev, GetDamageAmount(), DMG_SLASH);
		}
	}
	SetTouch(NULL);
}

//=========================================================
// PrescheduleThink
//=========================================================
void CHeadCrab::PrescheduleThink()
{
	// make the crab coo a little bit in combat state
	if (m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT(0, 5) < 0.1)
	{
		IdleSound();
	}
}

void CHeadCrab::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
		m_IdealActivity = ACT_RANGE_ATTACK1;
		SetTouch(&CHeadCrab::LeapTouch);
		break;
	}
	default:
	{
		CBaseMonster::StartTask(pTask);
	}
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================
bool CHeadCrab::CheckRangeAttack1(float flDot, float flDist)
{
	if (FBitSet(pev->flags, FL_ONGROUND) && flDist <= 256 && flDot >= 0.65)
	{
		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
bool CHeadCrab::CheckRangeAttack2(float flDot, float flDist)
{
	return false;
	// BUGBUG: Why is this code here?  There is no ACT_RANGE_ATTACK2 animation.  I've disabled it for now.
#if 0
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist > 64 && flDist <= 256 && flDot >= 0.5 )
	{
		return true;
	}
	return false;
#endif
}

bool CHeadCrab::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// Don't take any acid damage -- BigMomma's mortar is acid
	if ((bitsDamageType & DMG_ACID) != 0)
		flDamage = 0;

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// IdleSound
//=========================================================
#define CRAB_ATTN_IDLE (float)1.5
void CHeadCrab::IdleSound()
{
	if (FClassnameIs(pev, "monster_headcrab_poison"))
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, UTIL_VarArgs("headcrab_poison/idle%d.wav", RANDOM_LONG(1, 3)), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
	else
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// AlertSound
//=========================================================
void CHeadCrab::AlertSound()
{
	if (FClassnameIs(pev, "monster_headcrab_poison"))
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, UTIL_VarArgs("headcrab_poison/warning%d.wav", RANDOM_LONG(1, 3)), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
	else
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// AlertSound
//=========================================================
void CHeadCrab::PainSound()
{
	if (FClassnameIs(pev, "monster_headcrab_poison"))
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, UTIL_VarArgs("headcrab_poison/pain%d.wav", RANDOM_LONG(1, 3)), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
	else
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// DeathSound
//=========================================================
void CHeadCrab::DeathSound()
{
	if (m_bRailed == false)
	{
		if (FClassnameIs(pev, "monster_headcrab_poison"))
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, UTIL_VarArgs("headcrab_poison/pain%d.wav", RANDOM_LONG(1, 3)), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
		else
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
	}
}

Schedule_t* CHeadCrab::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
	{
		return &slHCRangeAttack1[0];
	}
	break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}


class CBabyCrab : public CHeadCrab
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	float GetDamageAmount() override
	{
		return gSkillData.headcrabDmgBite * 0.3;
	}
	bool CheckRangeAttack1(float flDot, float flDist) override;
	Schedule_t* GetScheduleOfType(int Type) override;
	int GetVoicePitch() override { return PITCH_NORM + RANDOM_LONG(40, 50); }
	float GetSoundVolue() override { return 0.8; }
	void MonsterThink()
	{
		if (pev->dmgtime < gpGlobals->time)
			Killed(pev, GIB_NORMAL);
		CHeadCrab::MonsterThink();
	}
};
LINK_ENTITY_TO_CLASS(monster_babycrab, CBabyCrab);

void CBabyCrab::Spawn()
{
	CHeadCrab::Spawn();
	SET_MODEL(ENT(pev), "models/baby_headcrab.mdl");
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 192;
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));
	pev->dmgtime = gpGlobals->time + 10;
	pev->health = gSkillData.headcrabHealth * 0.25; // less health than full grown
}

void CBabyCrab::Precache()
{
	PRECACHE_MODEL("models/baby_headcrab.mdl");
	CHeadCrab::Precache();
}


void CBabyCrab::SetYawSpeed()
{
	pev->yaw_speed = 120;
}


bool CBabyCrab::CheckRangeAttack1(float flDot, float flDist)
{
	if ((pev->flags & FL_ONGROUND) != 0)
	{
		if (pev->groundentity && (pev->groundentity->v.flags & (FL_CLIENT | FL_MONSTER)) != 0)
			return true;

		// A little less accurate, but jump from closer
		if (flDist <= 180 && flDot >= 0.55)
			return true;
	}

	return false;
}


Schedule_t* CBabyCrab::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_FAIL: // If you fail, try to jump!
		if (m_hEnemy != NULL)
			return slHCRangeAttack1Fast;
		break;

	case SCHED_RANGE_ATTACK1:
	{
		return slHCRangeAttack1Fast;
	}
	break;
	}

	return CHeadCrab::GetScheduleOfType(Type);
}

class CHeadcrabSuper : public CHeadCrab
{
	virtual float GetDamageAmount()
	{
		return gSkillData.headcrabDmgBite * 2;
	}
	int Classify()
	{
		return CLASS_DISLIKE_ALL;
	}
	void Spawn()
	{
		Precache();

		SET_MODEL(ENT(pev), "models/headcrab_super.mdl");
		UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));

		pev->health = gSkillData.headcrabHealth * 2;
		pev->solid = SOLID_SLIDEBOX;
		pev->movetype = MOVETYPE_STEP;
		pev->effects = 0;
		pev->view_ofs = Vector(0, 0, 20); // position of the eyes relative to monster's origin.
		pev->yaw_speed = 5;				  //!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?

		m_bloodColor = BLOOD_COLOR_CYAN;
		m_flFieldOfView = 0.5;			  // indicates the width of this monster's forward view cone ( as a dotproduct result )
		m_MonsterState = MONSTERSTATE_NONE;

		MonsterInit();
	}
	void Precache()
	{
		PRECACHE_MODEL("models/shgibs.mdl");
		PRECACHE_MODEL("models/headcrab_super.mdl");
		PRECACHE_MODEL("sprites/bblood.spr");
		PRECACHE_SOUND("superhc/cheek1.wav");
		PRECACHE_SOUND("superhc/cheek2.wav");
		PRECACHE_SOUND_ARRAY(pIdleSounds);
		PRECACHE_SOUND_ARRAY(pAlertSounds);
		PRECACHE_SOUND_ARRAY(pPainSounds);
		PRECACHE_SOUND_ARRAY(pAttackSounds);
		PRECACHE_SOUND_ARRAY(pDeathSounds);
		PRECACHE_SOUND_ARRAY(pBiteSounds);
	}

	void GibMonster()
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);
		
		int cSplat;

		for (cSplat = 0; cSplat < 6; cSplat++)
		{
			auto pGib = Create("sh_gibs", pev->origin, pev->angles, edict());

			if (pev)
			{
				// spawn the gib somewhere in the monster's bounding volume
				pGib->pev->origin.x = pev->absmin.x + pev->size.x * (RANDOM_FLOAT(0, 1));
				pGib->pev->origin.y = pev->absmin.y + pev->size.y * (RANDOM_FLOAT(0, 1));
				pGib->pev->origin.z = pev->absmin.z + pev->size.z * (RANDOM_FLOAT(0, 1)) + 1; // absmin.z is in the floor because the engine subtracts 1 to enlarge the box

				// make the gib fly away from the attack vector
				pGib->pev->velocity = g_vecAttackDir * -1;

				// mix in some noise
				pGib->pev->velocity.x += RANDOM_FLOAT(-0.25, 0.25);
				pGib->pev->velocity.y += RANDOM_FLOAT(-0.25, 0.25);
				pGib->pev->velocity.z += RANDOM_FLOAT(-0.25, 0.25);

				pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT(300, 400);

				pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
				pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

				if (pev->health > -50)
				{
					pGib->pev->velocity = pGib->pev->velocity * 0.7;
				}
				else if (pev->health > -200)
				{
					pGib->pev->velocity = pGib->pev->velocity * 2;
				}
				else
				{
					pGib->pev->velocity = pGib->pev->velocity * 4;
				}

				pGib->pev->solid = SOLID_BBOX;
				UTIL_SetSize(pGib->pev, Vector(0, 0, 0), Vector(0, 0, 0));
			}
		}
		
		SetThink(&CBaseMonster::SUB_Remove);
		pev->nextthink = gpGlobals->time;
	}
};

LINK_ENTITY_TO_CLASS(monster_headcrab_super, CHeadcrabSuper);
class SHGibs : public CBaseEntity
{
	void Spawn()
	{
		SET_MODEL(edict(), "models/shgibs.mdl");
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_BOUNCE;
		pev->nextthink = gpGlobals->time + 0.1;
		pev->dmgtime = gpGlobals->time + 10;
		pev->body = RANDOM_LONG(0, 5);
	}
	void Think()
	{
		pev->nextthink = gpGlobals->time + 0.1;
		pev->avelocity = pev->velocity * 2;
		if (pev->dmgtime < gpGlobals->time)
		{
			if (pev->renderamt == 0 && pev->armorvalue == 0)
			{
				pev->armorvalue = 1;
				pev->renderamt = 255;
				return;
			}

			if (pev->renderamt <= 0)
				UTIL_Remove(this);
			else
			{
				pev->renderfx = kRenderFxNone;
				pev->rendermode = kRenderTransTexture;
				pev->renderamt -= 2;
			}
		}
	}
	void Touch(CBaseEntity* pOther)
	{
		if (!pOther->IsBSPModel())
			return;

		pev->velocity = pev->velocity / 2;
		pev->angles.x = pev->angles.z = 0;
		if (pev->armortype < gpGlobals->time)
		{
			//Create("sh_spr", pev->origin, pev->angles, edict());
			TraceResult tr;
			UTIL_TraceLine(pev->origin, pev->velocity, dont_ignore_monsters, edict(), &tr);
			UTIL_DecalTrace(&tr, RANDOM_LONG(DECAL_BBLOOD1, DECAL_BBLOOD3));
			pev->armortype = gpGlobals->time + 0.5;
		}
	}
};

LINK_ENTITY_TO_CLASS(sh_gibs, SHGibs);

class CSHSpr : public CBaseEntity
{
	void Spawn()
	{
		SET_MODEL(edict(), "sprites/bblood.spr");
		pev->angles.x = 90;
		pev->origin.z += 1;
		pev->renderfx = kRenderFxPulseFast;
		pev->rendermode = kRenderTransAdd;
		pev->renderamt = 255;
		pev->frame = RANDOM_LONG(0, 3);
		pev->nextthink = gpGlobals->time + 0.1;
		pev->dmgtime = gpGlobals->time + 15;
	}
	void Think()
	{
		pev->nextthink = gpGlobals->time + 0.1;
		if (pev->dmgtime < gpGlobals->time)
		{
			if (pev->renderamt <= 0)
				UTIL_Remove(this);
			else
				pev->renderamt -= 2;
		}
	}
};

LINK_ENTITY_TO_CLASS(sh_spr, CSHSpr);
