/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

//=========================================================
// miller_human
//=========================================================

#include "extdll.h"
#include "plane.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"
#include "pm_materials.h"
#include "hgrunt.h"
#include "physical_bullet.h"
int g_fMillerQuestion; // true if an idle grunt asked a question. Cleared when someone answers.
//=========================================================
// monster-specific DEFINE's
//=========================================================
#define GRUNT_CLIP_SIZE 36	 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_VOL 0.35		 // volume of grunt sounds
#define MILLER_ATTN 0.7 // attenutation of grunt sentences
#define MILLER_HUM_LIMP_HEALTH 20
#define MILLER_HUM_DMG_HEADSHOT (DMG_BULLET | DMG_CLUB) // damage types that can kill a grunt with a single headshot.
#define MILLER_HUM_MINIMUM_HEADSHOT_DAMAGE 15			// must do at least this much damage in one shot to head to score a headshot kill
#define MILLER_HUM_SENTENCE_VOLUME (float)0.35			// volume of grunt sentences

#define MILLER_HUM_9MMAR (1 << 0)
#define MILLER_HUM_HANDGRENADE (1 << 1)
#define MILLER_HUM_GRENADELAUNCHER (1 << 2)
#define MILLER_HUM_SHOTGUN (1 << 3)
#define MILLER_HUM_M249 (1 << 4)
#define MILLER_HUM_M727 (1 << 5)
#define HITGROUP_HEAVY_VISOR 67

class CHMiller : public CHGrunt
{
public:
	void Spawn() override;
	void Precache() override;
	static const char* pSoundsMetal[];
	void SetYawSpeed() override; //overidden to lower YS
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void SetActivity(Activity NewActivity) override; //overriden to change anims

	bool CheckRangeAttack2(float flDot, float flDist) override;
	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;
	void Shotgun();
	void M249();
	void Killed(entvars_t* pevAttacker, int iGib) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	CBaseEntity* Kick();
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;

	int IRelationship(CBaseEntity* pTarget) override;

	void ArmorGibs(TraceResult* ptr, Vector Vel);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector m_vecTossVelocity;

	bool m_fThrowGrenade;
	bool m_fStanding;
	bool m_fFirstEncounter; // only put on the handsign show in the squad's first encounter.
	bool M_HasHelm = false;
	bool m_hasdroppedwpn = false;

	int m_cClipSize;
	int m_voicePitch;

	int m_iBrassShell;
	int m_iShotgunShell;

	int m_iSentence;
	int m_iShell;
	int m_iLink;

	int m_helmvisorDUR = 35;
	int m_iarmor_health_chest = 40;
	int m_iarmor_health_stomach = 50;
	int m_idShard;

	static const char* pGruntSentences[];
};

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_MILLER_HUM_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_MILLER_HUM_ESTABLISH_LINE_OF_FIRE, // move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_MILLER_HUM_COVER_AND_RELOAD,
	SCHED_MILLER_HUM_SWEEP,
	SCHED_MILLER_HUM_FOUND_ENEMY,
	SCHED_MILLER_HUM_WAIT_FACE_ENEMY,
	SCHED_MILLER_HUM_TAKECOVER_FAILED, // special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_MILLER_HUM_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE (bits_COND_SPECIAL1)

LINK_ENTITY_TO_CLASS(monster_miller_human, CHMiller);

TYPEDESCRIPTION CHMiller::m_SaveData[] =
	{
		DEFINE_FIELD(CHMiller, m_flNextGrenadeCheck, FIELD_TIME),
		DEFINE_FIELD(CHMiller, m_flNextPainTime, FIELD_TIME),
		//	DEFINE_FIELD( CHMiller, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
		DEFINE_FIELD(CHMiller, m_vecTossVelocity, FIELD_VECTOR),
		DEFINE_FIELD(CHMiller, m_fThrowGrenade, FIELD_BOOLEAN),
		DEFINE_FIELD(CHMiller, m_fStanding, FIELD_BOOLEAN),
		DEFINE_FIELD(CHMiller, m_fFirstEncounter, FIELD_BOOLEAN),
		DEFINE_FIELD(CHMiller, m_cClipSize, FIELD_INTEGER),
		DEFINE_FIELD(CHMiller, m_voicePitch, FIELD_INTEGER),
		//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
		//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
		DEFINE_FIELD(CHMiller, m_iSentence, FIELD_INTEGER),
		DEFINE_FIELD(CHMiller, m_helmDUR, FIELD_INTEGER),
		DEFINE_FIELD(CHMiller, m_helmvisorDUR, FIELD_INTEGER),
		DEFINE_FIELD(CHMiller, m_iarmor_health_chest, FIELD_INTEGER),
		DEFINE_FIELD(CHMiller, m_iarmor_health_stomach, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CHMiller, CSquadMonster);

const char* CHMiller::pGruntSentences[] =
	{
		"HG_GREN",	  // grenade scared grunt
		"HG_ALERT",	  // sees player
		"HG_MONSTER", // sees monster
		"HG_COVER",	  // running to cover
		"HG_THROW",	  // about to throw grenade
		"HG_CHARGE",  // running out to get the enemy
		"HG_TAUNT",	  // say rude things
};

enum MILLER_HUM_SENTENCE_TYPES
{
	MILLER_HUM_SENT_NONE = -1,
	MILLER_HUM_SENT_GREN = 0,
	MILLER_HUM_SENT_ALERT,
	MILLER_HUM_SENT_MONSTER,
	MILLER_HUM_SENT_COVER,
	MILLER_HUM_SENT_THROW,
	MILLER_HUM_SENT_CHARGE,
	MILLER_HUM_SENT_TAUNT,
};

//=========================================================
// Spawn
//=========================================================
void CHMiller::Spawn()
{
	Precache();
	
	SET_MODEL(ENT(pev), "models/hgrunt_heavy.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;

	if (g_iSkillLevel != SKILL_REALISM)
	{
		pev->health = round(gSkillData.hgruntHealth * 6.0);
	}
	else
	{
		pev->health = 300;
	}

	m_flFieldOfView = 0.25; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = MILLER_HUM_SENT_NONE;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded = false;
	m_fFirstEncounter = true; // this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	if (pev->weapons == 0)
	{
		pev->weapons = MILLER_HUM_SHOTGUN | MILLER_HUM_HANDGRENADE;
	}

	if (FBitSet(pev->weapons, MILLER_HUM_SHOTGUN))
	{
		SetBodygroup(GUN_GROUP, GUN_SHOTGUN);
		pev->weaponmodel = MAKE_STRING("models/h_spas.mdl");
		m_cClipSize = 4;
		if (g_iSkillLevel != SKILL_REALISM)
		{
			m_flDistTooFar = 384;
		}
		else
		{
			m_flDistTooFar = 1024;
		}
	}
	else if (FBitSet(pev->weapons, MILLER_HUM_M249))
	{
		pev->weaponmodel = MAKE_STRING("models/h_m249.mdl");
		m_cClipSize = 201;
	}
	else if (FBitSet(pev->weapons, MILLER_HUM_GRENADELAUNCHER))
	{
		pev->weaponmodel = MAKE_STRING("models/h_mp5.mdl");
		m_flDistTooFar = 2048+256;
		m_flDistLook = 4096; //idk if this is needed
		m_cClipSize = 30;
	}
	else if (FBitSet(pev->weapons, MILLER_HUM_M727))
	{
		pev->weaponmodel = MAKE_STRING("models/h_m727.mdl");
		m_flDistTooFar = 3072;
		m_flDistLook = 4096; //idk if this is needed
		m_cClipSize = 30;
	}
	else if (FBitSet(pev->weapons, MILLER_HUM_9MMAR))
	{
		pev->weaponmodel = MAKE_STRING("models/h_mp5.mdl");
		m_cClipSize = 30;
		m_flDistTooFar = 2048+192;
		m_flDistLook = 4096; //idk if this is needed
	}
	pev->skin = RANDOM_LONG(0, 1);
	m_cAmmoLoaded = m_cClipSize;
	CTalkMonster::g_talkWaitTime = 0;
	
	MonsterInit();
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHMiller::Precache()
{
	const char* pGibName;
	PRECACHE_MODEL("models/hgrunt_heavy.mdl");
	PRECACHE_MODEL("models/h_mp5.mdl");
	PRECACHE_MODEL("models/h_spas.mdl");
	PRECACHE_MODEL("models/h_m249.mdl");
	PRECACHE_MODEL("models/h_m727.mdl");

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");
	PRECACHE_SOUND("hgrunt/gr_727_1.wav");
	PRECACHE_SOUND("hgrunt/gr_727_2.wav");

	PRECACHE_SOUND("fgrunt/death1.wav");
	PRECACHE_SOUND("fgrunt/death2.wav");
	PRECACHE_SOUND("fgrunt/death3.wav");
	PRECACHE_SOUND("fgrunt/death4.wav");
	PRECACHE_SOUND("fgrunt/death5.wav");
	PRECACHE_SOUND("fgrunt/death6.wav");

	PRECACHE_SOUND("fgrunt/pain1.wav");
	PRECACHE_SOUND("fgrunt/pain2.wav");
	PRECACHE_SOUND("fgrunt/pain3.wav");
	PRECACHE_SOUND("fgrunt/pain4.wav");
	PRECACHE_SOUND("fgrunt/pain5.wav");
	PRECACHE_SOUND("fgrunt/pain6.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/sbarrel1.wav");
	PRECACHE_SOUND("weapons/dbarrel1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav"); // because we use the basemonster SWIPE animation event
	PRECACHE_SOUND("weapons/reload3.wav");

	PRECACHE_SOUND("debris/metal1.wav");
	PRECACHE_SOUND("debris/metal2.wav");
	PRECACHE_SOUND("debris/metal3.wav");

	UTIL_PrecacheOther("rpg_rocket");

	m_voicePitch = 50;
	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell
	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");
	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl");
	m_iLink = PRECACHE_MODEL("models/saw_link.mdl");

	pGibName = "models/metalplategibs.mdl";
	m_idShard = PRECACHE_MODEL((char*)pGibName);
}

const char* CHMiller::pSoundsMetal[] =
	{
		"debris/metal1.wav",
		"debris/metal2.wav",
		"debris/metal3.wav",
};

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHMiller::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK2:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = round(ys / 0.75);
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CHMiller::Classify()
{
	if (m_bPrehuman == false)
	{
		return CLASS_HUMAN_PASSIVE;
	}
	else
	{
		return CLASS_HUMAN_ALLY; // probably not a good idea to make him friendly under any circumstance
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHMiller::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case HGRUNT_AE_RELOAD:
	{
		if (FBitSet(pev->weapons, MILLER_HUM_SHOTGUN))
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/reload3.wav", 1, ATTN_NORM);
		else if (FBitSet(pev->weapons, MILLER_HUM_M249))
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/saw_reload2.wav", 1, ATTN_NORM);
		else
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM);
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
	}
	break;
	case HGRUNT_AE_GREN_TOSS:
	{
		Vector vecSrc = GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;
		UTIL_MakeVectors(pev->angles);
		//CGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5);
		
		CBaseEntity* pRocket = CBaseEntity::Create("rpg_rocket", vecSrc, pev->angles, edict());
		m_fThrowGrenade = false;
		m_flNextGrenadeCheck = gpGlobals->time + 15; // wait 15 before even looking again to see if a rpg can be fired.
													// !!!LATER - when in a group, only try to throw grenade if ordered.
		Vector angDir = UTIL_VecToAngles(pev->angles);
		SetBlending(0, angDir.x);
	}
	break;

	case HGRUNT_AE_GREN_LAUNCH:
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_GUN);
		CGrenade::ShootContact(pev, GetGunPosition(), m_vecTossVelocity);
		m_fThrowGrenade = false;
		if (g_iSkillLevel == SKILL_REALISM)
			m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT(2, 5); // wait a random amount of time before shooting again
		else
			m_flNextGrenadeCheck = gpGlobals->time + 6; // wait six seconds before even looking again to see if a grenade can be thrown.
	}
	break;

	case HGRUNT_AE_GREN_DROP:
	{
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3);
	}
	break;

	case HGRUNT_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MILLER_HUM_9MMAR))
		{
			Shoot();
			if (RANDOM_LONG(0, 1)) // the first round of the three round burst plays the sound and puts a sound in the world sound list.
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_GUN);
			else
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_GUN);
		}
		else if (FBitSet(pev->weapons, MILLER_HUM_SHOTGUN))
		{
			Shotgun();
		}
		else if (FBitSet(pev->weapons, MILLER_HUM_M727))
		{
			ShootM727();
			if (RANDOM_LONG(0, 1)) // the first round of the three round burst plays the sound and puts a sound in the world sound list.
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_727_1.wav", 1, ATTN_GUN);
			else
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_727_2.wav", 1, ATTN_GUN);
		}
		else
			M249();

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case HGRUNT_AE_BURST2:
	case HGRUNT_AE_BURST3:
	{
		if (FBitSet(pev->weapons, MILLER_HUM_9MMAR))
			Shoot();
		else if (FBitSet(pev->weapons, MILLER_HUM_M727))
			ShootM727();
		else
			M249();
	}
	break;
	case HGRUNT_AE_KICK:
	{
		CBaseEntity* pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300 + gpGlobals->v_up * 100;
			pHurt->TakeDamage(pev, pev, gSkillData.hgruntDmgKick*2, DMG_CLUB);
			if (pHurt->IsPlayer())
			{
				CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(pHurt);
				CBasePlayerWeapon* pWeapon = pPlayer->m_pActiveItem->GetWeaponPtr();
				pWeapon->m_flNextPrimaryAttack += 0.5;
			}
		}
	}
	break;

	case HGRUNT_AE_CAUGHT_ENEMY:
	{
		if (FOkToSpeak())
		{
			SENTENCEG_PlayRndSz(ENT(pev), "HG_ALERT", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);
			JustSpoke();
		}
	}
	break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// SetActivity
//=========================================================
void CHMiller::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
	{
		if (FBitSet(pev->weapons, MILLER_HUM_9MMAR) || FBitSet(pev->weapons, MILLER_HUM_M727) || FBitSet(pev->weapons, MILLER_HUM_M249))
		{
			if (m_fStanding)
				iSequence = LookupSequence("standing_mp5");
			else
				iSequence = LookupSequence("crouching_mp5");
		}
		else if (FBitSet(pev->weapons, MILLER_HUM_SHOTGUN))
		{
			if (m_fStanding)
				iSequence = LookupSequence("standing_shotgun");
			else
				iSequence = LookupSequence("crouching_shotgun");
		}
		else if (FBitSet(pev->weapons, MILLER_HUM_M727))
		{
			if (m_fStanding)
				iSequence = LookupSequence("standing_m727");
			else
				iSequence = LookupSequence("crouching_m727");
		}
		else
		{
			if (m_fStanding)
				iSequence = LookupSequence("standing_saw");
			else
				iSequence = LookupSequence("crouching_saw");
		}
	}
	break;
	case ACT_RANGE_ATTACK2:
	{
		if ((pev->weapons & MILLER_HUM_HANDGRENADE) != 0)
			iSequence = LookupSequence("throwgrenade");
		else
			iSequence = LookupSequence("launchgrenade");
	}
		break;
	case ACT_RUN:
	{
		if (pev->health <= MILLER_HUM_LIMP_HEALTH)
			iSequence = LookupActivity(ACT_RUN_HURT);
		else
			iSequence = LookupActivity(NewActivity);
	}
	break;
	case ACT_WALK:
	{
		if (pev->health <= MILLER_HUM_LIMP_HEALTH)
			iSequence = LookupActivity(ACT_WALK_HURT);
		else
			iSequence = LookupActivity(NewActivity);
	}
	break;
	case ACT_IDLE:
	{
		if (m_MonsterState == MONSTERSTATE_COMBAT)
			NewActivity = ACT_IDLE_ANGRY;
		iSequence = LookupActivity(NewActivity);
	}
	break;
	case ACT_RELOAD:
	{
		if (FBitSet(pev->weapons, MILLER_HUM_9MMAR || MILLER_HUM_M727))
			iSequence = LookupSequence("reload_mp5");
		else if (FBitSet(pev->weapons, MILLER_HUM_SHOTGUN))
			iSequence = LookupSequence("reload_shotgun");
		else
			iSequence = LookupSequence("reload_mp5");
			//iSequence = LookupSequence("reload_saw"); // doesn't exist yet TO-DO: hHgrunt saw reload anim
	}
	break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence; // Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0; // Set to the reset anim (if it's there)
	}
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
bool CHMiller::CheckRangeAttack2(float flDot, float flDist)
{
	if (!FBitSet(pev->weapons, (MILLER_HUM_HANDGRENADE | MILLER_HUM_GRENADELAUNCHER)))
	{
		return false;
	}

	// if the grunt isn't moving, it's ok to check.
	if (m_flGroundSpeed != 0)
	{
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck)
	{
		return m_fThrowGrenade;
	}

	if (m_vecEnemyLKP.z > pev->absmax.z)
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet(pev->weapons, MILLER_HUM_HANDGRENADE))
	{
		// find feet
		if (RANDOM_LONG(0, 1))
		{
			// magically know where they are
			vecTarget = Vector(m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z);
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget(pev->origin) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions(bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if (InSquad())
	{
		if (SquadMemberInRange(vecTarget, 256))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 2; // one full second.
			m_fThrowGrenade = false;
		}
	}

	if ((vecTarget - pev->origin).Length2D() <= 160)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}


	if (FBitSet(pev->weapons, MILLER_HUM_HANDGRENADE))
	{
		m_vecTossVelocity = ShootAtEnemy(vecTarget);
		// throw a hand grenade
		m_fThrowGrenade = true;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
	}
	else
	{
		Vector vecToss = VecCheckThrow(pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = false;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}



	return m_fThrowGrenade;
}

//=========================================================
// DeathSound
//=========================================================
void CHMiller::DeathSound()
{
	if (m_bRailed == false)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, UTIL_VarArgs("fgrunt/death%d.wav", RANDOM_LONG(1, 6)), 1, ATTN_NORM);
}

//=========================================================
// PainSound
//=========================================================
void CHMiller::PainSound()
{
	if (gpGlobals->time > m_flNextPainTime)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, UTIL_VarArgs("fgrunt/pain%d.wav", RANDOM_LONG(1, 6)), 1, ATTN_NORM);
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

void CHMiller::IdleSound()
{
	if (FOkToSpeak() && (0 != g_fMillerQuestion || RANDOM_LONG(0, 1)))
	{
		if (0 == g_fMillerQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0, 2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CHECK", MILLER_HUM_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fMillerQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_QUEST", MILLER_HUM_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fMillerQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "HG_IDLE", MILLER_HUM_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fMillerQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CLEAR", MILLER_HUM_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ANSWER", MILLER_HUM_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fMillerQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// Shoot
//=========================================================
void CHMiller::Shotgun()
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);
	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/dbarrel1.wav", 1, ATTN_GUN);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_REALISM)
		CPhysbullet::BulletCreate(12, gSkillData.plrDmgBuckshot, 5750, vecShootOrigin, vecShootDir, UTIL_DegreesToRadCone(16), CONE_2DEGREES, 0.75, 12, edict());
	else
	{
		CPhysbullet::BulletCreate(18, 11, 5750, vecShootOrigin, vecShootDir, CONE_10DEGREES, CONE_2DEGREES, 1, 12, edict());
	}
	m_cAmmoLoaded--; // take away a bullet!
#endif
	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}
//=========================================================
// Shoot
//=========================================================
void CHMiller::M249()
{
	if (m_cAmmoLoaded <= 0)
	{
		EMIT_SOUND(edict(), CHAN_AUTO, "weapons/357_cock1.wav", 1, ATTN_NORM);
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin, vecShellVelocity, pev->angles.y, m_iLink, TE_BOUNCE_SHELL);
	EjectBrass(vecShootOrigin, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
	#ifndef CLIENT_DLL
	if (g_iSkillLevel != SKILL_REALISM)
	{
		CPhysbullet::BulletCreate(1, gSkillData.monDmgMP5, 7000, vecShootOrigin, vecShootDir, CONE_7DEGREES, CONE_1DEGREES, 0.66, 556, edict());
	}
	else
	{
		CPhysbullet::BulletCreate(1, 34, 7000, vecShootOrigin, vecShootDir, CONE_6DEGREES, CONE_1DEGREES, 1, 556, edict());
	}
	#endif
	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--; // take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);

	char wpnsnd2[256];
	sprintf(wpnsnd2, "weapons/saw_fire%d.wav", RANDOM_LONG(1, 2));
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, wpnsnd2, 1, ATTN_GUN);
}

void CHMiller::Killed(entvars_t* pevAttacker, int iGib)
{
	if (m_hasdroppedwpn == false)
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		// switch to body group with no gun.
		pev->weaponmodel = 0;

		// now spawn a gun.
		if (FBitSet(pev->weapons, MILLER_HUM_SHOTGUN))
			DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
		else if (FBitSet(pev->weapons, MILLER_HUM_M249))
			DropItem("weapon_m249", vecGunPos, vecGunAngles);
		else if (FBitSet(pev->weapons, MILLER_HUM_9MMAR))
			DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		else if (FBitSet(pev->weapons, MILLER_HUM_GRENADELAUNCHER))
		{
			DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
			DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		}
		else
			DropItem("weapon_m727", vecGunPos, vecGunAngles);

		m_hasdroppedwpn = true;
	}

	if (ShouldGibMonster(iGib))
		m_bloodColor = BLOOD_COLOR_RED;
	CSquadMonster::Killed(pevAttacker, iGib);
}

//=========================================================
//=========================================================
CBaseEntity* CHMiller::Kick()
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;
}
//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t tlMillerHumFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slMillerHumFail[] =
	{
		{tlMillerHumFail,
			ARRAYSIZE(tlMillerHumFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK2,
			0,
			"Grunt Fail"},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t tlMillerHumCombatFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slMillerHumCombatFail[] =
	{
		{tlMillerHumCombatFail,
			ARRAYSIZE(tlMillerHumCombatFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2,
			0,
			"Grunt Combat Fail"},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlMillerHumVictoryDance[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_WAIT, (float)1.5},
		{TASK_GET_PATH_TO_ENEMY_CORPSE, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
};

Schedule_t slMillerHumVictoryDance[] =
	{
		{tlMillerHumVictoryDance,
			ARRAYSIZE(tlMillerHumVictoryDance),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE,
			0,
			"GruntVictoryDance"},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlMillerHumEstablishLineOfFire[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_MILLER_HUM_ELOF_FAIL},
		{TASK_GET_PATH_TO_ENEMY, (float)0},
		{TASK_GRUNT_SPEAK_SENTENCE, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slMillerHumEstablishLineOfFire[] =
	{
		{tlMillerHumEstablishLineOfFire,
			ARRAYSIZE(tlMillerHumEstablishLineOfFire),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK2 |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"GruntEstablishLineOfFire"},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t tlMillerHumFoundEnemy[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL1},
};

Schedule_t slMillerHumFoundEnemy[] =
	{
		{tlMillerHumFoundEnemy,
			ARRAYSIZE(tlMillerHumFoundEnemy),
			bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"GruntFoundEnemy"},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t tlMillerHumCombatFace1[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_WAIT, (float)1.5},
		{TASK_SET_SCHEDULE, (float)SCHED_MILLER_HUM_SWEEP},
};

Schedule_t slMillerHumCombatFace[] =
	{
		{tlMillerHumCombatFace1,
			ARRAYSIZE(tlMillerHumCombatFace1),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2,
			0,
			"Combat Face"},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t tlMillerHumSignalSuppress[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL2},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMillerHumSignalSuppress[] =
	{
		{tlMillerHumSignalSuppress,
			ARRAYSIZE(tlMillerHumSignalSuppress),
			bits_COND_ENEMY_DEAD |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_NO_AMMO_LOADED,

			bits_SOUND_DANGER,
			"SignalSuppress"},
};

Task_t tlMillerHumSuppress[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMillerHumSuppress[] =
	{
		{tlMillerHumSuppress,
			ARRAYSIZE(tlMillerHumSuppress),
			bits_COND_ENEMY_DEAD |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_NO_AMMO_LOADED,

			bits_SOUND_DANGER,
			"Suppress"},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t tlMillerHumWaitInCover[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)1},
};

Schedule_t slMillerHumWaitInCover[] =
	{
		{tlMillerHumWaitInCover,
			ARRAYSIZE(tlMillerHumWaitInCover),
			bits_COND_NEW_ENEMY |
				bits_COND_HEAR_SOUND |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK2,

			bits_SOUND_DANGER,
			"GruntWaitInCover"},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t tlMillerHumTakeCover1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_MILLER_HUM_TAKECOVER_FAILED},
		{TASK_WAIT, (float)0.2},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_GRUNT_SPEAK_SENTENCE, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_SET_SCHEDULE, (float)SCHED_MILLER_HUM_WAIT_FACE_ENEMY},
};

Schedule_t slMillerHumTakeCover[] =
	{
		{tlMillerHumTakeCover1,
			ARRAYSIZE(tlMillerHumTakeCover1),
			0,
			0,
			"TakeCover"},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlMillerHumGrenadeCover1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_COVER_FROM_ENEMY, (float)99},
		{TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, (float)384},
		{TASK_PLAY_SEQUENCE, (float)ACT_SPECIAL_ATTACK1},
		{TASK_CLEAR_MOVE_WAIT, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_SET_SCHEDULE, (float)SCHED_MILLER_HUM_WAIT_FACE_ENEMY},
};

Schedule_t slMillerHumGrenadeCover[] =
	{
		{tlMillerHumGrenadeCover1,
			ARRAYSIZE(tlMillerHumGrenadeCover1),
			0,
			0,
			"GrenadeCover"},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlMillerHumTossGrenadeCover1[] =
	{
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RANGE_ATTACK2, (float)0},
		{TASK_SET_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY},
};

Schedule_t slMillerHumTossGrenadeCover[] =
	{
		{tlMillerHumTossGrenadeCover1,
			ARRAYSIZE(tlMillerHumTossGrenadeCover1),
			0,
			0,
			"TossGrenadeCover"},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t tlMillerHumTakeCoverFromBestSound[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_COWER}, // duck and cover if cannot move from explosion
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_COVER_FROM_BEST_SOUND, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_TURN_LEFT, (float)179},
};

Schedule_t slMillerHumTakeCoverFromBestSound[] =
	{
		{tlMillerHumTakeCoverFromBestSound,
			ARRAYSIZE(tlMillerHumTakeCoverFromBestSound),
			0,
			0,
			"GruntTakeCoverFromBestSound"},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t tlMillerHumHideReload[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RELOAD},
};

Schedule_t slMillerHumHideReload[] =
	{
		{tlMillerHumHideReload,
			ARRAYSIZE(tlMillerHumHideReload),
			bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"GruntHideReload"}};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t tlMillerHumSweep[] =
	{
		{TASK_TURN_LEFT, (float)179},
		{TASK_WAIT, (float)1},
		{TASK_TURN_LEFT, (float)179},
		{TASK_WAIT, (float)1},
};

Schedule_t slMillerHumSweep[] =
	{
		{tlMillerHumSweep,
			ARRAYSIZE(tlMillerHumSweep),

			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_HEAR_SOUND,

			bits_SOUND_WORLD | // sound flags
				bits_SOUND_DANGER |
				bits_SOUND_PLAYER,

			"Grunt Sweep"},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMillerHumRangeAttack1A[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMillerHumRangeAttack1A[] =
	{
		{tlMillerHumRangeAttack1A,
			ARRAYSIZE(tlMillerHumRangeAttack1A),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_HEAR_SOUND |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_NO_AMMO_LOADED,

			bits_SOUND_DANGER,
			"Range Attack1A"},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMillerHumRangeAttack1B[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_IDLE_ANGRY},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMillerHumRangeAttack1B[] =
	{
		{tlMillerHumRangeAttack1B,
			ARRAYSIZE(tlMillerHumRangeAttack1B),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_NO_AMMO_LOADED |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"Range Attack1B"},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMillerHumRangeAttack2[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_GRUNT_CHECK_FIRE, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RANGE_ATTACK2},
		{TASK_SET_SCHEDULE, (float)SCHED_MILLER_HUM_WAIT_FACE_ENEMY}, // don't run immediately after throwing grenade.
};

Schedule_t slMillerHumRangeAttack2[] =
	{
		{tlMillerHumRangeAttack2,
			ARRAYSIZE(tlMillerHumRangeAttack2),
			0,
			0,
			"RangeAttack2"},
};

DEFINE_CUSTOM_SCHEDULES(CHMiller){
	slMillerHumFail,
	slMillerHumCombatFail,
	slMillerHumVictoryDance,
	slMillerHumEstablishLineOfFire,
	slMillerHumFoundEnemy,
	slMillerHumCombatFace,
	slMillerHumSignalSuppress,
	slMillerHumSuppress,
	slMillerHumWaitInCover,
	slMillerHumTakeCover,
	slMillerHumGrenadeCover,
	slMillerHumTossGrenadeCover,
	slMillerHumTakeCoverFromBestSound,
	slMillerHumHideReload,
	slMillerHumSweep,
	slMillerHumRangeAttack1A,
	slMillerHumRangeAttack1B,
	slMillerHumRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES(CHMiller, CSquadMonster);
//=========================================================
// Get Schedule!
//=========================================================
Schedule_t* CHMiller::GetSchedule()
{

	// clear old sentence
	m_iSentence = MILLER_HUM_SENT_NONE;

	// grunts place HIGH priority on running away from danger sounds.
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound)
		{
			if ((pSound->m_iType & bits_SOUND_DANGER) != 0)
			{
				// dangerous sound nearby!

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause
				// this may only affect a single individual in a squad.

				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "HG_GREN", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch (m_MonsterState)
	{
		case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if (HasConditions(bits_COND_ENEMY_DEAD))
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			// new enemy
			if (HasConditions(bits_COND_NEW_ENEMY))
			{
				if (InSquad())
				{
					MySquadLeader()->m_fEnemyEluded = false;

					if (!IsLeader())
					{
						return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
					}
					else
					{
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight"
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak()) // && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz(ENT(pev), "HG_ALERT", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									 (m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
									 (m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
									 (!m_hEnemy->IsMachine(m_hEnemy)))
								// monster
								SENTENCEG_PlayRndSz(ENT(pev), "HG_MONST", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);

							JustSpoke();
						}

						if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
						{
							return GetScheduleOfType(SCHED_MILLER_HUM_SUPPRESS);
						}
						else
						{
							return GetScheduleOfType(SCHED_MILLER_HUM_ESTABLISH_LINE_OF_FIRE);
						}
					}
				}
			}
			// no ammo
			else if (HasConditions(bits_COND_NO_AMMO_LOADED))
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo.
				// He's going to try to find cover to run to and reload, but rarely, if
				// none is available, he'll drop and reload in the open here.
				return GetScheduleOfType(SCHED_MILLER_HUM_COVER_AND_RELOAD);
			}

			// damaged just a little
			else if (HasConditions(bits_COND_LIGHT_DAMAGE))
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0, 99);

				if (iPercent <= 60 && m_hEnemy != NULL)
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);
						m_iSentence = MILLER_HUM_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
				}
				else
				{
					return GetScheduleOfType(SCHED_SMALL_FLINCH);
				}
			}
			// can kick
			else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
			{
				return GetScheduleOfType(SCHED_MELEE_ATTACK1);
			}
			// can grenade launch

			else if (FBitSet(pev->weapons, MILLER_HUM_GRENADELAUNCHER) && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				// shoot a grenade if you can
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			// can shoot
			else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			{
				if (InSquad())
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a
					// little time and give the player a chance to turn.
					if (MySquadLeader()->m_fEnemyEluded && !HasConditions(bits_COND_ENEMY_FACING_ME))
					{
						MySquadLeader()->m_fEnemyEluded = false;
						return GetScheduleOfType(SCHED_MILLER_HUM_FOUND_ENEMY);
					}
				}

				if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType(SCHED_RANGE_ATTACK1);
				}
				else if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType(SCHED_RANGE_ATTACK2);
				}
				else
				{
					// hide!
					return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
				}
			}

			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE) && RANDOM_LONG(0,1) == 1)
			{
				//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "HG_THROW", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE) && HasConditions(bits_COND_ENEMY_OCCLUDED))
			{
				//!!!KELLY - grunt cannot see the enemy and has just decided to
				// charge the enemy's position.
				if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);
					m_iSentence = MILLER_HUM_SENT_CHARGE;
					//JustSpoke();
				}

				return GetScheduleOfType(SCHED_MILLER_HUM_ESTABLISH_LINE_OF_FIRE);
			}
			else if (HasConditions(bits_COND_ENEMY_OCCLUDED))
			{
				//!!!KELLY - grunt is going to stay put for a couple seconds to see if
				// the enemy wanders back out into the open, or approaches the
				// grunt's covered position. Good place for a taunt, I guess?
				if (FOkToSpeak() && RANDOM_LONG(0, 1))
				{
					SENTENCEG_PlayRndSz(ENT(pev), "HG_TAUNT", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_STANDOFF);
			}

			if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			{
				return GetScheduleOfType(SCHED_MILLER_HUM_ESTABLISH_LINE_OF_FIRE);
			}
		}
	}

	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();
		ASSERT(pSound != NULL);
		if (pSound)
		{
			if (pSound && (pSound->m_iType & bits_SOUND_COMBAT | bits_SOUND_PLAYER) != 0) // Hear an enemy
			{
				return GetScheduleOfType(SCHED_INVESTIGATE_SOUND);
			}
		}
	}

	// no special cases here, call the base class
	return CSquadMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CHMiller::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		if (InSquad())
		{
			if (g_iSkillLevel >= SKILL_HARD && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "HG_THROW", MILLER_HUM_SENTENCE_VOLUME, MILLER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return slMillerHumTossGrenadeCover;
			}
			else
			{
				return &slMillerHumTakeCover[0];
			}
		}
		else
		{
			if (RANDOM_LONG(0, 1))
			{
				return &slMillerHumTakeCover[0];
			}
			else
			{
				return &slMillerHumGrenadeCover[0];
			}
		}
	}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	{
		return &slMillerHumTakeCoverFromBestSound[0];
	}
	case SCHED_MILLER_HUM_TAKECOVER_FAILED:
	{
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_FAIL);
	}
	break;
	case SCHED_MILLER_HUM_ELOF_FAIL:
	{
		// human grunt is unable to move to a position that allows him to attack the enemy.
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;
	case SCHED_MILLER_HUM_ESTABLISH_LINE_OF_FIRE:
	{
		return &slMillerHumEstablishLineOfFire[0];
	}
	break;
	case SCHED_RANGE_ATTACK1:
	{
		// randomly stand or crouch
		if (RANDOM_LONG(0, 9) == 0)
			m_fStanding = RANDOM_LONG(0, 1);

		if (m_fStanding)
			return &slMillerHumRangeAttack1B[0];
		else
			return &slMillerHumRangeAttack1A[0];
	}
	case SCHED_RANGE_ATTACK2:
	{
		return &slMillerHumRangeAttack2[0];
	}
	case SCHED_COMBAT_FACE:
	{
		return &slMillerHumCombatFace[0];
	}
	case SCHED_MILLER_HUM_WAIT_FACE_ENEMY:
	{
		return &slMillerHumWaitInCover[0];
	}
	case SCHED_MILLER_HUM_SWEEP:
	{
		return &slMillerHumSweep[0];
	}
	case SCHED_MILLER_HUM_COVER_AND_RELOAD:
	{
		return &slMillerHumHideReload[0];
	}
	case SCHED_MILLER_HUM_FOUND_ENEMY:
	{
		return &slMillerHumFoundEnemy[0];
	}
	case SCHED_VICTORY_DANCE:
	{
		if (InSquad())
		{
			if (!IsLeader())
			{
				return &slMillerHumFail[0];
			}
		}

		return &slMillerHumVictoryDance[0];
	}
	case SCHED_MILLER_HUM_SUPPRESS:
	{
		if (m_hEnemy->IsPlayer() && m_fFirstEncounter)
		{
			m_fFirstEncounter = false; // after first encounter, leader won't issue handsigns anymore when he has a new enemy
			return &slMillerHumSignalSuppress[0];
		}
		else
		{
			return &slMillerHumSuppress[0];
		}
	}
	case SCHED_FAIL:
	{
		if (m_hEnemy != NULL)
		{
			// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
			return &slMillerHumCombatFail[0];
		}

		return &slMillerHumFail[0];
	}
	default:
	{
		return CSquadMonster::GetScheduleOfType(Type);
	}
	}
}

//=========================================================
// TraceAttack - make sure we're not taking it in the mask
//=========================================================
void CHMiller::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	Vector vecTemp = pevAttacker->origin - (pev->absmin + (pev->size * 0.5));
	g_vecAttackDir = vecTemp.Normalize();
	Vector vecVelocity = -g_vecAttackDir * 200;

	m_bloodColor = BLOOD_COLOR_RED;

	if (ptr->iHitgroup == 0)
	{
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	// check for helmet shot
	
	if (ptr->iHitgroup == HITGROUP_HEAVY_VISOR)
	{
		if (m_helmvisorDUR > 0)
		{
			m_bloodColor = DONT_BLEED;
			m_helmvisorDUR -= 1;
			if (RANDOM_LONG(0,1) == 1)
				UTIL_Sparks(ptr->vecEndPos);
			flDamage = round(flDamage * 0.1);

			CBaseEntity::BulletRic(pevAttacker, vecDir, ptr, bitsDamageType, this); // easier way to handle ricochet

			if (m_helmvisorDUR <= 0)
			{
				m_bloodColor = BLOOD_COLOR_RED;
				// remove armor
				ArmorGibs(ptr, vecVelocity);
			}
		}
		else
		{
			m_bloodColor = BLOOD_COLOR_RED;
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	if (ptr->iHitgroup == HITGROUP_CHEST)
	{
		if (m_iarmor_health_chest > 0)
		{
			m_bloodColor = DONT_BLEED;
			m_iarmor_health_chest -= 1;
			if (RANDOM_LONG(0,1) == 1)
				UTIL_Sparks(ptr->vecEndPos);
			flDamage = round(flDamage * 0.05);

			CBaseEntity::BulletRic(pevAttacker, vecDir, ptr, bitsDamageType, this); // easier way to handle ricochet

			if (m_iarmor_health_chest <= 0)
			{
				m_bloodColor = BLOOD_COLOR_RED;
				// remove armor
				ArmorGibs(ptr, vecVelocity);
			}
		}
		else
		{
			m_bloodColor = BLOOD_COLOR_RED;
			flDamage = round(flDamage * 0.5);
		}
	}
	if (ptr->iHitgroup == HITGROUP_STOMACH)
	{
		if (m_iarmor_health_stomach > 0)
		{
			m_bloodColor = DONT_BLEED;
			m_iarmor_health_stomach -= 1;
			if (RANDOM_LONG(0,1) == 1)
				UTIL_Sparks(ptr->vecEndPos);
			flDamage = round(flDamage * 0.05);

			CBaseEntity::BulletRic(pevAttacker, vecDir, ptr, bitsDamageType, this); // easier way to handle ricochet

			if (m_iarmor_health_stomach <= 0)
			{
				m_bloodColor = BLOOD_COLOR_RED;
				// remove armor
				ArmorGibs(ptr, vecVelocity);
			}
		}
		else
		{
			m_bloodColor = BLOOD_COLOR_RED;
			flDamage = round(flDamage * 0.5);
		}
	}
	
	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
bool CHMiller::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Forget(bits_MEMORY_INCOVER);

	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// IRelationship - overridden because miller prioritizes targetting the player
//=========================================================
int CHMiller::IRelationship(CBaseEntity* pTarget)
{
	if (pTarget->IsPlayer())
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship(pTarget);
}

void CHMiller::ArmorGibs(TraceResult* ptr, Vector Vel)
{
	int iConditions = IScheduleFlags();
	if ((iConditions & bits_COND_HEAVY_DAMAGE) == 0)
	{
		SetConditions(bits_COND_LIGHT_DAMAGE);
	}
	
	EMIT_SOUND_DYN(ENT(pev), CHAN_AUTO, RANDOM_SOUND_ARRAY(pSoundsMetal), 1.0, ATTN_NORM, 0, RANDOM_LONG(80, 90));
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos);
	WRITE_BYTE(TE_BREAKMODEL);
	// position
	WRITE_COORD(ptr->vecEndPos.x);
	WRITE_COORD(ptr->vecEndPos.y);
	WRITE_COORD(ptr->vecEndPos.z);
	// size
	WRITE_COORD(pev->size.x/6);
	WRITE_COORD(pev->size.x/6);
	WRITE_COORD(pev->size.x/6);
	// velocity
	WRITE_COORD(Vel.x);
	WRITE_COORD(Vel.y);
	WRITE_COORD(Vel.z);
	// randomization
	WRITE_BYTE(10);
	// Model
	WRITE_SHORT(m_idShard); //model id#
	// # of shards
	WRITE_BYTE(4); // let client decide
	// duration
	WRITE_BYTE(25); // 2.5 seconds
	// flags
	WRITE_BYTE(BREAK_METAL);
	MESSAGE_END();
}