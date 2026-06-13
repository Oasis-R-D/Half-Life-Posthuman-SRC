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

// TO-DO: move to weapons folder

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"


#define CROWBAR_BODYHIT_VOLUME 128
#define CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS(weapon_sledge, CSledgeHammer);

void CSledgeHammer::Spawn()
{
	Precache();
	m_iId = WEAPON_SLEDGE;
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = WEAPON_NOCLIP;
	m_iSwingMode = SWING_NONE;
	FallInit(); // get ready to fall down.
}


void CSledgeHammer::Precache()
{
	PRECACHE_MODEL("models/v_melee.mdl");
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/cbar_hhit.wav");
	PRECACHE_SOUND("weapons/cbar_hhitbod.wav");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");

	m_usMelee = PRECACHE_EVENT(1, "events/melee.sc");
}

bool CSledgeHammer::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_SLEDGE;
	p->iWeight = CROWBAR_WEIGHT;
	return true;
}

bool CSledgeHammer::Deploy()
{
	m_iCrossHairType = CROSSHAIR_NONE;
	return DefaultDeploy("models/v_melee.mdl", "models/p_crowbar.mdl", MELEE_DRAW, "crowbar");
}

void CSledgeHammer::Holster()
{
	//Cancel any swing in progress.
	m_iSwingMode = SWING_NONE;
	SetThink(nullptr);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(MELEE_HOLSTER);
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


void CSledgeHammer::PrimaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) != 0)
		return;
	if (m_iSwingMode == SWING_NONE && !Swing(true))
	{
		SwingAgain();
	}
}

void CSledgeHammer::SecondaryAttack()
{
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK2) != 0)
		return;
	if (m_iSwingMode == SWING_NONE && !SwingHeavy(true))
	{
		SwingAgainHeavy();
	}
}

void CSledgeHammer::TertiaryAttack()
{
	if (m_iSwingMode != SWING_START_BIG)
	{
		SendWeaponAnim(MELEE_CHARGE);
		m_flBigSwingStart = gpGlobals->time;
	}

	m_iSwingMode = SWING_START_BIG;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.1);
	m_flNextTertiaryAttack = gpGlobals->time + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2;
}

void CSledgeHammer::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}

void CSledgeHammer::SmackHeavy()
{
	DecalGunshot(&m_trHit, BULLET_MONSTER_12MM);
}


void CSledgeHammer::SwingAgain()
{
	Swing(false);
}

void CSledgeHammer::SwingAgainHeavy()
{
	SwingHeavy(false);
}


bool CSledgeHammer::Swing(const bool bFirst)
{
	bool bDidHit = false;

	TraceResult tr;
	#ifndef CLIENT_DLL
	CBasePlayerWeapon::Recoil(RANDOM_LONG(1, -1), 0.9);
	#endif
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

	if (bFirst)
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usMelee,
			0.0, g_vecZero, g_vecZero, 0, 0, 0,
			0, 0, static_cast<int>(tr.flFraction < 1));
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	}

	

	if (tr.flFraction >= 1.0) // miss
	{
		if (bFirst)
		{
			m_flNextPrimaryAttack = GetNextAttackDelay(0.75f);
			m_flNextSecondaryAttack = GetNextAttackDelay(0.75f);
			m_flNextTertiaryAttack = gpGlobals->time + 1.25;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75f;
		}
	}
	else // hit
	{
#ifndef CLIENT_DLL
		bDidHit = true;
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity)
		{
			ClearMultiDamage();

			if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_pGameRules->IsMultiplayer())
			{
				// first swing does full damage
				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgSledge, gpGlobals->v_forward, &tr, DMG_CLUB);
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgSledge / 1.5, gpGlobals->v_forward, &tr, DMG_CLUB);
			}

			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}

#endif

		m_flNextPrimaryAttack = GetNextAttackDelay(0.5f);
		m_flNextSecondaryAttack = GetNextAttackDelay(0.5f);
		m_flNextTertiaryAttack = gpGlobals->time + 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;

#ifndef CLIENT_DLL

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool bHitWorld = true;

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

				bHitWorld = false;
			}
		}

		// play texture hit sound
		if (bHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlayMatSound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

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

		SetThink(&CSledgeHammer::Smack);
		pev->nextthink = gpGlobals->time + 0.1;
#endif
	}
	return bDidHit;
}

bool CSledgeHammer::SwingHeavy(const bool bFirst)
{
	bool bDidHit = false;

	TraceResult tr;
	#ifndef CLIENT_DLL
	CBasePlayerWeapon::Recoil(-2, 1.5);
	#endif
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

	if (bFirst)
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usMelee,
			0.0, g_vecZero, g_vecZero, 0, 0, 0,
			2, 0, static_cast<int>(tr.flFraction < 1));
			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	}


	if (tr.flFraction >= 1.0) // miss
	{
		if (bFirst)
		{
			m_flNextPrimaryAttack = GetNextAttackDelay(1.5f);
			m_flNextSecondaryAttack = GetNextAttackDelay(1.5f);
			m_flNextTertiaryAttack = gpGlobals->time + 2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
		}
	}
	else // hit
	{
#ifndef CLIENT_DLL
		bDidHit = true;
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity)
		{
			ClearMultiDamage();

			if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_pGameRules->IsMultiplayer())
			{
				// first swing does full damage
				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgSledge * 1.5, gpGlobals->v_forward, &tr, DMG_CLUB);
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack(m_pPlayer->pev, (gSkillData.plrDmgSledge / 1.5) * 1.5, gpGlobals->v_forward, &tr, DMG_CLUB);
			}

			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}

#endif

		m_flNextPrimaryAttack = GetNextAttackDelay(1.25f);
		m_flNextSecondaryAttack = GetNextAttackDelay(1.25f);
		m_flNextTertiaryAttack = gpGlobals->time + 1.75f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.25f;

#ifndef CLIENT_DLL

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool bHitWorld = true;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hhitbod.wav", 1, ATTN_NORM);

				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return true;
				else
					flVol = 0.1;

				bHitWorld = false;
			}
		}

		// play texture hit sound
		if (bHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlayMatSound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hhit.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;

		SetThink(&CSledgeHammer::SmackHeavy);
		pev->nextthink = gpGlobals->time + 0.12;
#endif
	}
	return bDidHit;
}

void CSledgeHammer::BigSwing()
{
	TraceResult tr;
	#ifndef CLIENT_DLL
	CBasePlayerWeapon::Recoil(-4, 0.7);
	#endif
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

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usMelee,
		0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0,
		1, 0, static_cast<int>(tr.flFraction < 1));

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	if (tr.flFraction >= 1.0) // miss
	{
		
		m_flNextPrimaryAttack = GetNextAttackDelay(1.75f);
		m_flNextSecondaryAttack = GetNextAttackDelay(1.75f);
		m_flNextTertiaryAttack = gpGlobals->time + 2.25f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.75f;
	}
	else // hit
	{
#ifndef CLIENT_DLL
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity)
		{
			ClearMultiDamage();

			float flDamage = (gpGlobals->time - m_flBigSwingStart) * gSkillData.plrDmgSledge + 25.0f;
			if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_pGameRules->IsMultiplayer())
			{
				// first swing does full damage
				pEntity->TraceAttack(m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB);
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack(m_pPlayer->pev, flDamage / 2, gpGlobals->v_forward, &tr, DMG_CLUB);
			}
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool bHitWorld = true;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hhitbod.wav", 1, ATTN_NORM);

				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return;
				else
					flVol = 0.1;

				bHitWorld = false;
			}
		}

		// play texture hit sound
		if (bHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlayMatSound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hhit.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;

		SetThink(&CSledgeHammer::SmackHeavy);
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay(1.5f);
		m_flNextSecondaryAttack = GetNextAttackDelay(1.5f);
		m_flNextTertiaryAttack = gpGlobals->time + 2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;
	}
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector addedvel = gpGlobals->v_forward * 100;
	m_pPlayer->pev->velocity.x += addedvel.x;
	m_pPlayer->pev->velocity.y += addedvel.y;
}

void CSledgeHammer::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iSwingMode == SWING_START_BIG)
	{
		if (gpGlobals->time > m_flBigSwingStart + 1)
		{
			m_iSwingMode = SWING_DOING_BIG;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
			SetThink(&CSledgeHammer::BigSwing);
			pev->nextthink = gpGlobals->time + 0.025;
		}
	}
	else
	{
		m_iSwingMode = SWING_NONE;
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.5)
		{
			iAnim = MELEE_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.4;
		}
		else if (flRand <= 0.9)
		{
			iAnim = MELEE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.77;
		}
		else
		{
			iAnim = MELEE_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.4;
		}

		SendWeaponAnim(iAnim);
	}
}
void CSledgeHammer::GetWeaponData(weapon_data_t& data)
{
	CBasePlayerWeapon::GetWeaponData(data);

	data.m_fInSpecialReload = static_cast<int>(m_iSwingMode);
}

void CSledgeHammer::SetWeaponData(const weapon_data_t& data)
{
	CBasePlayerWeapon::SetWeaponData(data);

	m_iSwingMode = data.m_fInSpecialReload;
}

int CSledgeHammer::iItemSlot()
{
	return 1;
}