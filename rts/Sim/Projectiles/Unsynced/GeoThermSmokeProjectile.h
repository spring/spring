/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GEO_THERM_SMOKE_H
#define GEO_THERM_SMOKE_H

#include "SmokeProjectile.h"

class CFeature;

class CGeoThermSmokeProjectile : public CSmokeProjectile
{
	CR_DECLARE(CGeoThermSmokeProjectile)
public:
	CGeoThermSmokeProjectile(
		const float3& pos,
		const float3& spd,
		int ttl,
		const CFeature* geo
	);

	void Update();
	void UpdateDir();

	void DrawOnMinimap(CVertexArray& lines, CVertexArray& points) {}

	static void GeoThermDestroyed(const CFeature* geo);

private:
	const CFeature* geo;
};

#endif // GEO_THERM_SMOKE_H
