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
// skill.cpp - code for skill level concerns
//=========================================================
#include "extdll.h"
#include "util.h"
#include "skill.h"

//=========================================================
// take the name of a cvar, tack a digit for the skill level
// on, and return the value.of that Cvar
// 
// UPDATE! now loops through until it finds a proper value!
// This lets you only input 1 value into skill.cfg and have it work (less clutter)
// 
//=========================================================
float GetSkillCvar(const char* pName)
{
	int iCount;
	float flValue;
	char szBuffer[64];

	int skillLevel = gSkillData.iSkillLevel == 4 ? 3 : gSkillData.iSkillLevel;

	int skips = 0;

	while (skillLevel > 0)
	{
		iCount = sprintf(szBuffer, "%s%d", pName, skillLevel);

		flValue = CVAR_GET_FLOAT(szBuffer);

		if (flValue > 0)
		{
			if (skips > 0)
				ALERT(at_console, "\n* GetSkillCVar Skipped %d times! Result: %s *\n", skips, szBuffer);
			return flValue;
		}
		else if (skillLevel == 1)
		{
			ALERT(at_console, "\n\n** GetSkillCVar Got a zero for %s **\n\n", szBuffer);
			return 0;
		}
		else
		{
			skips++;
			skillLevel--;
		}
	}
}
