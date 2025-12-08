#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"
#include "Blooddrops.h"
#include <string>

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
	static void SpawnRandomGibs(entvars_t* pevVictim);
	static void SpawnHL1Gibs(entvars_t* pevVictim);
	static void SpawnStickyGibs(entvars_t* pevVictim);
	static std::vector<std::vector<std::string>> GetNPCgibs(entvars_t* pevVictim);	

	int m_bloodColor;
	int m_cBloodDecals = 4; // how many blood decals this gib can place (1 per bounce until none remain).
	float m_lifeTime;
};

// START NPC GIB LISTS (FORK YOU C++) // MDL, BG, AMNT

std::vector<std::vector<std::string>> xenian_gibmap = 
{
		{"models/agibs.mdl", "0", "1"},
		{"models/agibs.mdl", "1", "1"},
		{"models/agibs.mdl", "2", "1"},
		{"models/agibs.mdl", "3", "1"}
};

std::vector<std::vector<std::string>> human_gibmap = 
{
		{"models/hgibs.mdl", "0", "1"},
		{"models/hgibs.mdl", "1", "1"},
		{"models/hgibs.mdl", "2", "1"},
		{"models/hgibs.mdl", "3", "1"},
		{"models/hgibs.mdl", "4", "1"},
		{"models/hgibs.mdl", "5", "1"}
};

std::vector<std::vector<std::string>> pitdrone_gibmap = 
{
		{"models/pit_drone_gibs.mdl", "0", "1"},
		{"models/pit_drone_gibs.mdl", "1", "1"},
		{"models/pit_drone_gibs.mdl", "2", "1"},
		{"models/pit_drone_gibs.mdl", "3", "1"},
		{"models/pit_drone_gibs.mdl", "4", "1"},
		{"models/pit_drone_gibs.mdl", "5", "1"}
};