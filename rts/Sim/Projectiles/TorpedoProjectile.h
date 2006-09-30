#ifndef TORPEDOPROJECTILE_H
#define TORPEDOPROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CTorpedoProjectile :
	public CWeaponProjectile
{
public:
	CTorpedoProjectile(const float3& pos,const float3& speed,CUnit* owner,float areaOfEffect,float maxSpeed,float tracking, int ttl,CUnit* target, WeaponDef *weaponDef);
	~CTorpedoProjectile(void);
	void DependentDied(CObject* o);
	void Collision(CUnit* unit);
	void Collision();

	float tracking;
	float3 dir;
	float maxSpeed;
	float curSpeed;
	int ttl;
	float areaOfEffect;
	CUnit* target;
	int nextBubble;
	float texx;
	float texy;

	void Update(void);
	void Draw(void);
};


#endif /* TORPEDOPROJECTILE_H */
