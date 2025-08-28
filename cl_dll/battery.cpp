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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Battery, Battery)
DECLARE_MESSAGE(m_Battery, Hunger)
DECLARE_MESSAGE(m_Battery, FireMode)
DECLARE_MESSAGE(m_Battery, LimbDMG)

bool CHudBattery::Init()
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;
	m_iHealth_head = 0;
	m_iHealth_chest = 0;
	m_iHealth_Larm = 0; // Make sure to make this opposite (Armhealth = 100 - Armhealth) since it counts down from 100 for the arms instead of counting up
	m_iHealth_Rarm = 0; // Make sure to make this opposite (Armhealth = 100 - Armhealth) since it counts down from 100 for the arms instead of counting up
	m_iHealth_stmch = 0;
	m_iHealth_Lleg = 0;	
	m_iHealth_Rleg = 0;	

	HOOK_MESSAGE(Battery);
	HOOK_MESSAGE(Hunger);
	HOOK_MESSAGE(FireMode);
	HOOK_MESSAGE(LimbDMG);

	gHUD.AddHudElem(this);

	return true;
}


bool CHudBattery::VidInit()
{
	int HUD_suit_empty = gHUD.GetSpriteIndex("suit_empty");
	int HUD_suit_full = gHUD.GetSpriteIndex("suit_full");
	int HUD_suit_dmg = gHUD.GetSpriteIndex("limb_dmgs");
	m_hSprite1 = m_hSprite2 = 0; // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect(HUD_suit_empty);
	m_prc2 = &gHUD.GetSpriteRect(HUD_suit_full);
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;
	m_iHunger = 0;
	m_iFireMode = 0;

	int HUD_FireMode = gHUD.GetSpriteIndex("firemode");;
	m_rFireMode = &gHUD.GetSpriteRect(HUD_FireMode);

	int HUD_Hunger = gHUD.GetSpriteIndex("hud_hunger");
	m_rHunger = &gHUD.GetSpriteRect(HUD_Hunger);

	return true;
}

bool CHudBattery::MsgFunc_Battery(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	int x = READ_SHORT();

	if (x != m_iBat)
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}

	return true;
}

bool CHudBattery::MsgFunc_Hunger(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	m_iHunger = READ_SHORT();

	return true;
}

bool CHudBattery::MsgFunc_FireMode(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	m_iFireMode = READ_SHORT();

	return true;
}

bool CHudBattery::MsgFunc_LimbDMG(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	m_iHealth_head = READ_BYTE();

	return true;
}

bool CHudBattery::Draw(float flTime)
{
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) != 0)
		return true;

	if (!gHUD.HasSuit())
		return true;

	int r, g, b, x, y, a;
	Rect rc;

	if (m_iHunger > 10)
	{
		UnpackRGB(r, g, b, RGB_YELLOWISH);
		a = MIN_ALPHA;
	}
	else
	{
		UnpackRGB(r, g, b, RGB_REDISH);
		a = 255;
		if (m_iHunger <= 0)
			a = (int)(fabs(sin(flTime * 20)) * 256.0);
	}

	if (gHUD.FlashingHUD > 0)
		a = (int)(fabs(sin(flTime * gEngfuncs.pfnRandomLong(10, 20))) * 256.0);

	ScaleColors(r, g, b, a);

	if (m_iHunger < 1000)
	{
		x = (m_prc1->right - m_prc1->left) * 6;
		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2 - (m_rHunger->bottom - m_rHunger->top) / 3;

		m_hHunger = gHUD.GetSpriteIndex("hud_hunger");
		SPR_Set(gHUD.GetSprite(m_hHunger), r, g, b);
		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_hHunger));

		//m_hHunger = gHUD.GetSprite(gHUD.GetSpriteIndex("hud_hunger"));
		//SPR_Set(m_hHunger, r, g, b);
		//SPR_DrawAdditive(0, x, y, m_rHunger);

		x += (m_prc1->right - m_prc1->left);
		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
		x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iHunger, r, g, b);

		x += (m_prc1->right - m_prc1->left) / 2;
		y += gHUD.m_iFontHeight / 4;
		FillRGBA(x, y, m_iHunger, gHUD.m_iFontHeight / 2, r, g, b, a);
	}

	if (m_iBat <= 0)
		return true;

	rc = *m_prc2;
	rc.top += m_iHeight * ((float)(100 - (V_min(100, m_iBat))) * 0.01); // battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	UnpackRGB(r, g, b, RGB_YELLOWISH);

	// Has health changed? Flash the health #
	if (0 != m_fFade)
	{
		if (m_fFade > FADE_TIME)
			m_fFade = FADE_TIME;

		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if (m_fFade <= 0)
		{
			a = 128;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA + (m_fFade / FADE_TIME) * 128;
	}
	else
		a = MIN_ALPHA;

	if (gHUD.FlashingHUD > 0)
		a = (int)(fabs(sin(flTime * gEngfuncs.pfnRandomLong(10, 20))) * 256.0);

	ScaleColors(r, g, b, a);

	int iOffset = (m_prc1->bottom - m_prc1->top) / 6;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = (m_prc1->right - m_prc1->left) * 3;

	// make sure we have the right sprite handles
	if (0 == m_hSprite1)
		m_hSprite1 = gHUD.GetSprite(gHUD.GetSpriteIndex("suit_empty"));
	if (0 == m_hSprite2)
		m_hSprite2 = gHUD.GetSprite(gHUD.GetSpriteIndex("suit_full"));

	SPR_Set(m_hSprite1, r, g, b);
	SPR_DrawAdditive(0, x, y - iOffset, m_prc1);

	if (rc.bottom > rc.top)
	{
		SPR_Set(m_hSprite2, r, g, b);
		SPR_DrawAdditive(0, x, y - iOffset + (rc.top - m_prc2->top), &rc);
	}

	x += (m_prc1->right - m_prc1->left);
	x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);

	// Fire Mode
	if (m_iFireMode > 0)
	{
		x = 0;
		y = 0;

		m_hFireMode = gHUD.GetSprite(gHUD.GetSpriteIndex("firemode"));
		SPR_Set(m_hFireMode, r, g, b);
		if (m_iFireMode == 1)
			SPR_DrawAdditive(0, x, y - iOffset, m_rFireMode);
		else
			SPR_DrawAdditive(1, x, y - iOffset, m_rFireMode);
	}

	return true;
}
