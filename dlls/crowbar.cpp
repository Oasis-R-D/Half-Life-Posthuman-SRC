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
	//UTIL_PrecacheOther("env_warpball");
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
	if (pev->weapons == 0)
	{
		pev->weapons = 1;
		return DefaultDeploy("models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_FIRSTDRAW, "crowbar");
	}
	return DefaultDeploy("models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar");
}

void CCrowbar::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(CROWBAR_HOLSTER);
}


void FindHullIntersection(const Vector& vecSrc, TraceResult& tr, const Vector& mins, const Vector& maxs, edict_t* pEntity)
{
	int i, j, k;
	float distance;
	const Vector* minmaxs[2] = {&mins, &maxs};
	TraceResult tmpTrace;
	Vector vecHullEnd = tr.vecEndPos;
	Vector vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine(vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace);
	if (tmpTrace.flFraction < 1.0)
	{
		tr = tmpTrace;
		return;
	}

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			for (k = 0; k < 2; k++)
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i]->x;
				vecEnd.y = vecHullEnd.y + minmaxs[j]->y;
				vecEnd.z = vecHullEnd.z + minmaxs[k]->z;

				UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace);
				if (tmpTrace.flFraction < 1.0)
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if (thisDistance < distance)
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


void CCrowbar::PrimaryAttack()
{
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = 1.18;
	pev->armortype = gpGlobals->time + 0.31;
	pev->armorvalue = gpGlobals->time + 0.71;
	SendWeaponAnim(CROWBAR_ATTACK1);
	EMIT_SOUND(m_pPlayer->edict(), CHAN_VOICE, RANDOM_LONG(0, 1) ? "zombie/zo_attack1.wav" : "zombie/zo_attack2.wav", 1, ATTN_NORM);
}

void CCrowbar::SecondaryAttack()
{
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = 1;
	pev->health = gpGlobals->time + 0.34;
	SendWeaponAnim(CROWBAR_ATTACK2);
	EMIT_SOUND(m_pPlayer->edict(), CHAN_VOICE, RANDOM_LONG(0, 1) ? "zombie/zo_attack1.wav" : "zombie/zo_attack2.wav", 1, ATTN_NORM);
}

void CCrowbar::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}


void CCrowbar::SwingAgain()
{
	Swing(false);
}


bool CCrowbar::Swing(bool fFirst)
{
	bool fDidHit = false;

	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			vecEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if (fFirst)
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usCrowbar,
			0.0, g_vecZero, g_vecZero, 0, 0, 0,
			0.0, 0, 0.0);
	}


	if (tr.flFraction >= 1.0)
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = 2.4;
			pev->armortype = gpGlobals->time + 0.67;
			pev->armorvalue = gpGlobals->time + 1.267;

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		SendWeaponAnim(CROWBAR_ATTACK1);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = true;
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_pGameRules->IsMultiplayer())
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar / 2, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

#endif
		
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = 2.4;
		pev->armortype = gpGlobals->time + 0.67;
		pev->armorvalue = gpGlobals->time + 1.267;

#ifndef CLIENT_DLL
		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool fHitWorld = true;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch (RANDOM_LONG(0, 2))
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM);
					break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM);
					break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM);
					break;
				}
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return true;
				else
					flVol = 0.1;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		SetThink(&CCrowbar::Smack);
		pev->nextthink = gpGlobals->time + 0.2;
	}
	return fDidHit;
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
		m_pPlayer->CrowbarFlinch = 0;
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
		if (false)
			pHit->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB);
		else
			pHit->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar * 2, gpGlobals->v_forward, &tr, DMG_CLUB);
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

		if (pHit->pev->deadflag != DEAD_NO)
		{
			if (pHit->Classify() == CLASS_ALIEN_MONSTER || pHit->Classify() == CLASS_ALIEN_MILITARY ||
				pHit->Classify() == CLASS_HUMAN_MILITARY || pHit->Classify() == CLASS_HUMAN_PASSIVE || pHit->Classify() == CLASS_PLAYER )
			{
				pHit->Killed(pev, GIB_ALWAYS);

				if (FClassnameIs(pev, "monster_headcrab_super"))
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
					m_pPlayer->Hunger += 10;
					m_pPlayer->TakeHealth(5, DMG_GENERIC);
				}

				if (m_pPlayer->Hunger > 100)
					m_pPlayer->Hunger = 100;
			MESSAGE_BEGIN(MSG_ONE, gmsgDamageLIMB, NULL, m_pPlayer->pev);
			WRITE_BYTE(m_pPlayer->health_head);
			WRITE_BYTE(m_pPlayer->health_chest);
			WRITE_BYTE(m_pPlayer->health_stomach);
			WRITE_BYTE(m_pPlayer->health_armL);
			WRITE_BYTE(m_pPlayer->health_armR);
			WRITE_BYTE(m_pPlayer->health_legL);
			WRITE_BYTE(m_pPlayer->health_legR);
			MESSAGE_END();
			}
		}

#ifndef CLIENT_DLL
		sound = UTIL_VarArgs("zombie/claw_strike%d.wav", RANDOM_LONG(1, 3));
#endif
		DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);
	}

	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, sound, 1, ATTN_NORM);
	m_pPlayer->pev->punchangle.z = RANDOM_LONG(-5, 5);
}

void CCrowbar::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(CROWBAR_IDLE);
	m_flTimeWeaponIdle = 3.8;
}