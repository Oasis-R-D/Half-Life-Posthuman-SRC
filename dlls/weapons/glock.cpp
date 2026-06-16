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
#include "physical_bullet.h"

#define PISTOL_FASTEST_REFIRE_TIME				0.15f	// spam clicking firerate
#define	PISTOL_ACCURACY_SHOT_PENALTY_TIME		0.25f	// Applied amount of time each shot adds to the time we must recover from
#define	PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME	2.25f	// Maximum penalty to deal out

LINK_ENTITY_TO_CLASS(weapon_glock, CGlock);
LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CGlock);

void CGlock::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}

void CGlock::Precache()
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/pl_gun1.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun2.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun3.wav"); //handgun

	m_usFireGlock1 = PRECACHE_EVENT(1, "events/glock1.sc");
	m_silenceevent = PRECACHE_EVENT(1, "events/glocksilence.sc");
}

bool CGlock::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return true;
}

bool CGlock::Deploy()
{
	if (g_pGameRules->IsMultiplayer())
		NotFirstDraw = true;

	m_iCrossHairType = CROSSHAIR_DEFAULT;
	m_flAccuracyPenalty = PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME;

	PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), m_silenceevent, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, (int)m_iSilenced, 0, 0, 0);

	if (!NotFirstDraw)
		return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW_FIRST, "onehanded", pev->body);
	return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded", pev->body);
}

void CGlock::Holster()
{
	m_flAccuracyPenalty = 0.0;
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
	PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), m_silenceevent, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);
}
void CGlock::SecondaryAttack()
{
	m_iSilenced = !m_iSilenced;
	if (pev->body == 0)
	{
		SendWeaponAnim(GLOCK_ADD_SILENCER, pev->body);
		m_flTimeWeaponIdle = 3.4;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 3.35;
	}
	else
	{
		SendWeaponAnim(GLOCK_HOLSTER, pev->body);
		m_flTimeWeaponIdle = 1.5;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 1.40;
	}
	m_fTimer = gpGlobals->time + 1;
}

void CGlock::ReloadSetAmmos()
{
	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase()))
	{
		// complete the reload.
		int j = V_min(iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

		// Add them to the clip
		if (m_iClip == 0)
		{
			if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 1)
			{
				m_iClip += 1;
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
			}
			else
			{
				m_iClip += j - 1;
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j - 1;
			}
		}
		else
		{
			m_iClip += j;
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;
		}

		m_pPlayer->TabulateAmmo();

		m_fInReload = false;
	}
}

void CGlock::ItemPostFrame()
{
	if (m_fTimer <= gpGlobals->time && m_fTimer != 0)
	{
		m_fTimer = 0;
		PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), m_silenceevent, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, (int)m_iSilenced, 0, 0, 0);
		pev->body = (int)m_iSilenced;
		if (pev->body == 0)
			SendWeaponAnim(GLOCK_DRAW_FIRST, pev->body);
	}
	CBasePlayerWeapon::ItemPostFrame();
}

void CGlock::ItemPreFrame()
{
	// Check our penalty time decay
	if ( ( m_flTimeSincePrimary + m_flNextPrimaryAttack < gpGlobals->time ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
	//ALERT(at_console, "m_flAccuracyPenalty: %f \n", m_flAccuracyPenalty);

	if (m_iClip <= 0)
		return;

	if ((m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)) == 0)
	{
		if (m_flSoonestPrimaryAttack < gpGlobals->time)
			m_flNextPrimaryAttack = 0;
	}
}

const Vector& CGlock::GetBulletSpread()
{		
	static Vector cone;

	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME, 0.0f, 1.0f ); 

	// We lerp from very accurate to inaccurate over time
	VectorLerp( VECTOR_CONE_1DEGREES/2, VECTOR_CONE_4DEGREES, ramp, cone );

	if ((m_pPlayer->m_afButtonLast & IN_RUN) != 0 && m_pPlayer->pev->velocity.Length() > 100)
		cone = cone + VECTOR_CONE_1DEGREES;

	return cone;
}

void CGlock::PrimaryAttack()
{
	GlockFire(GetBulletSpread().x, 0.5f);
}

void CGlock::GlockFire(float flSpread, float flCycleTime)
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flSoonestPrimaryAttack = GetNextAttackDelay(0.2);

		return;
	}

	m_flTimeSincePrimary = gpGlobals->time;
	m_flAccuracyPenalty += PISTOL_ACCURACY_SHOT_PENALTY_TIME;

	m_iClip--;

	int flags;

#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	}
	else
	{
		// non-silenced
		PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_NORM, 0.0, PE_MUZZLESMK, 0, 0, 0);
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

#ifndef CLIENT_DLL
	if (m_pPlayer->m_iWeaponStatus == 0 || m_pPlayer->m_iWeaponStatus == 2)
	{
		if (g_iSkillLevel != SKILL_REALISM)
		{
			if (!m_iSilenced)
			{
			// m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 1);

				CPhysbullet::BulletCreate(1, gSkillData.plrDmg9MM, 6000, vecSrc, vecAiming, flSpread, flSpread, 0.66, 9, m_pPlayer->edict());
			}
			else
			{
				// m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 1, 9);
				CPhysbullet::BulletCreate(1, (gSkillData.plrDmg9MM + 2), 6333, vecSrc, vecAiming, flSpread, flSpread, 0.66, 9, m_pPlayer->edict(), true);
			}
		}
		else // realism diff (hardcoded damages to prevent cheaters)
		{
	
			if (!m_iSilenced)
			{
			// m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 1);
				CPhysbullet::BulletCreate(1, 25, 6000, vecSrc, vecAiming, CONE_1DEGREES, CONE_1DEGREES, 1, 9, m_pPlayer->edict());
			}
			else
			{
				// m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 1, 9);
				CPhysbullet::BulletCreate(1, 26, 6100, vecSrc, vecAiming, CONE_1DEGREES, CONE_1DEGREES, 1, 9, m_pPlayer->edict());
			}
		}
	}
	else
	{
		CPhysbullet::BulletCreate(1, g_iSkillLevel == SKILL_REALISM ? 10 : 3, 3750, vecSrc, vecAiming, CONE_1DEGREES, CONE_1DEGREES, 1, 69, m_pPlayer->edict());
	}

	if ((m_pPlayer->pev->button & IN_DUCK) != 0)
	{
		CBasePlayerWeapon::Recoil(1, RANDOM_FLOAT(0.85, 1.00));
	}
	else
	{
		CBasePlayerWeapon::Recoil(1, RANDOM_FLOAT(1.00, 1.15));
	}
	#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireGlock1, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, (m_iClip == 0) ? 1 : 0, (int)m_iSilenced);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);
	m_flSoonestPrimaryAttack = UTIL_WeaponTimeBase() + PISTOL_FASTEST_REFIRE_TIME;

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CGlock::Reload()
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	bool iResult = DefaultReload(18, m_iClip > 0 ? GLOCK_RELOAD_NOT_EMPTY : GLOCK_RELOAD, 1.5);
	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
}

void CGlock::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// only idle if the slide isn't back
	if (m_iClip != 0)
	{
		if (m_pPlayer->pev->velocity.Length() > 384)
		{
			if (pev->weapons == 0)
			{
				SendWeaponAnim(GLOCK_IDLE_TO_SPRINT, pev->body);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.25;
				pev->weapons = 1;
			}
			else if (pev->weapons == 1)
			{
				SendWeaponAnim(GLOCK_SPRINT, pev->body);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.42;
			}
			return;
		}
		else
		{
			if (pev->weapons == 1)
			{
				SendWeaponAnim(GLOCK_SPRINT_TO_IDLE, pev->body);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.42;
				pev->weapons = 0;
				return;
			}
		}

		switch (UTIL_SharedRandomLong(m_pPlayer->random_seed, 1, 3))
		{
		case 1: SendWeaponAnim(GLOCK_IDLE1, pev->body), m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;		break;
		case 2: SendWeaponAnim(GLOCK_IDLE2, pev->body), m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.37;	break;
		case 3: SendWeaponAnim(GLOCK_IDLE3, pev->body), m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.67;	break;
		}
	}
}

class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_glockclip, CGlockAmmo);
LINK_ENTITY_TO_CLASS(ammo_9mmclip, CGlockAmmo);
