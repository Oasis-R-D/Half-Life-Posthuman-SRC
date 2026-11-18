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


#define CROWBAR_BODYHIT_VOLUME 128
#define CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS(weapon_l4dtest, CSwingMelee);

void CSwingMelee::Spawn()
{
	Precache();
	m_iId = WEAPON_L4D;
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit(); // get ready to fall down.
}


void CSwingMelee::Precache()
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

	//m_usCrowbar = PRECACHE_EVENT(1, "events/crowbar.sc");
}

bool CSwingMelee::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_L4D;
	p->iWeight = CROWBAR_WEIGHT;
	return true;
}



bool CSwingMelee::Deploy()
{
	EndSwing();

	m_iCrossHairType = CROSSHAIR_NONE;
	return DefaultDeploy("models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar");
}

void CSwingMelee::Holster()
{
	EndSwing();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(CROWBAR_HOLSTER);
}

void CSwingMelee::PrimaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0)
		return;
	m_flNextPrimaryAttack = GetNextAttackDelay(0.33);
	Vector2D start, end;
	
	switch (RANDOM_LONG(0, 3))
	{
	case 0: 
		start = Vector2D(-45, -25);
		end   = Vector2D(45, 25);
		break;
	case 1: 
		start = Vector2D(35, 35);
		end   = Vector2D(-35, -35);
		break;
	case 2: 
		start = Vector2D(-35, 0);
		end   = Vector2D(35, 0);
		break;
	case 3: 
		start = Vector2D(35, 0);
		end   = Vector2D(-35, 0);
		break;
	}

	StartSwing(0.25, 0, 12, start, end);
}