#include "gibs.h"

static const char* human_gibmap[][3] = // MDL, BG, AMNT
{
		{"models/hgibs.mdl", "0", "1"},
		{"models/hgibs.mdl", "1", "1"},
		{"models/hgibs.mdl", "2", "1"},
		{"models/hgibs.mdl", "3", "1"},
		{"models/hgibs.mdl", "4", "1"},
		{"models/hgibs.mdl", "5", "1"},
};

static const char* xenian_gibmap[][3] = // MDL, BG, AMNT
{
		{"models/agibs.mdl", "0", "1"},
		{"models/agibs.mdl", "1", "1"},
		{"models/agibs.mdl", "2", "1"},
		{"models/agibs.mdl", "3", "1"},
};

struct DefaultGibs
{
	const int MaxGibs;
};

// HACKHACK -- The gib velocity equations don't work
void CoolerGib::LimitVelocity()
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if (length > 1500.0)
		pev->velocity = pev->velocity.Normalize() * 1500; // This should really be sv_maxvelocity * 0.75 or something
}


void CoolerGib::SpawnStickyGibs(entvars_t* pevVictim, Vector vecOrigin, int coolerGibs)
{
	int i;

	for (i = 0; i < coolerGibs; i++)
	{
		CoolerGib* pGib = GetClassPtr((CoolerGib*)NULL);

		pGib->Spawn("models/stickygib.mdl");
		pGib->pev->body = RANDOM_LONG(0, 2);

		if (pevVictim)
		{
			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT(0, 1));
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT(0, 1));
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT(0, 1)) + 1; // absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity.y += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity.z += RANDOM_FLOAT(-0.25, 0.25);

			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT(300, 400);

			pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
			pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

			// copy owner's blood color
			pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();

			if (pevVictim->health > -50)
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			}
			else if (pevVictim->health > -200)
			{
				pGib->pev->velocity = pGib->pev->velocity * 2;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4;
			}


			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize(pGib->pev, Vector(0, 0, 0), Vector(0, 0, 0));
			pGib->SetTouch(&CoolerGib::StickyGibTouch);
			pGib->SetThink(NULL);

		}
		UTIL_VecToAngles(pGib->pev->velocity);
		#ifndef CLIENT_DLL
		CPhysblood::BloodCreate(2, 350, pGib->pev->origin, gpGlobals->v_forward, 1, pGib->m_bloodColor);
		#endif
		pGib->LimitVelocity();
	}
}

void CoolerGib::SpawnHeadGib(entvars_t* pevVictim)
{
	CoolerGib* pGib = GetClassPtr((CoolerGib*)NULL);

	pGib->Spawn("models/hgibs.mdl", 0); // throw one head

	if (pevVictim)
	{
		pGib->pev->origin = pevVictim->origin + pevVictim->view_ofs;

		edict_t* pentPlayer = FIND_CLIENT_IN_PVS(pGib->edict());

		if (RANDOM_LONG(0, 100) <= 5 && pentPlayer)
		{
			// 5% chance head will be thrown at player's face.
			entvars_t* pevPlayer;

			pevPlayer = VARS(pentPlayer);
			pGib->pev->velocity = ((pevPlayer->origin + pevPlayer->view_ofs) - pGib->pev->origin).Normalize() * 300;
			pGib->pev->velocity.z += 100;
		}
		else
		{
			pGib->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
		}


		pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
		pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

		// copy owner's blood color
		pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();

		if (pevVictim->health > -50)
		{
			pGib->pev->velocity = pGib->pev->velocity * 0.7;
		}
		else if (pevVictim->health > -200)
		{
			pGib->pev->velocity = pGib->pev->velocity * 2;
		}
		else
		{
			pGib->pev->velocity = pGib->pev->velocity * 4;
		}

	}
	UTIL_VecToAngles(pGib->pev->velocity);
	#ifndef CLIENT_DLL
	CPhysblood::BloodCreate(2, 350, pGib->pev->origin, gpGlobals->v_forward, 1, pGib->m_bloodColor);
	#endif
	pGib->LimitVelocity();
}

void CoolerGib::SpawnRandomGibs(entvars_t* pevVictim, int coolerGibs, const char (*GibData)[dataamnt], int rows)
{
	int i;
	for (i = 0; i < atoi(GibData[3][1]); i++)
	{
		CoolerGib* pGib = GetClassPtr((CoolerGib*)NULL);
		pGib->Spawn(GibData[1][1], atoi(GibData[2][1]));
		if (pevVictim)
		{
			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT(0, 1));
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT(0, 1));
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT(0, 1)) + 1; // absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity.y += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity.z += RANDOM_FLOAT(-0.25, 0.25);

			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT(300, 400);

			pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
			pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

			// copy owner's blood color
			pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();

			if (pevVictim->health > -50)
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			}
			else if (pevVictim->health > -200)
			{
				pGib->pev->velocity = pGib->pev->velocity * 2;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4;
			}

			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize(pGib->pev, Vector(0, 0, 0), Vector(0, 0, 0));
		}
		UTIL_VecToAngles(pGib->pev->velocity);
		#ifndef CLIENT_DLL
		CPhysblood::BloodCreate(2, 350, pGib->pev->origin, gpGlobals->v_forward, 1, pGib->m_bloodColor);
		#endif
		pGib->LimitVelocity();
	}
}

// start at one to avoid throwing random amounts of skulls (0th gib)
//const CoolerGibData HumanGibs = {"models/hgibs.mdl", 1, HUMAN_GIB_COUNT};
//const CoolerGibData AlienGibs = {"models/agibs.mdl", 0, ALIEN_GIB_COUNT};

void CoolerGib::SpawnHL1Gibs(entvars_t* pevVictim, int coolerGibs, bool human)
{
	SpawnRandomGibs(pevVictim, coolerGibs, human ? human_gibmap : xenian_gibmap, 60);
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CoolerGib::WaitTillLand()
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->velocity == g_vecZero)
	{
		if (pev->armortype == 0)
		{
			SetThink(&CoolerGib::SUB_StartFadeOut);
			pev->nextthink = gpGlobals->time + m_lifeTime;
		}

		// If you bleed, you stink!
		if (m_bloodColor != DONT_BLEED)
		{
			// ok, start stinkin!
			CSoundEnt::InsertSound(bits_SOUND_MEAT, pev->origin, 384, 25);
		}
	}
	else
	{
		if (g_Language != LANGUAGE_GERMAN && m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED && (RANDOM_LONG(0, 1) == 1))
		{
			PLAYBACK_EVENT_FULL(0, edict(), g_sParticleEvent, 0.0, pev->origin, g_vecZero, 0.0, 0.0, PE_NPCIMPACTCLUST, m_bloodColor, 0, 0);
		}
		pev->nextthink = gpGlobals->time + 0.25; // WAS 0.1
	}
}

//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CoolerGib::BounceGibTouch(CBaseEntity* pOther)
{
	Vector vecSpot;
	TraceResult tr;

	//if ( RANDOM_LONG(0,1) )
	//	return;// don't bleed everytime

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		pev->velocity = pev->velocity * 0.9;
		pev->angles.x = 0;
		pev->angles.z = 0;
		pev->avelocity.x = 0;
		pev->avelocity.z = 0;
	}
	else
	{
		if (m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED)
		{
			vecSpot = pev->origin + Vector(0, 0, 8); //move up a bit, and trace down.
			UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -24), ignore_monsters, ENT(pev), &tr);

			UTIL_BloodDecalTrace(&tr, m_bloodColor);

			m_cBloodDecals--;
		}
	}
}

//
// Sticky gib puts blood on the wall and stays put.
//
void CoolerGib::StickyGibTouch(CBaseEntity* pOther)
{
	Vector vecSpot;
	TraceResult tr;

	SetThink(&CoolerGib::SUB_Remove);
	pev->nextthink = gpGlobals->time + 10;

	if (!FClassnameIs(pOther->pev, "worldspawn"))
	{
		pev->nextthink = gpGlobals->time;
		return;
	}

	UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 32, ignore_monsters, ENT(pev), &tr);

	UTIL_BloodDecalTrace(&tr, m_bloodColor);

	pev->velocity = tr.vecPlaneNormal * -1;
	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

//
// Throw a chunk
//
void CoolerGib::Spawn(const char* szGibModel, int body)
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55; // deading the bounce a bit

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_SLIDEBOX; /// hopefully this will fix the VELOCITY TOO LOW crap
	pev->classname = MAKE_STRING("gib");

	SET_MODEL(ENT(pev), szGibModel);
	pev->body = body;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	pev->nextthink = gpGlobals->time + 0.1;
	m_lifeTime = 25;
	SetThink(&CoolerGib::WaitTillLand);
	SetTouch(&CoolerGib::BounceGibTouch);
	SetThink(&CoolerGib::WaitTillLand);
	SetTouch(&CoolerGib::BounceGibTouch);
}