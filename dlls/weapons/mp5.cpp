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
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "physical_bullet.h"
LINK_ENTITY_TO_CLASS(weapon_mp5, CMP5);
LINK_ENTITY_TO_CLASS(weapon_9mmAR, CMP5);

//=========================================================
//						   MP5EOD
//=========================================================
#define	MP5_ACCURACY_SHOT_PENALTY_TIME		0.125f	// Applied amount of time each shot adds to the time we must recover from
#define	MP5_ACCURACY_MAXIMUM_PENALTY_TIME	1.25f	// Maximum penalty to deal out

void CMP5::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_MAX_CLIP;

	FallInit(); // get ready to fall down.
}

void CMP5::Precache()
{
	PRECACHE_MODEL("models/v_9mmAR.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");
	PRECACHE_MODEL("models/grenade.mdl"); // grenade
	PRECACHE_MODEL("models/w_9mmARclip.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");
	PRECACHE_SOUND("weapons/hks1.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks2.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks3.wav"); // H to the K
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	m_usMP5 = PRECACHE_EVENT(1, "scripts/events/mp5.sc");
	m_usMP52 = PRECACHE_EVENT(1, "scripts/events/mp52.sc");
}

bool CMP5::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = MP5_WEIGHT;

	return true;
}

bool CMP5::Deploy()
{
	if (g_pGameRules->IsMultiplayer())
		NotFirstDraw = true;

	m_iCrossHairType = CROSSHAIR_DEFAULT;
	m_flAccuracyPenalty = MP5_ACCURACY_MAXIMUM_PENALTY_TIME;

	pev->armorvalue = 0;
	if (pev->armortype == 0)
		pev->armortype = 1;

	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(pev->armortype);
	MESSAGE_END();

	if (NotFirstDraw)
	{
		if (pev->weapons == 0)
			return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DRAW, "mp5");
		return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DRAW_M203, "mp5");
	}
	return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DRAW_FIRST, "mp5");
}

void CMP5::Holster()
{
	m_flAccuracyPenalty = 0.0;
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(0);
	MESSAGE_END();
}

void CMP5::ItemPreFrame()
{
	// Check our penalty time decay
	if ( ( m_flTimeSincePrimary + m_flNextPrimaryAttack < gpGlobals->time ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, MP5_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
	//ALERT(at_console, "m_flAccuracyPenalty: %f \n", m_flAccuracyPenalty);
}

const Vector& CMP5::GetBulletSpread()
{		
	static Vector cone;

	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, MP5_ACCURACY_MAXIMUM_PENALTY_TIME, 0.0f, 1.0f ); 

	// We lerp from very accurate to inaccurate over time
	VectorLerp( VECTOR_CONE_2DEGREES, VECTOR_CONE_5DEGREES, ramp, cone );

	if ((m_pPlayer->m_afButtonLast & IN_RUN) != 0 && m_pPlayer->pev->velocity.Length() > 100)
		cone = cone + VECTOR_CONE_2DEGREES;

	return cone;
}

void CMP5::PrimaryAttack()
{
	if (pev->weapons == 1)
	{
		// don't fire underwater
		if (m_pPlayer->pev->waterlevel == 3 || m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
			return;
		}

		m_flTimeSincePrimary = gpGlobals->time;

		PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_NONE, 0.0, PE_MUZZLESMK, PE_MS_SHOTGUN, 0, 0);
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
		m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;

		m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

		// we don't add in player velocity anymore.
		CGrenade::ShootContact(m_pPlayer->pev,
			m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
			gpGlobals->v_forward * 2000);

		int flags;
	#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
	#else
		flags = 0;
	#endif

		PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usMP52);

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.57;
		m_flNextTertiaryAttack = gpGlobals->time + 2.3;

		if (0 == m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]) // HEV suit - indicate out of ammo condition
		{
			m_pPlayer->SetSuitUpdate("!HEV_GOUT", false, 0);
			pev->frags = 2;
		}
		else
			pev->frags = 1;

		return;
	}
	
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0 && pev->armortype == 2 && pev->armorvalue == 0)
		return;

	if (m_pPlayer->pev->waterlevel == 3 || m_iClip <= 0) // don't fire underwater
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_NORM, 0.0, PE_MUZZLESMK, PE_MS_NORMAL, 0, 0);
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1); // player "shoot" animation

	Vector vecSrc = m_pPlayer->GetGunPosition(); // + gpGlobals->v_forward * 20 + gpGlobals->v_right * 5.3 + gpGlobals->v_up * -9.25;
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	Vector spread = GetBulletSpread();

	m_flTimeSincePrimary = gpGlobals->time;
	m_flAccuracyPenalty += MP5_ACCURACY_SHOT_PENALTY_TIME;

	#ifndef CLIENT_DLL
	if (m_pPlayer->m_iWeaponStatus == 0 || m_pPlayer->m_iWeaponStatus == 2)
	{
		if (g_iSkillLevel != SKILL_REALISM)
		{
			CPhysbullet::BulletCreate(1, gSkillData.plrDmgMP5, 6000, vecSrc, vecAiming, spread.x, spread.y, 0.66f, 9, m_pPlayer->edict());
		}
		else
		{
			CPhysbullet::BulletCreate(1, 25, 6000, vecSrc, vecAiming, spread.x, spread.y, 1, 9, m_pPlayer->edict());
		}
	}
	else
	{
		CPhysbullet::BulletCreate(1, g_iSkillLevel == SKILL_REALISM ? 10 : 3, 4000, vecSrc, vecAiming, spread.x, spread.y, 1, 69, m_pPlayer->edict());
	}

	CBasePlayerWeapon::Recoil((m_iClip % 2 == 0 || RANDOM_LONG(0,1) == 0) ? 0.8 : -0.9, 1);
	#endif

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usMP5, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	// Check if on burst fire
	if (pev->armortype == 2)
	{
		if (pev->armorvalue < 2)
		{
			pev->armorvalue++;

			m_flNextPrimaryAttack = GetNextAttackDelay(0.075);
		}
		else // Burst ended
		{
			pev->armorvalue = 0;
			m_flNextPrimaryAttack = GetNextAttackDelay(0.25);
		}
	}
	else
	{
		m_flNextPrimaryAttack = GetNextAttackDelay(0.075);
	}

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.075;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CMP5::SecondaryAttack()
{
	if (pev->weapons == 0)
	{
		if (0 == m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
			m_pPlayer->SetSuitUpdate("!HEV_GOUT", false, 0);
		else
		{
			pev->weapons = 1;
			SendWeaponAnim(MP5_IDLE_TO_M203);
			MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
			WRITE_SHORT(0);
			MESSAGE_END();
		}
	}
	else
	{
		pev->weapons = 0;
		SendWeaponAnim(MP5_M203_TO_IDLE);
		MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
		WRITE_SHORT(pev->armortype);
		MESSAGE_END();
	}
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = 0.37;
}

void CMP5::TertiaryAttack()
{
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.125;
	if ((m_pPlayer->m_afButtonLast & IN_ALT1) != 0)
		return;

	if (pev->armortype == 1)
	{
		pev->armortype = 2;
	}
	else if (pev->armortype == 2)
	{
		pev->armortype = 1;
	}
	EMIT_SOUND(edict(), CHAN_ITEM, "items/9mmclip2.wav", 1, ATTN_NORM);

	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(pev->armortype);
	MESSAGE_END();
}

void CMP5::Reload()
{
	if (pev->weapons == 1)
		return;

	DefaultReload(45, m_iClip == 0 ? MP5_RELOAD_EMPTY : MP5_RELOAD_TACTICAL, m_iClip == 0 ? 2.5f : 1.5f);
	pev->armorvalue = 0;
}

extern bool CanAttack(float attack_time, float curtime, bool isPredicted);

void CMP5::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (g_iSkillLevel != SKILL_REALISM)
	{
		if (pev->armortype == 2 && CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()) && pev->armorvalue <= 2 && pev->armorvalue > 0)
		{
			if ((m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && 0 == m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]))
			{
				m_fFireOnEmpty = true;
			}

			m_pPlayer->TabulateAmmo();
			PrimaryAttack();
		}
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (pev->weapons == 0)
	{
		if (RANDOM_LONG(0, 1))
			SendWeaponAnim(MP5_IDLE1), m_flTimeWeaponIdle = 5.37;
		else
			SendWeaponAnim(MP5_IDLE2), m_flTimeWeaponIdle = 4.5;
	}
	else
	{
		if (pev->frags == 2)
		{
			if (0 == m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
				SecondaryAttack();
			else
				pev->frags = 1;
		}
		else if (pev->frags == 0)
			SendWeaponAnim(MP5_IDLE_M203), m_flTimeWeaponIdle = 1.63;
		else
		{
			SendWeaponAnim(MP5_RELOAD_M203), m_flTimeWeaponIdle = 1.73;
			pev->frags = 0;
		}
	}
}

class CMP5AmmoClip : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmARclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(MP5_MAX_CLIP, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5clip, CMP5AmmoClip);
LINK_ENTITY_TO_CLASS(ammo_9mmAR, CMP5AmmoClip);



class CMP5Chainammo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_chainammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_CHAINBOX_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_9mmbox, CMP5Chainammo);


class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_ARgrenade.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_M203BOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5grenades, CMP5AmmoGrenade);
LINK_ENTITY_TO_CLASS(ammo_ARgrenades, CMP5AmmoGrenade);

