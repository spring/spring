/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MUZZLE_FLAME_H
#define MUZZLE_FLAME_H

#include "Sim/Projectiles/Projectile.h"

class CMuzzleFlame : public CProjectile
{
	CR_DECLARE_DERIVED(CMuzzleFlame)

public:
	CMuzzleFlame() { }
	CMuzzleFlame(const float3& pos, const float3& speed, const float3& dir, float size);

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override;

	int GetProjectilesCount() const override { return (numSmoke * 2); }

private:
	float size;
	int age;
	int numFlame;
	int numSmoke;

	std::vector<float3> randSmokeDir;
};


#endif /* MUZZLE_FLAME_H */
