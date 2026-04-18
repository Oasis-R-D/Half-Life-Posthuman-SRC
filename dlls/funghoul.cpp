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
// Funghoul
//=========================================================

// 112, 8, 32 could be a good blood color

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "decals.h"
#include "soundent.h"
#include "player.h"
#include "animation.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ZOMBIE_AE_ATTACK_RIGHT 0x01
#define ZOMBIE_AE_ATTACK_LEFT 0x02
#define ZOMBIE_AE_ATTACK_GUTS_GRAB 0x03
#define ZOMBIE_AE_ATTACK_GUTS_THROW 4
#define GONOME_AE_ATTACK_BITE_FIRST 19
#define GONOME_AE_ATTACK_BITE_SECOND 20
#define GONOME_AE_ATTACK_BITE_THIRD 21
#define GONOME_AE_ATTACK_BITE_FINISH 22
#define GONOME_AE_ATTACK_GRAB_START 23 	// grab the enemy
#define GONOME_AE_ATTACK_GRAB_BITE 24	// enemy is held
										// none for letting go (instead staggers backwards (player broke free))

// for the type variable
#define FUNGHOUL 0
#define FUNGHOUL_INFECTOR 1
#define FUNGHOUL_ADVSEC 2
#define FUNGHOUL_SPITTER 3

#define LIMBBREAK_THRESH 20

#pragma region projectile
class CFunghoulGuts : public CBaseEntity
{
public:
	using BaseClass = CBaseEntity;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;

	void Touch(CBaseEntity* pOther) override;

	void EXPORT Animate();

	static void Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity);

	static CFunghoulGuts* GonomeGutsCreate(const Vector& origin);

	void Launch(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity);

	int m_maxFrame;
};

TYPEDESCRIPTION CFunghoulGuts::m_SaveData[] =
	{
		DEFINE_FIELD(CFunghoulGuts, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CFunghoulGuts, CFunghoulGuts::BaseClass);

LINK_ENTITY_TO_CLASS(funghoulguts, CFunghoulGuts);

void CFunghoulGuts::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("funghoulguts");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	// TO-DO: probably shouldn't be assinging to x every time
	if (g_Language == LANGUAGE_GERMAN)
	{
		SET_MODEL(edict(), "sprites/bigspit.spr");
		pev->rendercolor.x = 0;
		pev->rendercolor.x = 255;
		pev->rendercolor.x = 0;
	}
	else
	{
		SET_MODEL(edict(), "sprites/bigspit.spr");
		pev->rendercolor.x = 128;
		pev->rendercolor.y = 48;
		pev->rendercolor.z = 0;
	}

	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	m_maxFrame = static_cast<int>(MODEL_FRAMES(pev->modelindex) - 1);
}

void CFunghoulGuts::Touch(CBaseEntity* pOther)
{
	// splat sound
	const auto iPitch = RANDOM_FLOAT(90, 110);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch);

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}

	if (0 == pOther->pev->takedamage)
	{
		TraceResult tr;
		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
		UTIL_BloodDrips(tr.vecEndPos, tr.vecPlaneNormal, BLOOD_COLOR_RED, 35);
	}
	else
	{
		pOther->TakeDamage(pev, pev, gSkillData.funghoulDmgBite, DMG_ACID);
	}

	SetThink(&CFunghoulGuts::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

void CFunghoulGuts::Animate()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (0 != pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}
}

void CFunghoulGuts::Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity)
{
	auto pGuts = GetClassPtr<CFunghoulGuts>(nullptr);
	pGuts->Spawn();

	UTIL_SetOrigin(pGuts->pev, vecStart);
	pGuts->pev->velocity = vecVelocity;
	pGuts->pev->owner = ENT(pevOwner);

	if (pGuts->m_maxFrame > 0)
	{
		pGuts->SetThink(&CFunghoulGuts::Animate);
		pGuts->pev->nextthink = gpGlobals->time + 0.1;
	}
}

CFunghoulGuts* CFunghoulGuts::GonomeGutsCreate(const Vector& origin)
{
	auto pGuts = GetClassPtr<CFunghoulGuts>(nullptr);
	pGuts->Spawn();

	pGuts->pev->origin = origin;

	return pGuts;
}

void CFunghoulGuts::Launch(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity)
{
	UTIL_SetOrigin(pev, vecStart);
	pev->velocity = vecVelocity;
	pev->owner = ENT(pevOwner);

	SetThink(&CFunghoulGuts::Animate);
	pev->nextthink = gpGlobals->time + 0.1;
}
#pragma endregion

enum
{
	TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1,
	TASK_STAGGER,
};


class CFunghoul : public CBaseMonster
{
public:
	using BaseClass = CBaseMonster;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void MonsterThink() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int IgnoreConditions() override;

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;

	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];

	// No range attacks
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override { return false; }
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckMeleeAttack2(float flDot, float flDist) override;

	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;

	void Killed(entvars_t* pevAttacker, int iGib) override;

	void RunTask(Task_t* pTask) override;
	void StartTask(Task_t* pTask) override;

	void SetActivity(Activity NewActivity) override;

	CUSTOM_SCHEDULES;

	int m_iType = FUNGHOUL;

	// dismemberment (yay :D)
	// TO-DO: add #defines for the stub locations such as: (pev->origin + gpglobals->v_right + 4)
	int m_iArmRh = 0; // disables part of swinging, reduces grab strength
	int m_iArmLh = 0;
	int m_iLegRh = 0; // forces into crawling
	int m_iLegLh = 0;

	float m_flNextThrowTime;

	//TODO: needs to be EHANDLE, save/restored or a save during a windup will cause problems
	CFunghoulGuts* m_pGonomeGuts;
	EHANDLE m_PlayerLocked;
};

TYPEDESCRIPTION CFunghoul::m_SaveData[] =
	{
		DEFINE_FIELD(CFunghoul, m_flNextThrowTime, FIELD_TIME),
		DEFINE_FIELD(CFunghoul, m_PlayerLocked, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CFunghoul, CFunghoul::BaseClass);

LINK_ENTITY_TO_CLASS(monster_funghoul, CFunghoul); // gonome but only melee
LINK_ENTITY_TO_CLASS(monster_funghoul_infector, CFunghoul); // Attacking holds player and does damage over time (player can escape by fighting the pull, stumbles funghoul)
LINK_ENTITY_TO_CLASS(monster_funghoul_spitter, CFunghoul); // gonome
LINK_ENTITY_TO_CLASS(monster_funghoul_advsec, CFunghoul); // gonome melee but tankier

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_FUNGHOUL_STAGGER = LAST_COMMON_SCHEDULE + 1,
};

const char* CFunghoul::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
};

const char* CFunghoul::pAttackMissSounds[] =
	{
		"zombie/claw_miss1.wav",
		"zombie/claw_miss2.wav",
};

const char* CFunghoul::pIdleSounds[] =
	{
		"funghoul/gonome_idle1.wav",
		"funghoul/gonome_idle2.wav",
		"funghoul/gonome_idle3.wav",
};

const char* CFunghoul::pAlertSounds[] =
	{
		"zombie/zo_alert10.wav",
		"zombie/zo_alert20.wav",
		"zombie/zo_alert30.wav",
};

const char* CFunghoul::pPainSounds[] =
	{
		"funghoul/gonome_pain1.wav",
		"funghoul/gonome_pain2.wav",
		"funghoul/gonome_pain3.wav",
		"funghoul/gonome_pain4.wav",
};

//=========================================================
// BarnacleVictimGrab - barnacle tongue just hit the monster,
// so play a hit animation, then play a cycling pull animation
// as the creature is hoisting the monster.
//=========================================================
Task_t tlFunghoulGrab[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_FAIL_SCHEDULE, (float) SCHED_FUNGHOUL_STAGGER }, // TO-DO: flinch
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_MELEE_ATTACK2},
		{TASK_SET_ACTIVITY, (float)ACT_SPECIAL_ATTACK1},
		{TASK_WAIT_INDEFINITE, (float)0}, // just cycle barnacle pull anim while barnacle hoists.
};

Schedule_t slFunghoulGrab[] =
	{
		{tlFunghoulGrab,
			ARRAYSIZE(tlFunghoulGrab),
			bits_COND_HEAVY_DAMAGE,
			bits_SOUND_NONE,
			"Victim Grab"}};

Task_t tlFunghoulStagger[] =
{
    { TASK_REMEMBER,     (float)bits_MEMORY_FLINCHED }, // Remember in my memory that I flinched
    { TASK_STOP_MOVING,  0                           }, // Stop moving
    { TASK_STAGGER, 0								 }, // Make a small flinch
};

Schedule_t slFunghoulStagger[] =
{
    {
        tlFunghoulStagger,            // This schedule use the "tlSmallFlinch" tasks array
        ARRAYSIZE(tlFunghoulStagger), // "ARRAYSIZE" will return 3 here and tell this schedule has 3 tasks
        0,                        // No condition bit can interrupt this schedule
        0,                        // No sound bit can interrupt this schedule
        "Stagger"            // This schedule is named "Small Flinch"
    },
};

/*
//=========================================================
// BarnacleVictimChomp - barnacle has pulled the prey to its
// mouth. Victim should play the BARNCLE_CHOMP animation
// once, then loop the BARNACLE_CHEW animation indefinitely
//=========================================================
Task_t tlFunghoulBiting[] = // probably uneeded, change pull loop above to the biting
	{
		{TASK_STOP_MOVING, 0},
		{TASK_PLAY_SEQUENCE, (float)ACT_BARNACLE_CHOMP},
		{TASK_SET_ACTIVITY, (float)ACT_BARNACLE_CHEW},
		{TASK_WAIT_INDEFINITE, (float)0}, // just cycle barnacle pull anim while barnacle hoists.
};

Schedule_t slFunghoulBiting[] =
	{
		{tlFunghoulBiting,
			ARRAYSIZE(tlFunghoulBiting),
			0,
			0,
			"Barnacle Chomp"}};
*/
Task_t tlGonomeVictoryDance[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_FAIL_SCHEDULE, SCHED_IDLE_STAND},
		{TASK_WAIT, 0.2},
		{TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE, 0},
		{TASK_WALK_PATH, 0},
		{TASK_WAIT_FOR_MOVEMENT, 0},
		{TASK_FACE_ENEMY, 0},
		{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE},
};

Schedule_t slGonomeVictoryDance[] =
	{
		{
			tlGonomeVictoryDance,
			ARRAYSIZE(tlGonomeVictoryDance),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE,
			bits_SOUND_NONE,
			"BabyVoltigoreVictoryDance" //Yup, it's a copy
		},
};

//=========================================================
// InvestigateSound - sends a monster to the location of the
// sound that was just heard, to check things out.
//=========================================================
Task_t tlFunghoulInvestigate[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_STORE_LASTPOSITION, (float)0},
		{TASK_GET_PATH_TO_BESTSOUND, (float)0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_IDLE},
		{TASK_WAIT, (float)6},
		{TASK_GET_PATH_TO_LASTPOSITION, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_CLEAR_LASTPOSITION, (float)0},
};

Schedule_t slFunghoulInvestigate[] =
	{
		{tlFunghoulInvestigate,
			ARRAYSIZE(tlFunghoulInvestigate),
			bits_COND_NEW_ENEMY |
				bits_COND_SEE_FEAR |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"FunghoulInvestigateSound"},
};

DEFINE_CUSTOM_SCHEDULES(CFunghoul){
	slGonomeVictoryDance,
	slFunghoulGrab,
	slFunghoulStagger,
	slFunghoulInvestigate,
};

IMPLEMENT_CUSTOM_SCHEDULES(CFunghoul, CBaseMonster);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CFunghoul::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFunghoul::SetYawSpeed()
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

void CFunghoul::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (ptr->iHitgroup == HITGROUP_CHEST || ptr->iHitgroup == HITGROUP_STOMACH)
	{
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST)) != 0)
		{
			if (m_iType == FUNGHOUL_ADVSEC)
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
	m_bloodColor = BLOOD_COLOR_RED; // switch it back to red
}

bool CFunghoul::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// Take 15% damage from bullets
	if (bitsDamageType == DMG_BULLET)
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.15;
	}

	// HACK HACK -- until we fix this.
	if (IsAlive())
		PainSound();
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CFunghoul::PainSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CFunghoul::AlertSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CFunghoul::IdleSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random idle sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pIdleSounds[RANDOM_LONG(0, ARRAYSIZE(pIdleSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CFunghoul::MonsterThink()
{
	if (m_iType == FUNGHOUL_INFECTOR && m_PlayerLocked)
	{
		if (m_PlayerLocked->IsPlayer())
		{
			auto player = m_PlayerLocked.Entity<CBasePlayer>();

			if (player)
			{
				if (player->IsAlive())
				{
					Vector towardsP = pev->origin - player->pev->origin;
					if (towardsP.Length2D() > 64) // player escaped
					{
						TaskFail();
						m_PlayerLocked = NULL;
						player->m_iSpeedOverride = -1;
					}
				}
				else
				{
					TaskFail();
					m_PlayerLocked = NULL;
				}
			}
		}
		else
		{
			auto monster = m_PlayerLocked.Entity<CBaseMonster>();

			if (monster)
			{
				if (monster->IsAlive())
				{
					Vector towardsP = pev->origin - monster->pev->origin;
					if (towardsP.Length2D() > 64) // player escaped
					{
						TaskFail();
						m_PlayerLocked = NULL;
					}
				}
				else // escape once enemy is dead
				{
					TaskFail();
					m_PlayerLocked = NULL;
				}
			}
		}
	}
	else if (m_Activity == ACT_SPECIAL_ATTACK1)
		TaskFail(); // make sure that it cannot get stuck

	CBaseMonster::MonsterThink();
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CFunghoul::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
		if (m_iArmRh >= LIMBBREAK_THRESH)
			break; // no arm

		// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.funghoulDmgSlash, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = -9;
				pHurt->pev->punchangle.x = 2;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 25;
			}
			// Play a random attack hit sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else // Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	break;

	case ZOMBIE_AE_ATTACK_LEFT:
	{
		if (m_iArmLh >= LIMBBREAK_THRESH)
			break; // no arm

		// do stuff for this event.
		//		ALERT( at_console, "Slash left!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.funghoulDmgSlash, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = 9;
				pHurt->pev->punchangle.x = 2;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 25;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	break;

	case ZOMBIE_AE_ATTACK_GUTS_GRAB: // unused
	{
		//Only if we still have an enemy at this point
		if (m_hEnemy)
		{
			Vector vecGutsPos, vecGutsAngles;
			GetAttachment(0, vecGutsPos, vecGutsAngles);

			if (!m_pGonomeGuts)
			{
				m_pGonomeGuts = CFunghoulGuts::GonomeGutsCreate(vecGutsPos);
			}

			//Attach to hand for throwing
			m_pGonomeGuts->pev->skin = entindex();
			m_pGonomeGuts->pev->body = 1;
			m_pGonomeGuts->pev->aiment = edict();
			m_pGonomeGuts->pev->movetype = MOVETYPE_FOLLOW;

			auto direction = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecGutsPos).Normalize();

			direction = direction + Vector(
										RANDOM_FLOAT(-0.05, 0.05),
										RANDOM_FLOAT(-0.05, 0.05),
										RANDOM_FLOAT(-0.05, 0));

			UTIL_BloodDrips(vecGutsPos, direction, BLOOD_COLOR_RED, 35);
		}
	}
	break;

	case ZOMBIE_AE_ATTACK_GUTS_THROW:
	{
		//Note: this check wasn't in the original. If an enemy dies during gut throw windup, this can be null and crash
		if (m_hEnemy)
		{
			Vector vecGutsPos, vecGutsAngles;
			GetAttachment(0, vecGutsPos, vecGutsAngles);

			UTIL_MakeVectors(pev->angles);

			if (!m_pGonomeGuts)
			{
				m_pGonomeGuts = CFunghoulGuts::GonomeGutsCreate(vecGutsPos);
			}

			auto direction = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecGutsPos).Normalize();

			direction = direction + Vector(
										RANDOM_FLOAT(-0.05, 0.05),
										RANDOM_FLOAT(-0.05, 0.05),
										RANDOM_FLOAT(-0.05, 0));

			UTIL_BloodDrips(vecGutsPos, direction, BLOOD_COLOR_RED, 35);

			//Detach from owner
			m_pGonomeGuts->pev->skin = 0;
			m_pGonomeGuts->pev->body = 0;
			m_pGonomeGuts->pev->aiment = nullptr;
			m_pGonomeGuts->pev->movetype = MOVETYPE_FLY;

			m_pGonomeGuts->Launch(pev, vecGutsPos, direction * 900);
		}
		else
		{
			UTIL_Remove(m_pGonomeGuts);
		}

		m_pGonomeGuts = nullptr;
	}
	break;
	case GONOME_AE_ATTACK_GRAB_START:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(74, gSkillData.funghoulDmgSlash/2, DMG_SLASH);
		if (pHurt && pHurt->IsAlive())
		{
			m_PlayerLocked = pHurt; // use to moniter distance
			if (pHurt->IsPlayer())
			{
				CBasePlayer* player = dynamic_cast<CBasePlayer*>(pHurt);
				player->m_iSpeedOverride = 30;
				player->pev->velocity = player->pev->velocity * 0.5;
			}
			else if ((pHurt->pev->flags & FL_MONSTER) != 0)
			{
				// freeze movement somehow someway
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
		{
			if (!pHurt)
			{
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
	}
	break;

	case GONOME_AE_ATTACK_GRAB_BITE:
	{	// TO-DO: add sfx (blood too?)
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "funghoul/placeholderBITE.wav", 0.75, ATTN_STATIC, 0, 100 + RANDOM_LONG(-15, 5));
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.funghoulDmgBite, DMG_POISON);
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
void CFunghoul::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/funghoul.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED; // TO-DO: custom color (replaces blue?)
	pev->health = gSkillData.funghoulHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.75;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	m_flNextThrowTime = gpGlobals->time;
	m_pGonomeGuts = nullptr;
	m_PlayerLocked = nullptr;

	m_flDistTooFar = 3072;
	m_flDistLook = 3072; //idk if this is needed

	// only have to compare the strings 3 times instead of every attack
	if (FClassnameIs(pev, "monster_funghoul_advsec"))
	{
		m_iType = FUNGHOUL_ADVSEC;
		pev->body = 2;
	}
	else if (FClassnameIs(pev, "monster_funghoul_infector"))
	{
		m_iType = FUNGHOUL_INFECTOR;
		pev->body = 4;
	}
	else if (FClassnameIs(pev, "monster_funghoul_spitter"))
	{
		m_iType = FUNGHOUL_SPITTER;
		pev->body = 3;
	}

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFunghoul::Precache()
{
	int i;

	PRECACHE_MODEL("models/funghoul.mdl");
	PRECACHE_MODEL("sprites/bigspit.spr");

	// TO-DO: isn't there a macro for this?
	for (i = 0; i < ARRAYSIZE(pAttackHitSounds); i++)
		PRECACHE_SOUND((char*)pAttackHitSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackMissSounds); i++)
		PRECACHE_SOUND((char*)pAttackMissSounds[i]);

	for (i = 0; i < ARRAYSIZE(pIdleSounds); i++)
		PRECACHE_SOUND((char*)pIdleSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAlertSounds); i++)
		PRECACHE_SOUND((char*)pAlertSounds[i]);

	for (i = 0; i < ARRAYSIZE(pPainSounds); i++)
		PRECACHE_SOUND((char*)pPainSounds[i]);

	PRECACHE_SOUND("funghoul/gonome_death2.wav");
	PRECACHE_SOUND("funghoul/gonome_death3.wav");
	PRECACHE_SOUND("funghoul/gonome_death4.wav");

	PRECACHE_SOUND("funghoul/gonome_jumpattack.wav");

	PRECACHE_SOUND("funghoul/gonome_melee1.wav");
	PRECACHE_SOUND("funghoul/gonome_melee2.wav");

	PRECACHE_SOUND("funghoul/gonome_run.wav");
	PRECACHE_SOUND("funghoul/gonome_eat.wav");

	PRECACHE_SOUND("funghoul/placeholderBITE.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CFunghoul::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity == ACT_RANGE_ATTACK1)
	{
		iIgnore |= bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_TOOFAR | bits_COND_ENEMY_OCCLUDED;
	}

	iIgnore |= bits_COND_LIGHT_DAMAGE;

	return iIgnore;
}

bool CFunghoul::CheckMeleeAttack1(float flDot, float flDist)
{
	if (m_iType == FUNGHOUL_INFECTOR || (m_iArmLh >= LIMBBREAK_THRESH && m_iArmRh >= LIMBBREAK_THRESH))
		return false; // no arms to swipe with (skill issue)

	if (flDist <= 64.0 && flDot >= 0.7 && m_hEnemy)
	{
		return (m_hEnemy->pev->flags & FL_ONGROUND) != 0;
	}

	return false;
}

bool CFunghoul::CheckMeleeAttack2(float flDot, float flDist)
{
	if (m_iType != FUNGHOUL_INFECTOR)
		return false;

	if (flDist <= 48.0 && flDot >= 0.7 && m_hEnemy)
	{
		return (m_hEnemy->pev->flags & FL_ONGROUND) != 0;
	}

	return false;
}

bool CFunghoul::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_iType != FUNGHOUL_SPITTER)
		return false;

	if (flDist < 256.0)
		return false;

	if (IsMoving() && flDist >= 512.0)
	{
		return false;
	}


	if (flDist > 64.0 && flDist <= 784.0 && flDot >= 0.5 && gpGlobals->time >= m_flNextThrowTime)
	{
		if (!m_hEnemy || (fabs(pev->origin.z - m_hEnemy->pev->origin.z) <= 256.0))
		{
			if (IsMoving())
			{
				m_flNextThrowTime = gpGlobals->time + 5.0;
			}
			else
			{
				m_flNextThrowTime = gpGlobals->time + 0.5;
			}

			return true;
		}
	}

	return false;
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t* CFunghoul::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND) && !HasConditions(bits_COND_SEE_ENEMY)) // investigate sounds
	{
		CSound* pSound;
		pSound = PBestSound();
		ASSERT(pSound != NULL);
		if (pSound)
		{
			if (pSound && (pSound->m_iType & bits_SOUND_COMBAT | bits_SOUND_PLAYER) != 0) // Hear an enemy
			{
				return GetScheduleOfType(SCHED_INVESTIGATE_SOUND);
			}
		}
	}

	return CBaseMonster::GetSchedule();
}

Schedule_t* CFunghoul::GetScheduleOfType(int Type)
{
	if (Type == SCHED_VICTORY_DANCE)
		return slGonomeVictoryDance;
	else if (Type == SCHED_MELEE_ATTACK2)
		return slFunghoulGrab;
	else if (Type == SCHED_FUNGHOUL_STAGGER)
		return slFunghoulStagger;
	else if (Type == SCHED_INVESTIGATE_SOUND)
		return slFunghoulInvestigate; // override this
	else
		return CBaseMonster::GetScheduleOfType(Type);
}

void CFunghoul::Killed(entvars_t* pevAttacker, int iGib)
{
	if (m_pGonomeGuts)
	{
		UTIL_Remove(m_pGonomeGuts);
		m_pGonomeGuts = nullptr;
	}

	if (m_PlayerLocked)
	{
		if (m_PlayerLocked->IsPlayer())
		{
			auto player = m_PlayerLocked.Entity<CBasePlayer>();

			if (player)
			{
				if (player && player->IsAlive())
					player->EnableControl(true);

				player->m_iSpeedOverride = -1;
			}
		}
		else
		{
			auto monster = m_PlayerLocked.Entity<CBaseMonster>();

			if (monster)
			{
				// free movement
			}
		}
	}

	m_PlayerLocked = nullptr;

	CBaseMonster::Killed(pevAttacker, iGib);
}

void CFunghoul::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_STAGGER:
	{
		if (m_fSequenceFinished)
		{
			TaskComplete();
		}
	}
	break;
	default:
		CBaseMonster::RunTask(pTask);
		break;
	}
}

void CFunghoul::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE:
	{
		if (m_pGonomeGuts)
		{
			UTIL_Remove(m_pGonomeGuts);
			m_pGonomeGuts = nullptr;
		}

		UTIL_MakeVectors(pev->angles);

		if (BuildRoute(m_vecEnemyLKP - 64 * gpGlobals->v_forward, 64, nullptr))
		{
			TaskComplete();
		}
		else
		{
			ALERT(at_aiconsole, "GonomeGetPathToEnemyCorpse failed!!\n");
			TaskFail();
		}
	}
	break;
	case TASK_STAGGER:
	{
		m_IdealActivity = ACT_BIG_FLINCH;

		if (m_iType == FUNGHOUL_INFECTOR && m_PlayerLocked)
		{
			if (m_PlayerLocked->IsPlayer())
			{
				auto player = m_PlayerLocked.Entity<CBasePlayer>();

				if (player && player->IsAlive())
				{
					player->m_iSpeedOverride = -1;
				}
			}
			else
			{
				auto monster = m_PlayerLocked.Entity<CBaseMonster>();

				if (monster)
				{
					if (monster->IsAlive())
					{
						// free
					}
				}
			}

			TaskFail();
			m_PlayerLocked = NULL;
		}
	}
	break;
	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

void CFunghoul::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	if (NewActivity != ACT_RANGE_ATTACK1 && m_pGonomeGuts)
	{
		UTIL_Remove(m_pGonomeGuts);
		m_pGonomeGuts = nullptr;
	}

	switch (NewActivity)
	{
	case ACT_MELEE_ATTACK1:
		if (m_hEnemy)
		{
			iSequence = LookupSequence("whip");
		}
		else
		{
			iSequence = LookupActivity(ACT_MELEE_ATTACK1);
		}
		break;
	case ACT_MELEE_ATTACK2:
		if (m_hEnemy)
		{
			iSequence = LookupSequence("bite"); // TO-DO: grab anim
		}
		else
		{
			iSequence = LookupActivity(ACT_MELEE_ATTACK2);
		}
		break;
	case ACT_RUN:
		if (m_hEnemy)
		{
			if ((pev->origin - m_hEnemy->pev->origin).Length() <= 512)
			{
				iSequence = LookupSequence("run");
			}
			else
			{
				iSequence = 2; // hopefully this is legal
			}
		}
		else
		{
			iSequence = LookupActivity(ACT_RUN);
		}
		break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence; // Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0; // Set to the reset anim (if it's there)
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	m_IdealActivity = NewActivity;
}