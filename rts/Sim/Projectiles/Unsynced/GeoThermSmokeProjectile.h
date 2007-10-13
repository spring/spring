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
	CFeature *geo;

public:
	static void GeoThermDestroyed(const CFeature* geo);
};

#endif
