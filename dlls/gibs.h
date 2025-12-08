#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"
#include "Blooddrops.h"

struct CoolerGibData
{
	const char* const ModelName;
	const int FirstSubModel;
	const int SubModelCount;

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
	static void SpawnHeadGib(entvars_t* pevVictim);
	static void SpawnRandomGibs(entvars_t* pevVictim, int coolerGibs, const char (&GibData)[][3]);
	static void SpawnHL1Gibs(entvars_t* pevVictim, int coolerGibs, bool human);
	static void SpawnStickyGibs(entvars_t* pevVictim, Vector vecOrigin, int coolerGibs);

	int m_bloodColor;
	int m_cBloodDecals = 4; // how many blood decals this gib can place (1 per bounce until none remain).
	float m_lifeTime;
};