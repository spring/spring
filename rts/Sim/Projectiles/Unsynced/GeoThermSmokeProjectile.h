/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GEOTHERMSMOKE_H
#define GEOTHERMSMOKE_H

#include "SmokeProjectile.h"

class CFeature;

class CGeoThermSmokeProjectile : public CSmokeProjectile
{
	CR_DECLARE(CGeoThermSmokeProjectile)
public:
	CGeoThermSmokeProjectile(const float3& pos,const float3& speed,int ttl,CFeature *geo);
	void Update();

	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points) {};
	CFeature *geo;

public:
	static void GeoThermDestroyed(const CFeature* geo);
};

#endif
