
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"
#include "Blooddrops.h"
#include <string> // worse than the first STD meaning that comes to mind
#pragma once

struct gib_data_t
{
    std::string gib_mdlname;
    int body;
    int amount;
    int type = 0;
};

//
// A gib is ONLY a chunk of a body, NOT a piece of wood/metal/rocks/etc.
//
class CoolerGib : public CBaseEntity
{
public:
	void Spawn(const char* szGibModel, int body = 0);
	void EXPORT BounceGibTouch(CBaseEntity* pOther);
	void EXPORT StickyGibTouch(CBaseEntity* pOther);
	void EXPORT WaitTillLand();
	void LimitVelocity();

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }
	static void SpawnHeadGib(entvars_t* pevVictim, CoolerGib* pGib);
	static void SpawnRandomGibs(entvars_t* pevVictim, Vector spawnposOVRDE = NULL);
	static void SpawnStickyGibs(entvars_t* pevVictim, CoolerGib* pGib);
	static std::vector<gib_data_t> GetNPCgibs(entvars_t* pevVictim);	
	int ShouldCollide(CBaseEntity* pentTouched) override;

	int m_bloodColor;
	int m_cBloodDecals = 4; // how many blood decals this gib can place (1 per bounce until none remain).
	float m_lifeTime;
};