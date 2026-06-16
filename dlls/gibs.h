
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"
#include "Blooddrops.h"
#include "weapons.h"
#include <string>

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
	void EXPORT PickUpThink();
	void EXPORT EatThink();
	void LimitVelocity();

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE | FCAP_IMPULSE_USE; }
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	static void SpawnHeadGib(entvars_t* pevVictim, CoolerGib* pGib);
	static void SpawnRandomGibs(entvars_t* pevVictim, Vector spawnposOVRDE = g_vecZero);
	static void SpawnStickyGibs(entvars_t* pevVictim, CoolerGib* pGib);
	static std::vector<gib_data_t> GetNPCgibs(CBaseEntity* pevVictim);	
	int ShouldCollide(CBaseEntity* pentTouched) override;

	bool m_bDisableFade;
	CBaseEntity* m_pGibbed;
	CBasePlayer* m_pEater;
	gib_data_t m_gdType;
	int m_bloodColor;
	int m_cBloodDecals = 4; // how many blood decals this gib can place (1 per bounce until none remain).
	float m_lifeTime;

	bool m_bLanded;

	float m_nextBurnLogic;
};