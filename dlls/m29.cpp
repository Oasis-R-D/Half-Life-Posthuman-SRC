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

#define	M29_ACCURACY_SHOT_PENALTY_TIME		0.25f	// Applied amount of time each shot adds to the time we must recover from
#define	M29_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out
#define firerate 0.25

extern bool CanAttack(float attack_time, float curtime, bool isPredicted);

LINK_ENTITY_TO_CLASS(weapon_m29, CM29);

bool CM29::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 12;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_M29;
	p->iWeight = PYTHON_WEIGHT;

	return true;
}

void CM29::Spawn()
{
	pev->classname = MAKE_STRING("weapon_m29"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_M29;
	SET_MODEL(ENT(pev), "models/w_357.mdl");

	m_iDefaultAmmo = 12;

	FallInit(); // get ready to fall down.
}


void CM29::Precache()
{
	PRECACHE_MODEL("models/v_m29R.mdl");
	PRECACHE_MODEL("models/v_m29L.mdl");
	PRECACHE_MODEL("models/w_357.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/357_reload1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("weapons/m29_fire1.wav");
	PRECACHE_SOUND("weapons/m29_fire2.wav");

	m_usFireM29 = PRECACHE_EVENT(1, "events/m29.sc");
}
void CM29::CalculateAmmo()
{
	int m_iclip1;
	int mod = m_iClip % 2;
	if (mod != 0)
	{
		m_iclip1 = m_iClip + 1;
		m_iCylR_ammo = m_iCylL_ammo = (m_iclip1 / 2);
		m_iCylL_ammo -= 1;
	}
	else
	{
		m_iCylR_ammo = m_iCylL_ammo = (m_iClip / 2);
	}
}
bool CM29::Deploy()
{
	if (g_pGameRules->IsMultiplayer())
		NotFirstDraw = true;

	m_iCrossHairType = CROSSHAIR_NOCENTER;
	m_flAccuracyPenalty = M29_ACCURACY_MAXIMUM_PENALTY_TIME;

	CalculateAmmo();

	return DefaultDeploy("models/v_m29R.mdl", "models/p_357.mdl", PYTHON_DRAW, "python", pev->body, "models/v_m29L.mdl", PYTHON_DRAW);
}


void CM29::Holster()
{
	m_flAccuracyPenalty = 0.0;
	m_pPlayer->altviewmodel = 0; // ALTVM CODE

	m_fInReload = false; // cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	m_flTimeAltWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(PYTHON_HOLSTER);
	SendWeaponAnim(PYTHON_HOLSTER, pev->body, true);
}

void CM29::SecondaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK2) != 0)
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.15;
		return;
	}
	if (m_iClip <= 0 || m_iCylR_ammo <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextSecondaryAttack = 0.15;
		}
		return;
	}

	m_iCylR_ammo--;

	m_flNextSecondaryAttack = firerate;

	Shoot(0);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CM29::PrimaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0)
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0 || m_iCylL_ammo <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
		}
		return;
	}

	m_iCylL_ammo--;

	m_flNextPrimaryAttack = firerate;

	Shoot(1);

	m_flTimeAltWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CM29::ItemPreFrame()
{
	// Check our penalty time decay
	if ( ( m_flTimeSincePrimary + m_flNextPrimaryAttack < gpGlobals->time ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, M29_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
	//ALERT(at_console, "m_flAccuracyPenalty: %f \n", m_flAccuracyPenalty);
}

const Vector& CM29::GetBulletSpread()
{		
	static Vector cone;

	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, M29_ACCURACY_MAXIMUM_PENALTY_TIME, 0.0f, 1.0f ); 

	// We lerp from very accurate to inaccurate over time
	VectorLerp( g_vecZero, VECTOR_CONE_5DEGREES, ramp, cone );

	return cone;
}

void CM29::Shoot(int gunnumb)
{
	float spread = GetBulletSpread().x;

	m_flTimeSincePrimary = gpGlobals->time;
	m_flAccuracyPenalty += M29_ACCURACY_SHOT_PENALTY_TIME;

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_iClip--;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	Vector vecDir;

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireM29, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, gunnumb, 0, 0, 0);

	Vector vecSrc;
	if (gunnumb == 0)
	{
		vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_right * 7.5;
	}
	else
	{
		vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_right * -7.5;
	}
	
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_HARD)
	{
		CPhysbullet::BulletCreate(1, round(gSkillData.plrDmg357*1.25), 6000, vecSrc, vecAiming, spread, spread, 1, 44, m_pPlayer->edict());
	}
	else
	{
		CPhysbullet::BulletCreate(1, 50, 6000, vecSrc, vecAiming, spread, spread, 1, 44, m_pPlayer->edict());
	}
	CBasePlayerWeapon::Recoil(2, 1);
#endif

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0); // no ammo
}

void CM29::Reload()
{
	if (m_pPlayer->ammo_357 <= 0)
		return;
	DefaultReload(12, PYTHON_RELOAD, 2.0, 0, true, PYTHON_RELOAD);
}


void CM29::WeaponIdle()
{
	ResetEmptySound();
	if (m_iClip > 0 && m_iCylL_ammo == 0 && m_iCylR_ammo == 0)
		CalculateAmmo();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0.0, 1.0);
		if (flRand <= 0.5)
		{
			iAnim = PYTHON_IDLE1;
			m_flTimeWeaponIdle = (70.0 / 30.0);
		}
		else if (flRand <= 0.7)
		{
			iAnim = PYTHON_IDLE2;
			m_flTimeWeaponIdle = (60.0 / 30.0);
		}
		else if (flRand <= 0.9)
		{
			iAnim = PYTHON_IDLE3;
			m_flTimeWeaponIdle = (88.0 / 30.0);
		}
		else
		{
			iAnim = PYTHON_FIDGET;
			m_flTimeWeaponIdle = (170.0 / 30.0);
		}

		SendWeaponAnim(iAnim, pev->body);
	}

	if (m_flTimeAltWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAltAnim;
	float flRand = RANDOM_FLOAT(0.0, 1.0);
	if (flRand <= 0.5)
	{
		iAltAnim = PYTHON_IDLE1;
		m_flTimeAltWeaponIdle = (70.0 / 30.0);
	}
	else if (flRand <= 0.7)
	{
		iAltAnim = PYTHON_IDLE2;
		m_flTimeAltWeaponIdle = (60.0 / 30.0);
	}
	else if (flRand <= 0.9)
	{
		iAltAnim = PYTHON_IDLE3;
		m_flTimeAltWeaponIdle = (88.0 / 30.0);
	}
	else
	{
		iAltAnim = PYTHON_FIDGET;
		m_flTimeAltWeaponIdle = (170.0 / 30.0);
	}

	SendWeaponAnim(iAltAnim, pev->body, true);
}

void CM29::ItemPostFrame() // completely overriden to make multiple changes
{
	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase()))
	{
		// complete the reload.
		int j = V_min(iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);
		// Add them to the clip
		m_iClip += j;
		CalculateAmmo();

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

		m_pPlayer->TabulateAmmo();

		m_fInReload = false;
	}
	if ((m_pPlayer->pev->button & IN_ATTACK) == 0)
	{
		m_flLastFireTime = 0.0f;
	}
	if ((m_pPlayer->pev->button & IN_ATTACK2) != 0 && CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
	{
		if (m_iCylR_ammo == 0)
		{
			m_fFireOnEmpty = true;
		}

		m_pPlayer->TabulateAmmo();
		SecondaryAttack();

		// code duplicated so you can fire them at the same time
		if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
		{
			if (m_iCylL_ammo == 0)
			{
				m_fFireOnEmpty = true;
			}

			m_pPlayer->TabulateAmmo();
			PrimaryAttack();
		}

	}
	/*else if ((m_pPlayer->pev->button & IN_ALT1) != 0 && m_flNextTertiaryAttack < gpGlobals->time)
	{
		TertiaryAttack();
	}*/ // no tertiary
	else if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
	{
		if (m_iCylL_ammo == 0)
		{
			m_fFireOnEmpty = true;
		}

		m_pPlayer->TabulateAmmo();
		PrimaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_RELOAD) != 0 && iMaxClip() != WEAPON_NOCLIP && !m_fInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if ((m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)) == 0)
	{
		// no fire buttons down

		m_fFireOnEmpty = false;

		if (!IsUseable() && m_flNextPrimaryAttack < (UseDecrement() ? 0.0 : gpGlobals->time))
		{
#ifndef CLIENT_DLL
			// weapon isn't useable, switch.
			if ((iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == 0 && g_pGameRules->GetNextBestWeapon(m_pPlayer, this))
			{
				m_flNextPrimaryAttack = (UseDecrement() ? 0.0 : gpGlobals->time) + 0.3;
				return;
			}
#endif
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if (m_iClip == 0 && (iFlags() & ITEM_FLAG_NOAUTORELOAD) == 0 && m_flNextPrimaryAttack < (UseDecrement() ? 0.0 : gpGlobals->time))
			{
				Reload();

				return;
			}
		}
		if (m_iClip <= round(0.2 * iMaxClip()) && m_hasbeeped == false && m_iClip != -1)
		{
			switch(RANDOM_LONG(1, 3))
			{
				case 1:
					EMIT_SOUND(m_pPlayer->edict(), CHAN_AUTO, "fvox/Lowammo1.wav", 1, ATTN_NORM);
				break;
				case 2:
					EMIT_SOUND(m_pPlayer->edict(), CHAN_AUTO, "fvox/Lowammo2.wav", 1, ATTN_NORM);				
				break;
				case 3:
					EMIT_SOUND(m_pPlayer->edict(), CHAN_AUTO, "fvox/Lowammo3.wav", 1, ATTN_NORM);	
				break;
			}
			
			m_hasbeeped = true;
		}
		
		WeaponIdle();
		return;
	}

	// catch all
	if (ShouldWeaponIdle())
	{
		WeaponIdle();
	}
}