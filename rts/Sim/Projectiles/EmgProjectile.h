#ifndef __EMG_PROJECTILE_H__
#define __EMG_PROJECTILE_H__

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

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

#endif // __EMG_PROJECTILE_H__
