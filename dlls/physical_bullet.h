#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "soundent.h"

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()

// OVERLOADS SOME ENTVARS:
// speed - the ideal magnitude of my velocity
class CPhysbullet : public CBaseEntity
{
	int m_iTrail;
public:
	static void BulletCreate(int BLLTamnt, float BLLTDamage, int BLLTSpeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, float BLLTGravity, int FlareType, edict_t *shooter, bool subsonic = false, float maxpenoverride = NULL); // add damage, spread and owner so entities calling this can give it the proper stuff
	void Spawn() override;
	void Precache() override;
	void EXPORT AirThink();
	void EXPORT BoltTouch(CBaseEntity* pOther);
	int ShouldCollide(CBaseEntity* pentTouched) override;
	void Stay();
	bool IsBullet() override { return true; }
	int Classify() override;
	
	static const char* pNearMissSounds[];

	edict_t* Owner;

	int m_Flare;
	int m_BulletAmount;
	int m_muzzlevelocity;

	Vector m_SpawnPos;
	Vector m_direction;
	Vector m_SpreadVect;

	double m_Spread;
	double m_SpreadVert;
	int m_BulletDamage;
	double m_Gravity;
	double m_distpenetrate;

	bool m_bsubsonic;

private:
	bool m_bHeavyDecal = false;
	bool m_haswizzed;
	double m_fPenoverride;
	Vector m_Endpos;
};