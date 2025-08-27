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
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "monsters.h"
#include "physical_bullet.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CM249::m_SaveData[] =
	{
		DEFINE_FIELD(CM249, m_flReloadStartTime, FIELD_FLOAT),
		DEFINE_FIELD(CM249, m_flReloadStart, FIELD_FLOAT),
		DEFINE_FIELD(CM249, m_bReloading, FIELD_BOOLEAN),
		DEFINE_FIELD(CM249, m_iFire, FIELD_INTEGER),
		DEFINE_FIELD(CM249, m_iSmoke, FIELD_INTEGER),
		DEFINE_FIELD(CM249, m_iLink, FIELD_INTEGER),
		DEFINE_FIELD(CM249, m_iShell, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CM249, CM249::BaseClass);
#endif

LINK_ENTITY_TO_CLASS(weapon_m249, CM249);

void CM249::Precache()
{
	PRECACHE_MODEL("models/v_saw.mdl");
	PRECACHE_MODEL("models/w_saw.mdl");
	PRECACHE_MODEL("models/p_saw.mdl");

	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl");
	m_iLink = PRECACHE_MODEL("models/saw_link.mdl");

	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");
	PRECACHE_SOUND("weapons/saw_fire1.wav");
	PRECACHE_SOUND("weapons/saw_fire2.wav");
	PRECACHE_SOUND("weapons/saw_fire3.wav");
}

void CM249::Spawn()
{
	pev->classname = MAKE_STRING("weapon_m249");
	Precache();
	m_iId = WEAPON_CHAINGUN;
	SET_MODEL(edict(), "models/w_saw.mdl");
	m_iDefaultAmmo = 200;
	m_bAlternatingEject = false;
	FallInit(); // get ready to fall down.
}

bool CM249::Deploy()
{
	m_pPlayer->pev->maxspeed = 200;
	return DefaultDeploy("models/v_saw.mdl", "models/p_saw.mdl", M249_DRAW, "mp5");
}

void CM249::Holster()
{
	SetThink(nullptr);
	SendWeaponAnim(M249_HOLSTER);
	m_bReloading = false;
	m_fInReload = false;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10.0, 15.0);
	m_pPlayer->pev->maxspeed = 0;
}

void CM249::Reload()
{
	if (DefaultReload(200, M249_RELOAD_START, 1.0))
	{
		m_bReloading = true;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3.43;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.43;
		m_flReloadStart = gpGlobals->time + 1;
	}
}

void CM249::WeaponIdle()
{
	ResetEmptySound();

	// Update auto-aim
	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_bReloading && gpGlobals->time > m_flReloadStart)
	{
		m_bReloading = false;
		pev->body = 0;
		SendWeaponAnim(M249_RELOAD_END, pev->body);
	}

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		const float flNextIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);
		int iAnim;
		if (flNextIdle <= 0.95)
		{
			iAnim = M249_SLOWIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
		}
		else
		{
			iAnim = M249_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.16;
		}
		SendWeaponAnim(iAnim, pev->body);
	}
}

void CM249::PrimaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fInReload)
		{
			PlayEmptySound();

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		}

		return;
	}

	--m_iClip;

	pev->body = RecalculateBody(m_iClip);

	m_bAlternatingEject = !m_bAlternatingEject;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	m_flNextAnimTime = UTIL_WeaponTimeBase() + 0.2;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();

	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	float vecSpread;

#ifdef CLIENT_DLL
	if (bIsMultiplayer())
#else
	if (g_pGameRules->IsMultiplayer())
#endif
	{
		if ((m_pPlayer->pev->button & IN_DUCK) != 0)
		{
			vecSpread = CONE_3DEGREES;
		}
		else if ((m_pPlayer->pev->button & (IN_MOVERIGHT |
											   IN_MOVELEFT |
											   IN_FORWARD |
											   IN_BACK)) != 0)
		{
			vecSpread = CONE_15DEGREES;
		}
		else
		{
			vecSpread = CONE_6DEGREES;
		}
	}
	else
	{
		if ((m_pPlayer->pev->button & IN_DUCK) != 0)
		{
			vecSpread = CONE_2DEGREES;
		}
		else if ((m_pPlayer->pev->button & (IN_MOVERIGHT |
											   IN_MOVELEFT |
											   IN_FORWARD |
											   IN_BACK)) != 0)
		{
			vecSpread = CONE_10DEGREES;
		}
		else
		{
			vecSpread = CONE_4DEGREES;
		}
	}

	//m_pPlayer->FireBullets(1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_MP5, 1);
	#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_HARD)
	{
		CPhysbullet::BulletCreate(1, gSkillData.plrDmgMP5, 7000, vecSrc, vecAiming, vecSpread, vecSpread, 0.75, 556, m_pPlayer->edict());
	}
	else
	{
		CPhysbullet::BulletCreate(1, 34, 7000, vecSrc, vecAiming, vecSpread * 0.75, vecSpread * 0.75, 1, 556, m_pPlayer->edict());
	}
	#endif
	SendWeaponAnim(M249_SHOOT1 + RANDOM_LONG(0, 2));
	const char* sound;
	switch (RANDOM_LONG(0, 2))
	{
	case 0: sound = "weapons/saw_fire1.wav"; break;
	case 1: sound = "weapons/saw_fire2.wav"; break;
	case 2: sound = "weapons/saw_fire3.wav"; break;
	}
	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, sound, 1, ATTN_NORM);

	Vector ori = pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 20 + gpGlobals->v_right * 4;
	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(100, 200) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
	EjectBrass(ori, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
	EjectBrass(ori, vecShellVelocity, pev->angles.y, m_iLink, TE_BOUNCE_NULL);

	if (0 == m_iClip)
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		{
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK);
		}
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2;
#ifndef CLIENT_DLL
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		CBasePlayerWeapon::Recoil(0.4, 1.125);
		break;
	case 1:
		CBasePlayerWeapon::Recoil(0.4, -1.125);
		break;
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	const Vector& vecVelocity = m_pPlayer->pev->velocity;

	const float flZVel = m_pPlayer->pev->velocity.z;

	Vector vecInvPushDir = gpGlobals->v_forward * 20.0;

	float flNewZVel = CVAR_GET_FLOAT("sv_maxspeed");

	if (vecInvPushDir.z >= 10.0)
		flNewZVel = vecInvPushDir.z;

	if (!g_pGameRules->IsDeathmatch())
	{
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - vecInvPushDir;

		// Restore Z velocity to make deathmatch easier.
		m_pPlayer->pev->velocity.z = flZVel;
	}
	else
	{
		const float flZTreshold = -(flNewZVel + 100.0);

		if (vecVelocity.x > flZTreshold)
		{
			m_pPlayer->pev->velocity.x -= vecInvPushDir.x;
		}

		if (vecVelocity.y > flZTreshold)
		{
			m_pPlayer->pev->velocity.y -= vecInvPushDir.y;
		}

		m_pPlayer->pev->velocity.z -= vecInvPushDir.z;
	}
#endif
}

int CM249::RecalculateBody(int iClip)
{
	if (iClip == 0)
	{
		return 8;
	}
	else if (iClip >= 0 && iClip <= 7)
	{
		return 9 - iClip;
	}
	else
	{
		return 0;
	}
}

int CM249::iItemSlot()
{
	return 4;
}

bool CM249::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = 200;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = 200;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CHAINGUN;
	p->iWeight = GAUSS_WEIGHT;

	return true;
}

void CM249::GetWeaponData(weapon_data_t& data)
{
	data.iuser1 = pev->body;
}

void CM249::SetWeaponData(const weapon_data_t& data)
{
	pev->body = data.iuser1;
}

class CAmmo556 : public CBasePlayerAmmo
{
public:
	using BaseClass = CBasePlayerAmmo;

	void Precache() override
	{
		PRECACHE_MODEL("models/w_saw_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}

	void Spawn() override
	{
		Precache();

		SET_MODEL(edict(), "models/w_saw_clip.mdl");

		BaseClass::Spawn();
	}

	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(200, "556", 200) != -1)
		{
			EMIT_SOUND(edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);

			return true;
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, CAmmo556);
