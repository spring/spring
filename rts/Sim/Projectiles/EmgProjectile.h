#ifndef __EMG_PROJECTILE_H__
#define __EMG_PROJECTILE_H__

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CEmgProjectile :
	public CWeaponProjectile
{
	CR_DECLARE(CEmgProjectile);
public:
	CEmgProjectile(const float3& pos,const float3& speed,CUnit* owner,const float3& color,float intensity, int ttl, WeaponDef *weaponDef);
	virtual ~CEmgProjectile();
	void Update(void);
	void Draw(void);
	void Collision(CUnit* unit);
	int ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed);

	int ttl;
	float intensity;
	float3 color;
};

#endif // __EMG_PROJECTILE_H__
