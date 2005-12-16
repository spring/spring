#ifndef WEAPONPROJECTILE_H
#define WEAPONPROJECTILE_H

#include "Projectile.h"
#include "DamageArray.h"

struct WeaponDef;

/*
* Base class for all projectiles originating from a weapon or having weapon-propertiers. Uses data from a weapon defenition.
*/
class CWeaponProjectile :
	public CProjectile
{
public:
	CWeaponProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef,CWeaponProjectile* interceptTarget);
	virtual ~CWeaponProjectile();

	virtual void Collision();
	virtual void Collision(CFeature* feature);
	virtual void Collision(CUnit* unit);
	virtual void Update();
	virtual void DrawUnitPart();

	static CWeaponProjectile *CreateWeaponProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef);
	bool targeted;				//if we are a nuke and a anti is on the way
	WeaponDef *weaponDef;
protected:
	CUnit *target;
	float3 targetPos;
	float3 startpos;
	DamageArray damages;
	int ttl;
	unsigned int modelDispList;

	bool TraveledRange();
	CWeaponProjectile* interceptTarget;
public:
	void DependentDied(CObject* o);
};


#endif /* WEAPONPROJECTILE_H */
