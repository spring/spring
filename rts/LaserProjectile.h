#pragma once
#include "weaponprojectile.h"
#include "damagearray.h"

class CLaserProjectile :
	public CWeaponProjectile
{
public:
	CLaserProjectile(const float3& pos,const float3& speed,CUnit* owner,const DamageArray& damages,float length,const float3& color,float intensity, WeaponDef *weaponDef, int ttl=1000);
	virtual ~CLaserProjectile();

	float3 dir;
	int ttl;
	float intensity;
	float3 color;
	DamageArray damages;
	float length;
	float curLength;
	float speedf;

	void Update(void);
	void Collision(CUnit* unit);
	void Draw(void);
};
