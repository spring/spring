/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXPLO_SPIKE_PROJECTILE_H
#define EXPLO_SPIKE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CExploSpikeProjectile : public CProjectile
{
	CR_DECLARE(CExploSpikeProjectile);

	/// used only by creg
	CExploSpikeProjectile();

public:
	CExploSpikeProjectile(
		CUnit* owner,
		const float3& pos,
		const float3& spd,
		float length,
		float width,
		float alpha,
		float alphaDecay
	);

	void Draw();
	void Update();

	void Init(const CUnit* owner, const float3& offset);

private:
	float length;
	float width;
	float alpha;
	float alphaDecay;
	float lengthGrowth;
	float3 color;
};

#endif // EXPLO_SPIKE_PROJECTILE_H
