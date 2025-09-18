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
// Entity that NPCs really like firing or throwing themselves at
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "weapons.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CTarget : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override;
    bool KeyValue(KeyValueData* pkvd);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int m_hitamnt;
	int m_maxhitamnt;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void StartMonster() override; // makes it not scream about the enemy being in a wall
	// No range attacks
	bool CheckRangeAttack1(float flDot, float flDist) override { return false; }
	bool CheckRangeAttack2(float flDot, float flDist) override { return false; }
    virtual bool CheckMeleeAttack1(float flDot, float flDist) override { return false; };
	virtual bool CheckMeleeAttack2(float flDot, float flDist) override { return false; };
    void MonsterThink() override { return; };

	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
};

LINK_ENTITY_TO_CLASS(info_monstarget, CTarget);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CTarget::Classify()
{
	return CLASS_OHTHEMISERY; // Everyone's nemesis
}

bool CTarget::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (m_maxhitamnt != -1)
	{
  		m_hitamnt += 1;
   		if (m_maxhitamnt <= m_hitamnt)
        	CTarget::Killed(pevAttacker, 0);
	}
    pev->health = 9999;
	return NULL;
}

void CTarget::Killed(entvars_t* pevAttacker, int iGib)
{
	CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
	if (pOwner)
	{
		pOwner->DeathNotice(pev);
	}
	UTIL_Remove(this);
}
bool CTarget::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "Max_hits"))
	{
		m_maxhitamnt = atoi(pkvd->szValue);
		return true;
	}
	else
	{
		return CBaseEntity::KeyValue(pkvd);
	}
}

void CTarget::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CTarget::Killed(pev, 0); // removes if triggered, useful for making it have a time limit
}
//=========================================================
// Spawn
//=========================================================
void CTarget::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/target.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
    pev->yaw_speed = 0;
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_NONE;
	m_bloodColor = DONT_BLEED;
	pev->health = 9999;
	m_flFieldOfView = 0;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTarget::Precache()
{
	PRECACHE_MODEL("models/target.mdl");

}
void CTarget::StartMonster()
{
	if (!FStringNull(pev->target)) // this monster has a target
	{
		// Find the monster's initial target entity, stash it
		m_pGoalEnt = CBaseEntity::Instance(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target)));

		if (!m_pGoalEnt)
		{
			ALERT(at_error, "ReadyMonster()--%s couldn't find target %s", STRING(pev->classname), STRING(pev->target));
		}
		else
		{
			// Monster will start turning towards his destination

			// set the monster up to walk a path corner path.
			// !!!BUGBUG - this is a minor bit of a hack.
			// JAYJAY
			m_movementGoal = MOVEGOAL_PATHCORNER;

			if (pev->movetype == MOVETYPE_FLY)
				m_movementActivity = ACT_FLY;
			else
				m_movementActivity = ACT_WALK;

			SetState(MONSTERSTATE_IDLE);
			ChangeSchedule(GetScheduleOfType(SCHED_IDLE_WALK));
		}
	}

	//SetState ( m_IdealMonsterState );
	//SetActivity ( m_IdealActivity );

	// Delay drop to floor to make sure each door in the level has had its chance to spawn
	// Spread think times so that they don't all happen at the same time (Carmack)
	SetThink(&CBaseMonster::CallMonsterThink);
	pev->nextthink += RANDOM_FLOAT(0.1, 0.4); // spread think times.

	if (!FStringNull(pev->targetname)) // wait until triggered
	{
		SetState(MONSTERSTATE_IDLE);
		// UNDONE: Some scripted sequence monsters don't have an idle?
		SetActivity(ACT_IDLE);
		ChangeSchedule(GetScheduleOfType(SCHED_WAIT_TRIGGER));
	}
}
