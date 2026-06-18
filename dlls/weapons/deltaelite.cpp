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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "physical_bullet.h"

#define	ELITE_ACCURACY_SHOT_PENALTY_TIME	0.5f	// Applied amount of time each shot adds to the time we must recover from
#define	ELITE_ACCURACY_MAXIMUM_PENALTY_TIME	1.5		// Maximum penalty to deal out

LINK_ENTITY_TO_CLASS(weapon_elite, CElite);
LINK_ENTITY_TO_CLASS(weapon_10mmhandgun, CElite);

bool CElite::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "10mm";
	p->iMaxAmmo1 = _10MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = ELITE_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_ELITE;
	p->iWeight = ELITE_WEIGHT;

	return true;
}

void CElite::Spawn()
{
	pev->classname = MAKE_STRING("weapon_10mmhandgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_ELITE;
	SET_MODEL(ENT(pev), "models/w_10mmhandgun.mdl");

	m_iDefaultAmmo = ELITE_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}

void CElite::Precache()
{
	PRECACHE_MODEL("models/v_10mmhandgun.mdl");
	PRECACHE_MODEL("models/w_10mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_MODEL("models/w_10mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/357_reload1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("weapons/357_shot1.wav");
	PRECACHE_SOUND("weapons/357_shot2.wav");
	m_usFireElite = PRECACHE_EVENT(1, "scripts/events/elite.sc");
}

bool CElite::Deploy()
{
	if (g_pGameRules->IsMultiplayer())
		NotFirstDraw = true;

	m_iCrossHairType = CROSSHAIR_DEFAULT;
	m_flAccuracyPenalty = ELITE_ACCURACY_MAXIMUM_PENALTY_TIME;

	return DefaultDeploy("models/v_10mmhandgun.mdl", "models/p_9mmhandgun.mdl", PYTHON_DRAW, "onehanded");
}


void CElite::Holster()
{
	m_fInReload = false; // cancel any reload in progress.
	m_flAccuracyPenalty = 0.0;
	if (m_pPlayer->m_iFOV != 0)
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(PYTHON_HOLSTER);
}

void CElite::ItemPreFrame()
{
	// Check our penalty time decay
	if ( ( m_flTimeSincePrimary + m_flNextPrimaryAttack < gpGlobals->time ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, ELITE_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
	//ALERT(at_console, "m_flAccuracyPenalty: %f \n", m_flAccuracyPenalty);
}

const Vector& CElite::GetBulletSpread()
{		
	static Vector cone;

	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, ELITE_ACCURACY_MAXIMUM_PENALTY_TIME, 0.0f, 1.0f ); 

	// We lerp from very accurate to inaccurate over time
	VectorLerp( g_vecZero, VECTOR_CONE_4DEGREES, ramp, cone );

	if ((m_pPlayer->m_afButtonLast & IN_RUN) != 0 && m_pPlayer->pev->velocity.Length() > 100)
		cone = cone + VECTOR_CONE_2DEGREES;

	return cone;
}

void CElite::PrimaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0)
		return;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_NORM, 0.0, PE_MUZZLESMK, 0, 0, 0);
	m_pPlayer->m_iWeaponVolume = 800; // middle of normal and loud
	m_pPlayer->m_iWeaponFlash = 384; // middle of bright and normal
	
	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);


	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector vecDir;
	//vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 1, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	//m_pPlayer->FireBullets(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 1);

	float spread = GetBulletSpread().x;
	m_flTimeSincePrimary = gpGlobals->time;
	m_flAccuracyPenalty += ELITE_ACCURACY_SHOT_PENALTY_TIME;

	#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_REALISM)
	{
		CPhysbullet::BulletCreate(1, gSkillData.plrDmg10MM, 6500, vecSrc, vecAiming, spread, spread, 0.66f, 10, m_pPlayer->edict());
	}
	else
	{
		CPhysbullet::BulletCreate(1, 35, 6500, vecSrc, vecAiming, spread, spread, 1, 10, m_pPlayer->edict());
	}

	if ((m_pPlayer->pev->button & IN_DUCK) != 0)
	{
		CBasePlayerWeapon::Recoil(1.5, RANDOM_FLOAT(0.85, 1.00));
	}
	else
	{
		CBasePlayerWeapon::Recoil(2, RANDOM_FLOAT(1.00, 1.15));
	}
	#endif

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// TO-DO: move to EV_HLDM
	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -10 + gpGlobals->v_forward * 19 + gpGlobals->v_right * 6, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireElite, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);
	AcousticMod(); 
	
	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.125;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CElite::Reload()
{
	if (m_pPlayer->eng_ammo_10mm <= 0)
		return;

	DefaultReload(ELITE_MAX_CLIP, PYTHON_RELOAD, 2.0, 0);
}


void CElite::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
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

	SendWeaponAnim(iAnim, 0);
}

void CElite::ItemPostFrame()
{
	CBasePlayerWeapon::ItemPostFrame();
}

class CEliteAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_10mmclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_10mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(3*AMMO_ELITECLIP_GIVE, "10mm", _10MM_MAX_CARRY) != -1) // 3 clips are in the model // TO-DO: make it into 1 clip
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_10mmclip, CEliteAmmo);
LINK_ENTITY_TO_CLASS(ammo_eliteclip, CEliteAmmo);
