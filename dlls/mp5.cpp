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

LINK_ENTITY_TO_CLASS(weapon_mp5, CMP5);
LINK_ENTITY_TO_CLASS(weapon_9mmAR, CMP5);


//=========================================================
//=========================================================
void CMP5::Spawn()
{
	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = 30;

	FallInit(); // get ready to fall down.
}


void CMP5::Precache()
{
	PRECACHE_MODEL("models/v_9mmAR.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");
	PRECACHE_MODEL("models/grenade.mdl"); // grenade
	PRECACHE_MODEL("models/w_9mmARclip.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shellTE_MODEL

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");
	PRECACHE_SOUND("weapons/hks1.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks2.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks3.wav"); // H to the K
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");
}

bool CMP5::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = 30;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = MP5_WEIGHT;

	return true;
}

bool CMP5::Deploy()
{
	pev->armorvalue = 0;
	if (pev->armortype == 0)
		pev->armortype = 1;
	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(pev->armortype);
	MESSAGE_END();
	if (NotFirstDraw)
	{
		if (pev->weapons == 0)
			return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DRAW, "mp5");
		return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DRAW_M203, "mp5");
	}
	return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DRAW_FIRST, "mp5");
}

void CMP5::Holster()
{
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(0);
	MESSAGE_END();
}

void CMP5::PrimaryAttack()
{
	if (pev->weapons == 1)
	{
		// don't fire underwater
		if (m_pPlayer->pev->waterlevel == 3)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
			return;
		}

		if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
		{
			PlayEmptySound();
			return;
		}

		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
		m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;

		m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

		// we don't add in player velocity anymore.
		CGrenade::ShootContact(m_pPlayer->pev,
			m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
			gpGlobals->v_forward * 1000);

		SendWeaponAnim(MP5_SHOOT_M203);

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 2.3;
		m_flTimeWeaponIdle = 0.57;
		m_flNextTertiaryAttack = gpGlobals->time + 2.3;

		if (0 == m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]) // HEV suit - indicate out of ammo condition
		{
			m_pPlayer->SetSuitUpdate("!HEV_GOUT", false, 0);
			pev->frags = 2;
		}
		else
			pev->frags = 1;

		return;
	}

	if (m_pPlayer->pev->waterlevel == 3 || m_iClip <= 0) // don't fire underwater
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1); // player "shoot" animation

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	m_pPlayer->FireBullets(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_MP5, 1);
	SendWeaponAnim(RANDOM_LONG(MP5_SHOOT1, MP5_SHOOT3));
	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM);

	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 20 + gpGlobals->v_right * 4, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	if (pev->armortype == 2)
	{
		if (pev->armorvalue < 2)
		{
			pev->armorvalue++;
			m_flNextPrimaryAttack = 0.066;
		}
		else
		{
			pev->armorvalue = 0;
			m_flNextPrimaryAttack = 0.25;
		}
	}
	else
		m_flNextPrimaryAttack = 0.066;

	m_flTimeWeaponIdle = 5;
	m_pPlayer->pev->punchangle.x -= 1;
	m_pPlayer->pev->punchangle.y += RANDOM_FLOAT(-1, 1);
}

void CMP5::SecondaryAttack()
{
	if (pev->weapons == 0)
	{
		if (0 == m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
			m_pPlayer->SetSuitUpdate("!HEV_GOUT", false, 0);
		else
		{
			pev->weapons = 1;
			SendWeaponAnim(MP5_IDLE_TO_M203);
			MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
			WRITE_SHORT(0);
			MESSAGE_END();
		}
	}
	else
	{
		pev->weapons = 0;
		SendWeaponAnim(MP5_M203_TO_IDLE);
		MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
		WRITE_SHORT(pev->armortype);
		MESSAGE_END();
	}
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = 0.37;
	m_flNextTertiaryAttack = gpGlobals->time + 0.37;
}

void CMP5::TertiaryAttack()
{
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.5;
	m_flNextTertiaryAttack = gpGlobals->time + 0.5;

	if (pev->armortype == 1)
	{
		ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "Switched to Burst-Fire Mode");
		pev->armortype = 2;
	}
	else if (pev->armortype == 2)
	{
		ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "Switched to Full-Auto Mode");
		pev->armortype = 1;
	}
	EMIT_SOUND(edict(), CHAN_ITEM, "items/9mmclip2.wav", 1, ATTN_NORM);

	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(pev->armortype);
	MESSAGE_END();
}

void CMP5::Reload()
{
	if (pev->weapons == 1)
		return;

	if (m_iClip == 0)
		DefaultReload(30, MP5_RELOAD_EMPTY, 3);
	else
		DefaultReload(30, MP5_RELOAD_TACTICAL, 2);

	pev->armorvalue = 0;
}

void CMP5::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	NotFirstDraw = true;

	if (pev->weapons == 0)
	{
		if (RANDOM_LONG(0, 1))
			SendWeaponAnim(MP5_IDLE1), m_flTimeWeaponIdle = 5.37;
		else
			SendWeaponAnim(MP5_IDLE2), m_flTimeWeaponIdle = 4.5;
	}
	else
	{
		if (pev->frags == 2)
		{
			if (0 == m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
				SecondaryAttack();
			else
				pev->frags = 1;
		}
		else if (pev->frags == 0)
			SendWeaponAnim(MP5_IDLE_M203), m_flTimeWeaponIdle = 1.63;
		else
		{
			SendWeaponAnim(MP5_RELOAD_M203), m_flTimeWeaponIdle = 1.73;
			pev->frags = 0;
		}
	}
}

class CMP5AmmoClip : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmARclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(30, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5clip, CMP5AmmoClip);
LINK_ENTITY_TO_CLASS(ammo_9mmAR, CMP5AmmoClip);



class CMP5Chainammo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_chainammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_CHAINBOX_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_9mmbox, CMP5Chainammo);


class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_ARgrenade.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_M203BOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5grenades, CMP5AmmoGrenade);
LINK_ENTITY_TO_CLASS(ammo_ARgrenades, CMP5AmmoGrenade);

//=========================================================
// M727
//=========================================================

LINK_ENTITY_TO_CLASS(weapon_m727, CM727);

void CM727::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_727.mdl"); //to-do: get a accurate m727 Wmodel
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

	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl"); // brass shellTE_MODEL

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
	p->iMaxAmmo1 = 200;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = 30;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M727;
	p->iWeight = MP5_WEIGHT;

	return true;
}

bool CM727::Deploy()
{
	pev->armorvalue = 0;
	if (pev->armortype == 0)
		pev->armortype = 1;
	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(pev->armortype);
	MESSAGE_END();
	if (NotFirstDraw)
	{
		if (pev->weapons == 0)
			return DefaultDeploy("models/v_727.mdl", "models/p_9mmAR.mdl", M727_DRAW, "mp5");
	}
	return DefaultDeploy("models/v_727.mdl", "models/p_9mmAR.mdl", M727_DRAW_FIRST, "mp5");
}

void CM727::Holster()
{
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgFireMode, NULL, m_pPlayer->pev);
	WRITE_SHORT(0);
	MESSAGE_END();
}

void CM727::PrimaryAttack()
{
	if (pev->weapons == 1)
	{
		// don't fire underwater
		if (m_pPlayer->pev->waterlevel == 3)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
			return;
		}

		if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
		{
			PlayEmptySound();
			return;
		}

		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
		m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;

		m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

		// we don't add in player velocity anymore.
		CGrenade::ShootContact(m_pPlayer->pev,
			m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
			gpGlobals->v_forward * 1000);

		SendWeaponAnim(MP5_SHOOT_M203);

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 2.3;
		m_flTimeWeaponIdle = 0.57;
		m_flNextTertiaryAttack = gpGlobals->time + 2.3;

		if (0 == m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]) // HEV suit - indicate out of ammo condition
		{
			m_pPlayer->SetSuitUpdate("!HEV_GOUT", false, 0);
			pev->frags = 2;
		}
		else
			pev->frags = 1;

		return;
	}

	if (m_pPlayer->pev->waterlevel == 3 || m_iClip <= 0) // don't fire underwater or if emptied
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1); // player "shoot" animation

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	m_pPlayer->FireBullets(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_M727, 1);
	SendWeaponAnim(RANDOM_LONG(RANDOM_LONG(M727_SHOOT1,M727_SHOOT2), M727_SHOOT3));
	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/727_hks1.wav", 1, ATTN_NORM);

	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(50, 70) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 9 + gpGlobals->v_right * 4, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	if (pev->armortype == 2)
	{
		if (pev->armorvalue < 2)
		{
			pev->armorvalue++;
			m_flNextPrimaryAttack = 0.10;
		}
		else
		{
			pev->armorvalue = 0;
			m_flNextPrimaryAttack = 0.25;
		}
	}
	else
		m_flNextPrimaryAttack = 0.0825;

	m_flTimeWeaponIdle = 5;
	m_pPlayer->pev->punchangle.x -= 1.125;
	m_pPlayer->pev->punchangle.y += RANDOM_FLOAT(-1.125, 1.125);
}

void CM727::SecondaryAttack()
{

}

void CM727::TertiaryAttack()
{
	
}

void CM727::Reload()
{
	if (pev->weapons == 1)
		return;

	if (m_iClip == 0)
		DefaultReload(30, M727_RELOAD_EMPTY, 2);
	else
		DefaultReload(30, M727_RELOAD_TACTICAL, 1);

	pev->armorvalue = 0;
}

void CM727::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	NotFirstDraw = true;

	if (pev->weapons == 0)
	{
		if (RANDOM_LONG(0, 1))
			SendWeaponAnim(M727_IDLE1), m_flTimeWeaponIdle = 2.5;
		else
			SendWeaponAnim(M727_IDLE2), m_flTimeWeaponIdle = 4;
	}
		
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
		bool bResult = (pOther->GiveAmmo(30, "556", 200) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_556mag, CM727AmmoClip);
