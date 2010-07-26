/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPHERE_PART_PROJECTILE_H
#define _SPHERE_PART_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CSpherePartProjectile : public CProjectile
{
	CR_DECLARE(CSpherePartProjectile);

public:
	CSpherePartProjectile(const float3& centerPos,int xpart,int ypart,float expansionSpeed,float alpha,int ttl,CUnit* owner,const float3 &color);
	~CSpherePartProjectile();

	float3 centerPos;
	float3 vectors[25];
	float3 color;

	float sphereSize;
	float expansionSpeed;

	int xbase;
	int ybase;

	float baseAlpha;
	int age;
	int ttl;
	float texx;
	float texy;
	void Draw();
	void Update();
	static void CreateSphere(float3 pos, float alpha, int ttl, float expansionSpeed , CUnit* owner, float3 color = float3(0.8, 0.8, 0.6));
};

/// This class makes a sphere-part-projectile via the explosion-generator
class CSpherePartSpawner : CProjectile
{
	CR_DECLARE(CSpherePartSpawner);

public:
	CSpherePartSpawner();
	~CSpherePartSpawner();

	virtual void Init(const float3& pos, CUnit* owner);

	float alpha;
	int ttl;
	float expansionSpeed;
	float3 color;
};

#endif /* _SPHERE_PART_PROJECTILE_H */
