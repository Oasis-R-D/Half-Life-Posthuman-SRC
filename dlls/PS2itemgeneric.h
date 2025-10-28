/*

22.08.18

"item_generic" entity for ps2hl. Acts like env_model from SoHL

Created by supadupaplex
Based on this article https://www.moddb.com/games/half-life/tutorials/where-is-poppy-your-first-custom-entity-part-2

*/

#pragma once

// Header files
#include "extdll.h"		// Required for KeyValueData
#include "util.h"		// Required Consts & Macros
#include "cbase.h"		// Required for CPointEntity

#define ITGN_DELAY_THINK 0.1f

class CItemGeneric : public CBaseAnimating
{
private:
	// Methods
	void Spawn();
	void Precache();
	bool KeyValue(KeyValueData *pkvd);
	void Think();
};
