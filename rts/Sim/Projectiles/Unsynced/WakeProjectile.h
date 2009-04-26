#ifndef WAKEPROJECTILE_H
#define WAKEPROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CWakeProjectile :
	public CProjectile
{
	CR_DECLARE(CWakeProjectile);
public:
	CWakeProjectile(const float3 pos,const float3 speed,float startSize,float sizeExpansion, CUnit* owner,float alpha,float alphaFalloff,float fadeupTime GML_PARG_H);
	virtual ~CWakeProjectile();
	void Update();
	void Draw();

	float alpha;
	float alphaFalloff;
	float alphaAdd;
	int alphaAddTime;
	float size;
	float sizeExpansion;
	float rotation;
	float rotSpeed;
};

#endif /* WAKEPROJECTILE_H */
