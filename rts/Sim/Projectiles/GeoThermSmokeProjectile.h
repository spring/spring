#ifndef GEOTHERMSMOKE_H
#define GEOTHERMSMOKE_H
#include "SmokeProjectile.h"


class CFeature;

class CGeoThermSmokeProjectile : public CSmokeProjectile
{
public:
	CGeoThermSmokeProjectile(const float3& pos,const float3& speed,int ttl,CFeature *geo);
	void Update();
	CFeature *geo;
};

#endif
