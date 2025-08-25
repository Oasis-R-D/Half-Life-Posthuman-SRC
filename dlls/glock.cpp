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
	m_usFireGlock2 = PRECACHE_EVENT(1, "events/glock2.sc");
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
	if (!NotFirstDraw)
		return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW_FIRST, "onehanded", pev->body);
	return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded", pev->body);
}

void CGlock::SecondaryAttack()
{
	//GlockFire(0.1, 0.2, false);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 1;
	pev->armortype = !pev->armortype;
	SendWeaponAnim(GLOCK_HOLSTER, pev->body);
	m_flTimeWeaponIdle = 2;
	pev->armorvalue = gpGlobals->time + 1;
}

void CGlock::ItemPostFrame()
{
	if (pev->armorvalue < gpGlobals->time && pev->armorvalue != 0)
	{
		pev->armorvalue = 0;
		pev->body = pev->armortype;
		SendWeaponAnim(GLOCK_DRAW, pev->body);
	}
	CBasePlayerWeapon::ItemPostFrame();
}

void CGlock::PrimaryAttack()
{
	if (pev->armortype == 0)
		GlockFire(pev->body ? 0.01 : 0.02, 0.1, true);
	else
		GlockFire(pev->body ? 0.008 : 0.02, 0.2, true);
}

void CGlock::GlockFire(float flSpread, float flCycleTime, bool fUseAutoAim)
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0)
		return;

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.2);
		return;
	}

	m_iClip--;

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
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 17 + gpGlobals->v_right * 8 + gpGlobals->v_up * -3;
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}
	if (g_iSkillLevel != SKILL_HARD)
	{
		if (pev->armortype == 0)
		{
		// m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 1);
		#ifndef CLIENT_DLL
			CPhysbullet::BulletCreate(1, gSkillData.plrDmg9MM, 6000, vecSrc, vecAiming, flSpread, flSpread, 0.66, 9, m_pPlayer->edict());
		#endif
		}
		else
		{
			// m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 1, 9);
			#ifndef CLIENT_DLL
			CPhysbullet::BulletCreate(1, (gSkillData.plrDmg9MM + 2), 6333, vecSrc, vecAiming, flSpread, flSpread, 0.66, 9, m_pPlayer->edict()); // make it not have tracers?
			#endif
		}
	}
	else // realism diff (hardcoded damages to prevent cheaters)
	{
	
		if (pev->armortype == 0)
		{
		// m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 1);
		#ifndef CLIENT_DLL
			CPhysbullet::BulletCreate(1, 25, 6000, vecSrc, vecAiming, CONE_1DEGREES, CONE_1DEGREES, 1, 9, m_pPlayer->edict());
		#endif
		}
		else
		{
			// m_pPlayer->FireBullets(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 1, 9);
			#ifndef CLIENT_DLL
			CPhysbullet::BulletCreate(1, 26, 6100, vecSrc, vecAiming, CONE_1DEGREES, CONE_1DEGREES, 1, 9, m_pPlayer->edict()); // make it not have tracers?
			#endif
		}
	}
	if (pev->armortype == 0)
	{
		SendWeaponAnim(m_iClip == 0 ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 0);
	}
	else
	{
		SendWeaponAnim(m_iClip == 0 ? GLOCK_SHOOT_EMPTY_SILENCER : GLOCK_SHOOT_SILENCER, 1);
	}
	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, pev->body ? "weapons/pl_gun1.wav" : "weapons/pl_gun3.wav", 1, ATTN_NORM);

	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 18 + gpGlobals->v_right * 6, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flTimeWeaponIdle = 1;
	m_pPlayer->pev->punchangle.y += RANDOM_LONG(-2, 2);
	if (pev->armortype == 0)
		if ((m_pPlayer->pev->button & IN_DUCK) != 0)
		{
			m_pPlayer->pev->punchangle.x -= 1;
		}
		else
		{
			m_pPlayer->pev->punchangle.x -= 2;
		}
	else
		if ((m_pPlayer->pev->button & IN_DUCK) != 0)
		{
			m_pPlayer->pev->punchangle.x -= 3;
		}
		else
		{
			m_pPlayer->pev->punchangle.x -= 4;
		}
}

void CGlock::Reload()
{
	DefaultReload(17, m_iClip == 0 ? GLOCK_RELOAD : GLOCK_RELOAD_NOT_EMPTY, 1.5, pev->body);
}

void CGlock::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	NotFirstDraw = true;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		if (m_pPlayer->pev->velocity.Length() > 384)
		{
			if (pev->weapons == 0)
			{
				SendWeaponAnim(GLOCK_IDLE_TO_SPRINT, pev->body);
				m_flTimeWeaponIdle = 0.25;
				pev->weapons = 1;
			}
			else if (pev->weapons == 1)
			{
				SendWeaponAnim(GLOCK_SPRINT, pev->body);
				m_flTimeWeaponIdle = 0.42;
			}
			return;
		}
		else
		{
			if (pev->weapons == 1)
			{
				SendWeaponAnim(GLOCK_SPRINT_TO_IDLE, pev->body);
				m_flTimeWeaponIdle = 0.42;
				pev->weapons = 0;
				return;
			}
		}

		switch (RANDOM_LONG(1, 3))
		{
		case 1: SendWeaponAnim(GLOCK_IDLE1, pev->body), m_flTimeWeaponIdle = 2; break;
		case 2: SendWeaponAnim(GLOCK_IDLE2, pev->body), m_flTimeWeaponIdle = 1.37; break;
		case 3: SendWeaponAnim(GLOCK_IDLE3, pev->body), m_flTimeWeaponIdle = 1.67; break;
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
