/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MUZZLE_FLAME_H
#define MUZZLE_FLAME_H

#include "Sim/Projectiles/Projectile.h"

class CMuzzleFlame : public CProjectile
{
	CR_DECLARE(CMuzzleFlame);

	void creg_Serialize(creg::ISerializer& s);
public:
	CMuzzleFlame(const float3& pos, const float3& speed, const float3& dir, float size);
	~CMuzzleFlame();

	void Draw();
	void Update();

	float3 dir;
	float size;
	int age;
	int numFlame;
	int numSmoke;

//	float3* randSmokeDir;
	std::vector<float3> randSmokeDir;
};


#endif /* MUZZLE_FLAME_H */
