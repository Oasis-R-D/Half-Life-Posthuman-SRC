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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "UserMessages.h"

class CHealthKit : public CItem
{
	void Spawn() override;
	void Precache() override;
	bool MyTouch(CBasePlayer* pPlayer) override;

	/*
	int		Save( CSave &save ) override; 
	int		Restore( CRestore &restore ) override;
	
	static	TYPEDESCRIPTION m_SaveData[];
*/
};


LINK_ENTITY_TO_CLASS(item_healthkit, CHealthKit);

/*
TYPEDESCRIPTION	CHealthKit::m_SaveData[] = 
{

};


IMPLEMENT_SAVERESTORE( CHealthKit, CItem);
*/

void CHealthKit::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_medkit.mdl");

	CItem::Spawn();
}

void CHealthKit::Precache()
{
	PRECACHE_MODEL("models/w_medkit.mdl");
	PRECACHE_SOUND("items/smallmedkit1.wav");
}

bool CHealthKit::MyTouch(CBasePlayer* pPlayer)
{
	if (pPlayer->pev->deadflag != DEAD_NO)
	{
		return false;
	}

	if (pPlayer->TakeHealth(gSkillData.healthkitCapacity + (RANDOM_LONG(-3, 10)), DMG_GENERIC))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
		WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();

		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM);
		pPlayer->health_armR += (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
		pPlayer->health_armL += (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
		pPlayer->health_legL -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
		pPlayer->health_legR -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
		pPlayer->health_head -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
		pPlayer->health_chest -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
		pPlayer->health_stomach -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
		if (pPlayer->health_armR > 100)
			pPlayer->health_armR = 100;
		if (pPlayer->health_armL > 100)
			pPlayer->health_armL = 100;
		if (pPlayer->health_legL < 0)
			pPlayer->health_legL = 0;
		if (pPlayer->health_legR < 0)
			pPlayer->health_legR = 0;
		if (pPlayer->health_head < 0)
			pPlayer->health_head = 0;
		if (pPlayer->health_chest < 0)
			pPlayer->health_chest = 0;
		if (pPlayer->health_stomach < 0)
			pPlayer->health_stomach = 0;
		MESSAGE_BEGIN(MSG_ONE, gmsgDamageLIMB, NULL, pPlayer->pev);
		WRITE_BYTE(pPlayer->health_head);
		WRITE_BYTE(pPlayer->health_chest);
		WRITE_BYTE(pPlayer->health_stomach);
		WRITE_BYTE(pPlayer->health_armL);
		WRITE_BYTE(pPlayer->health_armR);
		WRITE_BYTE(pPlayer->health_legL);
		WRITE_BYTE(pPlayer->health_legR);
		MESSAGE_END();
		//TODO: incorrect check here, but won't respawn due to respawn delay being -1 in singleplayer
		if (0 != g_pGameRules->ItemShouldRespawn(this))
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}

		return true;
	}

	return false;
}



//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT Off();
	void EXPORT Recharge();
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return (CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge;
	int m_iReactivate; // DeathMatch Delay until reactvated
	int m_iJuice;
	int m_iOn; // 0 = off, 1 = startup, 2 = going
	float m_flSoundTime;
};

TYPEDESCRIPTION CWallHealth::m_SaveData[] =
	{
		DEFINE_FIELD(CWallHealth, m_flNextCharge, FIELD_TIME),
		DEFINE_FIELD(CWallHealth, m_iReactivate, FIELD_INTEGER),
		DEFINE_FIELD(CWallHealth, m_iJuice, FIELD_INTEGER),
		DEFINE_FIELD(CWallHealth, m_iOn, FIELD_INTEGER),
		DEFINE_FIELD(CWallHealth, m_flSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CWallHealth, CBaseEntity);

LINK_ENTITY_TO_CLASS(func_healthcharger, CWallHealth);


bool CWallHealth::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style") ||
		FStrEq(pkvd->szKeyName, "height") ||
		FStrEq(pkvd->szKeyName, "value1") ||
		FStrEq(pkvd->szKeyName, "value2") ||
		FStrEq(pkvd->szKeyName, "value3"))
	{
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

void CWallHealth::Spawn()
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin); // set size and link into world
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model));
	m_iJuice = (gSkillData.healthchargerCapacity + (RANDOM_LONG(-5, 10)));
	pev->frame = 0;
}

void CWallHealth::Precache()
{
	PRECACHE_SOUND("items/medshot4.wav");
	PRECACHE_SOUND("items/medshotno1.wav");
	PRECACHE_SOUND("items/medcharge4.wav");
}


void CWallHealth::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if (!pActivator)
		return;
	// if it's not a player, ignore
	if (!pActivator->IsPlayer())
		return;

	auto player = static_cast<CBasePlayer*>(pActivator);

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		pev->frame = 1;
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if ((m_iJuice <= 0) || !player->HasSuit())
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM);
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25;
	SetThink(&CWallHealth::Off);

	// Time to recharge yet?

	if (m_flNextCharge >= gpGlobals->time)
		return;

	// Play the on sound or the looping charging sound
	if (0 == m_iOn)
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM);
		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM);
	}


	// charge the player
	if (player->TakeHealth(1, DMG_GENERIC))
	{
		m_iJuice--;
		player->health_armR += 1;
		player->health_legL -= 1;
		player->health_legR -= 1;
		player->health_head -= 1;
		player->health_chest -= 1;
		player->health_stomach -= 1;
		if (player->health_armR > 100)
			player->health_armR = 100;
		if (player->health_armL > 100)
			player->health_armL = 100;
		if (player->health_legL < 0)
			player->health_legL = 0;
		if (player->health_legR < 0)
			player->health_legR = 0;
		if (player->health_head < 0)
			player->health_head = 0;
		if (player->health_chest < 0)
			player->health_chest = 0;
		if (player->health_stomach < 0)
			player->health_stomach = 0;
		MESSAGE_BEGIN(MSG_ONE, gmsgDamageLIMB, NULL, player->pev);
		WRITE_BYTE(player->health_head);
		WRITE_BYTE(player->health_chest);
		WRITE_BYTE(player->health_stomach);
		WRITE_BYTE(player->health_armL);
		WRITE_BYTE(player->health_armR);
		WRITE_BYTE(player->health_legL);
		WRITE_BYTE(player->health_legR);
		MESSAGE_END();
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CWallHealth::Recharge()
{
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM);
	m_iJuice = gSkillData.healthchargerCapacity;
	pev->frame = 0;
	SetThink(&CWallHealth::SUB_DoNothing);
}

void CWallHealth::Off()
{
	// Stop looping sound.
	if (m_iOn > 1)
		STOP_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav");

	m_iOn = 0;

	if ((0 == m_iJuice) && ((m_iReactivate = g_pGameRules->FlHealthChargerRechargeTime()) > 0))
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(&CWallHealth::Recharge);
	}
	else
		SetThink(&CWallHealth::SUB_DoNothing);
}
