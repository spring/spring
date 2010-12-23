/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EMG_PROJECTILE_H
#define _EMG_PROJECTILE_H

#include "WeaponProjectile.h"

class CEmgProjectile : public CWeaponProjectile
{
	CR_DECLARE(CEmgProjectile);
public:
	CEmgProjectile(const float3& pos, const float3& speed, CUnit* owner,
			const float3& color, float intensity, int ttl,
			const WeaponDef* weaponDef);
	virtual ~CEmgProjectile();

	void Update();
	void Draw();
	void Collision(CUnit* unit);
	void Collision();

	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos,
			float shieldForce, float shieldMaxSpeed);

private:
	float intensity;
	float3 color;
};

#endif // _EMG_PROJECTILE_H
