#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
class CPhysblood : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int ShouldCollide(CBaseEntity* pentTouched) override;
	int ObjectCaps() override { return ((CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE); }
	void EXPORT AirThink();
	void EXPORT DropTouch(CBaseEntity* pOther);
	static void BloodCreate(unsigned int BLDamnt, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float BLLTGravity, int BloodType, bool isgib = false, float spread = RANDOM_FLOAT(CONE_45DEGREES, CONE_15DEGREES), bool speedRNG = true, bool pool = false); // add damage, spread and owner so entities calling this can give it the proper stuff

private:
	int m_BloodDropVel;
	int m_BloodType; // Always B positive!!!
	int m_opposite;
	Vector m_SpawnPos;
	Vector m_direction;
	float m_Spread;
	float m_Gravity;
	bool m_isgib;
	bool m_isPool;
	bool m_hashealed = false;
	bool m_randomspeed = false;

};
