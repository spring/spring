#ifndef __FLAME_PROJECTILE_H__
#define __FLAME_PROJECTILE_H__

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CFlameProjectile :
	public CWeaponProjectile
{
	CR_DECLARE(CFlameProjectile);
public:
	CFlameProjectile(const float3& pos, const float3& speed, const float3& spread,
		CUnit* owner, const WeaponDef* weaponDef, int ttl = 50 GML_PARG_H);
	~CFlameProjectile(void);
	float3 color;
	float3 color2;
	float intensity;
	float3 spread;
	float curTime;
	float physLife;  //precentage of lifetime when the projectile is active and can collide
	float invttl;
	void Update(void);
	void Draw(void);
	void Collision(CUnit* unit);
	void Collision(void);
	int ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed);
};

#endif // __FLAME_PROJECTILE_H__
