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

void CBasePlayerMelee::StartSwing(float length, float delay, int amount, Vector2D start, Vector2D end)
{
    m_flSwingStart = gpGlobals->time + delay;
    m_flSwingEnd = gpGlobals->time + length + delay;

    m_iRaycastAmount = amount;
    m_iRaycastsComplete = 0;
	m_flRaycastDelay = length / m_iRaycastAmount;

    m_2DvecStartAng = start;
    m_2DvecEndAng = end;

	ALERT(at_console, "START SWING\n");
}

void CBasePlayerMelee::EndSwing()
{
	m_flSwingStart = -1;
    m_flSwingEnd = -1;

    m_flRaycastDelay = 0;
    m_iRaycastAmount = 0;
    m_iRaycastsComplete = 0;

	ALERT(at_console, "END SWING\n");
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

void GetHullIntersection(const Vector& vecSrc, TraceResult& tr, const Vector& mins, const Vector& maxs, edict_t* pEntity)
{
	int i, j, k;
	float distance;
	const Vector* minmaxs[2] = {&mins, &maxs};
	TraceResult tmpTrace;
	Vector vecHullEnd = tr.vecEndPos;
	Vector vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine(vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace);
	if (tmpTrace.flFraction < 1.0)
	{
		tr = tmpTrace;
		return;
	}

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			for (k = 0; k < 2; k++)
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i]->x;
				vecEnd.y = vecHullEnd.y + minmaxs[j]->y;
				vecEnd.z = vecHullEnd.z + minmaxs[k]->z;

				UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace);
				if (tmpTrace.flFraction < 1.0)
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if (thisDistance < distance)
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

void CBasePlayerMelee::SwingRaycast()
{
#ifndef CLIENT_DLL
	if (m_flSwingEnd < 0)
        return; // Not in swing

	if (gpGlobals->time < m_flSwingStart)
		return; // In delay

    float elapsed = gpGlobals->time - m_flSwingStart;
	int raycastsNeeded = floor(elapsed / m_flRaycastDelay);

	if (raycastsNeeded <= m_iRaycastsComplete)
		return; // Not time to raycast
	 
	// Hit multiple times a frame to catch up when behind
	while (m_iRaycastsComplete < raycastsNeeded &&
		   m_iRaycastsComplete < m_iRaycastAmount)
	{
		m_iRaycastsComplete++;

		TraceResult tr;

		Vector raycastStart = m_pPlayer->GetGunPosition() - gpGlobals->v_forward * MELEE_EYEOFFSET;
		Vector raycastEnd = raycastStart + GetSwingDir() * 72;

		UTIL_TraceLine(raycastStart, raycastEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction >= 1.0)
		{
			UTIL_TraceHull(raycastStart, raycastEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
			if (tr.flFraction < 1.0)
			{
				// Calculate the point of intersection of the line (or hull) and the object we hit
				// This is and approximation of the "best" intersection
				CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
				if (!pHit || pHit->IsBSPModel())
					GetHullIntersection(raycastStart, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			}
		}

		raycastEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		UTIL_DebugPoint(raycastEnd, Vector(255, 0, 0));

		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity)
		{
			ClearMultiDamage();
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_SLASH);
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}

		if (m_iRaycastsComplete >= m_iRaycastAmount)
			EndSwing();
	}
#endif
}