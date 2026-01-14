/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//=========================================================
// Horde Maker - this is an entity that loops through nodes
// and spawns enemies if there are no players near the node
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"
#include "nodes.h"

// Monstermaker spawnflags
#define SF_HORDEMAKER_START_ON 1	  // start active ( if has targetname )
#define SF_HORDEMAKER_CYCLIC 4	  // drop one monster every time fired.
#define SF_HORDEMAKER_MONSTERCLIP 8 // Children are blocked by monsterclip

extern CGraph WorldGraph;

//=========================================================
// HordeMaker - this ent creates monsters during the game.
//=========================================================
class CHordeMaker : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT CyclicUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT MakerThink();
	void DeathNotice(entvars_t* pevChild) override; // monster maker children use this to tell the monster maker that they have died.
	void MakeMonster();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszMonsterClassname; // classname of the monster(s) that will be created.

	int m_cNumMonsters; // max number of monsters this ent can create


	int m_cLiveChildren;	// how many monsters made by this monster maker that are currently alive
	int m_iMaxLiveChildren; // max number of monsters that this maker may have out at one time.

	bool m_fActive;
	bool m_fFadeChildren; // should we make the children fadeout?
};

LINK_ENTITY_TO_CLASS(hordemaker, CHordeMaker);

TYPEDESCRIPTION CHordeMaker::m_SaveData[] =
	{
		DEFINE_FIELD(CHordeMaker, m_iszMonsterClassname, FIELD_STRING),
		DEFINE_FIELD(CHordeMaker, m_cNumMonsters, FIELD_INTEGER),
		DEFINE_FIELD(CHordeMaker, m_cLiveChildren, FIELD_INTEGER),
		DEFINE_FIELD(CHordeMaker, m_iMaxLiveChildren, FIELD_INTEGER),
		DEFINE_FIELD(CHordeMaker, m_fActive, FIELD_BOOLEAN),
		DEFINE_FIELD(CHordeMaker, m_fFadeChildren, FIELD_BOOLEAN),
};


IMPLEMENT_SAVERESTORE(CHordeMaker, CBaseMonster);

bool CHordeMaker::KeyValue(KeyValueData* pkvd)
{

	if (FStrEq(pkvd->szKeyName, "monstercount"))
	{
		m_cNumMonsters = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_imaxlivechildren"))
	{
		m_iMaxLiveChildren = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "monstertype"))
	{
		m_iszMonsterClassname = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}


void CHordeMaker::Spawn()
{
	if (0 == WorldGraph.m_fGraphPresent)
	{
		ALERT(at_warning, "CHordeMaker: No nodegraph present")
		REMOVE_ENTITY(edict());
		return;
	}

	pev->solid = SOLID_NOT;

	m_cLiveChildren = 0;
	Precache();
	if (!FStringNull(pev->targetname))
	{
		if ((pev->spawnflags & SF_HORDEMAKER_CYCLIC) != 0)
		{
			SetUse(&CHordeMaker::CyclicUse); // drop one monster each time we fire
		}
		else
		{
			SetUse(&CHordeMaker::ToggleUse); // so can be turned on/off
		}

		if (FBitSet(pev->spawnflags, SF_HORDEMAKER_START_ON))
		{ // start making monsters as soon as monstermaker spawns
			m_fActive = true;
			SetThink(&CHordeMaker::MakerThink);
		}
		else
		{ // wait to be activated.
			m_fActive = false;
			SetThink(&CHordeMaker::SUB_DoNothing);
		}
	}
	else
	{ // no targetname, just start.
		pev->nextthink = gpGlobals->time + m_flDelay;
		m_fActive = true;
		SetThink(&CHordeMaker::MakerThink);
	}

	if (m_cNumMonsters == 1)
	{
		m_fFadeChildren = false;
	}
	else
	{
		m_fFadeChildren = true;
	}
}

void CHordeMaker::Precache()
{
	CBaseMonster::Precache();

	UTIL_PrecacheOther(STRING(m_iszMonsterClassname));
}

//=========================================================
// MakeMonster-  this is the code that drops the monster
//=========================================================
void CHordeMaker::MakeMonster()
{
	edict_t* pent;
	entvars_t* pevCreate;

	if (m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren)
	{ // not allowed to make a new one yet. Too many live ones out right now.
		return;
	}

	int selectednode;
	bool nodevalid;
	Vector tryspawn;

	while (!nodevalid)
	{
		selectednode = RANDOM_LONG(0, WorldGraph.m_cNodes - 1)
		if ((WorldGraph.m_pNodes[selectednode].m_afNodeInfo & bits_NODE_LAND) != 0)
		{
			Vector nodevec = WorldGraph.m_pNodes[selectednode].m_vecOriginPeek;

			TraceResult Height;
			Util_TraceLine(nodevec, nodevec - gpGlobals->v_up * 64, dont_ignore_glass, NULL, &Height)
			tryspawn = Height.vecEndPos; // floor

			UTIL_TraceLine(Height.vecEndPos, Height.vecEndPos + gpGlobals->v_up * 128, dont_ignore_glass, NULL, &Height)
			if (Height.Length() > 72) // is ceiling tall enough?
				nodevalid = true; // found our spot
		}
	}

	Vector mins = tryspawn - Vector(256, 256, 0);
	Vector maxs = tryspawn + Vector(256, 256, 0);
	maxs.z = tryspawn + Vector(0, 0, 72);
	mins.z = tryspawn;

	CBaseEntity* pList[2];
	int count = UTIL_EntitiesInBox(pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER);
	if (0 != count)
	{
		// don't build a stack of monsters!
		return;
	}

	pent = CREATE_NAMED_ENTITY(m_iszMonsterClassname);

	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in MonsterMaker!\n");
		return;
	}

	// If I have a target, fire!
	if (!FStringNull(pev->target))
	{
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
	}

	pevCreate = VARS(pent);
	pevCreate->origin = tryspawn;
	pevCreate->angles = pev->angles; // TO-DO: make random
	SetBits(pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND);

	// Children hit monsterclip brushes
	if ((pev->spawnflags & SF_HORDEMAKER_MONSTERCLIP) != 0)
		SetBits(pevCreate->spawnflags, SF_MONSTER_HITMONSTERCLIP);

	DispatchSpawn(ENT(pevCreate));
	pevCreate->owner = edict();

	if (!FStringNull(pev->netname))
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++; // count this monster
	m_cNumMonsters--;

	if (m_cNumMonsters == 0)
	{
		// Disable this forever.  Don't kill it because it still gets death notices // TO-DO: can we remove it once the remaining children die?
		SetThink(NULL);
		SetUse(NULL);
	}
}

//=========================================================
// CyclicUse - drops one monster from the monstermaker
// each time we call this.
//=========================================================
void CHordeMaker::CyclicUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	MakeMonster();
}

//=========================================================
// ToggleUse - activates/deactivates the monster maker
//=========================================================
void CHordeMaker::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_fActive))
		return;

	if (m_fActive)
	{
		m_fActive = false;
		SetThink(NULL);
	}
	else
	{
		m_fActive = true;
		SetThink(&CHordeMaker::MakerThink);
	}

	pev->nextthink = gpGlobals->time;
}

//=========================================================
// MakerThink - creates a new monster every so often
//=========================================================
void CHordeMaker::MakerThink()
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	MakeMonster();
}


//=========================================================
//=========================================================
void CHordeMaker::DeathNotice(entvars_t* pevChild)
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_cLiveChildren--;

	if (!m_fFadeChildren)
	{
		pevChild->owner = NULL;
	}
}
