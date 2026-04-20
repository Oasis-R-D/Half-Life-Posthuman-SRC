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

#include <vector>
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

// Monstermaker spawnflags
#define SF_HORDEMAKER_START_ON 1	// start active ( if has targetname )
#define SF_HORDEMAKER_EXPENSIVECHECK 2 // check if the spawn will be seen by the player
#define SF_HORDEMAKER_CYCLIC 4	 	// drop one monster every time fired.
#define SF_HORDEMAKER_MONSTERCLIP 8 // Children are blocked by monsterclip
#define SF_HORDEMAKER_PREHUMAN 16 	// Children are spawned with PreHuman tag
#define SF_HORDEMAKER_AWARE 32 	// Children are spawned with nearest player as the enemy (and knowing where they are)

extern CGraph WorldGraph;

int lastspawnednode; // this is global so multiple ents can check it

std::vector<int> g_liValidNodes;

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

	float m_fCheckDist;

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
	else if (FStrEq(pkvd->szKeyName, "checkdist"))
	{
		m_fCheckDist = atof(pkvd->szValue);
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
	if (0 == WorldGraph.m_fGraphPresent)
	{
		ALERT(at_warning, "HordeMaker: No nodegraph present");
		return;
	}

	if (g_liValidNodes.empty()) // generate list of spawns
	{
		for (int i = 0; i < WorldGraph.m_cNodes; i++)
		{
			if ((WorldGraph.m_pNodes[i].m_afNodeInfo & bits_NODE_LAND) != 0) // only check land nodes
			{
				Vector nodevec = WorldGraph.m_pNodes[i].m_vecOriginPeek;

				TraceResult Height;
				UTIL_TraceLine(nodevec, nodevec - gpGlobals->v_up * 64, ignore_monsters, dont_ignore_glass, NULL, &Height); // get floor

				UTIL_TraceLine(Height.vecEndPos, Height.vecEndPos + gpGlobals->v_up * 72, ignore_monsters, dont_ignore_glass, NULL, &Height);
				if (Height.flFraction == 1.0) // is the ceiling tall enough?
				{
					g_liValidNodes.push_back(i); // valid node, add to list
				}
			}
		}
	}

	if (m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren)
	{ 
		return; // not allowed to make a new one yet. Too many live ones out right now.
	}

	edict_t* pent;
	entvars_t* pevCreate;

	int selectednode;
	Vector VecSpawn;

	while (true)
	{
		selectednode = g_liValidNodes[RANDOM_LONG(0, g_liValidNodes.size()-1)];
		if (selectednode == lastspawnednode) // there's probably a monster still here, throw out
			continue;
		else
			break;
	}

	lastspawnednode = selectednode;

	Vector nodevec = WorldGraph.m_pNodes[selectednode].m_vecOriginPeek;

	TraceResult Height;
	UTIL_TraceLine(nodevec, nodevec - gpGlobals->v_up * 32, ignore_monsters, dont_ignore_glass, NULL, &Height); // find floor
	VecSpawn = Height.vecEndPos; // floor

	if (m_fCheckDist != NULL && m_fCheckDist > 0)
	{
		Vector mins = VecSpawn - Vector(m_fCheckDist, m_fCheckDist, 0);
		Vector maxs = VecSpawn + Vector(m_fCheckDist, m_fCheckDist, 72);

		CBaseEntity* pList[2];
		int count = UTIL_EntitiesInBox(pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER);
		if (count != 0) // don't spawn npcs near players or other monsters
		{
			ALERT(at_aiconsole, "Too close, NPC not spawned!\n");
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
	}

	CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(CBaseEntity::Instance(FIND_CLIENT_IN_PVS(edict())));

	if ((pev->spawnflags & SF_HORDEMAKER_EXPENSIVECHECK) != 0)
	{
		TraceResult sightline;
		Vector checkspot = VecSpawn + Vector(0, 0, 32);

		if (pPlayer != nullptr)
		{
			UTIL_TraceLine(checkspot, pPlayer->EyePosition(), ignore_monsters, ignore_glass, NULL, &sightline);
			if (sightline.flFraction == 1.0)
			{
				Vector Grennormal = (checkspot - pPlayer->EyePosition()).Normalize();
				UTIL_MakeVectors(pPlayer->pev->v_angle);
				float dp = DotProduct(Grennormal, -gpGlobals->v_forward);
				if (dp < 0)
				{
					ALERT(at_aiconsole, "In view, NPC not spawned!\n");
					pev->nextthink = gpGlobals->time + 0.1;
					return;
				}
			}
		}
	}

	pent = CREATE_NAMED_ENTITY(m_iszMonsterClassname);

	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in HordeMaker!\n");
		SetThink(NULL);
		SetUse(NULL);
		return;
	}

	// If I have a target, fire!
	if (!FStringNull(pev->target))
	{
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
	}

	pevCreate = VARS(pent);
	pevCreate->origin = VecSpawn;
	pevCreate->angles = Vector(0, RANDOM_LONG(0, 360), 0);
	SetBits(pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND);
	
	CBaseEntity* ent = CBaseEntity::Instance(pent);
	if ((pev->spawnflags & SF_HORDEMAKER_PREHUMAN) != 0)
	{
		ent->m_bPrehuman = true;
	}

	// orginal pPlayer is only in PVS, this makes sure the monster KNOWS ALL
	if (!g_pGameRules->IsMultiplayer())
		pPlayer = dynamic_cast<CBasePlayer*>(UTIL_GetLocalPlayer());

	if ((pev->spawnflags & SF_HORDEMAKER_AWARE) != 0 && pPlayer != nullptr)
	{
		// TO-DO: this may not be all the needed code to make them spawn knowing and attacking the player
		CBaseMonster* pMonster = dynamic_cast<CBaseMonster*>(ent);
		if (pMonster != nullptr)
		{
			pMonster->m_movementGoal = MOVEGOAL_ENEMY;
			pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
			pMonster->PushEnemy(hEnemy, vecLocation);
		}
	}

	ALERT(at_aiconsole, "SPAWNED %s AT: (%f, %f, %f)\n", STRING(m_iszMonsterClassname), pevCreate->origin.x, pevCreate->origin.y, pevCreate->origin.z);
	
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
		// Disable this forever.  Don't kill it because it still gets death notices
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

	if (m_cLiveChildren == 0 && m_cNumMonsters == 0) // no more can spawn or are spawned
	{
		UTIL_Remove(this);
	}
}
