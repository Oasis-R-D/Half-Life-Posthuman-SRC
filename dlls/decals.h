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

#pragma once

//
// Dynamic Decals // Make sure to match it with world.cpp // haha the comments made a weener XD
//
enum decal_e
{
	DECAL_GUNSHOT1 = 0,
	DECAL_GUNSHOT2,
	DECAL_GUNSHOT3,
	DECAL_GUNSHOT4,
	DECAL_GUNSHOT5,
	DECAL_LAMBDA1,
	DECAL_LAMBDA2,
	DECAL_LAMBDA3,
	DECAL_LAMBDA4,
	DECAL_LAMBDA5,
	DECAL_LAMBDA6,
	DECAL_SCORCH1,
	DECAL_SCORCH2,
	DECAL_BLOOD1,
	DECAL_BLOOD2,
	DECAL_BLOOD3,
	DECAL_BLOOD4,
	DECAL_BLOOD5,
	DECAL_BLOOD6,
	DECAL_YBLOOD1,
	DECAL_YBLOOD2,
	DECAL_YBLOOD3,
	DECAL_YBLOOD4,
	DECAL_YBLOOD5,
	DECAL_YBLOOD6,
	DECAL_GLASSBREAK1,
	DECAL_GLASSBREAK2,
	DECAL_GLASSBREAK3,
	DECAL_BIGSHOT1,
	DECAL_BIGSHOT2,
	DECAL_BIGSHOT3,
	DECAL_BIGSHOT4,
	DECAL_BIGSHOT5,
	DECAL_SPIT1,
	DECAL_SPIT2,
	DECAL_BPROOF1,		// Bulletproof glass decal
	DECAL_GARGSTOMP1,	// Gargantua stomp crack
	DECAL_SMALLSCORCH1, // Small scorch mark
	DECAL_SMALLSCORCH2, // Small scorch mark
	DECAL_SMALLSCORCH3, // Small scorch mark
	DECAL_MOMMABIRTH,	// Big momma birth splatter
	DECAL_MOMMASPLAT,
	DECAL_BBLOOD1,
	DECAL_BBLOOD2,
	DECAL_BBLOOD3,
	DECAL_BLOODSPRAY1, // 45-52: For the blood drop system, human blood
	DECAL_BLOODSPRAY2,
	DECAL_BLOODSPRAY3,
	DECAL_BLOODSPRAY4,
	DECAL_BLOODSPRAY5,
	DECAL_BLOODSPRAY6,
	DECAL_BLOOD7,
	DECAL_BLOOD8,
	DECAL_ABLOODSPRAY1, // 53-59: For the blood drop system, xenian blood
	DECAL_ABLOODSPRAY2,
	DECAL_ABLOODSPRAY3,
	DECAL_ABLOODSPRAY4,
	DECAL_ABLOODSPRAY5,
	DECAL_ABLOODSPRAY6,
	DECAL_YBLOOD6_2,    // easiest way to include it in the drop system
	DECAL_XBLOODSPRAY1, // 60-66: For the blood drop system, race x blood
	DECAL_XBLOODSPRAY2,
	DECAL_XBLOODSPRAY3,
	DECAL_XBLOODSPRAY4,
	DECAL_XBLOODSPRAY5,
	DECAL_XBLOODSPRAY6,
	DECAL_XBLOODSPRAY7,
	DECAL_XBLOOD1,
	DECAL_XBLOOD2,
	DECAL_XBLOOD3,
	DECAL_XBLOOD4,
	DECAL_XBLOOD5,
	DECAL_BBLOODSPRAY1, //72-74: For the blood drop system, healing water (SPECIAL FX: HEALS IF HIT BY DROP)
	DECAL_BBLOODSPRAY2,
	DECAL_BBLOODSPRAY3,
	DECAL_OFSCORCH1,
	DECAL_OFSCORCH2,
	DECAL_OFSCORCH3,
	DECAL_NBLOODSPRAY1, //78-83: For the blood drop system, corruption juice (yum!)
	DECAL_NBLOODSPRAY2,
	DECAL_NBLOODSPRAY3,
	DECAL_NBLOODSPRAY4,
	DECAL_NBLOODSPRAY5,
	DECAL_NBLOODSPRAY6,
};

typedef struct
{
	const char* name;
	int index;
} DLL_DECALLIST;

extern DLL_DECALLIST gDecals[];
