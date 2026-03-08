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

#define CROWBAR_BODYHIT_VOLUME 128
#define CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS(weapon_crowbar, CCrowbar);
LINK_ENTITY_TO_CLASS(weapon_claws, CCrowbar);

void CCrowbar::Spawn()
{
	Precache();
	m_iId = WEAPON_CROWBAR;
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit(); // get ready to fall down.
}


void CCrowbar::Precache()
{
	PRECACHE_MODEL("models/v_crowbar.mdl");
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");

	m_usCrowbar = PRECACHE_EVENT(1, "events/crowbar.sc");

	UTIL_PrecacheOther("monster_zombie");
}

bool CCrowbar::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_CROWBAR;
	p->iWeight = CROWBAR_WEIGHT;
	return true;
}



bool CCrowbar::Deploy()
{
	m_pPlayer->CrowbarFlinch = 0;
	if (pev->weapons == 0)
	{
		pev->weapons = 1;
		return DefaultDeploy("models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_FIRSTDRAW, "crowbar");
	}
	return DefaultDeploy("models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar");
}

void CCrowbar::Holster()
{
	m_pPlayer->CrowbarFlinch = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(CROWBAR_HOLSTER);
}

void CCrowbar::PrimaryAttack()
{
	m_pPlayer->CrowbarFlinch = 0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = 1.18;
	pev->armortype = gpGlobals->time + 0.31;
	pev->armorvalue = gpGlobals->time + 0.71;
	SendWeaponAnim(CROWBAR_ATTACK1);
	EMIT_SOUND(m_pPlayer->edict(), CHAN_VOICE, RANDOM_LONG(0, 1) ? "zombie/zo_attack1.wav" : "zombie/zo_attack2.wav", 1, ATTN_NORM);
}

void CCrowbar::SecondaryAttack()
{
	m_pPlayer->CrowbarFlinch = 0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = 1.66;
	pev->health = gpGlobals->time + 0.34;
	SendWeaponAnim(CROWBAR_ATTACK2);
	EMIT_SOUND(m_pPlayer->edict(), CHAN_VOICE, RANDOM_LONG(0, 1) ? "zombie/zo_attack1.wav" : "zombie/zo_attack2.wav", 1, ATTN_NORM);
	#ifndef CLIENT_DLL
	CBasePlayerWeapon::Recoil(2, 0);
	#endif
}

void CCrowbar::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}

void CCrowbar::ItemPostFrame()
{
	if (pev->armortype < gpGlobals->time && pev->armortype != 0)
	{
		Hit(false);
		pev->armortype = 0;
	}
	if (pev->armorvalue < gpGlobals->time && pev->armorvalue != 0)
	{
		Hit(false);
		pev->armorvalue = 0;
	}
	if (pev->health < gpGlobals->time && pev->health != 0)
	{
		Hit(true);
		pev->health = 0;
	}

	if (m_pPlayer->CrowbarFlinch > 0 && pev->armortype == 0 && pev->armorvalue == 0 && pev->health == 0)
	{
		m_pPlayer->CrowbarFlinch = 0;
		if (m_pPlayer->CrowbarFlinch == 1)
		{
			SendWeaponAnim(CROWBAR_FLINCH_SMALL);
			m_flTimeWeaponIdle = 0.3;
		}
		if (m_pPlayer->CrowbarFlinch == 2)
		{
			SendWeaponAnim(CROWBAR_FLINCH_BIG);
			m_flTimeWeaponIdle = 0.5;
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CCrowbar::Hit(bool type)
{
	TraceResult tr;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	UTIL_TraceLine(vecSrc, vecSrc + gpGlobals->v_forward * 96, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);
	
	const char* sound = 0;

	if (tr.flFraction == 1)
	{
#ifndef CLIENT_DLL
		sound = UTIL_VarArgs("zombie/claw_miss%d.wav", RANDOM_LONG(1, 2));
#endif
	}
	else
	{
		auto pHit = CBaseEntity::Instance(tr.pHit);
		ClearMultiDamage();
		if (type == false)
		{
			pHit->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB);
			#ifndef CLIENT_DLL
			CBasePlayerWeapon::Recoil(0, 2); // TO-DO: make it go the direction of the hand
			#endif
		}
		else
		{
			pHit->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar * 2, gpGlobals->v_forward, &tr, DMG_CLUB);
			#ifndef CLIENT_DLL
			CBasePlayerWeapon::Recoil(1, 0); // TO-DO: make it go the direction of the hand
			#endif
		}
		ApplyMultiDamage(pev, m_pPlayer->pev);

		if (pHit->pev->deadflag != DEAD_NO)
		{
			if (pHit->Classify() == CLASS_ALIEN_MONSTER || pHit->Classify() == CLASS_ALIEN_MILITARY ||
				pHit->Classify() == CLASS_HUMAN_MILITARY || pHit->Classify() == CLASS_HUMAN_PASSIVE || pHit->Classify() == CLASS_PLAYER )
			{
				pHit->Killed(m_pPlayer->pev, GIB_ALWAYS);

				if (FClassnameIs(pHit->pev, "monster_headcrab_super"))
				{
					m_pPlayer->Hunger += 5;
					m_pPlayer->TakeHealth(20, DMG_GENERIC);
					m_pPlayer->health_armR += (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
					m_pPlayer->health_armL += (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
					m_pPlayer->health_legL -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
					m_pPlayer->health_legR -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
					m_pPlayer->health_head -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
					m_pPlayer->health_chest -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
					m_pPlayer->health_stomach -= (gSkillData.healthkitCapacity + (RANDOM_LONG(3, 10)));
					if (m_pPlayer->health_armR > 100)
						m_pPlayer->health_armR = 100;
					if (m_pPlayer->health_armL > 100)
						m_pPlayer->health_armL = 100;
					if (m_pPlayer->health_legL < 0)
						m_pPlayer->health_legL = 0;
					if (m_pPlayer->health_legR < 0)
						m_pPlayer->health_legR = 0;
					if (m_pPlayer->health_head < 0)
						m_pPlayer->health_head = 0;
					if (m_pPlayer->health_chest < 0)
						m_pPlayer->health_chest = 0;
					if (m_pPlayer->health_stomach < 0)
						m_pPlayer->health_stomach = 0;
				}
				else if (pHit->BloodColor() == BLOOD_COLOR_RED)
				{
					m_pPlayer->Hunger += 30;
					m_pPlayer->TakeHealth(gSkillData.healthkitCapacity, DMG_GENERIC);
				}
				else if (pHit->BloodColor() != DONT_BLEED)
				{
					m_pPlayer->Hunger += 15;
					m_pPlayer->TakeHealth(5, DMG_GENERIC);
				}

				if (m_pPlayer->Hunger > 100)
					m_pPlayer->Hunger = 100;
			}
		}

#ifndef CLIENT_DLL
		sound = UTIL_VarArgs("zombie/claw_strike%d.wav", RANDOM_LONG(1, 3));
#endif
		DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);
	}

	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, sound, 1, ATTN_NORM);
}

void CCrowbar::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(CROWBAR_IDLE);
	m_flTimeWeaponIdle = 3.8;
}