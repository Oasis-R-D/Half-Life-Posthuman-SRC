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

#define	M249_ACCURACY_SHOT_PENALTY_TIME		0.05f	// Applied amount of time each shot adds to the time we must recover from
#define	M249_ACCURACY_MAXIMUM_PENALTY_TIME	3.5f	// Maximum penalty to deal out

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

	m_usFireM249 = PRECACHE_EVENT(1, "scripts/events/m249.sc");
}

void CM249::Spawn()
{
	pev->classname = MAKE_STRING("weapon_m249");
	Precache();
	m_iId = WEAPON_CHAINGUN;
	SET_MODEL(edict(), "models/w_saw.mdl");
	m_iDefaultAmmo = M249_DEFAULT_GIVE;
	FallInit(); // get ready to fall down.
}

bool CM249::Deploy()
{
	if (g_pGameRules->IsMultiplayer())
		NotFirstDraw = true;

	m_iCrossHairType = CROSSHAIR_DEFAULT;
	m_flAccuracyPenalty = M249_ACCURACY_MAXIMUM_PENALTY_TIME;

	m_pPlayer->pev->maxspeed = 192;

	return DefaultDeploy("models/v_saw.mdl", "models/p_saw.mdl", M249_DRAW, "mp5");
}

void CM249::Holster()
{
	m_flAccuracyPenalty = 0.0;
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
	if (DefaultReload(M249_MAX_CLIP, M249_RELOAD_START, 1.0))
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
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

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

void CM249::ItemPreFrame()
{
	// Check our penalty time decay
	if ( ( (m_pPlayer->m_afButtonLast & IN_ATTACK) == 0) && ( m_flTimeSincePrimary + m_flNextPrimaryAttack < gpGlobals->time ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, M249_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
	//ALERT(at_console, "m_flAccuracyPenalty: %f \n", m_flAccuracyPenalty);
}

const Vector& CM249::GetBulletSpread()
{		
	static Vector cone;

	float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f, M249_ACCURACY_MAXIMUM_PENALTY_TIME, 0.0f, 1.0f ); 

	// We lerp from very accurate to inaccurate over time
	VectorLerp( g_vecZero, VECTOR_CONE_6DEGREES, ramp, cone );

	float vecSpread;

	if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING) && FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
	{
		vecSpread = CONE_2DEGREES;
	}
	else if ((m_pPlayer->pev->button & (IN_MOVERIGHT |
										IN_MOVELEFT |
										IN_FORWARD |
										IN_BACK)) != 0 || !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
	{
		if ((m_pPlayer->m_afButtonLast & IN_RUN) != 0 || !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
			vecSpread = CONE_10DEGREES*1.125;
		else
			vecSpread = CONE_10DEGREES;
	}
	else
	{
		vecSpread = CONE_4DEGREES;
	}

	for (int i = 0; i < 3; i++)
	{
		cone[i] += vecSpread;
	}

	return cone;
}

void CM249::PrimaryAttack()
{
	CM249::Shoot(false);
}

void CM249::SecondaryAttack()
{
	CM249::Shoot(true);
}

void CM249::Shoot(bool alt)
{
	m_iCrossHairType = alt ? CROSSHAIR_NONE : CROSSHAIR_DEFAULT;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fInReload)
		{
			PlayEmptySound();

			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.15;
		}

		return;
	}

	--m_iClip;

	pev->body = RecalculateBody(m_iClip);

	PLAYBACK_EVENT_FULL(FEV_HOSTONLY, m_pPlayer->edict(), g_sParticleEvent, 0.0, gpGlobals->v_forward, gpGlobals->v_forward, AC_NORM, 0.0, PE_MUZZLESMK, 0, 0, 0);
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_flNextAnimTime = UTIL_WeaponTimeBase() + 0.2;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();

	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	float vecSpread;

	vecSpread = GetBulletSpread().x;
	m_flTimeSincePrimary = gpGlobals->time;
	m_flAccuracyPenalty += M249_ACCURACY_SHOT_PENALTY_TIME;

	#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_REALISM)
	{
		CPhysbullet::BulletCreate(1, gSkillData.plrDmgMP5 + 1, 7000, vecSrc, vecAiming, vecSpread, vecSpread * (alt? 0.33 : 0.8), 0.75, 556, m_pPlayer->edict());
	}
	else
	{
		CPhysbullet::BulletCreate(1, 34, 7000, vecSrc, vecAiming, vecSpread * 0.75, vecSpread * 0.75, 1, 556, m_pPlayer->edict());
	}

	if (alt)
		CBasePlayerWeapon::Recoil(0.65, clampSine(cos(2*gpGlobals->time+RANDOM_FLOAT(-0.33, 0.33))*2, 0.7, 1.75), true);
	else
		CBasePlayerWeapon::Recoil(0.65, 1.125);
	#endif

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(
		flags, m_pPlayer->edict(), m_usFireM249, 0,
		g_vecZero, g_vecZero,
		0, 0,
		pev->body, 0,
		0, 0);

	if (0 == m_iClip)
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		{
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK);
		}
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + (g_iSkillLevel != SKILL_REALISM ? 0.085 : 0.06);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2;

#ifndef CLIENT_DLL
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
	return 3;
}

bool CM249::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = _556MM_MAX_CARRY;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = M249_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 2;
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
		if (pOther->GiveAmmo(M249_DEFAULT_GIVE, "556", _556MM_MAX_CARRY) != -1)
		{
			EMIT_SOUND(edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);

			return true;
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, CAmmo556);