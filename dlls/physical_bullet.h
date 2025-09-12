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
class CPhysbullet : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	void Stay();
	void EXPORT AirThink();
	void EXPORT BoltTouch(CBaseEntity* pOther);
	static void BulletCreate(int BLLTamnt, float BLLTDamage, int BLLTSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, float BLLTGravity, int FlareType, edict_t *shooter); // add damage, spread and owner so entities calling this can give it the proper stuff

private:
	int m_Flare;
	int m_BulletAmount;
	int m_muzzlevelocity;
	int m_maxricochet;
	int m_maxpenetrate;
	Vector m_SpawnPos;
	Vector m_direction;
	Vector m_Endpos;
	float m_Spread;
	float m_SpreadVert;
	float m_BulletDamage;
	float m_Gravity;
	bool m_haswizzed;
};
#endif
