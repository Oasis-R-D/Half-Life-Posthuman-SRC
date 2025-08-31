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
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "scripted.h"
#include "weapons.h"
#include "soundent.h"
#include "physical_bullet.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define BARNEY_AE_DRAW (2)
#define BARNEY_AE_SHOOT (3)
#define BARNEY_AE_HOLSTER (4)

#define BARNEY_BODY_GUNHOLSTERED 0
#define BARNEY_BODY_GUNDRAWN 1
#define BARNEY_BODY_GUNGONE 2

#define ARMOR_RANDOM -1
#define ARMOR_VEST 0
#define ARMOR_NONE 1

#define HEADWEAR_RANDOM -1
#define HEADWEAR_HELM 0
#define HEADWEAR_OFF 1

class CBarney : public CSquadMonster
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd);
	void Precache() override;
	void SetYawSpeed() override;
	int ISoundMask() override;
	void BarneyFirePistol();
	void AlertSound() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void StartTask(Task_t* pTask) override;
	int ObjectCaps() override { return CSquadMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	bool CheckRangeAttack1(float flDot, float flDist) override;

	// Override these to set behavior
	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;
	MONSTERSTATE GetIdealState() override;

	void DeathSound() override;
	void PainSound() override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool m_fGunDrawn;
	float m_painTime;
	float m_checkAttackTime;
	bool m_lastAttackCheck;
	int m_iShell;
	int m_iArmor;
	int m_iHelmet;
	int m_helmDUR = 3;
	bool m_bPrehuman;
	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS(monster_barney, CBarney);
LINK_ENTITY_TO_CLASS(monster_barney_adv, CBarney);

TYPEDESCRIPTION CBarney::m_SaveData[] =
	{
		DEFINE_FIELD(CBarney, m_fGunDrawn, FIELD_BOOLEAN),
		DEFINE_FIELD(CBarney, m_painTime, FIELD_TIME),
		DEFINE_FIELD(CBarney, m_checkAttackTime, FIELD_TIME),
		DEFINE_FIELD(CBarney, m_lastAttackCheck, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CBarney, CSquadMonster);

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t tlBaFollow[] =
	{
		{TASK_MOVE_TO_TARGET_RANGE, (float)128}, // Move within 128 of target ent (client)
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE},
};

Schedule_t slBaFollow[] =
	{
		{tlBaFollow,
			ARRAYSIZE(tlBaFollow),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"Follow"},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t tlBarneyEnemyDraw[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, 0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_ARM},
};

Schedule_t slBarneyEnemyDraw[] =
	{
		{tlBarneyEnemyDraw,
			ARRAYSIZE(tlBarneyEnemyDraw),
			0,
			0,
			"Barney Enemy Draw"}};

Task_t tlBaFaceTarget[] =
	{
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_TARGET, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE},
};

Schedule_t slBaFaceTarget[] =
	{
		{tlBaFaceTarget,
			ARRAYSIZE(tlBaFaceTarget),
			//bits_COND_CLIENT_PUSH |
				bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"FaceTarget"},
};


Task_t tlIdleBaStand[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},			// repick IDLESTAND every two seconds.
		//{TASK_TLK_HEADRESET, (float)0}, // reset head position
};

Schedule_t slIdleBaStand[] =
	{
		{tlIdleBaStand,
			ARRAYSIZE(tlIdleBaStand),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_SMELL |
				bits_COND_PROVOKED,

			bits_SOUND_COMBAT | // sound flags - change these, and you'll break the talking code.
				//bits_SOUND_PLAYER		|
				//bits_SOUND_WORLD		|

				bits_SOUND_DANGER |
				bits_SOUND_MEAT | // scents
				bits_SOUND_CARCASS |
				bits_SOUND_GARBAGE,
			"IdleStand"},
};

DEFINE_CUSTOM_SCHEDULES(CBarney){
	slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand,
};


IMPLEMENT_CUSTOM_SCHEDULES(CBarney, CSquadMonster);

void CBarney::StartTask(Task_t* pTask)
{
	CSquadMonster::StartTask(pTask);
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CBarney::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_CARCASS |
		   bits_SOUND_MEAT |
		   bits_SOUND_GARBAGE |
		   bits_SOUND_DANGER |
		   bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CBarney::Classify()
{
	if (m_bPrehuman == 0)
	{
		return CLASS_HUMAN_PASSIVE;
	}
	else
	{
		return CLASS_HUMAN_ALLY;
	}
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney::AlertSound()
{
	if (m_hEnemy != NULL)
	{
		//if (FOkToSpeak())
		{
			PlaySentence("BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney::SetYawSpeed()
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
bool CBarney::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 1024 && flDot >= 0.5)
	{
		if (gpGlobals->time > m_checkAttackTime)
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector(0, 0, 55);
			CBaseEntity* pEnemy = m_hEnemy;
			Vector shootTarget = ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP);
			UTIL_TraceLine(shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr);
			m_checkAttackTime = gpGlobals->time + 1;
			if (tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy))
				m_lastAttackCheck = true;
			else
				m_lastAttackCheck = false;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return false;
}


//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney::BarneyFirePistol()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects |= EF_MUZZLEFLASH;

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;

	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	if (FClassnameIs(pev, "monster_barney_adv"))
	{
		FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 1024, BULLET_PLAYER_BUCKSHOT, 1);
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM, 0, 100 + pitchShift);
		EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 100 + pitchShift);
		pev->framerate = 0.5;
	}
	else
	{
		float cone;
		switch (g_iSkillLevel)
		{
		case SKILL_EASY: cone = CONE_8DEGREES; break;
		case SKILL_MEDIUM: cone = CONE_5DEGREES; break;
		case SKILL_HARD: cone = CONE_2DEGREES; break;
		}
		//FireBullets(1, vecShootOrigin, vecShootDir, cone, 1024, BULLET_MONSTER_9MM, 1);
		#ifndef CLIENT_DLL
		if (g_iSkillLevel != SKILL_HARD)
		{
			CPhysbullet::BulletCreate(1, gSkillData.monDmg9MM, 6000, vecShootOrigin, vecShootDir, cone, cone, 0.66, 9, edict());
		}
		else
		{
			CPhysbullet::BulletCreate(1, 25, 6000, vecShootOrigin, vecShootDir, cone, cone, 1, 9, edict());
		}
		#endif
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "barney/ba_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift);
	}

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--; // take away a bullet!
}
bool CBarney::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "Armor"))
	{
	m_iArmor = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "Helmet"))
	{
	m_iHelmet = atoi(pkvd->szValue);
		return true;
	}
	else
	{
		return CBaseEntity::KeyValue(pkvd);
	}
}
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBarney::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;

	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		SetBodygroup(2, BARNEY_BODY_GUNDRAWN);
		m_fGunDrawn = true;
		break;

	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		SetBodygroup(2, BARNEY_BODY_GUNHOLSTERED);
		m_fGunDrawn = false;
		break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarney::Spawn()
{
	Precache();
	if (FBitSet(pev->spawnflags, SF_PREHUMAN))
	{
		m_bPrehuman = 1;
	}
	else if (FBitSet(pev->spawnflags, SF_NOTINHARD))
	{
		if (g_iSkillLevel == SKILL_HARD)
		{
			SetThink(&CBarney::SUB_Remove);
			pev->nextthink = gpGlobals->time;
			return;
		}
	}
	else if (FBitSet(pev->spawnflags, SF_ONLYINHARD))
	{
		if (g_iSkillLevel != SKILL_HARD)
		{
			SetThink(&CBarney::SUB_Remove);
			pev->nextthink = gpGlobals->time;
			return;
		}
	}
	if (FClassnameIs(pev, "monster_barney_adv"))
	{
		SET_MODEL(ENT(pev), "models/barney_adv.mdl");
		pev->health = 55;
		m_cAmmoLoaded = pev->armortype = 8;
	}
	else
	{
		SET_MODEL(ENT(pev), "models/barney.mdl");
		if (g_iSkillLevel != SKILL_HARD)
		{
			pev->health = gSkillData.barneyHealth;
		}
		else
		{
			pev->health = 100;
		}
		m_cAmmoLoaded = pev->armortype = 17;
	}
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->view_ofs = Vector(0, 0, 50);  // position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	SetBodygroup(2, 0);
	m_fGunDrawn = false;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	//SetUse(&CBarney::FollowerUse);
	if (m_iArmor == ARMOR_RANDOM)
		{
		SetBodygroup(0, RANDOM_LONG(0, 1));
		}
	else if (m_iArmor == ARMOR_VEST)
		{
		SetBodygroup(0, ARMOR_VEST);
		}
	else
		{
		SetBodygroup(0, ARMOR_NONE);
		}

		if (m_iHelmet == HEADWEAR_RANDOM)
		{
		SetBodygroup(3, RANDOM_LONG(0, 1));
		}
	else if (m_iHelmet == HEADWEAR_HELM)
		{
		SetBodygroup(3, HEADWEAR_HELM);
		}
	else
		{
		SetBodygroup(3, HEADWEAR_OFF);
		}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarney::Precache()
{
	if (FClassnameIs(pev, "monster_barney_adv"))
	{
		PRECACHE_MODEL("models/barney_adv.mdl");
		m_iShell = PRECACHE_MODEL("models/shotgunshell.mdl");
	}
	else
	{
		PRECACHE_MODEL("models/barney.mdl");
		m_iShell = PRECACHE_MODEL("models/shell.mdl");
	}

	PRECACHE_SOUND("barney/ba_attack1.wav");
	PRECACHE_SOUND("barney/ba_attack2.wav");

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	CSquadMonster::Precache();
}


//=========================================================
// PainSound
//=========================================================
void CBarney::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CBarney::DeathSound()
{
	if (m_bRailed == false)
	{
		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM);
			break;
		}
	}
}


void CBarney::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST)) != 0)
		{
			if (GetBodygroup(0) != 1)
			{
				if (g_iSkillLevel != SKILL_HARD)
				{
					flDamage = flDamage / 2;
				}
				else
				{
					flDamage = round(flDamage * 0.66);
				}
			}
			else
			{
				flDamage = flDamage;
				break;
			}
		}
		break;
	case 10:
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)) != 0)
		{
			if (GetBodygroup(3) != 1)
			{
				if (flDamage < 45 && m_helmDUR > 0)
				{
					m_helmDUR -= 1;
					if (m_helmDUR == 0)
					{
						SetBodygroup(3, HEADWEAR_OFF); // Make this shoot off a helmet at some point :D
					}
					flDamage = round(flDamage * 0.2);
					UTIL_Sparks(ptr->vecEndPos);
					Vector vecTracerDir = vecDir;

					vecTracerDir = vecTracerDir * -512;
					#ifndef CLIENT_DLL
					CPhysbullet::BulletCreate(1, gSkillData.plrDmgBuckshot, 3500, ptr->vecEndPos, Vector(RANDOM_FLOAT(3.14, -3.14), RANDOM_FLOAT(3.14, -3.14), RANDOM_FLOAT(3.14, -3.14)) , 5.0, 5.0, 0.8, 12, edict());
					#endif
				}
				else if (flDamage > 44 && m_helmDUR > 0)
				{
					m_helmDUR = 0;
					SetBodygroup(3, HEADWEAR_OFF);
				}
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CBarney::Killed(entvars_t* pevAttacker, int iGib)
{
	if (GetBodygroup(2) != BARNEY_BODY_GUNGONE)
	{ // drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup(2, BARNEY_BODY_GUNGONE);
		GetAttachment(0, vecGunPos, vecGunAngles);

		if (FClassnameIs(pev, "monster_barney_adv")) //tf? That still exists?
			CBaseEntity* pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
		else
			CBaseEntity* pGun = DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
	}

	SetUse(NULL);
	CSquadMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CBarney::GetScheduleOfType(int Type)
{
	Schedule_t* psched;

	switch (Type)
	{
	case SCHED_ARM_WEAPON:
		if (m_hEnemy != NULL)
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used'
		psched = CSquadMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget; // override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CSquadMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;
	}

	return CSquadMonster::GetScheduleOfType(Type);
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t* CBarney::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER) != 0)
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}
	if (HasConditions(bits_COND_ENEMY_DEAD))
	{
		PlaySentence("BA_KILL", 4, VOL_NORM, ATTN_NORM);
	}
	if (m_cAmmoLoaded <= 0)
	{
		m_cAmmoLoaded = pev->armortype;
		return GetScheduleOfType(SCHED_RELOAD);
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		// always act surprized with a new enemy
		if (HasConditions(bits_COND_NEW_ENEMY) && HasConditions(bits_COND_LIGHT_DAMAGE))
			return GetScheduleOfType(SCHED_SMALL_FLINCH);

		// wait for one schedule to draw gun
		if (!m_fGunDrawn)
			return GetScheduleOfType(SCHED_ARM_WEAPON);

		if (HasConditions(bits_COND_HEAVY_DAMAGE))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}
		break;
	}

	return CSquadMonster::GetSchedule();
}

MONSTERSTATE CBarney::GetIdealState()
{
	return CSquadMonster::GetIdealState();
}

//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadBarney : public CBaseMonster
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_ALIEN_MONSTER; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static const char* m_szPoses[3];
};

const char* CDeadBarney::m_szPoses[] = {"lying_on_back", "lying_on_side", "lying_on_stomach"};

bool CDeadBarney::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_barney_dead, CDeadBarney);

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadBarney::Spawn()
{
	PRECACHE_MODEL("models/barney.mdl");
	SET_MODEL(ENT(pev), "models/barney.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead barney with bad pose\n");
	}
	// Corpses have less health
	pev->health = 8; //gSkillData.barneyHealth;

	MonsterInitDead();
}
