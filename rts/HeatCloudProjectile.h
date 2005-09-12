#ifndef HEATCLOUDPROJECTILE_H
#define HEATCLOUDPROJECTILE_H
// HeatCloudProjectile.h: interface for the CHeatCloudProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Projectile.h"

class CHeatCloudProjectile : public CProjectile  
{
public:
public:
	virtual void Draw();
	virtual void Update();
	CHeatCloudProjectile(const float3 pos,const float3 speed,const float temperature,const float size, CUnit* owner);  //projectile start at size 0 and ends at size size
	CHeatCloudProjectile(const float3 pos,const float3 speed,const float temperature,const float size, float sizegrowth, CUnit* owner);  //size is initial size and sizegrowth is size increase per tick
	virtual ~CHeatCloudProjectile();

	float heat;
	float maxheat;
	float heatFalloff;
	float size;
	float sizeGrowth;
	float sizemod;
	float sizemodmod;
};

#endif /* HEATCLOUDPROJECTILE_H */
