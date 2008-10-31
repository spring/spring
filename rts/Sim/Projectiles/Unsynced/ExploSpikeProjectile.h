#ifndef EXPLOSPIKEPROJECTILE_H
#define EXPLOSPIKEPROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CExploSpikeProjectile :
	public CProjectile
{
	CR_DECLARE(CExploSpikeProjectile);
public:
	CExploSpikeProjectile();
	CExploSpikeProjectile(const float3& pos,const float3& speed,float length,float width,float alpha,float alphaDecay,CUnit* owner GML_PARG_H);
	~CExploSpikeProjectile(void);
	void Update(void);
	void Draw(void);

	float length;
	float width;
	float alpha;
	float alphaDecay;
	float lengthGrowth;
	float3 dir;
	float3 color;

	virtual void Init(const float3& pos, CUnit *owner GML_PARG_H);
};

#endif
