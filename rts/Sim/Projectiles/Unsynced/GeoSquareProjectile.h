/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GEO_SQUARE_PROJECTILE_H
#define GEO_SQUARE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CGeoSquareProjectile : public CProjectile
{
	CR_DECLARE(CGeoSquareProjectile);
public:
	CGeoSquareProjectile(const float3& p1, const float3& p2, const float3& v1, const float3& v2, float w1, float w2);
	virtual ~CGeoSquareProjectile();

	void Draw();
	void Update();

	float3 p1, p2, v1, v2; ///< FIXME what is this?
	float w1, w2; ///< FIXME what is this?
	float r, g, b, a; ///< RGBA color
};


#endif /* GEO_SQUARE_PROJECTILE_H */
