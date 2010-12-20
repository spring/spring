/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TORPEDO_PROJECTILE_H
#define TORPEDO_PROJECTILE_H

#include "WeaponProjectile.h"

class CTorpedoProjectile : public CWeaponProjectile
{
	CR_DECLARE(CTorpedoProjectile);
public:
	CTorpedoProjectile(const float3& pos, const float3& speed, CUnit* owner,
			float areaOfEffect, float maxSpeed, float tracking, int ttl,
			CUnit* target, const WeaponDef* weaponDef);
	~CTorpedoProjectile();
	void DependentDied(CObject* o);
	void Collision(CUnit* unit);
	void Collision();

	void Update();
	void Draw();

private:
	float tracking;
	float maxSpeed;
	float curSpeed;
	float areaOfEffect;
	CUnit* target;
	int nextBubble;
	float texx;
	float texy;
};


#endif /* TORPEDO_PROJECTILE_H */
