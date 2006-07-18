#ifndef SPHEREPARTPROJECTILE_H
#define SPHEREPARTPROJECTILE_H

#include "Projectile.h"

class CSpherePartProjectile :
	public CProjectile
{
public:
	CSpherePartProjectile(const float3& centerPos,int xpart,int ypart,float expansionSpeed,float alpha,int ttl,CUnit* owner);
	~CSpherePartProjectile(void);

	float3 centerPos;
	float3 vectors[25];

	float sphereSize;
	float expansionSpeed;

	int xbase;
	int ybase;

	float baseAlpha;
	int age;
	int ttl;
	float texx;
	float texy;
	void Draw(void);
	void Update(void);
	static void CreateSphere(float3 pos, float alpha, int ttl, float expansionSpeed , CUnit* owner);
};


#endif /* SPHEREPARTPROJECTILE_H */
