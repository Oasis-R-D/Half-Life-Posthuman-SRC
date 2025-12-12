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
#include "decals.h"
#include "skill.h"

LINK_ENTITY_TO_CLASS(weapon_spitthrower, CSpitThrower);

void CSpitThrower::Precache()
{
	PRECACHE_MODEL("models/v_spitthrower.mdl");
	PRECACHE_MODEL("models/w_spitthrower.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");
	PRECACHE_MODEL("models/spit.mdl");

	PRECACHE_SOUND("weapons/sptthrwr_loop.wav");

	UTIL_PrecacheOther("monster_bullchicken");
}

void CSpitThrower::Spawn()
{
	pev->classname = MAKE_STRING("weapon_spitthrower");
	Precache();
	m_iId = WEAPON_SPITTHROWER;
	SET_MODEL(edict(), "models/w_spitthrower.mdl");
	if (g_iSkillLevel != SKILL_HARD)
	{
		m_iDefaultAmmo = 50;
	}
	else
	{
		m_iDefaultAmmo = 25;
	}
	FallInit(); // get ready to fall down.
}

bool CSpitThrower::Deploy()
{
	return DefaultDeploy("models/v_spitthrower.mdl", "models/p_9mmhandgun.mdl", EGON_DRAW, "bow");
}

void CSpitThrower::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	if ((m_pPlayer->m_afButtonLast & IN_ATTACK) == 0)
	{
		STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/sptthrwr_loop.wav");
		pev->armortype = 0;
	}
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;
	SendWeaponAnim(EGON_IDLE1);
	m_flTimeWeaponIdle = 2;
	
}

void CSpitThrower::PrimaryAttack()
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.2);
		return;
	}

	m_iClip--;

	m_flNextPrimaryAttack = 0.1;
	m_flTimeWeaponIdle = 1;

	if (pev->armortype < gpGlobals->time)
	{
		EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/sptthrwr_loop.wav", 1, ATTN_NORM);
		pev->armortype = gpGlobals->time + 2.5;
	}

	SendWeaponAnim(EGON_FIRE1);

	for (int i = 0; i < 3; i++)
	{
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 32 + gpGlobals->v_right * 4;
		Create("env_spit", vecSrc, pev->angles, m_pPlayer->edict());
	}
}

bool CSpitThrower::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "spit";
	if (g_iSkillLevel != SKILL_HARD)
	{
	p->iMaxAmmo1 = 75;
	}
	else
	{
	p->iMaxAmmo1 = 50;
	}
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = 25;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SPITTHROWER;
	p->iWeight = EGON_WEIGHT;

	return true;
}

void CSpitThrower::Holster()
{
	CBasePlayerWeapon::Holster();
}

void CSpitThrower::Reload()
{
	DefaultReload(25, EGON_DRAW, 1.5);
}

class CSpitAmmo : public CBasePlayerAmmo
{
	void Precache() override
	{
		PRECACHE_MODEL("models/w_spit.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	void Spawn() override
	{
		Precache();
		SET_MODEL(edict(), "models/w_spit.mdl");
		CBasePlayerAmmo::Spawn();
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(25, "spit", 250) != -1)
		{
			EMIT_SOUND(edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(ammo_spit, CSpitAmmo);

class CEnvSpit : public CBaseEntity
{	
	Vector m_SpreadVect;
	int iSquidSpitSprite;
	void Spawn()
	{
		iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr"); // client side spittle.
		SET_MODEL(edict(), "models/spit.mdl");
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_BOUNCE;
		//pev->velocity = gpGlobals->v_forward * 1000 + gpGlobals->v_right * RANDOM_LONG(-8, 8) + gpGlobals->v_up * RANDOM_LONG(-8, 8);
		m_SpreadVect = Vector(RANDOM_FLOAT(CONE_6DEGREES, -CONE_6DEGREES), RANDOM_FLOAT(CONE_6DEGREES, -CONE_6DEGREES), RANDOM_FLOAT(CONE_6DEGREES, -CONE_6DEGREES));
		pev->velocity = (gpGlobals->v_forward + m_SpreadVect) * 1750; // Applies spread and velocity
		pev->framerate = 1;
		pev->dmgtime = gpGlobals->time + 2;
		pev->nextthink = gpGlobals->time + 0.1;
		pev->renderamt = 255;
		pev->rendermode = kRenderTransAdd;
	}
	void Touch(CBaseEntity* pOther)
	{
		float iPitch = RANDOM_FLOAT(90, 110);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", .5, ATTN_NORM, 0, iPitch);
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", .5, ATTN_NORM, 0, iPitch);
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", .5, ATTN_NORM, 0, iPitch);
			break;
		}

		if (DAMAGE_NO != pOther->pev->takedamage)
		{
			int damage = 0;
			switch (g_iSkillLevel)
			{
			case SKILL_EASY: damage = 6; break;
			case SKILL_MEDIUM: damage = 5; break;
			case SKILL_HARD: damage = 20; break;
			}
			pOther->TakeDamage(VARS(pev->owner), VARS(pev->owner), damage, DMG_ACID);
		}

		TraceResult tr;
		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0, 1));

		// make some flecks
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(tr.vecEndPos.x); // pos
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_COORD(tr.vecPlaneNormal.x); // dir
		WRITE_COORD(tr.vecPlaneNormal.y);
		WRITE_COORD(tr.vecPlaneNormal.z);
		WRITE_SHORT(iSquidSpitSprite); // model
		WRITE_BYTE(5);				   // count
		WRITE_BYTE(30);				   // speed
		WRITE_BYTE(80);				   // noise ( client will divide by 100 )
		MESSAGE_END();
		UTIL_Remove(this);
	}
	void Think()
	{
		pev->angles = UTIL_VecToAngles(pev->velocity);
		pev->nextthink = gpGlobals->time + 0.1;
		if (pev->dmgtime < gpGlobals->time)
			UTIL_Remove(this);
	}
	int ShouldCollide(CBaseEntity* pentTouched)
{
	if (pentTouched->pev->classname == pev->classname)
	{
		return 0;
	}
	return 1;
}
};

LINK_ENTITY_TO_CLASS(env_spit, CEnvSpit);
