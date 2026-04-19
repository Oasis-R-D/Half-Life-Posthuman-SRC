#include "gibs.h"

LINK_ENTITY_TO_CLASS(cool_gib, CoolerGib);
// START NPC GIB LISTS
// TYPES: 0 || null - default, 1 - head, 2 - sticky

// TO-DO: fix alien gibs being tiny (make small ones headcrab only)
std::vector<gib_data_t> xenian_gibmap =
{		// 			MDL 			   BG # TYPE
		gib_data_t{"models/agibs.mdl", 0, 1},
		gib_data_t{"models/agibs.mdl", 1, 1},
		gib_data_t{"models/agibs.mdl", 2, 1},
		gib_data_t{"models/agibs.mdl", 3, 1},
		gib_data_t{"models/agibs.mdl", 4, 1},
		gib_data_t{"models/agibs.mdl", 5, 1},
};

std::vector<gib_data_t> human_gibmap =
{		// 			MDL 			   BG # TYPE
		gib_data_t{"models/hgibs.mdl", 0, 1, 1},
		gib_data_t{"models/hgibs.mdl", 1, 1},
		gib_data_t{"models/hgibs.mdl", 2, 1},
		gib_data_t{"models/hgibs.mdl", 3, 1},
		gib_data_t{"models/hgibs.mdl", 4, 1},
		gib_data_t{"models/hgibs.mdl", 5, 1},
};

std::vector<gib_data_t> pitdrone_gibmap =
{		// 			MDL 			   		    BG # TYPE
		gib_data_t{"models/pit_drone_gibs.mdl", 0, 2},  // claws
		gib_data_t{"models/pit_drone_gibs.mdl", 1, 1},  // tentacle 1
		gib_data_t{"models/pit_drone_gibs.mdl", 2, 1},  // tentacle 2
		gib_data_t{"models/pit_drone_gibs.mdl", 3, 2},  // knee?
		gib_data_t{"models/pit_drone_gibs.mdl", 4, 1},  // tail
		gib_data_t{"models/pit_drone_gibs.mdl", 5, 1},  // generic?
		gib_data_t{"models/pit_drone_gibs.mdl", 6, 1},  // back spike
};				

std::vector<gib_data_t> funghoul_gibmap =
{		// 			MDL 			   BG # TYPE
		gib_data_t{"models/fung_gibs.mdl", 0, 1, 1},
		gib_data_t{"models/fung_gibs.mdl", 1, 1},
		gib_data_t{"models/fung_gibs.mdl", 2, 1},
		gib_data_t{"models/fung_gibs.mdl", 3, 1},
		gib_data_t{"models/fung_gibs.mdl", 4, 1},
		gib_data_t{"models/fung_gibs.mdl", 5, 1},
};

void CoolerGib::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!pActivator->IsPlayer() || m_pEater && pev->velocity == g_vecZero)
		return;

	CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(pActivator);

	if (pPlayer->m_bPrehuman == true || pPlayer->m_bInGrenade) // TO-DO: make it a general in offhand anim var
		return; // no snack for you!!

	CBasePlayerWeapon* pWpn = pPlayer->m_pActiveItem->GetWeaponPtr();

	if (pWpn && (gpGlobals->time < pWpn->m_flNextOffhandAttack || pWpn->IsDual()))
		return;

	if (pPlayer->Hunger > 97)
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "player/eatfail.wav", 0.85, 1.2);
		return;
	}

	pPlayer->altviewmodel = MAKE_STRING("models/v_ohgrenade.mdl");
	pWpn->SendWeaponAnim(OH_GRAB, 0, true); // body is 0 since prehuman players cannot eat anyways
	pWpn->m_flNextOffhandAttack = gpGlobals->time + 1.33;
	pWpn->m_flNextSecondaryAttack = pWpn->m_flNextPrimaryAttack = pWpn->m_flNextTertiaryAttack = 1.2f;

	pPlayer->m_bNoMove = true;

	m_bDisableFade = true;
	m_pEater = pPlayer;
	SetThink(&CoolerGib::PickUpThink);
	pev->nextthink = gpGlobals->time + 0.333; // delay before "grabbed"
}

void CoolerGib::PickUpThink()
{
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	SetThink(&CoolerGib::EatThink);
	pev->nextthink = gpGlobals->time + 0.3; // delay before "eaten"
}

void CoolerGib::EatThink()
{
	// play sound (TO-DO: make positioned propery?)
	const char* sound = 0;
	sound = UTIL_VarArgs("player/eat%d.wav", RANDOM_LONG(1, 3));
	EMIT_SOUND_DYN(m_pEater->edict(), CHAN_AUTO, sound, 0.8, 1.2, 0, 95+RANDOM_LONG(0,10));

	// VFX
	PLAYBACK_EVENT_FULL(0, edict(), g_sParticleEvent, 0.0, m_pEater->Center(), m_pEater->pev->angles, 8, 0.0, PE_NPCIMPACTCLUST, m_bloodColor, 0, 1);
	UTIL_BloodDrips(m_pEater->Center(), m_pEater->pev->angles, m_bloodColor, 8);

	if (m_bloodColor == BLOOD_COLOR_RED)
	{
		// tasty!
		m_pEater->Hunger += RANDOM_LONG(5, 6);
		m_pEater->TakeHealth(5, DMG_GENERIC);
		m_pEater->health_armR += RANDOM_LONG(4, 7);
		m_pEater->health_armL += RANDOM_LONG(4, 7);
		m_pEater->health_legL -= RANDOM_LONG(4, 7);
		m_pEater->health_legR -= RANDOM_LONG(4, 7);
		m_pEater->health_head -= RANDOM_LONG(4, 7);
		m_pEater->health_chest -= RANDOM_LONG(4, 7);
		m_pEater->health_stomach -= RANDOM_LONG(4, 7);
		if (m_pEater->health_armR > 100)
			m_pEater->health_armR = 100;
		if (m_pEater->health_armL > 100)
			m_pEater->health_armL = 100;
		if (m_pEater->health_legL < 0)
			m_pEater->health_legL = 0;
		if (m_pEater->health_legR < 0)
			m_pEater->health_legR = 0;
		if (m_pEater->health_head < 0)
			m_pEater->health_head = 0;
		if (m_pEater->health_chest < 0)
			m_pEater->health_chest = 0;
		if (m_pEater->health_stomach < 0)
			m_pEater->health_stomach = 0;
	}
	else if (m_bloodColor != DONT_BLEED)
	{
		// slop!
		int increment = RANDOM_LONG(2, 3);
		m_pEater->Hunger += RANDOM_LONG(2, 3);
		m_pEater->TakeHealth(increment + 1, DMG_GENERIC);
		m_pEater->health_armR += RANDOM_LONG(3, 6);
		m_pEater->health_armL += RANDOM_LONG(3, 6);
		m_pEater->health_legL -= RANDOM_LONG(3, 6);
		m_pEater->health_legR -= RANDOM_LONG(3, 6);
		m_pEater->health_head -= RANDOM_LONG(3, 6);
		m_pEater->health_chest -= RANDOM_LONG(3, 6);
		m_pEater->health_stomach -= RANDOM_LONG(3, 6);
		if (m_pEater->health_armR > 100)
			m_pEater->health_armR = 100;
		if (m_pEater->health_armL > 100)
			m_pEater->health_armL = 100;
		if (m_pEater->health_legL < 0)
			m_pEater->health_legL = 0;
		if (m_pEater->health_legR < 0)
			m_pEater->health_legR = 0;
		if (m_pEater->health_head < 0)
			m_pEater->health_head = 0;
		if (m_pEater->health_chest < 0)
			m_pEater->health_chest = 0;
		if (m_pEater->health_stomach < 0)
			m_pEater->health_stomach = 0;
	}

	if (m_pEater->Hunger > 100)
		m_pEater->Hunger = 100;

	m_pEater->m_bNoMove = false;

	SetThink(&CoolerGib::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

// HACKHACK -- The gib velocity equations don't work
void CoolerGib::LimitVelocity()
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if (length > 1500.0)
		pev->velocity = pev->velocity.Normalize() * 1500; // This should really be sv_maxvelocity * 0.75 or something
}


void CoolerGib::SpawnStickyGibs(entvars_t* pevVictim, CoolerGib* pGib)
{
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
	PLAYBACK_EVENT_FULL(0, pGib->edict(), g_sParticleEvent, 0.0, pGib->pev->origin, -gpGlobals->v_forward, 0.0, 0.0, PE_NPCIMPACTCLUST, pGib->m_bloodColor, 0, 0);
	pGib->LimitVelocity();
}

void CoolerGib::SpawnHeadGib(entvars_t* pevVictim, CoolerGib* pGib)
{
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
	PLAYBACK_EVENT_FULL(0, pGib->edict(), g_sParticleEvent, 0.0, pGib->pev->origin, -gpGlobals->v_forward, 0.0, 0.0, PE_NPCIMPACTCLUST, pGib->m_bloodColor, 0, 0);
	#endif
	pGib->LimitVelocity();
}

void CoolerGib::SpawnRandomGibs(entvars_t* pevVictim, Vector spawnposOVRDE)
{
	int i, p, amnt, body;
	CBaseEntity* pVictim = CBaseEntity::Instance(pevVictim);
	std::vector<gib_data_t> gibmap = GetNPCgibs(pVictim);
	int size = gibmap.size();

	for (i = 0; i < size; i++) // loops through rows
	{
		int type = 0;
		amnt = gibmap[i].amount;
		body = gibmap[i].body;
		if (gibmap[i].type != 0)
			type = gibmap[i].type;

		for (p = 0; p < amnt; p++) // spawns amount dictated in the row's third collumn
		{
			CoolerGib* pGib = GetClassPtr((CoolerGib*)NULL);
			pGib->Spawn(gibmap[i].gib_mdlname.c_str(), body); // spawns gib with model at collumn 1 and body at collumn 2
			pGib->m_gdType = gibmap[i];
			if (pVictim)
			{
				pGib->m_pGibbed = pVictim;

				if (pVictim->m_iBurnTimer > 0)
					pGib->m_iBurnTimer = (RANDOM_LONG(0, pVictim->m_iBurnTimer));
			}

			switch (type)
			{
				case 1:
					SpawnHeadGib(pevVictim, pGib);
					break;
				case 2:
					SpawnStickyGibs(pevVictim, pGib);
					break;
			}
			if (type != 0)
				continue; // loop back if it uses diff spawn type

			if (pevVictim) // probably uneeded
			{
				// spawn the gib somewhere in the monster's bounding volume
				if (spawnposOVRDE == g_vecZero) // this will make npcs directly at the origin not gib properly
				{
					pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT(0, 1));
					pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT(0, 1));
					pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT(0, 1)) + 1; // absmin.z is in the floor because the engine subtracts 1 to enlarge the box
				}
				else
				{
					pGib->pev->origin.x = spawnposOVRDE.x + RANDOM_FLOAT(-4, 4);
					pGib->pev->origin.y = spawnposOVRDE.y + RANDOM_FLOAT(-4, 4);
					pGib->pev->origin.z = spawnposOVRDE.z + RANDOM_FLOAT(-4, 4);
				}

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
				pGib->m_bloodColor = pVictim->BloodColor();

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
			PLAYBACK_EVENT_FULL(0, pGib->edict(), g_sParticleEvent, 0.0, pGib->pev->origin, -gpGlobals->v_forward, 0.0, 0.0, PE_NPCIMPACTCLUST, pGib->m_bloodColor, 0, 0);
			pGib->LimitVelocity();
		}
	}
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
		if (m_bShouldPool && UTIL_ShouldShowBlood(m_bloodColor) == true)
		{
			int time = 6 - trunc(m_iPoolTime/10); // range
			Vector origin;

			// spawns 1 every 0.1 seconds
			int count = 0;
			int contents;

			if (time > RANDOM_LONG(1,2)) // wait a bit
			{
				// fetch blood pool pos
				Vector boneOrg = pev->origin;

				do // get the location
				{
					count++;

					// get a random spot in the radius
					// could make this focus on only a random quadrant to make it directional
					// time x4 makes the radius jump 4 units each time
					float theter = RANDOM_FLOAT(0, 1) * (2 * 3.141592);
					float x = boneOrg.x + (3 * time) * cos(theter);
					float y = boneOrg.y + (3 * time) * sin(theter);

					origin = Vector(x, y, pev->origin.z + 1);
					contents = UTIL_PointContents(origin);
					
					// check if in view of the origin?

					if (count > 64) // no valid spots or VERY unlucky
						break;
				} while (contents != CONTENT_EMPTY); // don't spawn in walls

				if (count > 64)
				{
					m_bShouldPool = false; // bigger radius probably won't fix it, and if it does then that would be going through a wall (bad)
					ALERT(at_console, "failed to continue blood pool!\n");
				}
				else
					CPhysblood::BloodCreate(1, 0, origin, -gpGlobals->v_up, 1.0, m_bloodColor, false, 0, false, true); // TO-DO: make not play sfx
			}

			m_iPoolTime--;

			if (m_iPoolTime <= 0) // stop bleeding
				m_bShouldPool = false;
		}

		// don't fade if
		// - hasn't been 25 seconds
		// - hasn't finished pooling
		// - told not to
		if (!m_bDisableFade && m_lifeTime < gpGlobals->time && m_bShouldPool == false)
		{
			SetThink(&CoolerGib::SUB_StartFadeOut);
			pev->nextthink = gpGlobals->time;
		}

		// If you bleed, you stink!
		if (!m_bLanded && m_bloodColor != DONT_BLEED)
		{
			// ok, start stinkin!
			CSoundEnt::InsertSound(bits_SOUND_MEAT, pev->origin, 384, 25);
		}
		pev->nextthink = gpGlobals->time + 0.1;
		m_bLanded = true;
	}
	else
	{
		if (m_cBloodDecals > 0 && UTIL_ShouldShowBlood(m_bloodColor) == true && (RANDOM_LONG(0, 1) == 1))
		{
			PLAYBACK_EVENT_FULL(0, edict(), g_sParticleEvent, 0.0, pev->origin, g_vecZero, 0.0, 0.0, PE_NPCIMPACTCLUST, m_bloodColor, 0, 0);
		}
		pev->nextthink = gpGlobals->time + 0.25; // WAS 0.1
	}

	if (m_iBurnTimer > 0)
	{
		if(pev->waterlevel > 0) 
			m_iBurnTimer = 0;

		if (m_iBurnTimer > 200)
			m_iBurnTimer = 200;

		else
		{
			int max = 1; // max particles / 4

			int iBurnAmnt = ceil(m_iBurnTimer/10);
			if (iBurnAmnt > max) 
				iBurnAmnt = max;
			
			for (int i = 0; i < iBurnAmnt; i++) // spawns particle - EACH SPAWNS 4
			{
				Vector VecflameOrg = pev->origin;
				VecflameOrg.x += -3 + 3 * (RANDOM_FLOAT(0.25, 0.75));
				VecflameOrg.y += -3 + 3 * (RANDOM_FLOAT(0.25, 0.75));
				VecflameOrg.z += -2 + 2 * (RANDOM_FLOAT(0, 0.5)) + 1;

				PLAYBACK_EVENT_FULL(0, edict(), g_sParticleEvent, 0.0, VecflameOrg, g_vecZero, 0.0, 0.0, PE_FIRE, 0, 0, 0);
			}

			if ((trunc(m_iBurnTimer/10) * 10) == m_iBurnTimer)
			{
				if (RANDOM_LONG(0, 4) == 4)
				{
					Vector VecSpreadOrg = Center();
					VecSpreadOrg.z = pev->absmin.z + 5;

					// make sure there isn't already fire there
					CBaseEntity* pList[2];
					int count = UTIL_EntitiesInBox(pList, 2, VecSpreadOrg - Vector(12, 12, 0), VecSpreadOrg + Vector(12, 12, 8), FL_FIRE);
					if (0 == count) // don't spawn monsters near players or other monsters
					{
						CFire::FireCreate(VecSpreadOrg, 8, 10, 1, this); // spread fire around, cause chaos
					}
				}
			}
			//ALERT(at_console, "burn: %d health: %f particleamnt: %i\n", m_iBurnTimer, pev->health, iBurnAmnt);
			m_iBurnTimer--;
		}
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

	SetThink(&CoolerGib::SUB_Remove); // TO-DO: make fall off the wall after a bit)
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
	pev->classname = MAKE_STRING("cool_gib");

	SET_MODEL(ENT(pev), szGibModel);
	pev->body = body;

	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
	pev->nextthink = gpGlobals->time + 0.1;
	m_lifeTime = gpGlobals->time + 25;
	SetThink(&CoolerGib::WaitTillLand);
	SetTouch(&CoolerGib::BounceGibTouch);
}

std::vector<gib_data_t> CoolerGib::GetNPCgibs(CBaseEntity* pVictim)
{
	if (FClassnameIs(pVictim->pev, "monster_pitdrone"))
		return pitdrone_gibmap;
	else if (pVictim->Classify() == CLASS_FUNGAL) // this won't work once more fungus npcs are added
		return funghoul_gibmap;
	else 
	{
		switch (pVictim->BloodColor())
		{
			default:
			case BLOOD_COLOR_RED:
				return human_gibmap;
				break;
			case BLOOD_COLOR_YELLOW:
			case BLOOD_COLOR_GREEN:
				return xenian_gibmap;
				break;
		}
	}
}

int CoolerGib::ShouldCollide(CBaseEntity* pentTouched)
{
	if (pentTouched->IsPlayer())
		return 1;
	if (!pentTouched->IsBSPModel())
	{
		return 0;
	}

	return 1;
}