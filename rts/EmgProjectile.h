#pragma once
#include "weaponprojectile.h"
#include "damagearray.h"

class CEmgProjectile :
	public CWeaponProjectile
{
public:
	CEmgProjectile(const float3& pos,const float3& speed,CUnit* owner,const DamageArray& damages,const float3& color,float intensity, int ttl, WeaponDef *weaponDef);
	virtual ~CEmgProjectile();

	int ttl;
	float intensity;
	float3 color;
	DamageArray damages;

	void Update(void);
	void Collision(CUnit* unit);
	void Draw(void);
};
