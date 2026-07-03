#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "game.h"
#include "player.h"
#include "UserMessages.h"
#include "decals.h"

class CHeadCrab : public CBaseMonster // TO-DO: probably needs to be talk monster to add the "don't shoot me or me get mad" code
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
	void Touch(CBaseEntity* pOther)
	{
		if (m_bPrehuman == false)
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