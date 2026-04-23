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

#define	SG_ACCURACY_SHOT_PENALTY_TIME		1	// Applied amount of time each shot adds to the time we must recover from
#define	SG_ACCURACY_MAXIMUM_PENALTY_TIME	3.5f		// Maximum penalty to deal out

// TO-DO: fix shell coming out of weapon on draw if holstered before pumped (play pumping animation)
LINK_ENTITY_TO_CLASS(weapon_shotgun, CShotgun);

void CShotgun::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");

	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;

	FallInit(); // get ready to fall
}

void CShotgun::Holster()
{
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(0);
	MESSAGE_END();
}

void CShotgun::Precache()
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shotgunshell.mdl"); // shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/dbarrel1.wav"); //shotgun
	PRECACHE_SOUND("weapons/sbarrel1.wav"); //shotgun

	PRECACHE_SOUND("weapons/reload1.wav"); // shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav"); // shotgun reload

	PRECACHE_SOUND("weapons/357_cock1.wav"); // gun empty sound
}

bool CShotgun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return true;
}



bool CShotgun::Deploy()
{
	if (g_pGameRules->IsMultiplayer())
		NotFirstDraw = true;

	m_flAccuracyPenalty = SG_ACCURACY_MAXIMUM_PENALTY_TIME;

	m_flPumpTime = 0; // Hack, should probably find a way to tell it to pump the gun after the draw is done

	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT((bool)m_iFiremode ? 3 : 4);
	if (g_iSkillLevel == SKILL_HARD)
		m_iCrossHairType = CROSSHAIR_NOCENTER;
	else
		m_iCrossHairType = (bool)m_iFiremode ? CROSSHAIR_DUCKBILL : CROSSHAIR_NOCENTER;
	MESSAGE_END();

	if (m_pPlayer->m_iWeaponStatus == 1 || m_pPlayer->m_iWeaponStatus == 3) // training
	{
		if (!NotFirstDraw)
			return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW_FIRST, "shotgun");								// Change it to the training SG model
		return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", (bool)m_iFiremode ? SHOTGUN_DRAW_SEMI : SHOTGUN_DRAW, "shotgun");	// Change it to the training SG model
	}
	else
	{
		if (!NotFirstDraw)
			return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW_FIRST, "shotgun");
		return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", (bool)m_iFiremode ? SHOTGUN_DRAW_SEMI : SHOTGUN_DRAW, "shotgun");
	}
}

void CShotgun::ItemPreFrame()
{
	// Check our penalty time decay
	if ( /*( (m_pPlayer->m_afButtonLast & IN_ATTACK) == 0) &&*/ ( m_flTimeSincePrimary + m_flNextPrimaryAttack < gpGlobals->time ) )
	{
		m_flAccuracyPenalty -= 2*gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, SG_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
	//ALERT(at_console, "m_flAccuracyPenalty: %f \n", m_flAccuracyPenalty);
}

const Vector& CShotgun::GetBulletSpread()
{		
	static Vector cone;

	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, SG_ACCURACY_MAXIMUM_PENALTY_TIME, 0.0f, 1.0f);
	if (g_iSkillLevel != SKILL_HARD)
	{
		// We lerp from very accurate to inaccurate over time
		VectorLerp(m_iFiremode == 0 ? VECTOR_CONE_8DEGREES : VECTOR_CONE_10DEGREES, m_iFiremode == 0 ? VECTOR_CONE_20DEGREES : VECTOR_CONE_15DEGREES, ramp, cone);
	}
	else
	{
		// We lerp from very accurate to inaccurate over time
		VectorLerp(0.013095, VECTOR_CONE_10DEGREES, ramp, cone);
	}

	if ((m_pPlayer->m_afButtonLast & IN_RUN) != 0 && m_pPlayer->pev->velocity.Length() > 100)
		cone = cone + VECTOR_CONE_2DEGREES;

	return cone;
}


void CShotgun::PrimaryAttack()
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

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_SG, 0.0, PE_MUZZLESMKSG, 0, 0, 0);
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition(); // + gpGlobals->v_forward * 20 + gpGlobals->v_right * 4 + gpGlobals->v_up * -8;
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	float spread = GetBulletSpread().x;
	float spreadvert = m_iFiremode == 0 ? GetBulletSpread().x : CONE_2DEGREES;

	m_flTimeSincePrimary = gpGlobals->time;
	m_flAccuracyPenalty += SG_ACCURACY_SHOT_PENALTY_TIME;

	//m_pPlayer->FireBullets(9, vecSrc, vecAiming, spread, 2048, BULLET_PLAYER_BUCKSHOT, 1);
	#ifndef CLIENT_DLL
	if (m_pPlayer->m_iWeaponStatus == 0 || m_pPlayer->m_iWeaponStatus == 2)
	{
		if (g_iSkillLevel != SKILL_HARD)
		{
			CPhysbullet::BulletCreate(6, gSkillData.plrDmgBuckshot, 5750, vecSrc, vecAiming, spread, spreadvert, 0.75, 12, m_pPlayer->edict());
		}
		else
		{
			CPhysbullet::BulletCreate(9, 11, 5750, vecSrc, vecAiming, spread, spread, 1, 12, m_pPlayer->edict()); // 1.5 degree spread
		}
	}
	else
	{
		CPhysbullet::BulletCreate(3, g_iSkillLevel == SKILL_HARD ? 3.33f : 1, 3750, vecSrc, vecAiming, spread, spread, 1, 69, m_pPlayer->edict());
	}
	#endif

	if (m_iFiremode == 0)
	{
		SendWeaponAnim(m_iClip == 0 ? SHOTGUN_SHOOT1_PUMP_EMPTY : SHOTGUN_SHOOT1_PUMP);
		m_flPumpTime = gpGlobals->time + 0.5;
	}
	else
	{
		SendWeaponAnim(SHOTGUN_SHOOT1_SEMI);
		Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
		EjectBrass(m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 10 + gpGlobals->v_right * 4, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHOTSHELL); 
	}

	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM);
	AcousticMod(); 

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0) // HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_iFiremode == 1 ? 0.2 : 0.9;
	m_flNextTertiaryAttack = gpGlobals->time + m_iFiremode == 1 ? 0.2 : 0.9;
	m_flTimeWeaponIdle = 1;
	m_fInSpecialReload = 0;
	
	pev->armortype = 1;

#ifndef CLIENT_DLL
	if ((m_pPlayer->pev->button & IN_DUCK) != 0)
	{
		CBasePlayerWeapon::Recoil(3, 2);
	}
	else
	{
		CBasePlayerWeapon::Recoil(4, 2);
	}
#endif
}


void CShotgun::SecondaryAttack()
{
	if (g_iSkillLevel == SKILL_HARD)
	{
		return;
	}

	if ((m_pPlayer->m_afButtonLast & IN_ATTACK2) != 0)
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 1)
	{
		Reload();
		PlayEmptySound();
		return;
	}

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_DSG, 0.0, PE_MUZZLESMKSG, 0, 1, 0);
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flTimeSincePrimary = gpGlobals->time;

	Vector vecSrc = m_pPlayer->GetGunPosition();// + gpGlobals->v_forward * 20 + gpGlobals->v_right * 4 + gpGlobals->v_up * -8;
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	float spread = m_iFiremode == 0 ? CONE_10DEGREES : CONE_20DEGREES;
	float spreadvert = m_iFiremode == 0 ? CONE_10DEGREES : CONE_2DEGREES;
	m_flTimeSincePrimary = gpGlobals->time;
	m_flAccuracyPenalty += 2 * SG_ACCURACY_SHOT_PENALTY_TIME;

	//m_pPlayer->FireBullets(18, vecSrc, vecAiming, spread, 2048, BULLET_PLAYER_BUCKSHOT, 1);
	#ifndef CLIENT_DLL
	if (m_pPlayer->m_iWeaponStatus == 0 || m_pPlayer->m_iWeaponStatus == 2)
	{
		CPhysbullet::BulletCreate(12, gSkillData.plrDmgBuckshot, 5750, vecSrc, vecAiming, spread, spreadvert, 0.8, 12, m_pPlayer->edict());
	}
	else
	{
		CPhysbullet::BulletCreate(6, g_iSkillLevel == SKILL_HARD ? 3.33f : 1, 3750, vecSrc, vecAiming, CONE_4DEGREES, CONE_3DEGREES, 1, 69, m_pPlayer->edict());
	}
	#endif
	
	//m_pPlayer->FireBullets(18, vecSrc, vecAiming, Vector(0.25, 0.02, 0.25), 2048, BULLET_PLAYER_BUCKSHOT, 1);

	if (m_iFiremode == 0)
		SendWeaponAnim(m_iClip == 0 ? SHOTGUN_SHOOT2_PUMP_EMPTY : SHOTGUN_SHOOT2_PUMP);
	else
		SendWeaponAnim(SHOTGUN_SHOOT2_SEMI);

	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/dbarrel1.wav", 1, ATTN_NORM);
	AcousticMod();

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0) // HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(m_iFiremode == 1 ? 1.5 : 1.2);
	m_flNextTertiaryAttack = gpGlobals->time + m_iFiremode == 1 ? 1.5 : 1.2;
	m_flTimeWeaponIdle = 2;
	m_fInSpecialReload = 0;

	if (m_iFiremode == 1)
		m_flPumpTime = gpGlobals->time + 1;
	else
		m_flPumpTime = gpGlobals->time + 0.6;

	pev->armortype = 2;

#ifndef CLIENT_DLL
	if ((m_pPlayer->pev->button & IN_DUCK) != 0)
	{
		CBasePlayerWeapon::Recoil(7, 4);
	}
	else
		{
		CBasePlayerWeapon::Recoil(10, 5);
	}
#endif
}

void CShotgun::TertiaryAttack()
{
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 1.03; // TO-DO: exact value
	m_flTimeWeaponIdle = m_flNextTertiaryAttack = gpGlobals->time + 1.13;

	SendWeaponAnim(m_iFiremode == 1 ? SHOTGUN_SEMI_TO_PUMP: SHOTGUN_PUMP_TO_SEMI);

	if (m_iFiremode == 0)
		m_iFiremode = 1;
	else
		m_iFiremode = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT((bool)m_iFiremode ? 3 : 4);
	MESSAGE_END();
	if (g_iSkillLevel == SKILL_HARD)
		m_iCrossHairType = CROSSHAIR_NOCENTER;
	else
		m_iCrossHairType = (bool)m_iFiremode ? CROSSHAIR_DUCKBILL : CROSSHAIR_NOCENTER;
}

void CShotgun::Reload()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		if (m_iClip == 0)
		{
			SendWeaponAnim(m_iFiremode == 1 ? SHOTGUN_RELOAD_START_EMPTY_SEMI : SHOTGUN_RELOAD_START_EMPTY);
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.6;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.6;
			m_flNextPrimaryAttack = GetNextAttackDelay(1.6);
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.6;
			m_fInSpecialReload = 2;
		}
		else
		{
			SendWeaponAnim(m_iFiremode == 1 ? SHOTGUN_RELOAD_START_SEMI : SHOTGUN_RELOAD_START);
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

		SendWeaponAnim(m_iFiremode == 1 ? SHOTGUN_RELOAD_INSERT_SEMI : SHOTGUN_RELOAD_INSERT);

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}


void CShotgun::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && 0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != 9 && 0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload();
			}
			else
			{
				SendWeaponAnim(m_iFiremode == 1 ? SHOTGUN_RELOAD_END_SEMI : SHOTGUN_RELOAD_END);
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			switch (RANDOM_LONG(1, 3))
			{
			case 1: SendWeaponAnim(m_iFiremode == 1 ? SHOTGUN_IDLE1_SEMI : SHOTGUN_IDLE1), m_flTimeWeaponIdle = 1.93; break;
			case 2: SendWeaponAnim(m_iFiremode == 1 ? SHOTGUN_IDLE2_SEMI : SHOTGUN_IDLE2), m_flTimeWeaponIdle = 3.43; break;
			case 3: SendWeaponAnim(m_iFiremode == 1 ? SHOTGUN_IDLE3_SEMI : SHOTGUN_IDLE3), m_flTimeWeaponIdle = 3.23; break;
			}
		}
	}
}

void CShotgun::ItemPostFrame()
{
	if (m_flPumpTime < gpGlobals->time && m_flPumpTime != 0)
	{
		Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
		EjectBrass(m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -14 + gpGlobals->v_forward * 8 + gpGlobals->v_right * 4, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHOTSHELL); 
		
		if (pev->armortype == 2)
		{
			Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
			EjectBrass(m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -14 + gpGlobals->v_forward * 8 + gpGlobals->v_right * 4, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHOTSHELL);  
		}
		
		m_flPumpTime = 0;
	}
	CBasePlayerWeapon::ItemPostFrame();
}


class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_shotbox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_shotbox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_buckshot, CShotgunAmmo);
