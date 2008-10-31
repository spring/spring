#ifndef REPULSEGFX_H
#define REPULSEGFX_H

#include "Sim/Projectiles/Projectile.h"

class CUnit;

class CRepulseGfx :
	public CProjectile
{
	CR_DECLARE(CRepulseGfx);
public:
	CRepulseGfx(CUnit* owner,CProjectile* repulsed,float maxDist,float3 color GML_PARG_H);
	~CRepulseGfx(void);

	void DependentDied(CObject* o);
	void Draw(void);
	void Update(void);

	CProjectile* repulsed;
	float sqMaxDist;
	int age;
	float3 color;

	float difs[25];
};

#endif
