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
	CExploSpikeProjectile(const float3& pos,const float3& speed, float length, float width, float alpha, float alphaDecay, CUnit* owner);
	~CExploSpikeProjectile();

	void Draw();
	void Update();

	virtual void Init(const float3& pos, CUnit* owner);

	float length;
	float width;
	float alpha;
	float alphaDecay;
	float lengthGrowth;
	float3 dir;
	float3 color;
};

#endif // EXPLO_SPIKE_PROJECTILE_H
