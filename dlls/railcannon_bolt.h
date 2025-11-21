#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"

// UNDONE: Save/restore this?  Don't forget to set classname

// OVERLOADS SOME ENTVARS:
// speed - the ideal magnitude of my velocity

class CCrossbowBolt : public CBaseEntity
{
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	void EXPORT BubbleThink();
	void EXPORT BoltTouch(CBaseEntity* pOther);
	void EXPORT ExplodeThink();
	void Stay();
	int m_iTrail;

public:
	static CCrossbowBolt* BoltCreate(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner);
};