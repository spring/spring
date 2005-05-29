#ifndef MUZZLEFLAME_H
#define MUZZLEFLAME_H
/*pragma once removed*/
#include "Projectile.h"

class CMuzzleFlame :
	public CProjectile
{
public:
	CMuzzleFlame(const float3& pos,const float3& speed,const float3& dir,float size);
	~CMuzzleFlame(void);

	void Draw(void);
	void Update(void);

	float3 dir;
	float size;
	int age;
	int numFlame;
	int numSmoke;

	float3* randSmokeDir;
};


#endif /* MUZZLEFLAME_H */
