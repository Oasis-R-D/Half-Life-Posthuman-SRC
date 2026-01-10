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
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "monsters.h"
#include "physical_bullet.h"
// Precaches the ammo and queues the ammo info for sending to clients
void AddAmmoNameToAmmoRegistry(const char* szAmmoname, const char* weaponName)
{
	// make sure it's not already in the registry
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if (stricmp(CBasePlayerItem::AmmoInfoArray[i].pszName, szAmmoname) == 0)
			return; // ammo already in registry, just quite
	}


	giAmmoIndex++;
	ASSERT(giAmmoIndex < MAX_AMMO_SLOTS);
	if (giAmmoIndex >= MAX_AMMO_SLOTS)
		giAmmoIndex = 0;

	auto& ammoType = CBasePlayerItem::AmmoInfoArray[giAmmoIndex];

	ammoType.pszName = szAmmoname;
	ammoType.iId = giAmmoIndex; // yes, this info is redundant
	ammoType.WeaponName = weaponName;
}

bool CBasePlayerWeapon::CanDeploy()
{
	bool bHasAmmo = false;

	if (!pszAmmo1())
	{
		// this weapon doesn't use ammo, can always deploy.
		return true;
	}

	if (pszAmmo1())
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0);
	}
	if (pszAmmo2())
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0);
	}
	if (m_iClip > 0)
	{
		bHasAmmo |= true;
	}
	if (!bHasAmmo)
	{
		return false;
	}

	return true;
}

bool CBasePlayerWeapon::DefaultReload(int iClipSize, int iAnim, float fDelay, int body, bool altvm, int iAltAnim)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || V_min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]) == 0)
		return false;

	SendWeaponAnim(iAnim, body);
	if (altvm && iAltAnim != -1)
		SendWeaponAnim(iAltAnim, body, true);

	m_fInReload = true;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;
	m_pPlayer->pev->punchangle.z += RANDOM_LONG(-5, 5);
	m_hasbeeped = false;
	return true;
}

void CBasePlayerWeapon::ResetEmptySound()
{
	m_iPlayEmptySound = true;
}

bool CanAttack(float attack_time, float curtime, bool isPredicted)
{
#if defined(CLIENT_WEAPONS)
	if (!isPredicted)
#else
	if (1)
#endif
	{
		return (attack_time <= curtime) ? true : false;
	}
	else
	{
		return ((static_cast<int>(std::floor(attack_time * 1000.0)) * 1000.0) <= 0.0) ? true : false;
	}
}

void CBasePlayerWeapon::ReloadSetAmmos()
{
	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase()))
	{
		// complete the reload.
		int j = V_min(iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

		m_pPlayer->TabulateAmmo();

		m_fInReload = false;
	}
}

void CBasePlayerWeapon::ItemPostFrame()
{
	ReloadSetAmmos();
	if (m_pPlayer->m_bInGrenadeDelay && m_fGrenadeFireDelay < gpGlobals->time)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgGrenadeHUD, NULL, m_pPlayer->pev);
		WRITE_BYTE(m_pPlayer->m_iGrenadeType);
		WRITE_BYTE(m_pPlayer->m_iGrenadeAmnt);
		MESSAGE_END();
		ShootGrenade(m_pPlayer->m_iGrenadeType);
		m_pPlayer->m_bInGrenadeDelay = false;
		m_pPlayer->m_bInGrenade = false; // TO-DO: move this to per weapon grenade anims since this is for the animations
	}
	if ((m_pPlayer->pev->button & IN_ATTACK) == 0)
	{
		m_flLastFireTime = 0.0f;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) != 0 && CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
	{
		if (pszAmmo2() && 0 == m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()])
		{
			m_fFireOnEmpty = true;
		}

		m_pPlayer->TabulateAmmo();
		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ALT1) != 0 && m_flNextTertiaryAttack < gpGlobals->time)
	{
		TertiaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_SCORE) != 0 && m_flNextGrenadeAttack < gpGlobals->time)
	{
		if (m_pPlayer->m_iGrenadeAmnt <= 0)
		{
			m_flNextGrenadeAttack = gpGlobals->time + 3;
			m_pPlayer->SetSuitUpdate("!HEV_GOUT", false, 0);
			m_pPlayer->m_iGrenadeAmnt = 0;
		}
		else
		{
			m_pPlayer->m_bInGrenade = true;
			m_pPlayer->m_bInGrenadeDelay = true;

			ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "Grenade Start");

			m_pPlayer->m_iGrenadeAmnt--;

			m_flNextGrenadeAttack = gpGlobals->time + 2;
			m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flNextTertiaryAttack = 1.25f;
			m_fGrenadeFireDelay = gpGlobals->time + 0.5f;

			GrenadeAttack();
			m_pPlayer->altviewmodel = MAKE_STRING("models/v_ohgrenade.mdl");
			SendWeaponAnim(OH_THROW, m_pPlayer->m_bPrehuman, true);
			if (m_pPlayer->m_iGrenadeAmnt == 1)
			{
				switch (RANDOM_LONG(1, 3))
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
			}
		}
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
	{
		if ((m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && 0 == m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]))
		{
			m_fFireOnEmpty = true;
		}

		m_pPlayer->TabulateAmmo();
		PrimaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_RELOAD) != 0 && iMaxClip() != WEAPON_NOCLIP && !m_fInReload && m_iClip != iMaxClip())
	{
		// reload when reload key is pressed and the clip is not full.
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
void CBasePlayerWeapon::ShootGrenade(int type)
{
	static float flMultiplier;
	float time;
	int maxvel;
	switch (type)
	{
		default:
		case 0: // HE
			flMultiplier = 6.5f;
			maxvel = 1000;
			break;
		case 1: // IMPACT
			flMultiplier = 10;
			maxvel = 1500;
		case 2: // FLASH
			flMultiplier = 9;
			maxvel = 1400;
			break;
		case 3: // LANDMINE
			flMultiplier = 6.5f;
			maxvel = 1000;
			break;
		case 4: // SMOKE
			flMultiplier = 6.5f;
			maxvel = 1000;
			break;
		case 5: // BRICK
			flMultiplier = 10;
			maxvel = 1500;
		case 6: // SIGNAL FLARE?
			flMultiplier = 6.5f;
			maxvel = 1000;
			break;
	}
	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 8 + gpGlobals->v_right * -6 + gpGlobals->v_up * -4;
	Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

	if (angThrow.x < 0)
		angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
	else
		angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

	
	float flVel = (90 - angThrow.x) * flMultiplier;
	if (flVel > maxvel)
		flVel = maxvel;

	UTIL_MakeVectors(angThrow);

	Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;
	switch (type) // -1 time is impact
	{
		default:
		case 0: // H.E
			time = 3;
			break;
		case 1: // IMPACT
			time = -1;
			break;
		case 2: // FLASH
			time = 2;
			break;
		case 3: // LANDMINE
			time = 2; // doesn't matter
			break;
		case 4: // SMOKE
			time = 2; // doesn't matter
			break;
		case 5: // BRICK
			time = 2; // doesn't matter
			break;
		case 6: // Signal Flare?
			time = RANDOM_FLOAT(0.45, 0.55);
			break;
	}
	CGrenade::ShootOffhand(m_pPlayer->pev, vecSrc, vecThrow, type, time);
}

void CBasePlayer::SelectLastItem()
{
	if (!m_pLastItem)
	{
		return;
	}

	if (m_pActiveItem && !m_pActiveItem->CanHolster())
	{
		return;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	CBasePlayerItem* pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;

	auto weapon = m_pActiveItem->GetWeaponPtr();

	if (weapon)
	{
		weapon->m_ForceSendAnimations = true;
	}

	m_pActiveItem->Deploy();

	if (weapon)
	{
		weapon->m_ForceSendAnimations = false;
	}

	m_pActiveItem->UpdateItemInfo();
}
//=========================================================
// Deagle (why? WHY NOT!)
//=========================================================
#ifndef CLIENT_DLL
TYPEDESCRIPTION CEagle::m_SaveData[] =
	{
		DEFINE_FIELD(CEagle, m_bSpotVisible, FIELD_BOOLEAN),
		DEFINE_FIELD(CEagle, m_bLaserActive, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CEagle, CEagle::BaseClass);
#endif

LINK_ENTITY_TO_CLASS(weapon_eagle, CEagle);

void CEagle::Precache()
{
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_desert_eagle.mdl");
	m_iShell = PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND("weapons/desert_eagle_reload.wav");
	PRECACHE_SOUND("weapons/desert_eagle_sight.wav");
	PRECACHE_SOUND("weapons/desert_eagle_sight2.wav");
}

void CEagle::Spawn()
{
	pev->classname = MAKE_STRING("weapon_eagle");
	Precache();
	m_iId = WEAPON_EAGLE;
	SET_MODEL(edict(), "models/w_desert_eagle.mdl");
	m_iDefaultAmmo = 10;
	FallInit();
}

bool CEagle::Deploy()
{
	m_bSpotVisible = true;

	return DefaultDeploy(
		"models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl",
		EAGLE_DRAW,
		"onehanded");
}

void CEagle::Holster()
{
	m_fInReload = false;

#ifndef CLIENT_DLL
	if (m_pLaser)
	{
		m_pLaser->Killed(nullptr, GIB_NEVER);
		m_pLaser = nullptr;
		m_bSpotVisible = false;
	}
#endif

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10.0, 15.0);

	SendWeaponAnim(EAGLE_HOLSTER);
}

void CEagle::WeaponIdle()
{
#ifndef CLIENT_DLL
	UpdateLaser();
#endif

	ResetEmptySound();

	// Update autoaim
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && 0 != m_iClip)
	{
		const float flNextIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		int iAnim;

		if (m_bLaserActive)
		{
			if (flNextIdle > 0.5)
			{
				iAnim = EAGLE_IDLE5;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
			}
			else
			{
				iAnim = EAGLE_IDLE4;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
			}
		}
		else
		{
			if (flNextIdle <= 0.3)
			{
				iAnim = EAGLE_IDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
			}
			else
			{
				if (flNextIdle > 0.6)
				{
					iAnim = EAGLE_IDLE3;
					m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.633;
				}
				else
				{
					iAnim = EAGLE_IDLE2;
					m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
				}
			}
		}

		SendWeaponAnim(iAnim);
	}
}

void CEagle::PrimaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0)
		return;
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();

		// Note: this is broken in original Op4 since it uses gpGlobals->time when using prediction
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fInReload)
		{
			if (m_fFireOnEmpty)
			{
				PlayEmptySound();
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
			}
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = 2048;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_flTimeSincePrimary = gpGlobals->time;

	--m_iClip;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	if (m_pLaser && m_bLaserActive)
	{
		m_pLaser->pev->effects |= EF_NODRAW;
		m_pLaser->SetThink(&CEagleLaser::Revive);
		m_pLaser->pev->nextthink = gpGlobals->time + 0.6;
	}
#endif

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();

	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	const float flSpread = m_bLaserActive ? 0.001 : 0.1;
	
	#ifndef CLIENT_DLL
	CPhysbullet::BulletCreate(1, 200, 7500, vecSrc, vecAiming, flSpread, flSpread, 0.8, 420, m_pPlayer->edict());
	CBasePlayerWeapon::Recoil(5, 10);
	#endif

	SendWeaponAnim(m_iClip == 0 ? EAGLE_SHOOT_EMPTY : EAGLE_SHOOT);
	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", 1, ATTN_NORM);


	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 15;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 6, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + (m_bLaserActive ? 0.5 : 0.05);

	if (0 == m_iClip)
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK);
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10.0, 15.0);

#ifndef CLIENT_DLL
	UpdateLaser();
#endif
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector addedvel = -gpGlobals->v_forward * 200;
	m_pPlayer->pev->velocity.x += addedvel.x;
	m_pPlayer->pev->velocity.y += addedvel.y;
	m_pPlayer->pev->velocity.z += addedvel.z;
}

void CEagle::SecondaryAttack()
{
#ifndef CLIENT_DLL
	m_bLaserActive = !m_bLaserActive;

	if (!m_bLaserActive)
	{
		if (m_pLaser)
		{
			m_pLaser->Killed(nullptr, GIB_NEVER);

			m_pLaser = nullptr;

			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/desert_eagle_sight2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		}
	}
#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CEagle::Reload()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		const bool bResult = DefaultReload(10, 0 != m_iClip ? EAGLE_RELOAD : EAGLE_RELOAD_NOSHOT, 1.5);

#ifndef CLIENT_DLL
		// Only turn it off if we're actually reloading
		if (bResult && m_pLaser && m_bLaserActive) // Should the laser be ported to the client so that it follows the laser instead? (idk if the attachments work with angles though)
		{
			m_pLaser->pev->effects |= EF_NODRAW;
			m_pLaser->SetThink(&CEagleLaser::Revive);
			m_pLaser->pev->nextthink = gpGlobals->time + 1.6;

			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
		}
#endif

		if (bResult)
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10.0, 15.0);
		}
	}
}

void CEagle::UpdateLaser()
{
#ifndef CLIENT_DLL
	// Don't turn on the laser if we're in the middle of a reload.
	if (m_fInReload)
	{
		return;
	}

	if (m_bLaserActive && m_bSpotVisible)
	{
		if (!m_pLaser)
		{
			m_pLaser = CEagleLaser::CreateSpot();

			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/desert_eagle_sight.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		}

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);

		Vector vecSrc = m_pPlayer->GetGunPosition();

		Vector vecEnd = vecSrc + gpGlobals->v_forward * 8192.0;

		TraceResult tr;

		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr);

		UTIL_SetOrigin(m_pLaser->pev, tr.vecEndPos);
	}
#endif
}

int CEagle::iItemSlot()
{
	return 2;
}

bool CEagle::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = 10;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_EAGLE;
	p->iWeight = PYTHON_WEIGHT;
	return true;
}

void CEagle::GetWeaponData(weapon_data_t& data)
{
	BaseClass::GetWeaponData(data);

	data.iuser1 = static_cast<int>(m_bLaserActive);
}

void CEagle::SetWeaponData(const weapon_data_t& data)
{
	BaseClass::SetWeaponData(data);

	m_bLaserActive = data.iuser1 != 0;
}

void CEagle::ReloadSetAmmos()
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
class CEagleAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		// TODO: could probably use a better model
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
		if (pOther->GiveAmmo(7, "357", _357_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(ammo_eagleclip, CEagleAmmo);