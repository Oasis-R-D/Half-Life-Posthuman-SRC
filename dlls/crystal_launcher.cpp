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
#include "physical_bullet.h"


LINK_ENTITY_TO_CLASS(weapon_crystallauncher, CCrystal_launcher);

void CCrystal_launcher::Spawn()
{
	Precache();
	m_iId = WEAPON_CRYST;
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");

	m_iDefaultAmmo = CRYST_DEFAULT_GIVE;

	FallInit(); // get ready to fall
}


void CCrystal_launcher::Precache()
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/dbarrel1.wav"); //shotgun
	PRECACHE_SOUND("weapons/sbarrel1.wav"); //shotgun

	PRECACHE_SOUND("weapons/reload1.wav"); // shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav"); // shotgun reload

	PRECACHE_SOUND("weapons/357_cock1.wav"); // gun empty sound
}

bool CCrystal_launcher::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CRYST_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CRYST;
	p->iWeight = CRYST_WEIGHT;

	return true;
}



bool CCrystal_launcher::Deploy()
{
	return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW_SEMI, "shotgun");
}

void CCrystal_launcher::PrimaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0)
		return;

	if (m_pPlayer->pev->waterlevel == 3) // don't fire underwater
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		if (m_iClip == 0)
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 20 + gpGlobals->v_right * 4 + gpGlobals->v_up * -8;
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	//Vector spread = pev->armorvalue == 0 ? VECTOR_CONE_5DEGREES : VECTOR_CONE_10DEGREES;
	float spread = pev->armorvalue == 0 ? CONE_5DEGREES : 0.17432;
	float spreadvert = pev->armorvalue == 0 ? CONE_5DEGREES : 0.01746;
	//m_pPlayer->FireBullets(9, vecSrc, vecAiming, spread, 2048, BULLET_PLAYER_BUCKSHOT, 1);
	#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_HARD)
	{
		CPhysbullet::BulletCreate(9, gSkillData.plrDmgBuckshot, 5750, vecSrc, vecAiming, spread, spreadvert, 0.75, 12, m_pPlayer->edict());
	}
	else
	{
		CPhysbullet::BulletCreate(9, 11, 5750, vecSrc, vecAiming, CONE_2DEGREES, CONE_2DEGREES, 1, 12, m_pPlayer->edict());
	}
	#endif
	SendWeaponAnim(SHOTGUN_SHOOT1_SEMI);
	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;

	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM);
	
	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0) // HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = 0.2;
	m_flTimeWeaponIdle = 1;
	m_fInSpecialReload = 0;
}


void CCrystal_launcher::SecondaryAttack()
{
	m_crystaltype = m_crystaltype + 1;
	if (m_crystaltype >= 3)
	{
		m_crystaltype = 0;
	}
	if (m_crystaltype == 0)
	{
	ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "Crystal type is bounce"); // purple
	}
	else if (m_crystaltype == 1)
	{
		ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "Crystal type is explode"); // green
	}
	else if (m_crystaltype == 2)
	{
		ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "Crystal type is pebis"); // orange
	}
}

void CCrystal_launcher::Reload()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 3)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		if (m_iClip == 0)
		{
			SendWeaponAnim(SHOTGUN_RELOAD_START_SEMI);
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.37;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.37;
			m_flNextPrimaryAttack = GetNextAttackDelay(0.37);
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.37;
			m_fInSpecialReload = 1;
		}
		else
		{
			SendWeaponAnim(SHOTGUN_RELOAD_START_SEMI);
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.37;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.37;
			m_flNextPrimaryAttack = GetNextAttackDelay(0.37);
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.37;
			m_fInSpecialReload = 1;
		}
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if (RANDOM_LONG(0, 1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

		SendWeaponAnim(SHOTGUN_RELOAD_INSERT_SEMI);

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 3;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}


void CCrystal_launcher::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && 0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != 3 && 0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload();
			}
			else
			{
				SendWeaponAnim(SHOTGUN_RELOAD_END_SEMI);
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			switch (RANDOM_LONG(1, 3))
			{
			case 1: SendWeaponAnim(SHOTGUN_IDLE1_SEMI), m_flTimeWeaponIdle = 1.93; break;
			case 2: SendWeaponAnim(SHOTGUN_IDLE2_SEMI), m_flTimeWeaponIdle = 3.43; break;
			case 3: SendWeaponAnim(SHOTGUN_IDLE3_SEMI), m_flTimeWeaponIdle = 3.23; break;
			}
		}
	}
}

