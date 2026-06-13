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

//=========================================================
// M727
//=========================================================
#define	M727_ACCURACY_SHOT_PENALTY_TIME		0.125f	// Applied amount of time each shot adds to the time we must recover from
#define	M727_ACCURACY_MAXIMUM_PENALTY_TIME	0.5f	// Maximum penalty to deal out

LINK_ENTITY_TO_CLASS(weapon_m727, CM727);

void CM727::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_727.mdl");
	m_iId = WEAPON_M727;
	m_iDefaultAmmo = 30;
	FallInit(); // get ready to fall down.
}

void CM727::Precache()
{
	PRECACHE_MODEL("models/w_727.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");
	PRECACHE_MODEL("models/v_727.mdl");
	PRECACHE_MODEL("models/grenade.mdl"); // grenade
	PRECACHE_MODEL("models/w_727mag.mdl");

	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl"); // brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");
	PRECACHE_SOUND("weapons/727_hks1.wav"); // H to the K 727 times
	PRECACHE_SOUND("weapons/727_hks2.wav"); // H to the K 727 times
	PRECACHE_SOUND("weapons/727_hks3.wav"); // H to the K 727 times
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");
}

bool CM727::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = _556MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M727;
	p->iWeight = MP5_WEIGHT;

	return true;
}

bool CM727::Deploy()
{
	if (g_pGameRules->IsMultiplayer())
		NotFirstDraw = true;

	m_iCrossHairType = CROSSHAIR_DEFAULT;
	m_flAccuracyPenalty = M727_ACCURACY_MAXIMUM_PENALTY_TIME;

	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(firemode ? 3 : 1);
	MESSAGE_END();

	if (NotFirstDraw)
	{
		if (pev->weapons == 0)
			return DefaultDeploy("models/v_727.mdl", "models/p_9mmAR.mdl", M727_DRAW, "mp5");
	}
	return DefaultDeploy("models/v_727.mdl", "models/p_9mmAR.mdl", M727_DRAW_FIRST, "mp5");
}

#ifndef CLIENT_DLL
// spray pattern from https://steamcommunity.com/sharedfiles/filedetails/?id=3533371752 (m4a1)
void CM727::TestSprayPat(int bulletnum)
{
	static float pattern[31][2] =
	{
		{-0.4,0},
		{-0.7, -0.575},
		{-0.9, 0.53125},
		{-0.7, -0.55625},
		{-1, 0.525},
		{-0.8375, -0.575},
		{-1.25, 0.50625},
		{-1, 0.175},
		{-0.8125, 0.8},
		{-0.9125, 0.65625},
		{-1, 0.9375},
		{-0.9, -0.65625},
		{-0.80625, 0.78125},
		{-1, -1.1875},
		{-0.9125, -0.78125},
		{-0.925, 1.0625},
		{-1, 0.90625},
		{-0.83125, -0.8125},
		{-0.85625, -0.625},
		{-1.05, -0.25},
		{-0.85625, -1.0625},
		{-0.8125, -0.25},
		{-0.85625, -0.9375},
		{-0.9375, 0.125},
		{-0.8125, 1.3125},
		{-0.8875, -1.0625},
		{-0.83125, 1.15625},
		{-0.99375, 1.18625},
		{-0.96875, -1.34375},
		{-0.86875, -1.34375},
		{-0.86875, 1.14375}
	};
	float recup;
	float recside;

	recup = pattern[bulletnum][0];
	recside = 0.9f * pattern[bulletnum][1];
	return Recoil(-recup, -recside, true);
}
#endif

void CM727::Holster()
{
	m_flAccuracyPenalty = 0.0;
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(0);
	MESSAGE_END();
}

void CM727::ItemPreFrame()
{
	// Check our penalty time decay
	if ( ( m_flTimeSincePrimary + m_flNextPrimaryAttack < gpGlobals->time ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, M727_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
	//ALERT(at_console, "m_flAccuracyPenalty: %f \n", m_flAccuracyPenalty);
}

const Vector& CM727::GetBulletSpread()
{		
	static Vector cone;

	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, M727_ACCURACY_MAXIMUM_PENALTY_TIME, 0.0f, 1.0f ); 

	// We lerp from very accurate to inaccurate over time
	VectorLerp( VECTOR_CONE_1DEGREES/2, VECTOR_CONE_3DEGREES, ramp, cone );
	
	if ((m_pPlayer->m_afButtonLast & IN_RUN) != 0 && m_pPlayer->pev->velocity.Length() > 100)
		cone = cone + VECTOR_CONE_1DEGREES;

	return cone;
}

void CM727::PrimaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0 && firemode == true)
		return;

	if (m_pPlayer->pev->waterlevel == 3 || m_iClip <= 0) // don't fire underwater or if emptied
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_flNextSecondaryAttack = 0.15;

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_M727, 0.0, PE_MUZZLESMK, 0, 0, 0);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1); // player "shoot" animation

	Vector vecSrc = m_pPlayer->GetGunPosition(); // + gpGlobals->v_forward * 20 + gpGlobals->v_right * 9 + gpGlobals->v_up * -10;
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector spread = GetBulletSpread();
	m_flTimeSincePrimary = gpGlobals->time;
	m_flAccuracyPenalty += M727_ACCURACY_SHOT_PENALTY_TIME;

	//m_pPlayer->FireBullets(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_M727, 1);
	#ifndef CLIENT_DLL
	if (m_pPlayer->m_iWeaponStatus == 0 || m_pPlayer->m_iWeaponStatus == 2)
	{
		if (g_iSkillLevel != SKILL_REALISM)
		{
			CPhysbullet::BulletCreate(1, gSkillData.plrDmgM727, 7000, vecSrc, vecAiming, spread.x, spread.y, 0.66, 556, m_pPlayer->edict());
		}
		else
		{
			CPhysbullet::BulletCreate(1, 34, 7000, vecSrc, vecAiming, spread.x, spread.y, 1, 556, m_pPlayer->edict());
		}
	}
	else
	{
		CPhysbullet::BulletCreate(1, g_iSkillLevel == SKILL_REALISM ? 10 : 3, 4000, vecSrc, vecAiming, CONE_1DEGREES, CONE_1DEGREES, 1, 69, m_pPlayer->edict());
	}
	#endif

	SendWeaponAnim(RANDOM_LONG(M727_SHOOT1, M727_SHOOT3));

	char wpnsnd2[256];
	sprintf(wpnsnd2, "weapons/727_hks%d.wav", RANDOM_LONG(1, 3));
	EMIT_SOUND(edict(), CHAN_WEAPON, wpnsnd2, 1, ATTN_NORM);
	AcousticMod(); 

	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -5 + gpGlobals->v_forward * 11 + gpGlobals->v_right * 6, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	m_flNextPrimaryAttack = g_iSkillLevel == SKILL_REALISM ? 0.0727 : 0.1;

	m_flTimeWeaponIdle = 5;

#ifndef CLIENT_DLL
	/*
	TestSprayPat(iMaxClip() - m_iClip); // breaks the camera sometimes
	*/
	CBasePlayerWeapon::Recoil(0.85, 1.2);
#endif
}

void CM727::TertiaryAttack()
{
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.125;
	if ((m_pPlayer->m_afButtonLast & IN_ALT1) != 0)
		return;

	if (firemode == true)
	{
		firemode = false;
	}
	else
	{
		firemode = true;
	}
	EMIT_SOUND(edict(), CHAN_ITEM, "items/9mmclip2.wav", 1, ATTN_NORM);
	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(firemode ? 3 : 1);
	MESSAGE_END();
}

void CM727::Reload()
{
	DefaultReload(31, m_iClip == 0 ? M727_RELOAD_EMPTY : M727_RELOAD_TACTICAL, m_iClip == 0 ? 1.8 : 1.2);
}

void CM727::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (RANDOM_LONG(0, 1))
		SendWeaponAnim(M727_IDLE1), m_flTimeWeaponIdle = 2.5;
	else
		SendWeaponAnim(M727_IDLE2), m_flTimeWeaponIdle = 4;
		
}

class CM727AmmoClip : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_727mag.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_727mag.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(30, "556", _556MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_556mag, CM727AmmoClip);