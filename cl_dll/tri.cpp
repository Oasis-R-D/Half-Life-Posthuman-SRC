//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

#include "particleman.h"
#include "tri.h"
extern IParticleMan* g_pParticleMan;

// RENDERERS START
#include "../renderer/bsprenderer.h"
#include "../renderer/propmanager.h"
#include "../renderer/particle_engine.h"
#include "../renderer/watershader.h"
#include "../renderer/mirrormanager.h"

#include "studio.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
	//	RecClDrawNormalTriangles();

	#ifdef TRINITY
	// RENDERERS START
	// 2012-02-25
	R_DrawNormalTriangles();
	// RENDERERS END
	#endif

	gHUD.m_Spectator.DrawOverview();
}


/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
	//	RecClDrawTransparentTriangles();

	#ifdef TRINITY
	// RENDERERS START
	// 2012-02-25
	R_DrawTransparentTriangles();
	// RENDERERS END
	#endif


	if (g_pParticleMan)
		g_pParticleMan->Update();
}
