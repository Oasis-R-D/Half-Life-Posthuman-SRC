#pragma once

#include "game.h"


class CFireVoxel
{
public:
	void Think();
	
	Vector origin;
	
	float thinktime;
	
	float spreadTime;

	float dmgTime;

	int heat;

private:
	const void SpawnParticles(int size);
	void DealDamage(int size);
	bool CheckFall(float size);
};

class CFireManager : public CBaseEntity
{
public:
	CFireManager();
	~CFireManager();

	void Spawn() override;
	void Precache() override;
	
	
	void AddFire(Vector pos, int heat);
	void ExtinguishFire(Vector pos, int heat, int radiusSquared);
	void EXPORT ManagerThink();
	bool MergeFirePoints(Vector pos, int heat);
	void FireExplosion(Vector pos, int radius, int heat);

private:
	void MergeInTemp();
	void RemoveDead();
	

	std::vector<std::unique_ptr<CFireVoxel>> voxels;
	std::vector<std::unique_ptr<CFireVoxel>> tempvoxels;
};

inline Vector translateToFireSpace(Vector& vec)
{
	int firesize = sv_firesize.value;
	vec.x = firesize * round(vec.x / firesize);
	vec.y = firesize * round(vec.y / firesize);
	vec.z = firesize * ceil(vec.z / firesize); // attempt to NOT get stuck in floors
	return vec;
}

extern CFireManager* FireManager;

class CFireProjectile : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int ShouldCollide(CBaseEntity* pentTouched) override;
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
	void EXPORT AirThink();
	void EXPORT DropTouch(CBaseEntity* pOther);
	static void FireShoot(unsigned int BLDamnt, int heat, int BLDSpeed, Vector VecSpawnPos, Vector vecDir, float BLLTGravity, float spread = 0); // add damage, spread and owner so entities calling this can give it the proper stuff

private:
	int m_vecVel;
	Vector m_SpawnPos;
	Vector m_vecDir;
	float m_Spread;
	float m_Gravity;
};