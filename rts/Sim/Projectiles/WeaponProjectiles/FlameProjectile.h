/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FLAME_PROJECTILE_H_
#define _FLAME_PROJECTILE_H_

#include "WeaponProjectile.h"

class CFlameProjectile : public CWeaponProjectile
{
	CR_DECLARE(CFlameProjectile);
public:
	CFlameProjectile(const float3& pos, const float3& speed,
			const float3& spread, CUnit* owner, const WeaponDef* weaponDef,
			int ttl = 50);
	~CFlameProjectile();

	void Update();
	void Draw();
	void Collision(CUnit* unit);
	void Collision();

	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos,
			float shieldForce, float shieldMaxSpeed);

private:
	float3 color;
	float3 color2;
	float intensity;
	float3 spread;
	float curTime;
	/// precentage of lifetime when the projectile is active and can collide
	float physLife;
	float invttl;
};

#endif // _FLAME_PROJECTILE_H_
