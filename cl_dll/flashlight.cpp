/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

extern bool g_iNightVision;
extern bool g_iFlashLight;

bool prehuman;

#define HUNGERSTATE_ENTER 2
#define HUNGERSTATE_IDLEIN 1
#define HUNGERSTATE_IDLEOUT 0
#define HUNGERSTATE_EXIT 3

DECLARE_MESSAGE(m_Flash, FlashBat)
DECLARE_MESSAGE(m_Flash, Flashlight)
DECLARE_MESSAGE(m_Flash, Hunger)

#define BAT_NAME "sprites/%d_Flashlight.spr"

bool CHudFlashlight::Init()
{
	m_fHungerState == HUNGERSTATE_IDLEOUT;
	m_iOldHunger = -1;
	m_fFade = 0;
	m_fOn = false;

	HOOK_MESSAGE(Flashlight);
	HOOK_MESSAGE(FlashBat);
	HOOK_MESSAGE(Hunger);

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return true;
}

void CHudFlashlight::Reset()
{
	m_fFade = 0;
	m_fOn = false;
}

bool CHudFlashlight::VidInit()
{
	int HUD_flash_empty = gHUD.GetSpriteIndex("flash_empty");
	int HUD_flash_full = gHUD.GetSpriteIndex("flash_full");
	int HUD_flash_beam = gHUD.GetSpriteIndex("flash_beam");

	m_hSprite1 = gHUD.GetSprite(HUD_flash_empty);
	m_hSprite2 = gHUD.GetSprite(HUD_flash_full);
	m_hBeam = gHUD.GetSprite(HUD_flash_beam);
	m_prc1 = &gHUD.GetSpriteRect(HUD_flash_empty);
	m_prc2 = &gHUD.GetSpriteRect(HUD_flash_full);
	m_prcBeam = &gHUD.GetSpriteRect(HUD_flash_beam);
	m_iWidth = m_prc2->right - m_prc2->left;

	return true;
}

bool CHudFlashlight::MsgFunc_FlashBat(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ((float)x) / 100.0;

	return true;
}

bool CHudFlashlight::MsgFunc_Flashlight(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_fOn = READ_BYTE() != 0;
	int x = READ_BYTE();
	prehuman = READ_BYTE();
	m_iBat = x;
	m_flBat = ((float)x) / 100.0;

	return true;
}

bool CHudFlashlight::MsgFunc_Hunger(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	int value = READ_SHORT();
	m_iOldHunger = (m_iOldHunger == -1) ? value : m_iHunger;
	m_iHunger = value;

	return true;
}

extern char * UTIL_VarArgs_client(const char* format, ...);


bool CHudFlashlight::Draw(float flTime)
{
	if (!prehuman)
	{
		g_iNightVision = m_fOn;
		g_iFlashLight = false;
	}
	else
	{
		g_iFlashLight = m_fOn;
		g_iNightVision = false;
	}

	if (prehuman) // draw normal flashlight hud
	{
		if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_FLASHLIGHT | HIDEHUD_ALL)) != 0)
			return true;

		if (!gHUD.HasSuit())
			return true;

		int r, g, b, x, y, a;
		Rect rc;

		if (m_fOn)
			a = 225;
		else
			a = MIN_ALPHA;

		if (gHUD.FlashingHUD > 0)
		{
			a = (int)(fabs(sin(flTime * gEngfuncs.pfnRandomLong(10, 20))) * 256.0);
			m_flBat = (fabs(sin(flTime * gEngfuncs.pfnRandomLong(10, 20))) * 1); // make the values go haywire
		}

		if (m_flBat < 0.20)
			UnpackRGB(r, g, b, RGB_REDISH);
		else
			UnpackRGB(r, g, b, RGB_YELLOWISH);

		ScaleColors(r, g, b, a);

		y = (m_prc1->bottom - m_prc2->top) / 2;
		x = ScreenWidth - m_iWidth - m_iWidth / 2;

		// Draw the flashlight casing
		SPR_Set(m_hSprite1, r, g, b);
		SPR_DrawAdditive(0, x, y, m_prc1);

		if (m_fOn)
		{ // draw the flashlight beam
			x = ScreenWidth - m_iWidth / 2;

			SPR_Set(m_hBeam, r, g, b);
			SPR_DrawAdditive(0, x, y, m_prcBeam);
		}

		// draw the flashlight energy level
		x = ScreenWidth - m_iWidth - m_iWidth / 2;
		int iOffset = m_iWidth * (1.0 - m_flBat);
		if (iOffset < m_iWidth)
		{
			rc = *m_prc2;
			rc.left += iOffset;

			SPR_Set(m_hSprite2, r, g, b);
			SPR_DrawAdditive(0, x + iOffset, y, &rc);
		}
	}
	else // draw hunger hud
	{
		if ((gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) != 0)
			return true;

		int r, g, b, x, y, a;
		Rect rc;

		// check if hud needs to enter or exit
		if ((m_fHungerState == HUNGERSTATE_IDLEOUT) && (m_iHunger <= 10 || m_iHunger % 2 == 0 || (abs(m_iHunger - m_iOldHunger)) > 5)) // show hud
		{
			m_fHungerState = HUNGERSTATE_ENTER;
			m_fHungerTimeStart = flTime;
		}
		else if (m_fHungerState == HUNGERSTATE_IDLEIN && !(m_iHunger <= 10 || m_iHunger % 2 == 0))
		{
			m_fHungerState = HUNGERSTATE_EXIT;
			m_fHungerTimeStart = flTime;
		}

		if (m_fHungerState == HUNGERSTATE_IDLEOUT)
			return true; // doesn't need drawn

		a = 225;

		UnpackRGB(r, g, b, RGB_REDISH);

		ScaleColors(r, g, b, a);

		float timedif = flTime - m_fHungerTimeStart;
		float pos = ScreenHeight / 32;

		switch(m_fHungerState)
		{
			case HUNGERSTATE_EXIT:
			{
				double math = ((3*pow(timedif, 2)) - (2*pow(timedif, 3)));
				if (timedif < 1)
				{
					float Ybuffer = (-math * pos + pos) - (m_prc1->bottom + m_prc2->top);
					y = round(Ybuffer);
				}
				else
				{
					m_fHungerState = HUNGERSTATE_IDLEOUT;
					return true;
				}
				break;
			}
			case HUNGERSTATE_ENTER: 
			{
				double math = ((3*pow(timedif, 2)) - (2*pow(timedif, 3)));
				if (timedif < 1)
				{
					float Ybuffer = (math * pos) - (m_prc1->bottom + m_prc2->top);
					y = round(Ybuffer);
				}
				else
				{
					m_fHungerState = HUNGERSTATE_IDLEIN;
					y = -(m_prc1->bottom + m_prc2->top)/2;
				}
				break;
			}
			case HUNGERSTATE_IDLEIN: y = -(m_prc1->bottom + m_prc2->top)/2; break;
		}

		//gEngfuncs.pfnCenterPrint(UTIL_VarArgs_client("%d, %d, %d, %d\n", m_iHunger, m_fHungerState, y, (abs(m_iHunger - m_iOldHunger))));
		x = ScreenWidth - m_iWidth - m_iWidth / 2;

		// Draw the flashlight casing
		SPR_Set(m_hSprite1, r, g, b);
		SPR_DrawAdditive(0, x, y, m_prc1);

		// draw the flashlight energy level
		x = ScreenWidth - m_iWidth - m_iWidth / 2;
		float buff = m_iHunger / 100;
		int iOffset = m_iWidth * (buff-1.0);
		if (iOffset < m_iWidth)
		{
			rc = *m_prc2;
			rc.left += iOffset;

			SPR_Set(m_hSprite2, r, g, b);
			SPR_DrawAdditive(0, x + iOffset, y, &rc);
		}
	}

	return true;
}
