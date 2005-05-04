#pragma once
#include "weaponprojectile.h"
#include "damagearray.h"

class CFlameProjectile :
	public CWeaponProjectile
{
public:
	CFlameProjectile(const float3& pos,const float3& speed,const float3& spread,CUnit* owner,const DamageArray& damages, WeaponDef *weaponDef, int ttl=50);
	~CFlameProjectile(void);

	float3 spread;
	float curTime;
	float invttl;
	DamageArray damages;
	void Update(void);
	void Draw(void);
	void Collision(CUnit* unit);
	void Collision(void);
};
