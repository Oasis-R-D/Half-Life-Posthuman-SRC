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
// YOU KNOW WHAT THAT MEANS ><(((°>
// (uncut archer enemy)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "flyingmonster.h"
#include "nodes.h"
#include "soundent.h"
#include "animation.h"
#include "effects.h"
#include "weapons.h"
#include "physical_bullet.h" // TO-DO: actual projectile

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CArcherSpike : public CBaseEntity
{
	int m_iTrail;
public:
	void Precache() override;
	void Spawn() override;

	int Classify() override { return CLASS_NONE; }

	static void Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles);
	void EXPORT SpikeTouch(CBaseEntity* pOther);
	void EXPORT BubbleThink();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_maxFrame;
};

LINK_ENTITY_TO_CLASS(archerspike, CArcherSpike);

TYPEDESCRIPTION CArcherSpike::m_SaveData[] =
	{
		DEFINE_FIELD(CArcherSpike, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CArcherSpike, CBaseEntity);

void CArcherSpike::BubbleThink()
{
	if (pev->waterlevel != 0)
		UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1f, pev->origin, 1);

	pev->nextthink = gpGlobals->time + 0.1;
}

void CArcherSpike::Precache()
{
	PRECACHE_MODEL("models/pit_drone_spike.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");

	m_iTrail = PRECACHE_MODEL("sprites/RCtrail.spr");
}

void CArcherSpike::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("archerspike");

	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags |= FL_MONSTER;
	pev->health = 1;

	SET_MODEL(ENT(pev), "models/pit_drone_spike.mdl");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

	SetThink(&CArcherSpike::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.1;

	SetTouch(&CArcherSpike::SpikeTouch);

	// TRAIL START
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex());		 // entity
	WRITE_SHORT(m_iTrail);			 // model
	WRITE_BYTE(RANDOM_LONG(2, 3));	 // life
	WRITE_BYTE(2);					 // width
	WRITE_BYTE(128);				 // r, g, b
	WRITE_BYTE(128);				 // r, g, b
	WRITE_BYTE(128);				 // r, g, b
	WRITE_BYTE(RANDOM_LONG(60, 80)); // brightness
	MESSAGE_END();
}

void CArcherSpike::Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles)
{
	CArcherSpike* pSpit = GetClassPtr((CArcherSpike*)NULL);

	pSpit->pev->angles = vecAngles;
	UTIL_SetOrigin(pSpit->pev, vecStart);

	pSpit->Spawn();

	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
}

void CArcherSpike::SpikeTouch(CBaseEntity* pOther)
{
	// splat sound
	int iPitch = RANDOM_FLOAT(120, 140);

	if (0 == pOther->pev->takedamage)
	{
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "weapons/xbow_hit1.wav", VOL_NORM, ATTN_NORM, 0, iPitch);
	}
	else
	{
		if (g_iSkillLevel != SKILL_REALISM)
		{
			pOther->TakeDamage(pev, pev, gSkillData.pitdroneDmgSpit, DMG_SLASH);
		}
		else
		{
			pOther->TakeDamage(pev, pev, 40, DMG_SLASH);
		}
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "weapons/xbow_hitbod1.wav", VOL_NORM, ATTN_NORM, 0, iPitch);
	}

	SetTouch(nullptr);

	//Stick it in the world for a bit
	//TODO: maybe stick it on any entity that reports FL_WORLDBRUSH too?
	if (0 == strcmp("worldspawn", STRING(pOther->pev->classname)))
	{
		const auto vecDir = pev->velocity.Normalize();

		const auto vecOrigin = pev->origin - vecDir * 6;

		UTIL_SetOrigin(pev, vecOrigin);

		auto v41 = UTIL_VecToAngles(vecDir);

		pev->angles = UTIL_VecToAngles(vecDir);
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_FLY;

		pev->angles.z = RANDOM_LONG(0, 360);

		pev->velocity = g_vecZero;
		pev->avelocity = g_vecZero;

		SetThink(&CBaseEntity::SUB_FadeOut);
		pev->nextthink = gpGlobals->time + 90.0;
	}
	else
	{
		//Hit something else, remove
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

#define ICHTHYOSAUR_SPEED 100

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

// UNDONE: Save/restore here
class CArcher : public CFlyingMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	CUSTOM_SCHEDULES;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;

	void Killed(entvars_t* pevAttacker, int iGib) override;
	void BecomeDead() override;

	void EXPORT CombatUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT BiteTouch(CBaseEntity* pOther);

	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;

	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;

	float ChangeYaw(int speed) override;
	Activity GetStoppedActivity() override;
	void SetActivity(Activity NewActivity) override;

	void Move(float flInterval) override;
	void MoveExecute(CBaseEntity* pTargetEnt, const Vector& vecDir, float flInterval) override;
	void MonsterThink() override;
	void Stop() override;
	void Swim();
	Vector DoProbe(const Vector& Probe);

	float VectorToPitch(const Vector& vec);
	float FlPitchDiff();
	float ChangePitch(int speed);

	Vector m_SaveVelocity;
	float m_idealDist;

	float m_flBlink;

	float m_flEnemyTouched;
	bool m_bOnAttack;

	float m_flMaxSpeed;
	float m_flMinSpeed;
	float m_flMaxDist;

	CBeam* m_pBeam;

	float m_flNextAlert;

	float m_flLastPitchTime;

	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pAttackSounds[];
	static const char* pBiteSounds[];
	static const char* pDieSounds[];
	static const char* pPainSounds[];

	void IdleSound() override;
	void AlertSound() override;
	void AttackSound();
	void BiteSound();
	void DeathSound() override;
	void PainSound() override;
	bool FVisible(CBaseEntity* pEntity) override;
};

LINK_ENTITY_TO_CLASS(monster_archer, CArcher);

TYPEDESCRIPTION CArcher::m_SaveData[] =
	{
		DEFINE_FIELD(CArcher, m_SaveVelocity, FIELD_VECTOR),
		DEFINE_FIELD(CArcher, m_idealDist, FIELD_FLOAT),
		DEFINE_FIELD(CArcher, m_flBlink, FIELD_FLOAT),
		DEFINE_FIELD(CArcher, m_flEnemyTouched, FIELD_FLOAT),
		DEFINE_FIELD(CArcher, m_bOnAttack, FIELD_BOOLEAN),
		DEFINE_FIELD(CArcher, m_flMaxSpeed, FIELD_FLOAT),
		DEFINE_FIELD(CArcher, m_flMinSpeed, FIELD_FLOAT),
		DEFINE_FIELD(CArcher, m_flMaxDist, FIELD_FLOAT),
		DEFINE_FIELD(CArcher, m_flNextAlert, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CArcher, CFlyingMonster);

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
bool CArcher::FVisible(CBaseEntity* pEntity)
{
	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecTargetOrigin;

	if (FBitSet(pEntity->pev->flags, FL_NOTARGET))
		return false;

	vecLookerOrigin = pev->origin + pev->view_ofs; //look through the caller's 'eyes'

	// Check for heads // TO-DO: isn't this extraneous since if the head is visible, the min or max probably is too
	vecTargetOrigin = pEntity->EyePosition();
	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT(pev) /*pentIgnore*/, &tr);

	if (tr.flFraction == 1.0) // Line of sight is valid
		return true;

	return false;
}

const char* CArcher::pIdleSounds[] =
	{
		"ichy/ichy_idle1.wav",
		"ichy/ichy_idle2.wav",
		"ichy/ichy_idle3.wav",
		"ichy/ichy_idle4.wav",
};

const char* CArcher::pAlertSounds[] =
	{
		"ichy/ichy_alert2.wav",
		"ichy/ichy_alert3.wav",
};

const char* CArcher::pAttackSounds[] =
	{
		"ichy/ichy_attack1.wav",
		"ichy/ichy_attack2.wav",
};

const char* CArcher::pBiteSounds[] =
	{
		"ichy/ichy_bite1.wav",
		"ichy/ichy_bite2.wav",
};

const char* CArcher::pPainSounds[] =
	{
		"ichy/ichy_pain2.wav",
		"ichy/ichy_pain3.wav",
		"ichy/ichy_pain5.wav",
};

const char* CArcher::pDieSounds[] =
	{
		"ichy/ichy_die2.wav",
		"ichy/ichy_die4.wav",
};

#define EMIT_ICKY_SOUND(chan, array) \
	EMIT_SOUND_DYN(ENT(pev), chan, array[RANDOM_LONG(0, ARRAYSIZE(array) - 1)], 1.0, 0.6, 0, RANDOM_LONG(95, 105));


void CArcher::IdleSound()
{
	EMIT_ICKY_SOUND(CHAN_VOICE, pIdleSounds);
}

void CArcher::AlertSound()
{
	EMIT_ICKY_SOUND(CHAN_VOICE, pAlertSounds);
}

void CArcher::AttackSound()
{
	EMIT_ICKY_SOUND(CHAN_VOICE, pAttackSounds);
}

void CArcher::BiteSound()
{
	EMIT_ICKY_SOUND(CHAN_WEAPON, pBiteSounds);
}

void CArcher::DeathSound()
{
	EMIT_ICKY_SOUND(CHAN_VOICE, pDieSounds);
}

void CArcher::PainSound()
{
	EMIT_ICKY_SOUND(CHAN_VOICE, pPainSounds);
}

//=========================================================
// monster-specific tasks and states
//=========================================================
enum
{
	TASK_ICHTHYOSAUR_CIRCLE_ENEMY = LAST_COMMON_TASK + 1,
	TASK_ICHTHYOSAUR_SWIM,
	TASK_ICHTHYOSAUR_FLOAT,
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

static Task_t tlArchSwimAround[] =
	{
		{TASK_SET_ACTIVITY, (float)ACT_WALK},
		{TASK_ICHTHYOSAUR_SWIM, 0.0},
};

static Schedule_t slArchSwimAround[] =
	{
		{tlArchSwimAround,
			ARRAYSIZE(tlArchSwimAround),
			bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_SEE_ENEMY |
				bits_COND_NEW_ENEMY |
				bits_COND_HEAR_SOUND,
			bits_SOUND_PLAYER |
				bits_SOUND_COMBAT,
			"SwimAround"},
};

static Task_t tlArchSwimAgitated[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_RUN},
		{TASK_WAIT, (float)2.0},
};

static Schedule_t slArchSwimAgitated[] =
	{
		{tlArchSwimAgitated,
			ARRAYSIZE(tlArchSwimAgitated),
			0,
			0,
			"SwimAgitated"},
};


static Task_t tlArchCircleEnemy[] =
	{
		{TASK_SET_ACTIVITY, (float)ACT_WALK},
		{TASK_ICHTHYOSAUR_CIRCLE_ENEMY, 0.0},
};

static Schedule_t slArchCircleEnemy[] =
	{
		{tlArchCircleEnemy,
			ARRAYSIZE(tlArchCircleEnemy),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2,
			0,
			"CircleEnemy"},
};


Task_t tlArchTwitchDie[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SOUND_DIE, (float)0},
		{TASK_DIE, (float)0},
};

Schedule_t slArchTwitchDie[] =
	{
		{tlArchTwitchDie,
			ARRAYSIZE(tlArchTwitchDie),
			0,
			0,
			"Die"},
};

Task_t tlArchFloat[] =
	{
		{TASK_ICHTHYOSAUR_FLOAT, (float)0},
};

Schedule_t slArchFloat[] =
	{
		{tlArchFloat,
			ARRAYSIZE(tlArchFloat),
			0,
			0,
			"Float"},
};

DEFINE_CUSTOM_SCHEDULES(CArcher){
	slArchSwimAround,
	slArchSwimAgitated,
	slArchCircleEnemy,
	slArchTwitchDie,
	slArchFloat,
};
IMPLEMENT_CUSTOM_SCHEDULES(CArcher, CFlyingMonster);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CArcher::Classify()
{
	return CLASS_ALIEN_MONSTER;
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
bool CArcher::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDot >= 0.7 && m_flEnemyTouched > gpGlobals->time - 0.2)
	{
		return true;
	}
	return false;
}

void CArcher::BiteTouch(CBaseEntity* pOther)
{
	// bite if we hit who we want to eat
	if (pOther == m_hEnemy)
	{
		m_flEnemyTouched = gpGlobals->time;
		m_bOnAttack = true;
	}
}

void CArcher::CombatUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_bOnAttack))
		return;

	m_bOnAttack = !m_bOnAttack;
}

//=========================================================
// CheckRangeAttack1  - swim in for a chomp
//
//=========================================================
bool CArcher::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDot > -0.7 && (m_bOnAttack || (flDist <= 192 && m_idealDist <= 192)) && flDist < 192)
	{
		return true;
	}

	return false;
}

//=========================================================
// CheckRangeAttack2  - fire projectile
//
//=========================================================
bool CArcher::CheckRangeAttack2(float flDot, float flDist)
{
	if (flDot > 0.5 && flDist > 64 && flDist <= 1280)
	{
		return true;
	}
	return false;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CArcher::SetYawSpeed()
{
	pev->yaw_speed = 100;
}

//=========================================================
// Killed - overrides CFlyingMonster.
//=========================================================
void CArcher::Killed(entvars_t* pevAttacker, int iGib)
{
	CBaseMonster::Killed(pevAttacker, iGib);
	pev->velocity = Vector(0, 0, 0);
}

void CArcher::BecomeDead()
{
	pev->takedamage = DAMAGE_YES; // don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health.
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.
}

#define ICHTHYOSAUR_AE_SHAKE_RIGHT 1
#define ARCHER_AE_SHOOT 2

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CArcher::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	bool bDidAttack = false;
	switch (pEvent->event)
	{
	case ICHTHYOSAUR_AE_SHAKE_RIGHT:
	{
		if (m_hEnemy != NULL && CBaseEntity::FVisible(m_hEnemy))
		{
			CBaseEntity* pHurt = m_hEnemy;

			if (m_flEnemyTouched < gpGlobals->time - 0.2 && (m_hEnemy->BodyTarget(pev->origin) - pev->origin).Length() > (32 + 16 + 32))
				break;

			Vector vecShootDir = ShootAtEnemy(pev->origin);
			UTIL_MakeAimVectors(pev->angles);

			if (DotProduct(vecShootDir, gpGlobals->v_forward) > 0.707)
			{
				m_bOnAttack = true;
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 300;
				if (pHurt->IsPlayer())
				{
					pHurt->pev->angles.x += RANDOM_FLOAT(-35, 35);
					pHurt->pev->angles.y += RANDOM_FLOAT(-90, 90);
					pHurt->pev->angles.z = 0;
					pHurt->pev->fixangle = 1;
				}
				pHurt->TakeDamage(pev, pev, gSkillData.archerDmgShake, DMG_SLASH);
			}
		}
		BiteSound();

		bDidAttack = true;
	}
	break;
	case ARCHER_AE_SHOOT:
	{
		//char wpnsnd2[256];
		//sprintf(wpnsnd2, "weapons/saw_fire%d.wav", RANDOM_LONG(1, 2));
		//EMIT_SOUND(ENT(pev), CHAN_WEAPON, wpnsnd2, 1, ATTN_GUN);

		Vector vecSpitOffset;
		Vector vecSpitDir;

		UTIL_MakeVectors(pev->angles);

		// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
		// we should be able to read the position of bones at runtime for this info.
		vecSpitOffset = GetGunPosition();
		if (m_hEnemy) // fixes crash
			vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();
		else
			vecSpitDir = gpGlobals->v_forward;

		CArcherSpike::Shoot(pev, vecSpitOffset, vecSpitDir * 900, UTIL_VecToAngles(vecSpitDir));
		m_bOnAttack = true;
	}
	break;
	default:
		CFlyingMonster::HandleAnimEvent(pEvent);
		break;
	}

	if (bDidAttack)
	{
		Vector vecSrc = pev->origin + gpGlobals->v_forward * 32;
		UTIL_Bubbles(vecSrc - Vector(8, 8, 8), vecSrc + Vector(8, 8, 8), 16);
	}
}

//=========================================================
// Spawn
//=========================================================
void CArcher::Spawn()
{
	Precache();
	
	SET_MODEL(ENT(pev), "models/archer.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, -32), Vector(32, 32, 32));

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	if (g_iSkillLevel != SKILL_REALISM)
	{
		pev->health = gSkillData.archerHealth;
	}
	else
	{
		pev->health = 225;
	}
	m_HackedGunPos = Vector(0, 6, 4);
	pev->view_ofs = Vector(0, 0, 16);
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	SetBits(pev->flags, FL_SWIM);
	SetFlyingSpeed(ICHTHYOSAUR_SPEED);
	SetFlyingMomentum(2.5); // Set momentum constant

	m_afCapability = bits_CAP_RANGE_ATTACK1 | bits_CAP_SWIM;

	MonsterInit();

	SetTouch(&CArcher::BiteTouch);
	SetUse(&CArcher::CombatUse);

	m_idealDist = 384;
	m_flMinSpeed = 80;
	m_flMaxSpeed = 300;
	m_flMaxDist = 384;

	Vector Forward;
	UTIL_MakeVectorsPrivate(pev->angles, Forward, 0, 0);
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CArcher::Precache()
{
	UTIL_PrecacheOther("archerspike");

	PRECACHE_MODEL("models/archer.mdl");

	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);
	PRECACHE_SOUND_ARRAY(pDieSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CArcher::GetSchedule()
{
	// ALERT( at_console, "GetSchedule( )\n" );
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
		m_flightSpeed = 80;
		return GetScheduleOfType(SCHED_IDLE_WALK);

	case MONSTERSTATE_ALERT:
		m_flightSpeed = 150;
		return GetScheduleOfType(SCHED_IDLE_WALK);

	case MONSTERSTATE_COMBAT:
		m_flMaxSpeed = 266;
		// eat them
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}
		// fire bolts
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}
		// chase them down and eat them
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && m_hEnemy->pev->waterlevel > 0)
		{
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}
		if (HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			m_bOnAttack = true;
		}
		if (pev->health < pev->max_health - 5)
		{
			m_bOnAttack = true;
		}

		return GetScheduleOfType(SCHED_STANDOFF);
	}

	return CFlyingMonster::GetSchedule();
}


//=========================================================
//=========================================================
Schedule_t* CArcher::GetScheduleOfType(int Type)
{
	// ALERT( at_console, "GetScheduleOfType( %d ) %d\n", Type, m_bOnAttack );
	switch (Type)
	{
	case SCHED_IDLE_WALK:
		return slArchSwimAround;
	case SCHED_STANDOFF:
		return slArchCircleEnemy;
	case SCHED_FAIL:
		return slArchSwimAgitated;
	case SCHED_DIE:
		if (pev->deadflag == DEAD_DEAD)
		{
			//Already dead, immediately switch to float.
			return slArchFloat;
		}

		return slArchTwitchDie;
	case SCHED_CHASE_ENEMY:
		AttackSound();
	}

	return CBaseMonster::GetScheduleOfType(Type);
}



//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CArcher::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_ICHTHYOSAUR_CIRCLE_ENEMY:
		break;
	case TASK_ICHTHYOSAUR_SWIM:
		break;
	case TASK_SMALL_FLINCH:
		if (m_idealDist > 128)
		{
			m_flMaxDist = 512;
			m_idealDist = 512;
		}
		else
		{
			m_bOnAttack = true;
		}
		CFlyingMonster::StartTask(pTask);
		break;

	case TASK_ICHTHYOSAUR_FLOAT:
	{
		const int sequenceIndex = LookupSequence("dead_float");

		//Don't restart the animation if we're restoring.
		if (pev->sequence != sequenceIndex)
		{
			SetSequenceByName("dead_float");
		}
		break;
	}

	default:
		CFlyingMonster::StartTask(pTask);
		break;
	}
}

void CArcher::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_ICHTHYOSAUR_CIRCLE_ENEMY:
		if (m_hEnemy == NULL)
		{
			TaskComplete();
		}
		else if (FVisible(m_hEnemy))
		{
			Vector vecFrom = m_hEnemy->EyePosition();

			Vector vecDelta = (pev->origin - vecFrom).Normalize();
			Vector vecSwim = CrossProduct(vecDelta, Vector(0, 0, 1)).Normalize();

			if (DotProduct(vecSwim, m_SaveVelocity) < 0)
				vecSwim = vecSwim * -1.0;

			Vector vecPos = vecFrom + vecDelta * m_idealDist + vecSwim * 32;

			// ALERT( at_console, "vecPos %.0f %.0f %.0f\n", vecPos.x, vecPos.y, vecPos.z );

			TraceResult tr;

			UTIL_TraceHull(vecFrom, vecPos, ignore_monsters, large_hull, m_hEnemy->edict(), &tr);

			if (tr.flFraction > 0.5)
				vecPos = tr.vecEndPos;

			m_SaveVelocity = m_SaveVelocity * 0.8 + 0.2 * (vecPos - pev->origin).Normalize() * m_flightSpeed;

			// ALERT( at_console, "m_SaveVelocity %.2f %.2f %.2f\n", m_SaveVelocity.x, m_SaveVelocity.y, m_SaveVelocity.z );

			if (HasConditions(bits_COND_ENEMY_FACING_ME) && m_hEnemy->FVisible(this))
			{
				m_flNextAlert -= 0.1;

				if (m_idealDist < m_flMaxDist)
				{
					m_idealDist += 4;
				}
				if (m_flightSpeed > m_flMinSpeed)
				{
					m_flightSpeed -= 2;
				}
				else if (m_flightSpeed < m_flMinSpeed)
				{
					m_flightSpeed += 2;
				}
				if (m_flMinSpeed < m_flMaxSpeed)
				{
					m_flMinSpeed += 0.5;
				}
			}
			else
			{
				m_flNextAlert += 0.1;

				if (m_idealDist > 128)
				{
					m_idealDist -= 4;
				}
				if (m_flightSpeed < m_flMaxSpeed)
				{
					m_flightSpeed += 4;
				}
			}
			// ALERT( at_console, "%.0f\n", m_idealDist );
		}
		else
		{
			m_flNextAlert = gpGlobals->time + 0.2;
		}

		if (m_flNextAlert < gpGlobals->time)
		{
			// ALERT( at_console, "AlertSound()\n");
			AlertSound();
			m_flNextAlert = gpGlobals->time + RANDOM_FLOAT(3, 5);
		}

		break;
	case TASK_ICHTHYOSAUR_SWIM:
		if (m_fSequenceFinished)
		{
			TaskComplete();
		}
		break;
	case TASK_DIE:
		if (m_fSequenceFinished)
		{
			pev->deadflag = DEAD_DEAD;

			TaskComplete();
		}
		break;

	case TASK_ICHTHYOSAUR_FLOAT:
		pev->angles.x = UTIL_ApproachAngle(0, pev->angles.x, 20);
		pev->velocity = pev->velocity * 0.8;
		if (pev->waterlevel > 1 && pev->velocity.z < 64)
		{
			pev->velocity.z += 4;
		}
		else
		{
			pev->velocity.z -= 4;
		}
		// ALERT( at_console, "%f\n", pev->velocity.z );
		break;

	default:
		CFlyingMonster::RunTask(pTask);
		break;
	}
}



float CArcher::VectorToPitch(const Vector& vec)
{
	float pitch;
	if (vec.z == 0 && vec.x == 0)
		pitch = 0;
	else
	{
		pitch = (int)(atan2(vec.z, sqrt(vec.x * vec.x + vec.y * vec.y)) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	return pitch;
}

//=========================================================
void CArcher::Move(float flInterval)
{
	CFlyingMonster::Move(flInterval);
}

float CArcher::FlPitchDiff()
{
	float flPitchDiff;
	float flCurrentPitch;

	flCurrentPitch = UTIL_AngleMod(pev->angles.z);

	if (flCurrentPitch == pev->idealpitch)
	{
		return 0;
	}

	flPitchDiff = pev->idealpitch - flCurrentPitch;

	if (pev->idealpitch > flCurrentPitch)
	{
		if (flPitchDiff >= 180)
			flPitchDiff = flPitchDiff - 360;
	}
	else
	{
		if (flPitchDiff <= -180)
			flPitchDiff = flPitchDiff + 360;
	}
	return flPitchDiff;
}

float CArcher::ChangePitch(int speed)
{
	if (pev->movetype == MOVETYPE_FLY)
	{
		float diff = FlPitchDiff();
		float target = 0;
		if (m_IdealActivity != GetStoppedActivity())
		{
			if (diff < -20)
				target = 45;
			else if (diff > 20)
				target = -45;
		}

		if (m_flLastPitchTime == 0)
		{
			m_flLastPitchTime = gpGlobals->time - gpGlobals->frametime;
		}

		float delta = gpGlobals->time - m_flLastPitchTime;

		m_flLastPitchTime = gpGlobals->time;

		if (delta > 0.25)
		{
			delta = 0.25;
		}

		pev->angles.x = UTIL_Approach(target, pev->angles.x, 220.0 * delta);
	}
	return 0;
}

float CArcher::ChangeYaw(int speed)
{
	if (pev->movetype == MOVETYPE_FLY)
	{
		float diff = FlYawDiff();
		float target = 0;

		if (m_IdealActivity != GetStoppedActivity())
		{
			if (diff < -20)
				target = 20;
			else if (diff > 20)
				target = -20;
		}

		if (m_flLastZYawTime == 0)
		{
			m_flLastZYawTime = gpGlobals->time - gpGlobals->frametime;
		}

		float delta = gpGlobals->time - m_flLastZYawTime;

		m_flLastZYawTime = gpGlobals->time;

		if (delta > 0.25)
		{
			delta = 0.25;
		}

		pev->angles.z = UTIL_Approach(target, pev->angles.z, 220.0 * delta);
	}
	return CFlyingMonster::ChangeYaw(speed);
}


Activity CArcher::GetStoppedActivity()
{
	if (pev->movetype != MOVETYPE_FLY) // UNDONE: Ground idle here, IDLE may be something else
		return ACT_IDLE;
	return ACT_WALK;
}

void CArcher::SetActivity(Activity NewActivity)
{
	const float frame = pev->frame;

	CFlyingMonster::SetActivity(NewActivity);

	//Restore belly up state.
	if (pev->deadflag == DEAD_DEAD)
	{
		SetSequenceByName("dead_float");
		pev->frame = frame;
	}
}

void CArcher::MoveExecute(CBaseEntity* pTargetEnt, const Vector& vecDir, float flInterval)
{
	m_SaveVelocity = vecDir * m_flightSpeed;
}


void CArcher::MonsterThink()
{
	CFlyingMonster::MonsterThink();

	if (pev->deadflag == DEAD_NO)
	{
		if (m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			Swim();
		}
	}
}

void CArcher::Stop()
{
	if (!m_bOnAttack)
		m_flightSpeed = 80.0;
}

void CArcher::Swim()
{
	int retValue = 0;

	Vector start = pev->origin;

	Vector Angles;
	Vector Forward, Right, Up;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		pev->angles.x = 0;
		pev->angles.y += RANDOM_FLOAT(-45, 45);
		ClearBits(pev->flags, FL_ONGROUND);

		Angles = Vector(-pev->angles.x, pev->angles.y, pev->angles.z);
		UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

		pev->velocity = Forward * 200 + Up * 200;

		return;
	}

	if (m_bOnAttack && m_flightSpeed < m_flMaxSpeed)
	{
		m_flightSpeed += 25;
	}
	if (m_flightSpeed < 180)
	{
		if (m_IdealActivity == ACT_RUN)
			SetActivity(ACT_WALK);
		if (m_IdealActivity == ACT_WALK)
			pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "walk %.2f\n", pev->framerate );
	}
	else
	{
		if (m_IdealActivity == ACT_WALK)
			SetActivity(ACT_RUN);
		if (m_IdealActivity == ACT_RUN)
			pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "run  %.2f\n", pev->framerate );
	}

/*
	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 80 );
		m_pBeam->PointEntInit( pev->origin + m_SaveVelocity, entindex( ) );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->SetColor( 255, 180, 96 );
		m_pBeam->SetBrightness( 192 );
	}
*/
#define PROBE_LENGTH 150
	Angles = UTIL_VecToAngles(m_SaveVelocity);
	Angles.x = -Angles.x;
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH * Forward);
	r = DoProbe(start + PROBE_LENGTH / 3 * Forward + Right);
	l = DoProbe(start + PROBE_LENGTH / 3 * Forward - Right);
	u = DoProbe(start + PROBE_LENGTH / 3 * Forward + Up);
	d = DoProbe(start + PROBE_LENGTH / 3 * Forward - Up);

	Vector SteeringVector = f + r + l + u + d;
	m_SaveVelocity = (m_SaveVelocity + SteeringVector / 2).Normalize();

	Angles = Vector(-pev->angles.x, pev->angles.y, pev->angles.z);
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);
	// ALERT( at_console, "%f : %f\n", Angles.x, Forward.z );

	float flDot = DotProduct(Forward, m_SaveVelocity);
	if (flDot > 0.5)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed;
	else if (flDot > 0)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed * (flDot + 0.5);
	else
		pev->velocity = m_SaveVelocity = m_SaveVelocity * 80;

	// ALERT( at_console, "%.0f %.0f\n", m_flightSpeed, pev->velocity.Length() );


	// ALERT( at_console, "Steer %f %f %f\n", SteeringVector.x, SteeringVector.y, SteeringVector.z );

	/*
	m_pBeam->SetStartPos( pev->origin + pev->velocity );
	m_pBeam->RelinkBeam( );
*/

	// ALERT( at_console, "speed %f\n", m_flightSpeed );

	Angles = UTIL_VecToAngles(m_SaveVelocity);

	// Smooth Pitch
	//
	if (Angles.x > 180)
		Angles.x = Angles.x - 360;
	pev->angles.x = UTIL_Approach(Angles.x, pev->angles.x, 50 * 0.1);
	if (pev->angles.x < -80)
		pev->angles.x = -80;
	if (pev->angles.x > 80)
		pev->angles.x = 80;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;
	// ALERT( at_console, "Y %.0f %.0f\n", Angles.y, pev->angles.y );

	if (fabs(Angles.y - pev->angles.y) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y;
	}
	if (fabs(Angles.y - pev->angles.y + 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y + 360;
	}
	if (fabs(Angles.y - pev->angles.y - 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y - 360;
	}

	float speed = m_flightSpeed * 0.1;

	// ALERT( at_console, "speed %.0f %f\n", turn, speed );
	if (fabs(turn) > speed)
	{
		if (turn < 0.0)
		{
			turn = -speed;
		}
		else
		{
			turn = speed;
		}
	}
	pev->angles.y += turn;
	pev->angles.z -= turn;
	pev->angles.y = fmod((pev->angles.y + 360.0), 360.0);

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8 + turn;

	// ALERT( at_console, "yaw %f : %f\n", turn, yaw_adj );

	SetBoneController(0, -yaw_adj / 4.0);

	// Roll Smoothing
	//
	turn = 360;
	if (fabs(Angles.z - pev->angles.z) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z;
	}
	if (fabs(Angles.z - pev->angles.z + 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z + 360;
	}
	if (fabs(Angles.z - pev->angles.z - 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z - 360;
	}
	speed = m_flightSpeed / 2 * 0.1;
	if (fabs(turn) < speed)
	{
		pev->angles.z += turn;
	}
	else
	{
		if (turn < 0.0)
		{
			pev->angles.z -= speed;
		}
		else
		{
			pev->angles.z += speed;
		}
	}
	if (pev->angles.z < -20)
		pev->angles.z = -20;
	if (pev->angles.z > 20)
		pev->angles.z = 20;

	UTIL_MakeVectorsPrivate(Vector(-Angles.x, Angles.y, Angles.z), Forward, Right, Up);

	// UTIL_MoveToOrigin ( ENT(pev), pev->origin + Forward * speed, speed, MOVE_STRAFE );
}


Vector CArcher::DoProbe(const Vector& Probe)
{
	Vector WallNormal = Vector(0, 0, -1); // WATER normal is Straight Down for fish.
	float frac;
	bool bBumpedSomething = ProbeZ(pev->origin, Probe, &frac);

	TraceResult tr;
	TRACE_MONSTER_HULL(edict(), pev->origin, Probe, dont_ignore_monsters, edict(), &tr);
	if (0 != tr.fAllSolid || tr.flFraction < 0.99)
	{
		if (tr.flFraction < 0.0)
			tr.flFraction = 0.0;
		if (tr.flFraction > 1.0)
			tr.flFraction = 1.0;
		if (tr.flFraction < frac)
		{
			frac = tr.flFraction;
			bBumpedSomething = true;
			WallNormal = tr.vecPlaneNormal;
		}
	}

	if (bBumpedSomething && (m_hEnemy == NULL || tr.pHit != m_hEnemy->edict()))
	{
		Vector ProbeDir = Probe - pev->origin;

		Vector NormalToProbeAndWallNormal = CrossProduct(ProbeDir, WallNormal);
		Vector SteeringVector = CrossProduct(NormalToProbeAndWallNormal, ProbeDir);

		float SteeringForce = m_flightSpeed * (1 - frac) * (DotProduct(WallNormal.Normalize(), m_SaveVelocity.Normalize()));
		if (SteeringForce < 0.0)
		{
			SteeringForce = -SteeringForce;
		}
		SteeringVector = SteeringForce * SteeringVector.Normalize();

		return SteeringVector;
	}
	return Vector(0, 0, 0);
}
