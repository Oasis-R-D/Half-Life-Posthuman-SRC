#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CPhysbullet : public CBaseEntity
{
	void Spawn() override;
	void Precache() override;
	int Classify() override;
  int BulletDAMAGE;
	void EXPORT AirThink();
	void EXPORT BoltTouch(CBaseEntity* pOther);

public:
	static CPhysbullet* BulletCreate(float BLLTDamage, Vector VecSpawnPos, Vector vecDir, Vector vecSpread, int FlareType); // add damage, spread and owner so entities calling this can give it the proper stuff
};
LINK_ENTITY_TO_CLASS(phys_bullet, CPhysbullet);
