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

struct bullet_data_t
{
    unsigned int amount = 1;
    unsigned int damage = 0;
	unsigned int muzzlevel = 5;
	int type = 9;
	Vector org;
	Vector dir;
	float spread = 0;
	float vertspread = 0;
	float gravity = 1.0;
	edict_t* pShooter;
	bool subsonic = false;
	float penetrate_override = 0;
	CBaseEntity* pIgnore = NULL;
};

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()

// OVERLOADS SOME ENTVARS:
// speed - the ideal magnitude of my velocity
class CPhysbullet : public CBaseEntity
{
	int m_iTrail;
public:
	static void BulletCreate(unsigned int BLLTamnt, unsigned int BLLTdamage, unsigned int BLLTspeed, Vector VecSpawnPos, Vector vecDir, float vecSpread, float vecSpreadvert, float BLLTGravity, int BLLTtype, edict_t *shooter, bool subsonic = false, float maxpenoverride = NULL, CBaseEntity* pIgnore = nullptr); // add damage, spread and owner so entities calling this can give it the proper stuff
	static void BulletCreate(bullet_data_t* data); // add damage, spread and owner so entities calling this can give it the proper stuff
	void Spawn() override;
	void Precache() override;
	void EXPORT AirThink();
	void EXPORT BulletImpact(CBaseEntity* pOther);
	int ShouldCollide(CBaseEntity* pentTouched) override;
	bool IsBullet() override { return true; }
	
	void FindWaterSurface();

	static const char* pNearMissSounds[];

	edict_t* Owner;

	CBaseEntity* m_pIgnore;
	int m_Flare;
	unsigned int m_iMuzzleVel;

	Vector m_SpawnPos;
	Vector m_vecDir;

	double m_Spread;
	double m_SpreadVert;
	double m_flPenetrationPow;

	bool m_bsubsonic;

private:
	bool m_haswizzed;
	bool m_bTryRefl;
	bool m_bTryPen;

	double m_fPenoverride = NULL;
	Vector m_Endpos;
};