#pragma once

#include "game.h"


class CFireVoxel
{
public:
	void Think();
	
	Vector origin;
	
	float thinktime;
	
	float spreadTime;

	int heat;

private:
	const void SpawnParticles(int size);
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