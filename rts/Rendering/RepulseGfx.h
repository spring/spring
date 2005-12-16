#ifndef REPULSEGFX_H
#define REPULSEGFX_H

#include "Projectile.h"

class CUnit;

class CRepulseGfx :
	public CProjectile
{
public:
	CRepulseGfx(CUnit* owner,CProjectile* repulsed,float repulseSpeed,float maxDist);
	~CRepulseGfx(void);

	void DependentDied(CObject* o);
	void Draw(void);
	void Update(void);

	CProjectile* repulsed;
	float repulseSpeed;
	float sqMaxDist;
	int age;

	float difs[25];
};

#endif
