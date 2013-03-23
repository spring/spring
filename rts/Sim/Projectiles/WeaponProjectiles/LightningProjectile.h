/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LIGHTNING_PROJECTILE_H
#define LIGHTNING_PROJECTILE_H

#include "WeaponProjectile.h"

class CWeapon;

class CLightningProjectile : public CWeaponProjectile
{
	CR_DECLARE(CLightningProjectile);
public:
	CLightningProjectile(const ProjectileParams& params);

	void Update();
	void Draw();
	void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);

private:
	float3 color;

	#define displacements_size 10
	float displacements[displacements_size];
	float displacements2[displacements_size];
};

#endif /* LIGHTNING_PROJECTILE_H */
