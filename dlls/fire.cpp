/*
*
//
/// Copyright PackMail Industries 2026-2026, no rights reserved. (I'll sue you breh)
//
*
*/

/*

===== fire.cpp ==========================================================

  implementation of voxel based fire in GoldSource

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "decals.h"
#include "UserMessages.h"

// C++ STD stuff
#include <vector>
#include <iterator>
#include <memory>
#include <map>

#include "fire.h"

CFireManager* FireManager;

#pragma region FireManager

CFireManager::CFireManager()
{
	if (FireManager)
	{
		ALERT(at_error, "Do not create multiple instances of FireManager::\n");
		return;
	}

	FireManager = this;
}

CFireManager::~CFireManager()
{
	if (FireManager != this)
		return;

	FireManager = nullptr;
}

void CFireManager::Spawn()
{
	Precache();

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CFireManager::ManagerThink);
}

void CFireManager::Precache()
{
	// Flag this entity for removal if it's not the actual FireManager entity.
	if (FireManager != this)
	{
		UTIL_Remove(this);
		return;
	}

	PRECACHE_SOUND("soundscape_knockoffs/levels/Sector I/mediumfire_loop.wav");
	PRECACHE_SOUND("soundscape_knockoffs/levels/Sector I/ember_loop.wav");
	PRECACHE_SOUND("soundscape_knockoffs/levels/Sector I/carfire_loop.wav");
}

// Does std::erase_if()
void CFireManager::RemoveDead()
{
	auto lambda = [](const auto& obj)
	{ return obj.second->heat <= 0; };

	voxelMap.erase
	(
        std::remove_if
		(
			voxelMap.begin(), 
			voxelMap.end(), 
			lambda
		),

		voxelMap.end()
    );
}

void CFireManager::ExtinguishFire(Vector pos, int heat, int radiusSquared)
{
	if (voxelMap.empty())
		return;

	if (radiusSquared > 0)
	{
		for (const auto& voxFire : voxelMap)
		{
			if ((voxFire.second->origin - pos).LengthSquared() < radiusSquared)
				voxFire.second->heat -= heat;
		}
	}
	else
	{
		auto& voxFire = voxelMap.find(pos);
		if (voxFire != voxelMap.end() && (voxFire->second->origin - pos).LengthSquared() < radiusSquared)
			voxFire->second->heat -= heat;
	}
}

void CFireManager::AddFire(Vector pos, int heat)
{	
	// Check to make sure we aren't trying to add fire where fire already is
	if (!MergeFirePoints(pos, heat))
	{
		auto newFire = std::make_unique<CFireVoxel>();
		newFire->origin = pos;
		newFire->heat = heat;
		newFire->thinktime = gpGlobals->time + RANDOM_FLOAT(0.01, 0.1);
		newFire->spreadTime = gpGlobals->time + RANDOM_FLOAT(0.5, 1);
		voxelTemp.emplace_back(pos, std::move(newFire));

		if (RANDOM_LONG(0, 2) == 0)
			return;

		TraceResult GroundCheck;
		Vector newPos = pos;
		newPos.x += RANDOM_FLOAT(-8, 8);
		newPos.y += RANDOM_FLOAT(-8, 8);
		UTIL_TraceLine(newPos, newPos-Vector(0,0,sv_firesize.value*1.5), ignore_monsters, NULL, &GroundCheck);
		if (RANDOM_LONG(0,1))
			UTIL_DecalTrace(&GroundCheck, RANDOM_LONG(DECAL_SMALLSCORCH1, DECAL_SMALLSCORCH3), sv_firesize.value*1.5);
		else
			UTIL_DecalTrace(&GroundCheck, RANDOM_LONG(DECAL_SCORCH1, DECAL_SCORCH1), sv_firesize.value*1.5);
	}
}

void CFireManager::MoveFire(Vector pos_original, Vector pos_new)
{
	auto node = voxelMap.extract(pos_original);
	node.key() = pos_new;
	node.mapped()->origin = pos_new;
	voxelMap.insert(std::move(node));
}

void CFireManager::MergeInTemp()
{
	if (voxelTemp.empty())
		return;

	voxelMap.insert(std::make_move_iterator(voxelTemp.begin()), std::make_move_iterator(voxelTemp.end()));

	if (!voxelTemp.empty())
		ALERT(at_error, "CFireManager: data leftover in voxelTemp!\n");
}

bool CFireManager::MergeFirePoints(Vector pos, int heat) 
{
	if (!voxelMap.empty())
	{
		auto& voxFire = voxelMap.find(pos);
		if (voxFire != voxelMap.end())

		if (voxFire != voxelMap.end())
		{
			voxFire->second->heat += heat;
			return true;
		}
	}

	// check voxelTemp aswell
	for (auto& tempvoxel : voxelTemp)
	{
		if (tempvoxel.first == pos)
		{
			tempvoxel.second->heat += heat;
			return true;
		}
	}

	return false;
}

void CFireManager::ManagerThink()
{
	// move in fire added from last think or elsewhere
	MergeInTemp();

	// nothing to simulate
	if (voxelMap.empty())
	{
		pev->nextthink = gpGlobals->time + 0.5;
		return;
	}
	
	pev->nextthink = gpGlobals->time;

	// destroy dead fires this frame
	RemoveDead();

	for (const auto& voxFire : voxelMap)
	{
		if (voxFire.second->thinktime <= gpGlobals->time)
			voxFire.second->Think();
	}
}

// Spawn fire in a ball
void CFireManager::FireExplosion(Vector pos, int radius, int heat)
{
	int firesize = sv_firesize.value; // spread out more
	firesize *= 2;
	int totalDist = radius * firesize;

	for (int i = -radius; i < radius; i++)
	{
		for (int j = -radius; j < radius; j++)
		{
			Vector newPos = pos + Vector(j * firesize, i * firesize, 0);
			float dist = (newPos-pos).Length();

			if (dist > totalDist)
				continue;

			AddFire(newPos, (dist / totalDist) * heat);
		}
	}
}

//=========================================================
#pragma endregion
#pragma region Per voxel functions
//=========================================================

void CFireVoxel::Think()
{
	const int firesize = sv_firesize.value;
	thinktime = gpGlobals->time + 0.125;

	SpawnParticles(firesize*0.66);
	
	DealDamage(firesize); 

	// don't run spread code while falling
	if (CheckFall(((float)firesize / 2) + 2))
		return;

	heat -= 1;

	if (spreadTime > gpGlobals->time || heat < 160)
		return;

	// spread
	Vector VecFireSpread;
	int times = 0;
	int opp1, opp2;
		
	do {
		if (times >= 10) // don't spawn if it isn't finding any good spots
		{
			//ALERT(at_warning, "Env_Fire: couldn't spawn fire!\n");
			return;
		}
		times += 1;

		opp1 = RANDOM_LONG(-1, 0);
		opp2 = RANDOM_LONG(-1, 0);
		
		if (opp1 == 0)
			opp1 = 1;
		if (opp2 == 0)
			opp2 = 1;
			
		// spreads in 8 directions
		VecFireSpread = origin;
		VecFireSpread.x += firesize * opp1;
		VecFireSpread.y += firesize * opp2;
	} while (UTIL_PointContents(VecFireSpread) == CONTENTS_SOLID);

	spreadTime = gpGlobals->time + RANDOM_LONG(4, 10);

	FireManager->AddFire(VecFireSpread, 150);
	heat -= 150;
}

void FireRadiusDamage2(Vector vecSrc, float flDamage, float flRadius)
{
	CBaseEntity* pEntity = NULL;
	TraceResult tr;
	Vector vecSpot;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		if (pEntity->pev->iuser4 == -16)
			continue;

		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			if (pEntity->pev->deadflag == DEAD_NO && pEntity->pev->waterlevel == 0)
				pEntity->pev->iuser4 += 33;

			pEntity->TakeDamage(FireManager->pev, FireManager->pev, flDamage, DMG_BURN);
		}
	}
}

void CFireVoxel::DealDamage(int size)
{
	if (dmgTime > gpGlobals->time)
		return;

	::FireRadiusDamage2(origin, 5, (float)size * 0.75);
	dmgTime = gpGlobals->time + 0.5;
}

const void CFireVoxel::SpawnParticles(int size)
{	// Spawn visuals
	Vector VecflameOrg = origin;
	VecflameOrg.x += RANDOM_LONG(-size, size);
	VecflameOrg.y += RANDOM_LONG(-size, size);
	VecflameOrg.z += RANDOM_LONG(-size,	   0);

	//if (RANDOM_LONG(0, heat) < 50)
		//return;

	if (heat < 1000)
		PLAYBACK_EVENT_FULL(0, NULL, g_sParticleEvent, 0.0, VecflameOrg, g_vecZero, 0.0, 0.0, PE_FIRE, 0, 0, 0);
	else
	{
		heat = V_min(heat, 1250);
		PLAYBACK_EVENT_FULL(0, NULL, g_sParticleEvent, 0.0, VecflameOrg, g_vecZero, 0.0, 0.0, PE_FIRE, 0, 0, 1);
	}
}

bool CFireVoxel::CheckFall(float size)
{
	Vector newOrigin = origin - Vector(0, 0, size);

	TraceResult GroundCheck;
	UTIL_TraceLine(origin, newOrigin, ignore_monsters, NULL, &GroundCheck);

	if (GroundCheck.flFraction != 1)
		return false;

	// Either merge the fire with the one below or move it
	if (!FireManager->MergeFirePoints(newOrigin, heat))
		FireManager->MoveFire(origin, newOrigin);
	else
		heat = 0;

	heat -= 3; // falling is very bad

	return true;
}

//=========================================================
#pragma endregion
#pragma region Projectile
//=========================================================

LINK_ENTITY_TO_CLASS(phys_fire, CFireProjectile);
void CFireProjectile::FireShoot(unsigned int BLDamnt, int heat, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float BLLTGravity, float spread)
{
	for (unsigned int i = 0; i < BLDamnt; i++) // Allows multishot
	{
		// Create a new entity with CFireProjectile private data
		CFireProjectile* pBlood = GetClassPtr((CFireProjectile*)NULL);
		pBlood->pev->classname = MAKE_STRING("phys_fire");
		pBlood->m_vecVel = BLDSpeed;
		pBlood->m_SpawnPos = VecSpawnPos;
		pBlood->m_vecDir = vecDir;
		pBlood->m_Spread = spread;
		pBlood->m_Gravity = BLLTGravity;
		pBlood->pev->dmg = heat;
		pBlood->Spawn();
	}
}

void CFireProjectile::Spawn()
{
	Precache();

	//SET_MODEL(ENT(pev), "sprites/blood.spr");

	pev->movetype = MOVETYPE_TOSS; // makes it have gravity
	pev->solid = SOLID_BBOX;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin(pev, m_SpawnPos);

	// TO-DO: use radial spread, this is not the proper way to do spread
	pev->velocity = ((m_vecDir + RANDOM_VECTOR(-m_Spread, m_Spread)) * m_vecVel); // Applies spread and velocity, also applies the chance to have the entry wound droplets
	pev->gravity = m_Gravity;
	pev->owner = NULL;

	SetTouch(&CFireProjectile::DropTouch);
	SetThink(&CFireProjectile::AirThink);
	pev->nextthink = gpGlobals->time;
}

void CFireProjectile::Precache()
{
	//PRECACHE_MODEL("sprites/blood.spr");
}

void CFireProjectile::DropTouch(CBaseEntity* pOther)
{
	Vector org = UTIL_GetGlobalTrace().vecEndPos;
	translateToFireSpace(org);

	FireManager->AddFire(org, pev->dmg);

	UTIL_Remove(this);
}

void CFireProjectile::AirThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	PLAYBACK_EVENT_FULL(0, edict(), g_sParticleEvent, 0.0, pev->origin + pev->velocity * 0.1, g_vecZero, 0.0, 0.0, PE_FIRE, 0, 0, 0);

	if (pev->waterlevel == 0)
		return;

	SetThink(&CFireProjectile::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

int CFireProjectile::ShouldCollide(CBaseEntity* pentTouched)
{
	if (pentTouched->IsBSPModel())
		return 1;
	else
		return 0;
}

#pragma endregion