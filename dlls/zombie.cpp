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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "defaultai.h"
#include "talkmonster.h"
#include "schedule.h"
#include "soundent.h"
#include "game.h"
#include "headercrab.h"
#include "gibs.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ZOMBIE_AE_ATTACK_RIGHT 0x01
#define ZOMBIE_AE_ATTACK_LEFT 0x02
#define ZOMBIE_AE_ATTACK_BOTH 0x03

#define ZOMBIE_FLINCH_DELAY 2 // at most one flinch every n secs

class CZombie : public CTalkMonster
{
public:
	int m_iHeadCrabHealth;
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int IgnoreConditions() override;
	float m_flNextFlinch;

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void AttackSound();

	static const char* pAttackSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];

	// No range attacks
	bool CheckRangeAttack1(float flDot, float flDist) override { return false; }
	bool CheckRangeAttack2(float flDot, float flDist) override { return false; }
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
	{
		if (ptr->iHitgroup == HITGROUP_HEAD)
		{
			m_bloodColor = BLOOD_COLOR_YELLOW;
			m_iHeadCrabHealth -= flDamage;
			flDamage = flDamage*0.7;
			if (m_iHeadCrabHealth <= 0 && m_iHeadCrabHealth >= -15)
				flDamage = 200;
			else if (m_iHeadCrabHealth < -15 && pev->body != 1)
			{
				pev->body = 1;
				flDamage = 200;
				//spawn the headcrab gibs
				CoolerGib::SpawnRandomGibs(pev, pev->origin + Vector(0, 0, 68));
			}
		}
		else
		{
			m_bloodColor = BLOOD_COLOR_RED;
		}

		if (ptr->iHitgroup == HITGROUP_CHEST || ptr->iHitgroup == HITGROUP_STOMACH)
		{
			if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST)) != 0)
			{
				if (FClassnameIs(pev, "monster_zombie_barney") || FClassnameIs(pev, "monster_zombie_soldier"))
				{
					if (g_iSkillLevel != SKILL_HARD)
					{
						flDamage = round(flDamage * 0.8);
					}
					else
					{
						flDamage = round(flDamage * 0.7);
					}
					if (RANDOM_LONG(0, 1) == 1)
						UTIL_Sparks(ptr->vecEndPos);
				}
			}
		
		}
		CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
	}
	void Killed(entvars_t* pevAttacker, int iGib)
	{
		if (m_LastHitGroup != HITGROUP_HEAD && m_iHeadCrabHealth > 2)
		{
			if (RANDOM_LONG(0, 2) == 0) //33% of unlatching occuring
			{
				pev->body = 1;
				UTIL_MakeVectors(pev->angles);
				char* monster;
				if (FClassnameIs(pev, "monster_zombie_fast"))
					monster = "monster_headcrab_fast";
				else
					monster = "monster_headcrab";
				CBaseEntity* headcrab = Create(monster, pev->origin + Vector(0, 0, 68), pev->angles, edict());
				headcrab->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;
				if (m_bPrehuman == 1 || FBitSet(pev->spawnflags, SF_PREHUMAN))
				{
					CHeadCrab* ActualHeadcrab = dynamic_cast<CHeadCrab*>(headcrab);
					ActualHeadcrab->pev->armorvalue = 1;
					ActualHeadcrab->m_bPrehuman = 1;
					ActualHeadcrab->pev->health = round(m_iHeadCrabHealth/3);
					ALERT(at_console, "Headcrab SHOULD hate you\n");
				}
				headcrab->pev->velocity = gpGlobals->v_forward * 128;
			}
		}
		CTalkMonster::Killed(pevAttacker, iGib);
	}

	int ObjectCaps() override { return CBaseMonster::ObjectCaps() | FCAP_IMPULSE_USE; }

	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;

	void TalkInit()
	{
		CTalkMonster::TalkInit();
		if (FClassnameIs(pev, "monster_zombie_fast"))
			m_voicePitch = 120;
		else
			m_voicePitch = 80;

		// replace with zombie sfx
		m_szGrp[TLK_ANSWER] = "HG_ANSWER";
		m_szGrp[TLK_QUESTION] = "HG_QUEST";
		m_szGrp[TLK_IDLE] = "HG_IDLE";
		m_szGrp[TLK_STARE] = "HG_CHECK";
		m_szGrp[TLK_USE] = "HG_TAUNT";
		m_szGrp[TLK_UNUSE] = "HG_TAUNT";
		m_szGrp[TLK_STOP] = "HG_TAUNT";

		m_szGrp[TLK_NOSHOOT] = "HG_TAUNT";
		m_szGrp[TLK_HELLO] = "HG_TAUNT";

		m_szGrp[TLK_PLHURT1] = "HG_TAUNT";
		m_szGrp[TLK_PLHURT2] = "HG_TAUNT";
		m_szGrp[TLK_PLHURT3] = "HG_TAUNT";

		m_szGrp[TLK_PHELLO] = "HG_TAUNT";
		m_szGrp[TLK_PIDLE] = "HG_TAUNT";
		m_szGrp[TLK_PQUESTION] = "HG_TAUNT";

		m_szGrp[TLK_SMELL] = "HG_TAUNT";

		m_szGrp[TLK_WOUND] = "HG_TAUNT";
		m_szGrp[TLK_MORTAL] = "HG_TAUNT";
	}
	void RunTask(Task_t* pTask)
	{
		switch (pTask->iTask)
		{
		case TASK_MOVE_TO_TARGET_RANGE:
		{
			float distance;

			if (m_hTargetEnt == NULL)
				TaskFail();
			else
			{
				distance = (m_vecMoveGoal - pev->origin).Length2D();
				// Re-evaluate when you think your finished, or the target has moved too far
				if ((distance < pTask->flData) || (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length() > pTask->flData * 0.5)
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					distance = (m_vecMoveGoal - pev->origin).Length2D();
					FRefreshRoute();
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if (distance < pTask->flData)
				{
					TaskComplete();
					RouteClear(); // Stop moving
				}
				else if (FClassnameIs(pev, "monster_zombie_fast"))
					m_movementActivity = ACT_RUN;
				else
					m_movementActivity = ACT_WALK;
			}

			break;
		}
		break;
		case TASK_MOVE_AWAY_PATH:
		{
			Vector dir = pev->angles;
			dir.y = pev->ideal_yaw + 180;
			Vector move;

			UTIL_MakeVectorsPrivate(dir, move, NULL, NULL);
			dir = pev->origin + move * pTask->flData;
			if (MoveToLocation(FClassnameIs(pev, "monster_zombie_fast") ? ACT_RUN : ACT_WALK, 2, dir))
			{
				TaskComplete();
			}
			else if (FindCover(pev->origin, pev->view_ofs, 0, CoverRadius()))
			{
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + 2;
				TaskComplete();
			}
			else
			{
				// nowhere to go?
				TaskFail();
			}
		}
		break;
		default:
			CTalkMonster::RunTask(pTask);
			break;
		}
	}

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS(monster_zombie, CZombie);
LINK_ENTITY_TO_CLASS(monster_zombie_barney, CZombie);
LINK_ENTITY_TO_CLASS(monster_zombie_soldier, CZombie);
LINK_ENTITY_TO_CLASS(monster_zombie_fast, CZombie);
LINK_ENTITY_TO_CLASS(monster_zombie_hev, CZombie);

const char* CZombie::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
};

const char* CZombie::pAttackMissSounds[] =
	{
		"zombie/claw_miss1.wav",
		"zombie/claw_miss2.wav",
};

const char* CZombie::pAttackSounds[] =
	{
		"zombie/zo_attack1.wav",
		"zombie/zo_attack2.wav",
};

const char* CZombie::pIdleSounds[] =
	{
		"zombie/zo_idle1.wav",
		"zombie/zo_idle2.wav",
		"zombie/zo_idle3.wav",
		"zombie/zo_idle4.wav",
};

const char* CZombie::pAlertSounds[] =
	{
		"zombie/zo_alert10.wav",
		"zombie/zo_alert20.wav",
		"zombie/zo_alert30.wav",
};

const char* CZombie::pPainSounds[] =
	{
		"zombie/zo_pain1.wav",
		"zombie/zo_pain2.wav",
};

Task_t tlZbFollow[] =
	{
		{TASK_MOVE_TO_TARGET_RANGE, (float)128}, // Move within 128 of target ent (client)
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE},
};

Schedule_t slZbFollow[] =
	{
		{tlZbFollow,
			ARRAYSIZE(tlZbFollow),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"Follow"},
};

Task_t tlZbFaceTarget[] =
	{
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_TARGET, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE},
};

Schedule_t slZbFaceTarget[] =
	{
		{tlZbFaceTarget,
			ARRAYSIZE(tlZbFaceTarget),
			bits_COND_CLIENT_PUSH |
				bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"FaceTarget"},
};

Task_t tlIdleZbStand[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},			// repick IDLESTAND every two seconds.
		{TASK_TLK_HEADRESET, (float)0}, // reset head position
};

Schedule_t slIdleZbStand[] =
	{
		{tlIdleZbStand,
			ARRAYSIZE(tlIdleZbStand),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_SMELL |
				bits_COND_PROVOKED,

			bits_SOUND_COMBAT | // sound flags - change these, and you'll break the talking code.
								// bits_SOUND_PLAYER		|
								// bits_SOUND_WORLD		|

				bits_SOUND_DANGER |
				bits_SOUND_MEAT | // scents
				bits_SOUND_CARCASS |
				bits_SOUND_GARBAGE,
			"IdleStand"},
};

Task_t tlZbStopFollowing[] =
	{
		{TASK_CANT_FOLLOW, (float)0},
};

Schedule_t slZbStopFollowing[] =
	{
		{tlZbStopFollowing,
			ARRAYSIZE(tlZbStopFollowing),
			0,
			0,
			"ZombieStopFollowing"},
};

DEFINE_CUSTOM_SCHEDULES(CZombie){
	slZbFollow,
	slZbFaceTarget,
	slIdleZbStand,
	slZbStopFollowing,
};

IMPLEMENT_CUSTOM_SCHEDULES(CZombie, CTalkMonster);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CZombie::Classify()
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
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie::SetYawSpeed()
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}

bool CZombie::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	bool ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) != 0)
	{
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == NULL)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) != 0 || IsFacing(pevAttacker, pev->origin))
			{
				m_bPrehuman = 1;
				ALERT(at_console, "Zombie hates you\n");
				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(true);
			}
			else
			{
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
	}

	return ret;
}

void CZombie::PainSound()
{
	if (FClassnameIs(pev, "monster_zombie_fast"))
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "zombie_fast/die.wav", 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 9));
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 9));
}

void CZombie::AlertSound()
{
	if (FClassnameIs(pev, "monster_zombie_fast"))
	{
		if (m_hEnemy != NULL)
		{
			if ((m_hEnemy->Center() - Center()).Length2D() < 1024)
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "zombie_fast/alert_close.wav", 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 9));
			else
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "zombie_fast/alert_far.wav", 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 9));
		}
	}
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 9));
}

void CZombie::IdleSound()
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}

void CZombie::AttackSound()
{
	if (FClassnameIs(pev, "monster_zombie_fast"))
	{
		const char* sound;
		{
			switch (RANDOM_LONG(1, 2))
			{
			case 1: sound = "zombie_fast/frenzy.wav"; break;
			case 2: sound = "zombie_fast/scream.wav"; break;
			}
		}
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, sound, 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
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

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case ZOMBIE_AE_ATTACK_LEFT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash left!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
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

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case ZOMBIE_AE_ATTACK_BOTH:
	{
		// do stuff for this event.
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgBothSlash, DMG_SLASH);
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

		if (RANDOM_LONG(0, 1))
			AttackSound();
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
void CZombie::Spawn()
{
	Precache();
	m_iHeadCrabHealth = 30;
	if (FClassnameIs(pev, "monster_zombie_barney"))
	{
		SET_MODEL(ENT(pev), "models/zombie_barney.mdl");
		if (g_iSkillLevel != SKILL_HARD)
		{
		pev->health = gSkillData.zombieHealth * 1.5;
		}
		else
		{
		pev->health = 90;
		}

	}
	else if (FClassnameIs(pev, "monster_zombie_soldier"))
	{
		SET_MODEL(ENT(pev), "models/zombie_soldier.mdl");
		if (g_iSkillLevel != SKILL_HARD)
		{
		pev->health = gSkillData.zombieHealth * 2;
		}
		else
		{
		pev->health = 95;
		}
	}
	else if (FClassnameIs(pev, "monster_zombie_fast"))
	{
		SET_MODEL(ENT(pev), "models/zombie_fast.mdl");
		if (g_iSkillLevel != SKILL_HARD)
		{
		pev->health = gSkillData.zombieHealth -10;
		}
		else
		{
		pev->health = 75; // bro has literally no protection
		}
	}
	else if (FClassnameIs(pev, "monster_zombie_hev")) // unused
	{
		SET_MODEL(ENT(pev), "models/zombie_hev.mdl");
		pev->health = 10;
	}
	else
	{
		SET_MODEL(ENT(pev), "models/zombie.mdl");
		pev->health = gSkillData.zombieHealth;
		if (g_iSkillLevel != SKILL_HARD)
		{
		pev->health = gSkillData.zombieHealth;
		}
		else
		{
		pev->health = 85;
		}
	}

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.

	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
	if (m_bPrehuman == 0)
	{
		SetUse(&CZombie::FollowerUse);
	}
	
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache()
{
	if (FClassnameIs(pev, "monster_zombie_barney"))
		PRECACHE_MODEL("models/zombie_barney.mdl");
	else if (FClassnameIs(pev, "monster_zombie_soldier"))
		PRECACHE_MODEL("models/zombie_soldier.mdl");
	else if (FClassnameIs(pev, "monster_zombie_hev"))
		PRECACHE_MODEL("models/zombie_hev.mdl");
	else if (FClassnameIs(pev, "monster_zombie_fast"))
	{
		PRECACHE_MODEL("models/zombie_fast.mdl");
		PRECACHE_SOUND("zombie_fast/alert_close.wav");
		PRECACHE_SOUND("zombie_fast/alert_far.wav");
		PRECACHE_SOUND("zombie_fast/breath.wav");
		PRECACHE_SOUND("zombie_fast/die.wav");
		PRECACHE_SOUND("zombie_fast/frenzy.wav");
		PRECACHE_SOUND("zombie_fast/gurgle.wav");
		PRECACHE_SOUND("zombie_fast/scream.wav");
	}
	else
		PRECACHE_MODEL("models/zombie.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);

	UTIL_PrecacheOther("monster_headcrab");
	TalkInit();
	CTalkMonster::Precache();
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CZombie::IgnoreConditions()
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

Schedule_t* CZombie::GetScheduleOfType(int Type)
{
	Schedule_t* psched;

	switch (Type)
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slZbFaceTarget; // override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slZbFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleZbStand;
		}
		else
			return psched;
	case SCHED_CANT_FOLLOW:
		return slZbStopFollowing;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

Schedule_t* CZombie::GetSchedule()
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (m_hEnemy == NULL && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(false);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

//=========================================================
// DEAD GONOME PROP
//=========================================================
class CDeadGonome : public CBaseMonster
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_ALIEN_PASSIVE; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static char* m_szPoses[3];
};

char* CDeadGonome::m_szPoses[] = {"dead_on_stomach1", "dead_on_back", "dead_on_side"};

bool CDeadGonome::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_gonome_dead, CDeadGonome);

//=========================================================
// ********** DeadGonome SPAWN **********
//=========================================================
void CDeadGonome::Spawn()
{
	PRECACHE_MODEL("models/gonome.mdl");
	SET_MODEL(ENT(pev), "models/gonome.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_GREEN;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead gonome with bad pose\n");
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}