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
#include "physical_cryst.h"
#include "physical_bullet.h"

bool CCorruptedWPN::CanAttack(float attack_time, float curtime, bool isPredicted)
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

LINK_ENTITY_TO_CLASS(weapon_corrupted, CCorruptedWPN);

void CCorruptedWPN::Spawn()
{
	Precache();
	m_iId = WEAPON_CORRUPT;
	SET_MODEL(ENT(pev), "models/w_corruptWPN.mdl");

	m_iDefaultAmmo = CRYST_DEFAULT_GIVE;

	FallInit(); // get ready to fall
}


void CCorruptedWPN::Precache()
{
	PRECACHE_MODEL("models/v_corruptWPN.mdl");
	PRECACHE_MODEL("models/w_corruptWPN.mdl");
	PRECACHE_MODEL("models/p_corruptWPN.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/357_shot1.wav"); // python
	PRECACHE_SOUND("weapons/357_shot2.wav");

	PRECACHE_SOUND("weapons/sbarrel1.wav"); //shotgun

	PRECACHE_SOUND("weapons/saw_fire1.wav"); // M249
	PRECACHE_SOUND("weapons/saw_fire2.wav");
	PRECACHE_SOUND("weapons/saw_fire3.wav");

	PRECACHE_SOUND("weapons/727_hks1.wav"); // H to the K 727 times
	PRECACHE_SOUND("weapons/727_hks2.wav"); // H to the K 727 times
	PRECACHE_SOUND("weapons/727_hks3.wav"); // H to the K 727 times

	PRECACHE_SOUND("weapons/hks1.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks2.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks3.wav"); // H to the K

	PRECACHE_SOUND("weapons/pl_gun1.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun3.wav"); //handgun

	PRECACHE_SOUND("weapons/357_cock1.wav"); // gun empty sound

	m_stainevent = PRECACHE_EVENT(1, "events/bloodspray.sc");
	m_silenceevent = PRECACHE_EVENT(1, "events/glocksilence.sc"); // bodygroup change event (repurposed from glock)
}

bool CCorruptedWPN::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = 200;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 200;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CORRUPT;
	p->iWeight = CRYST_WEIGHT;

	return true;
}



bool CCorruptedWPN::Deploy()
{
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_silenceevent, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, m_iCurrWPN, 0, 0, 0);
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_stainevent, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, RANDOM_LONG(0, 1), 0, 0, 0); // TO-DO: update random long with current skin amnts
	return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW_SEMI, "shotgun");
}

void CCorruptedWPN::PrimaryAttack()
{
	bool m_bSemi = false;
	if ((m_iCurrWPN == 0) || (m_iCurrWPN == 2) || (m_iCurrWPN == 3))
		m_bSemi = true;
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0 && m_bSemi == true)
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	int blltamnt = 1;
	int iblltDMG = 2;
	int glocksound = RANDOM_LONG(0, 1);
	float spreadhorz;
	float spreadvert;
	char wpnsnd2[256];

	switch (m_iCurrWPN)
	{
		case 0: // glock
			iblltDMG = gSkillData.plrDmg9MM;
			m_flNextPrimaryAttack = 0.1;
			recoily = 2;
			recoilx = 2;
			spreadhorz = spreadvert = 0.01;
			
			sprintf(wpnsnd2, "weapons/pl_gun%d.wav", glocksound ? 1 : 3);
			break;
		case 1: // mp5
			iblltDMG = gSkillData.plrDmgMP5;
			m_flNextPrimaryAttack = 0.066;
			recoily = 1;
			recoilx = 1;
			spreadhorz = spreadvert = CONE_1DEGREES;
			sprintf(wpnsnd2, "weapons/hks%d.wav", RANDOM_LONG(1, 3));
			break;
		case 2: // python
			iblltDMG = gSkillData.plrDmg357;
			m_flNextPrimaryAttack = 0.125;
			recoily = 5;
			recoilx = 2;
			spreadhorz = spreadvert = CONE_1DEGREES;
			sprintf(wpnsnd2, "weapons/357_shot%d.wav", RANDOM_LONG(1, 2));
			break;
		case 3: // spas-12
			iblltDMG = gSkillData.plrDmgBuckshot;
			blltamnt = 9;
			m_flNextPrimaryAttack = 0.2;
			recoily = 4;
			recoilx = 2;
			if (g_iSkillLevel != SKILL_HARD)
			{
				spreadhorz = 0.17432;
				spreadvert = 0.01746;
			}
			else
			{
				spreadhorz = spreadvert = 0.013095;
			}
			sprintf(wpnsnd2, "weapons/sbarrel%d.wav", 1);
			break;
		case 4: // m727
			iblltDMG = gSkillData.plrDmgM727;
			m_flNextPrimaryAttack = 0.0727;
			recoily = 1;
			recoilx = 1;
			spreadhorz = spreadvert = CONE_1DEGREES;
			sprintf(wpnsnd2, "weapons/727_hks%d.wav", RANDOM_LONG(1, 3));
			break;
		case 5: // m249
			iblltDMG = gSkillData.plrDmgMP5;
			m_flNextPrimaryAttack = 0.06;
			//(0.4, 1.125)
			recoily = 0.4;
			recoilx = 1.125;
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
				spreadhorz = spreadvert = vecSpread;
			}
			sprintf(wpnsnd2, "weapons/saw_fire%d.wav", RANDOM_LONG(1, 3));
			break;
	}

	m_pPlayer->m_iWeaponVolume = RANDOM_LONG(600, 1000);
	m_pPlayer->m_iWeaponFlash = RANDOM_LONG(128, 512);

	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, wpnsnd2, RANDOM_FLOAT(0.75, 0.95), ATTN_GUN, 0, RANDOM_LONG(50, 250));

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);


	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	Vector vecDir;
	int ibllttype;
	switch (RANDOM_LONG(0, 5))
	{
		case 0:
			ibllttype = 9;
			break;
		case 1:
			ibllttype = 556;
			break;
		case 2:
			ibllttype = 762;
			break;
		case 3:
			ibllttype = 12;
			break;
		case 4:
			ibllttype = 357;
			break;
		case 5:
			ibllttype = 69; //rbbr
			break;
	}
	#ifndef CLIENT_DLL
		CPhysbullet::BulletCreate(blltamnt, iblltDMG, RANDOM_LONG(5750, 7500), vecSrc, vecAiming, spreadhorz, spreadvert, RANDOM_FLOAT(0.60, 1), ibllttype, m_pPlayer->edict()); // dmg set to 2 since corruption enemies don't use normal health
	#endif

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);


	m_flTimeWeaponIdle = 1;
#ifndef CLIENT_DLL
		CBasePlayerWeapon::Recoil(recoily, recoilx);
#endif


}

void CCorruptedWPN::TertiaryAttack()
{

}


void CCorruptedWPN::WeaponIdle()
{

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && 0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			
			Reload();

		}
		else
		{
			switch (RANDOM_LONG(1, 3))
			{
			case 1: SendWeaponAnim(SHOTGUN_IDLE1_SEMI), m_flTimeWeaponIdle = 1.93; break;
			case 2: SendWeaponAnim(SHOTGUN_IDLE2_SEMI), m_flTimeWeaponIdle = 3.43; break;
			case 3: SendWeaponAnim(SHOTGUN_IDLE3_SEMI), m_flTimeWeaponIdle = 3.23; break;
			}
		}
	}
}

void CCorruptedWPN::ItemPostFrame()
{
	int iReloadAMNT;

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_stainevent, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, m_corrskin, 0, 0, 0); // repurposed for skin changing

	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase()))
	{	
		m_iCurrWPN += RANDOM_LONG(1, 4);
		if (m_iCurrWPN == 9)	  // 4 over
			m_iCurrWPN = 3;
		else if (m_iCurrWPN == 8) // 3 over
			m_iCurrWPN = 2;
		else if (m_iCurrWPN == 7) // 2 over
			m_iCurrWPN = 1;
		else if (m_iCurrWPN == 6) // 1 over
			m_iCurrWPN = 0;
		
		switch (m_iCurrWPN)
		{
			case 0: // glock
				iReloadAMNT = 17;
				break;
			case 1: // mp5
				iReloadAMNT = 30;
				break;
			case 2: // python
				iReloadAMNT = 6;
				break;
			case 3: // spas-12
				iReloadAMNT = 9;
				break;
			case 4: // m727
				iReloadAMNT = 30;
				break;
			case 5: // m249
				iReloadAMNT = 50;
				break;
		}
		// Add them to the clip
		m_iClip = iReloadAMNT;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= iReloadAMNT;

		m_pPlayer->TabulateAmmo();

		m_fInReload = false;
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
		//m_pPlayer->pev->button &= ~IN_ALT1;
	}
	else if ((m_pPlayer->pev->button & IN_SCORE) != 0 && m_flNextGrenadeAttack < gpGlobals->time && m_igrenadeAMNT > 0)
	{
		m_igrenadeAMNT--;
		m_flNextGrenadeAttack = gpGlobals->time + 2.5;
		GrenadeAttack();
		if (m_igrenadeAMNT == 0)
			m_pPlayer->SetSuitUpdate("!HEV_GOUT", false, 0);

		m_pPlayer->pev->button &= ~IN_SCORE;
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

void CCorruptedWPN::Reload()
{
	int iReloadAMNT;
	switch (m_iCurrWPN)
	{
		case 0: // glock
			iReloadAMNT = 17;
			break;
		case 1: // mp5
			iReloadAMNT = 30;
			break;
		case 2: // python
			iReloadAMNT = 6;
			break;
		case 3: // spas-12
			iReloadAMNT = 9;
			break;
		case 4: // m727
			iReloadAMNT = 30;
			break;
		case 5: // m249
			iReloadAMNT = 50;
			break;
	}
	DefaultReload(iReloadAMNT, M727_RELOAD_EMPTY, 1.8);
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_silenceevent, 0.5, g_vecZero, g_vecZero, 0.0, 0.0, m_iCurrWPN, 0, 0, 0);
}

