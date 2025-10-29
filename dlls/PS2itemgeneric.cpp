/*

22.08.18

"item_generic" entity for ps2hl

Created by supadupaplex

*/

#include "PS2itemgeneric.h"

// Link entity
LINK_ENTITY_TO_CLASS(item_generic, CItemGeneric);


// Methods //

// Precache handler
void CItemGeneric::Precache()
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

// Spawn handler
void CItemGeneric::Spawn()
{
	// Precache model
	Precache();

	// Set the model
	SET_MODEL(ENT(pev), STRING(pev->model));

	// Check if sequence is loaded
	if (pev->sequence == -1)
	{
		// Failed to load sequence
		ALERT(at_console, "item_generic: cant load animation sequence ...");
		pev->sequence = 0;
	}

	// Prepare sequence
	pev->frame = 0;
	m_fSequenceLoops = 1;
	ResetSequenceInfo();

	// BBox
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	pev->solid = SOLID_NOT;

	// Set think delay
	pev->nextthink = gpGlobals->time + ITGN_DELAY_THINK;
}

// Parse keys
bool CItemGeneric::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "model"))
	{
		// Set model
		pev->model = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "body"))
	{
		// Set body
		pev->body = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		// Set sequence
		pev->sequence = LookupSequence(pkvd->szValue);
		return true;
	}
	else
		return CBaseEntity::KeyValue(pkvd);
}

// Think handler
void CItemGeneric::Think()
{
	// Set delay
	pev->nextthink = gpGlobals->time + ITGN_DELAY_THINK;

	// Call animation handler
	StudioFrameAdvance(0);
}

