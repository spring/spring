#ifndef WEAPONPROJECTILE_H
#define WEAPONPROJECTILE_H

#include "Projectile.h"
#include "Sim/Misc/DamageArray.h"

struct WeaponDef;
class CPlasmaRepulser;
/*
* Base class for all projectiles originating from a weapon or having weapon-properties. Uses data from a weapon definition.
*/
class CWeaponProjectile : public CProjectile
{
public:
	CWeaponProjectile();
	CWeaponProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef,const DamageArray& damages,CWeaponProjectile* interceptTarget);
	virtual ~CWeaponProjectile();

	virtual void Collision();
	virtual void Collision(CFeature* feature);
	virtual void Collision(CUnit* unit);
	virtual void Update();
	virtual void DrawUnitPart();
	virtual int ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed){return 0;};	//return 0=unaffected,1=instant repulse,2=gradual repulse

	static CWeaponProjectile *CreateWeaponProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef);
	bool targeted;				//if we are a nuke and a anti is on the way
	WeaponDef *weaponDef;
	CUnit *target;
	float3 targetPos;
	DamageArray damages;
protected:
	float3 startpos;
	int ttl;
	unsigned int modelDispList;

	bool TraveledRange();
	CWeaponProjectile* interceptTarget;
public:
	void DependentDied(CObject* o);
};


#endif /* WEAPONPROJECTILE_H */
