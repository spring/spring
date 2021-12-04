/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GEO_SQUARE_PROJECTILE_H
#define GEO_SQUARE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CGeoSquareProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CGeoSquareProjectile)
public:
	CGeoSquareProjectile() { }
	CGeoSquareProjectile(
		const float3& p1,
		const float3& p2,
		const float3& v1,
		const float3& v2,
		float xsize,
		float ysize
	);

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override {}

	int GetProjectilesCount() const override { return 1; }

	void SetColor(float r, float g, float b, float a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

private:
	float3 p1; // start
	float3 p2; // end
	float3 v1;
	float3 v2;

	float xsize;
	float ysize;
	float r, g, b, a; ///< RGBA color
};


#endif /* GEO_SQUARE_PROJECTILE_H */
