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
#include "gamerules.h"
#include "UserMessages.h"
#include "railcannon_bolt.h"

#define BOLT_AIR_VELOCITY 10000
#define BOLT_WATER_VELOCITY 5000

LINK_ENTITY_TO_CLASS(weapon_crossbow, CCrossbow);
LINK_ENTITY_TO_CLASS(weapon_railcannon, CCrossbow);

void CCrossbow::Spawn()
{
	Precache();
	m_iId = WEAPON_CROSSBOW;
	SET_MODEL(ENT(pev), "models/w_crossbow.mdl");

	m_iDefaultAmmo = 1;

	FallInit(); // get ready to fall down.
}

void CCrossbow::Precache()
{
	PRECACHE_MODEL("models/w_crossbow.mdl");
	PRECACHE_MODEL("models/v_crossbow.mdl");
	PRECACHE_MODEL("models/p_crossbow.mdl");

	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/xbow_reload1.wav");

	UTIL_PrecacheOther("crossbow_bolt");

	m_usCrossbow = PRECACHE_EVENT(1, "events/crossbow1.sc");
	m_usCrossbow2 = PRECACHE_EVENT(1, "events/crossbow2.sc");
}


bool CCrossbow::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "bolts";
	p->iMaxAmmo1 = 10;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = WEAPON_CROSSBOW;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return true;
}


bool CCrossbow::Deploy()
{
	if (g_pGameRules->IsMultiplayer())
		NotFirstDraw = true;

	return DefaultDeploy("models/v_crossbow.mdl", "models/p_crossbow.mdl", CROSSBOW_DRAW1, "bow");
}

void CCrossbow::Holster()
{
	m_fInReload = false; // cancel any reload in progress.

	if (m_pPlayer->m_iFOV != 0)
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(CROSSBOW_HOLSTER1);
}

void CCrossbow::PrimaryAttack()
{
	FireBolt();
}

void CCrossbow::FireBolt()
{
	TraceResult tr;

	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	
	m_flTimeSincePrimary = gpGlobals->time;

	m_iClip--;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usCrossbow, 0.0, g_vecZero, g_vecZero, 0, 0, m_iClip, 0, 0, 0);

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_LOUD, 0.0, PE_MUZZLESMK, 2, 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 20 + gpGlobals->v_right * 6 + gpGlobals->v_up * -5;
	Vector vecDir = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

#ifndef CLIENT_DLL
	CCrossbowBolt* pBolt = CCrossbowBolt::BoltCreate(vecSrc, gpGlobals->v_forward, BOLT_AIR_VELOCITY, m_pPlayer); 
	// Fixed the underwater firing because for some reason fixing some obscure bug in the base codebase messes with the gameplay too much smh

	CBasePlayerWeapon::Recoil(8, 2);
#endif

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.75);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
}


void CCrossbow::SecondaryAttack()
{
	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	}
	else if (m_pPlayer->m_iFOV != 20)
	{
		m_pPlayer->m_iFOV = 20;
	}

	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
}


void CCrossbow::Reload()
{
	if (DefaultReload(1, CROSSBOW_RELOAD, 2))
	{
		if (m_pPlayer->m_iFOV != 0)
		{
			SecondaryAttack();
		}
		
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/xbow_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));
	}
}


void CCrossbow::WeaponIdle()
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES); // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound();

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.75)
		{
			if (0 != m_iClip)
			{
				SendWeaponAnim(CROSSBOW_IDLE1);
			}
			else
			{
				SendWeaponAnim(CROSSBOW_IDLE2);
			}
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		}
		else
		{
			if (0 != m_iClip)
			{
				SendWeaponAnim(CROSSBOW_FIDGET1);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
			}
			else
			{
				SendWeaponAnim(CROSSBOW_FIDGET2);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 30.0;
			}
		}
	}
}

class CCrossbowAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_crossbow_clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_crossbow_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(2, "bolts", 10) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_crossbow, CCrossbowAmmo);
LINK_ENTITY_TO_CLASS(ammo_railcannon, CCrossbowAmmo);