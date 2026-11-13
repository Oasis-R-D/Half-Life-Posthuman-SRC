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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "monsters.h"

void CBasePlayerMelee::StartSwing()
{
    m_flSwingStart = gpGlobals->time;

    float timeSpan = m_flSwingEnd - m_flSwingStart;

    m_flRaycastDelay = m_iRaycastAmount / timeSpan;
    // Calculate delay between raycasts
}

void CBasePlayerMelee::EndSwing()
{

}

void CBasePlayerMelee::SwingRaycast()
{
    m_flSwingTime = gpGlobals->time;
}