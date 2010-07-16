/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TORPEDOPROJECTILE_H
#define TORPEDOPROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CTorpedoProjectile :
	public CWeaponProjectile
{
	CR_DECLARE(CTorpedoProjectile);
public:
	CTorpedoProjectile(const float3& pos, const float3& speed, CUnit* owner, float areaOfEffect,
		float maxSpeed, float tracking, int ttl, CUnit* target, const WeaponDef* weaponDef);
	~CTorpedoProjectile(void);
	void DependentDied(CObject* o);
	void Collision(CUnit* unit);
	void Collision();

	float tracking;
	float maxSpeed;
	float curSpeed;
	float areaOfEffect;
	CUnit* target;
	int nextBubble;
	float texx;
	float texy;

	void Update(void);
	void Draw(void);
};


#endif /* TORPEDOPROJECTILE_H */
