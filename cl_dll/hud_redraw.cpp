/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// hud_redraw.cpp
//
#include "hud.h"
#include "cl_util.h"
#include "text_utils.h"

#include "vgui_TeamFortressViewport.h"

#define MAX_LOGO_FRAMES 56

int grgLogoFrame[MAX_LOGO_FRAMES] =
	{
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 14, 15,
		16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		29, 29, 29, 29, 29, 28, 27, 26, 25, 24, 30, 31};


extern bool g_iVisibleMouse;

float HUD_GetFOV();

extern float IN_GetMouseSensitivity();

// Think
void CHud::Think()
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	int newfov;
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if ((pList->p->m_iFlags & HUD_ACTIVE) != 0)
			pList->p->Think();
		pList = pList->pNext;
	}

	newfov = HUD_GetFOV();
	if (newfov == 0)
	{
		m_iFOV = default_fov->value;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if (m_iFOV == default_fov->value)
	{
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = IN_GetMouseSensitivity() * ((float)newfov / (float)V_max(default_fov->value, 90.0f)) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	// think about default fov
	if (m_iFOV == 0)
	{ // only let players adjust up in fov,  and only if they are not overriden by something else
		m_iFOV = V_max(default_fov->value, 90);
	}

	if (0 != gEngfuncs.IsSpectateOnly())
	{
		m_iFOV = gHUD.m_Spectator.GetFOV(); // default_fov->value;
	}
}

int safe_snprintf(char *buffer, int buffersize, const char *format, ...)
{
	va_list	args;
	int	result;

	if( buffersize <= 0 )
		return -1;

	va_start( args, format );
	result = _vsnprintf( buffer, buffersize, format, args );
	va_end( args );

	if( result >= buffersize )
	{
		buffer[buffersize - 1] = '\0';
		return -1;
	}

	return result;
}

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
bool CHud::Redraw(float flTime, bool intermission)
{

	m_fOldTime = m_flTime; // save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	static float m_flShotTime = 0;

	// Clock was reset, reset delta
	if (m_flTimeDelta < 0)
		m_flTimeDelta = 0;

	// Bring up the scoreboard during intermission
	if (gViewPort)
	{
		if (m_iIntermission && !intermission)
		{
			// Have to do this here so the scoreboard goes away
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideScoreBoard();
			gViewPort->UpdateSpectatorPanel();
		}
		else if (!m_iIntermission && intermission)
		{
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideVGUIMenu();
			gViewPort->ShowScoreBoard();
			gViewPort->UpdateSpectatorPanel();

			// Take a screenshot if the client's got the cvar set
			if (CVAR_GET_FLOAT("hud_takesshots") != 0)
				m_flShotTime = flTime + 1.0; // Take a screenshot in a second
		}
	}

	if (0 != m_flShotTime && m_flShotTime < flTime)
	{
		gEngfuncs.pfnClientCmd("snapshot\n");
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// if no redrawing is necessary
	// return 0;

	m_Caption.Update(flTime, m_flTimeDelta);

	// draw all registered HUD elements
	if (0 != m_pCvarDraw->value)
	{
		HUDLIST* pList = m_pHudList;

		while (pList)
		{
			if (!intermission)
			{
				if ((pList->p->m_iFlags & HUD_ACTIVE) != 0 && (m_iHideHUDDisplay & HIDEHUD_ALL) == 0)
					pList->p->Draw(flTime);
			}
			else
			{ // it's an intermission,  so only draw hud elements that are set to draw during intermissions
				if ((pList->p->m_iFlags & HUD_INTERMISSION) != 0)
					pList->p->Draw(flTime);
			}

			pList = pList->pNext;
		}
	}

	// are we in demo mode? do we need to draw the logo in the top corner?
	if (0 != m_iLogo)
	{
		int x, y, i;

		if (m_hsprLogo == 0)
			m_hsprLogo = LoadSprite("sprites/%d_logo.spr");

		SPR_Set(m_hsprLogo, 250, 250, 250);

		x = SPR_Width(m_hsprLogo, 0);
		x = ScreenWidth - x;
		y = SPR_Height(m_hsprLogo, 0) / 2;

		// Draw the logo at 20 fps
		int iFrame = (int)(flTime * 20) % MAX_LOGO_FRAMES;
		i = grgLogoFrame[iFrame] - 1;

		SPR_DrawAdditive(i, x, y, NULL);
	}

	/*
	if ( g_iVisibleMouse )
	{
		void IN_GetMousePos( int *mx, int *my );
		int mx, my;

		IN_GetMousePos( &mx, &my );
		
		if (m_hsprCursor == 0)
		{
			char sz[256];
			sprintf( sz, "sprites/cursor.spr" );
			m_hsprCursor = SPR_Load( sz );
		}

		SPR_Set(m_hsprCursor, 250, 250, 250 );
		
		// Draw the logo at 20 fps
		SPR_DrawAdditive( 0, mx, my, NULL );
	}
	*/

	if (m_pCvarShowPos && m_pCvarShowPos->value > 0)
	{
		extern Vector v_origin, v_angles;

		cl_entity_t* pl = gEngfuncs.GetLocalPlayer();

		const Vector pos = m_pCvarShowPos->value == 2 ? pl->origin : v_origin;
		const Vector ang = m_pCvarShowPos->value == 2 ? pl->angles : v_angles;
		const char* posType = m_pCvarShowPos->value == 2 ? "ent" : "view";

		const int x = ScreenWidth/2;
		int y = 4;
		const int textHeight = ConsoleText::LineHeight();
		char posBuf[256];

		safe_snprintf(posBuf, sizeof(posBuf), "pos (%s): %.2f %.2f %.2f", posType, pos.x, pos.y, pos.z);
		ConsoleText::DrawString(x, y, ScreenWidth, posBuf, 255, 255, 255);
		y += textHeight;

		safe_snprintf(posBuf, sizeof(posBuf), "ang (%s): %.2f %.2f %.2f", posType, ang.x, ang.y, ang.z);
		ConsoleText::DrawString(x, y, ScreenWidth, posBuf, 255, 255, 255);
		y += textHeight;

		safe_snprintf(posBuf, sizeof(posBuf), "velocity: %.2f", m_velocity.Length());
		ConsoleText::DrawString(x, y, ScreenWidth, posBuf, 255, 255, 255);
	}

	return true;
}

void ScaleColors(int& r, int& g, int& b, int a)
{
	float x = (float)a / 255;
	r = (int)(r * x);
	g = (int)(g * x);
	b = (int)(b * x);
}

int CHud::DrawHudString(int xpos, int ypos, int iMaxX, const char* szIt, int r, int g, int b)
{
	return xpos + gEngfuncs.pfnDrawString(xpos, ypos, szIt, r, g, b);
}

int CHud::DrawHudNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b)
{
	char szString[32];
	sprintf(szString, "%d", iNumber);
	return DrawHudStringReverse(xpos, ypos, iMinX, szString, r, g, b);
}

// draws a string from right to left (right-aligned)
int CHud::DrawHudStringReverse(int xpos, int ypos, int iMinX, const char* szString, int r, int g, int b)
{
	return xpos - gEngfuncs.pfnDrawStringReverse(xpos, ypos, szString, r, g, b);
}

int CHud::DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b)
{
	int iWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	int k;

	if (iNumber > 0)
	{
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			k = iNumber / 100;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
			SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if ((iFlags & DHN_3DIGITS) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100) / 10;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
			SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if ((iFlags & (DHN_3DIGITS | DHN_2DIGITS)) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
		SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
		x += iWidth;
	}
	else if ((iFlags & DHN_DRAWZERO) != 0)
	{
		SPR_Set(GetSprite(m_HUD_number_0), r, g, b);

		// SPR_Draw 100's
		if ((iFlags & DHN_3DIGITS) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if ((iFlags & (DHN_3DIGITS | DHN_2DIGITS)) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones

		SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0));
		x += iWidth;
	}

	return x;
}

int CHud::DrawHudNumberSm( int x, int y, int iFlags, int iNumber, int r, int g, int b)
{
	int iWidth = GetSpriteRect(m_HUD_number_sm_0).right - GetSpriteRect(m_HUD_number_sm_0).left;
	int k;
	
	if (iNumber > 0)
	{
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			 k = iNumber/100;
			SPR_Set(GetSprite(m_HUD_number_sm_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_sm_0 + k));
			x += iWidth;
		}
		else if ((iFlags & DHN_3DIGITS_SM) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}
		
		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100)/10;
			SPR_Set(GetSprite(m_HUD_number_sm_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_sm_0 + k));
			x += iWidth;
		}
		else if ((iFlags & (DHN_3DIGITS_SM | DHN_2DIGITS_SM)) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}
	
		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set(GetSprite(m_HUD_number_sm_0 + k), r, g, b );
		SPR_DrawAdditive(0,  x, y, &GetSpriteRect(m_HUD_number_sm_0 + k));
		x += iWidth;
	}
	else if ((iFlags & DHN_DRAWZERO_SM) != 0) 
	{
		// SPR_Draw 100's
		if ((iFlags & DHN_3DIGITS_SM) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if ((iFlags & (DHN_3DIGITS_SM | DHN_2DIGITS_SM)) != 0)
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		SPR_Set(GetSprite(m_HUD_number_sm_0), r, g, b );
		SPR_DrawAdditive(0,  x, y, &GetSpriteRect(m_HUD_number_sm_0));
		x += iWidth;
	}

	return x;
}

int CHud::GetNumWidth(int iNumber, int iFlags)
{
	if ((iFlags & (DHN_3DIGITS)) != 0)
		return 3;

	if ((iFlags & (DHN_3DIGITS_SM)) != 0)
		return 3;

	if ((iFlags & (DHN_2DIGITS)) != 0)
		return 2;

	if ((iFlags & (DHN_2DIGITS_SM)) != 0)
		return 2;

	if (iNumber <= 0)
	{
		if ((iFlags & (DHN_DRAWZERO)) != 0)
			return 1;

		if ((iFlags & (DHN_DRAWZERO_SM)) != 0)
			return 1;

		else
			return 0;
	}

	if (iNumber < 10)
		return 1;

	if (iNumber < 100)
		return 2;

	return 3;
}

int CHud::GetHudNumberWidth(int number, int width, int flags)
{
	const int digitWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;

	int totalDigits = 0;

	if (number > 0)
	{
		totalDigits = static_cast<int>(log10(number)) + 1;
	}
	else if ((flags & DHN_DRAWZERO) != 0)
	{
		totalDigits = 1;
	}

	totalDigits = V_max(totalDigits, width);

	return totalDigits * digitWidth;
}

int CHud::DrawHudNumberReverse(int x, int y, int number, int flags, int r, int g, int b)
{
	if (number > 0 || (flags & DHN_DRAWZERO) != 0)
	{
		const int digitWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;

		int remainder = number;

		do
		{
			const int digit = remainder % 10;
			const int digitSpriteIndex = m_HUD_number_0 + digit;

			//This has to happen *before* drawing because we're drawing in reverse
			x -= digitWidth;

			SPR_Set(GetSprite(digitSpriteIndex), r, g, b);
			SPR_DrawAdditive(0, x, y, &GetSpriteRect(digitSpriteIndex));

			remainder /= 10;
		} while (remainder > 0);
	}

	return x;
}


int CHud::ConsoleText::DrawString(int xpos, int ypos, int iMaxX, const char *szString, int r, int g, int b, int length)
{
	char buf[512] = {0};
	const char* str = buf;

	if (length < 0) {
		str = szString;
	} else {
		length = V_min(length, sizeof(buf) - 1);
		strncpy(buf, szString, length);
		buf[length] = '\0';
	}

	gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
	return DrawConsoleString(xpos, ypos, str);
}

int CHud::ConsoleText::DrawString(int xpos, int ypos, const char *szString, int r, int g, int b, int length)
{
	return DrawString(xpos, ypos, ScreenWidth, szString, r, g, b, length);
}

int CHud::ConsoleText::DrawNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b)
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawStringReverse( xpos, ypos, iMinX, szString, r, g, b );
}

int CHud::ConsoleText::DrawFloatNumberString(int xpos, int ypos, int iMinX, float number, int r, int g, int b)
{
	char szString[32];
	sprintf( szString, "%g", number );
	return DrawStringReverse( xpos, ypos, iMinX, szString, r, g, b );
}

int CHud::ConsoleText::DrawStringReverse(int x, int ypos, int iMinX, const char *szString, int r, int g, int b, int length)
{
	x -= LineWidth(szString, length);
	if (x < iMinX)
		x = iMinX;
	return DrawString(x, ypos, ScreenWidth, szString, r, g, b, length);
}

int CHud::ConsoleText::LineWidth(const char *szString, int length)
{
	char buf[1024] = {0};
	const char* str = buf;

	if (length < 0) {
		str = szString;
	} else {
		length = V_min(length, sizeof(buf) - 1);
		strncpy(buf, szString, length);
		buf[length] = '\0';
	}

	int width, height;
	gEngfuncs.pfnDrawConsoleStringLen(str, &width, &height);
	return width;
}

int CHud::ConsoleText::WidestCharacterWidth()
{
	int width, height;
	gEngfuncs.pfnDrawConsoleStringLen("M", &width, &height);
	return width;
}

int CHud::ConsoleText::LineHeight()
{
	int width, height;
	gEngfuncs.pfnDrawConsoleStringLen("YAW", &width, &height);
	return height;
}

int CHud::ConsoleText::DrawMultiLineString(const char *str, int xpos, int ypos, int xmax, const int LineHeight, int r, int g, int b)
{
	const char *ch = str;
	while(*ch)
	{
		const char *next_line = ch;
		for(; *next_line != '\n' && *next_line != '\0'; next_line++)
			;

		const int lineLength = next_line - ch;
		if (lineLength > 0)
		{
			const int lineWidth = CHud::UtfText::LineWidth(ch, lineLength);
			const int numberOfLines = (lineWidth + xmax - xpos - 1) / (xmax - xpos);

			int lineLengthRest = lineLength;
			for (int i=0; i<numberOfLines; ++i)
			{
				int renderLineLength = i == 0 ? (lineLength - lineLength/numberOfLines * (numberOfLines-1)) : V_min(lineLength/numberOfLines, lineLengthRest);
				if (renderLineLength > 0)
				{
					while(isalpha(ch[renderLineLength]) || ch[renderLineLength] == '_' || isdigit(ch[renderLineLength]))
						renderLineLength++;
					if (ch[renderLineLength] == '\'' && isalpha(ch[renderLineLength+1]))
						renderLineLength += 2;
					if (ch[renderLineLength] == '"')
						renderLineLength++;
					if (ch[renderLineLength] == ':')
						renderLineLength++;

					lineLengthRest -= renderLineLength;

					if (i > 0)
					{
						while(isspace(*ch))
						{
							++ch;
							--renderLineLength;
						}
					}

					CHud::UtfText::DrawString( xpos, ypos, xmax, ch, r, g, b, renderLineLength );
					ypos += LineHeight;
					ch += renderLineLength;
				}
			}
		}

		ch = next_line;
		if (*ch == '\n')
			ch++;
	}
	return ypos;
}

std::vector<std::pair<int, int>> CHud::ConsoleText::CalcLineOffsets(const char* str, int maxwidth)
{
	std::vector<std::pair<int, int>> lineOffsets;

	WordBoundaries boundaries = SplitIntoWordBoundaries(str);

	unsigned int startWordIndex = 0;
	for (unsigned int j=0; j<boundaries.size();)
	{
		const int width = CHud::UtfText::LineWidth(str + boundaries[startWordIndex].wordStart, boundaries[j].wordEnd - boundaries[startWordIndex].wordStart);
		if (width > maxwidth) {
			if (j == startWordIndex) {
				lineOffsets.push_back(std::make_pair(boundaries[startWordIndex].wordStart, boundaries[startWordIndex].wordEnd));
				startWordIndex = ++j;
			} else {
				lineOffsets.push_back(std::make_pair(boundaries[startWordIndex].wordStart, boundaries[j-1].wordEnd));
				startWordIndex = j;
			}
		} else {
			if (j == boundaries.size() - 1) {
				lineOffsets.push_back(std::make_pair(boundaries[startWordIndex].wordStart, boundaries[j].wordEnd));
			}
			else if (boundaries[j].newline){
				lineOffsets.push_back(std::make_pair(boundaries[startWordIndex].wordStart, boundaries[j].wordEnd));
				startWordIndex = j+1;
			}

			++j;
		}
	}

	return lineOffsets;
}