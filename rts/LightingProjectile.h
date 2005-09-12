#ifndef LIGHTINGPROJECTILE_H
#define LIGHTINGPROJECTILE_H

#include "Projectile.h"

class CWeapon;

class CLightingProjectile :
	public CProjectile
{
public:
	CLightingProjectile(const float3& pos,const float3& end,CUnit* owner,const float3& color, int ttl=10,CWeapon* weap=0);
	~CLightingProjectile(void);

	int ttl;
	float3 color;
	float3 endPos;
	CWeapon* weapon;

	float displacements[10];
	float displacements2[10];
	
	void Update(void);
	void Draw(void);
	void DependentDied(CObject* o);
};


#endif /* LIGHTINGPROJECTILE_H */
