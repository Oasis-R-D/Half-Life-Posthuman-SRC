/***
*
* what the sigma???!!!
*
****/

#include "squadmonster.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define GRUNT_CLIP_SIZE 36	 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_VOL 0.35		 // volume of grunt sounds
#define GRUNT_ATTN ATTN_NORM // attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH 20
#define HGRUNT_DMG_HEADSHOT (DMG_BULLET | DMG_CLUB) // damage types that can kill a grunt with a single headshot.
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE 15			// must do at least this much damage in one shot to head to score a headshot kill
#define HGRUNT_SENTENCE_VOLUME (float)0.35			// volume of grunt sentences

#define HGRUNT_9MMAR (1 << 0)
#define HGRUNT_HANDGRENADE (1 << 1)
#define HGRUNT_GRENADELAUNCHER (1 << 2)
#define HGRUNT_SHOTGUN (1 << 3)
#define HGRUNT_M249 (1 << 4)
#define HGRUNT_M727 (1 << 5)

#define ARMOR_GROUP 0
#define HEAD_GROUP 1
#define TORSO_GROUP 2
#define GUN_GROUP 3

#define ARMOR_DEF 0
#define ARMOR_ENGI 1

#define HEAD_GASMASK 0
#define HEAD_GASMASK_BLACK 1
#define HEAD_HELM_1 2
#define HEAD_HELM_2 3
#define HEAD_HELM_3 4
#define HEAD_HELM_4 5
#define HEAD_HELM_5 6
#define HEAD_HELM_6 7
#define HEAD_HELM_7 8
#define HEAD_COMMANDER 9
#define HEAD_SHOTGUN 10
#define HEAD_M249_1 11
#define HEAD_M249_2 12
#define HEAD_M203_1 13
#define HEAD_M203_2 14
#define HEAD_MEDIC 15
#define HEAD_MEDIC_BLACK 16
#define HEAD_ENGI 17
#define HEAD_CIGAR 18
#define HEAD_CIGAR_H 19 // H are variants with their hands out (no gloves)
#define HEAD_NOHELM 20
#define HEAD_NOHELM_H 21

#define TORSO_GRUNT 0
#define TORSO_M249 1
#define TORSO_SHOTGUN 2
#define TORSO_ENGI 3
#define TORSO_MED 4

#define GUN_MP5 0
#define GUN_SHOTGUN 1
#define GUN_M249 2
#define GUN_M727 3
#define GUN_NONE 4

#define HITGROUP_HELMET 69
#define HITGROUP_PROPANETANK 67

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define HGRUNT_AE_RELOAD (2)
#define HGRUNT_AE_KICK (3)
#define HGRUNT_AE_BURST1 (4)
#define HGRUNT_AE_BURST2 (5)
#define HGRUNT_AE_BURST3 (6)
#define HGRUNT_AE_GREN_TOSS (7)
#define HGRUNT_AE_GREN_LAUNCH (8)
#define HGRUNT_AE_GREN_DROP (9)
#define HGRUNT_AE_CAUGHT_ENEMY (10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define HGRUNT_AE_DROP_GUN (11)		// grunt (probably dead) is dropping his mp5.

class CHGrunt : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	int ISoundMask() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	bool FCanCheckAttacks() override;
	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	void CheckAmmo() override;
	void SetActivity(Activity NewActivity) override;
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;
	Vector GetGunPosition() override;
	void Shoot();
	void Shotgun();
	void M249();
	void ShootM727();
	void ClipSize(int clipsize);
	void PrescheduleThink() override;
	void SpeakSentence();
	void Killed(entvars_t* pevAttacker, int iGib) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	CBaseEntity* Kick();
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;

	int IRelationship(CBaseEntity* pTarget) override;

	bool FOkToSpeak();
	void JustSpoke();

	int PoolAtt() override { return 1; }

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
	bool m_medic = false;
	bool m_fuel = false;
	bool m_bHeavyGrunt = false;
	int m_cClipSize;
	int m_tankhealth = 20;
	int m_voicePitch;

	int m_iBrassShell;
	int m_iShotgunShell;

	int m_iSentence;
	int m_iShell;
	int m_iLink;

	int m_helmDUR;

	static const char* pGruntSentences[];
};
