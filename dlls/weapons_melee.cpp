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

#define MELEE_EYEOFFSET 8

void CBasePlayerMelee::StartSwing(float length, int amount, Vector2D start, Vector2D end)
{
    m_flSwingStart = gpGlobals->time;
    m_flSwingEnd = gpGlobals->time + length;

    m_flRaycastDelay = m_iRaycastAmount / length;
    m_iRaycastAmount = amount;
    m_iRaycastsComplete = 0;

    m_2DvecStartAng = start;
    m_2DvecEndAng = end;

	ALERT(at_console, "START SWING\n");
}

void CBasePlayerMelee::EndSwing()
{
	m_flSwingStart = 0;
    m_flSwingEnd = 0;

    m_flRaycastDelay = 0;
    m_iRaycastAmount = 0;
    m_iRaycastsComplete = 0;
}

Vector CBasePlayerMelee::GetSwingDir()
{
	Vector2D Raycast2DAng = Vector2DLerp(m_2DvecStartAng, m_2DvecEndAng, (float)m_iRaycastsComplete / m_iRaycastAmount);

    // Convert to radians
    Raycast2DAng.x = Raycast2DAng.x * 0.01745329251994329576923690768489;
    Raycast2DAng.y = Raycast2DAng.y * 0.01745329251994329576923690768489;

    // Calculate 3D vector dir
    float forwardWeight = cos(Raycast2DAng.x) * cos(Raycast2DAng.y);
    float rightWeight   = sin(Raycast2DAng.x) * cos(Raycast2DAng.y);
    float upWeight      = sin(Raycast2DAng.y);

    Vector RaycastDir = (gpGlobals->v_forward * forwardWeight) 
                      + (gpGlobals->v_right * rightWeight) 
                      + (gpGlobals->v_up * upWeight);

    return RaycastDir.Normalize();
}

void CBasePlayerMelee::SwingRaycast()
{
    m_flSwingTime = gpGlobals->time; // useless?

    if (m_flSwingEnd < gpGlobals->time)
        return;

    float elapsed = gpGlobals->time - m_flSwingStart;
	ALERT(at_console, "SWING %f\n", elapsed);
    // Should work?
    if (floor(elapsed / m_flRaycastDelay) >= m_iRaycastsComplete)
        return; // not time for another raycast yet

	m_iRaycastsComplete++;

    
    Vector RaycastStart = m_pPlayer->GetGunPosition() - gpGlobals->v_forward * MELEE_EYEOFFSET;

    Vector DebugVec = RaycastStart + GetSwingDir() * 64;

#ifndef CLIENT_DLL
    UTIL_DebugPoint(DebugVec);
#endif // !CLIENT_DLL

	if (m_iRaycastsComplete >= m_iRaycastAmount)
		EndSwing();
}