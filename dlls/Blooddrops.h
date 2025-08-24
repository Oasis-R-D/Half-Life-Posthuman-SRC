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
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
#ifndef CLIENT_DLL
class CPhysblood : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	void Stay();
	void EXPORT AirThink();
	void EXPORT BoltTouch(CBaseEntity* pOther);
	static void BloodCreate(int BLDamnt, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float BLLTGravity); // add damage, spread and owner so entities calling this can give it the proper stuff

private:
	int m_BloodDropVel;
	Vector m_SpawnPos;
	Vector m_direction;
	float m_Spread;
	float m_Gravity;
};
#endif
