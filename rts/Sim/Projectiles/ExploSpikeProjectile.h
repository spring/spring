#ifndef EXPLOSPIKEPROJECTILE_H
#define EXPLOSPIKEPROJECTILE_H

#include "Projectile.h"

class CExploSpikeProjectile :
	public CProjectile
{
public:
	CExploSpikeProjectile(const float3& pos,const float3& speed,float length,float width,float alpha,float alphaDecay,CUnit* owner);
	~CExploSpikeProjectile(void);
	void Update(void);
	void Draw(void);

	float length;
	float width;
	float alpha;
	float alphaDecay;
	float lengthGrowth;
	float3 dir;
};

#endif
