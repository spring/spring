#ifndef WEAPONPROJECTILE_H
#define WEAPONPROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
#include "Sim/Misc/DamageArray.h"

struct WeaponDef;
class CPlasmaRepulser;
/*
* Base class for all projectiles originating from a weapon or having weapon-properties. Uses data from a weapon definition.
*/
class CWeaponProjectile : public CProjectile
{
	CR_DECLARE(CWeaponProjectile);
public:
	CWeaponProjectile();
	CWeaponProjectile(const float3& pos, const float3& speed, CUnit* owner,
			CUnit* target, const float3 &targetPos, const WeaponDef *weaponDef,
			CWeaponProjectile* interceptTarget, bool synced, int ttl = 1 GML_PARG_H);
	virtual ~CWeaponProjectile();

	virtual void Collision();
	virtual void Collision(CFeature* feature);
	virtual void Collision(CUnit* unit);
	virtual void Update();
	virtual int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed) { return 0; };	//return 0=unaffected,1=instant repulse,2=gradual repulse

	virtual void DrawUnitPart();
	// should not be here
	void DrawS3O(void);

	// true if we are a nuke and an anti is on the way
	bool targeted;
	const WeaponDef* weaponDef;
	std::string weaponDefName;
	CUnit *target;
	float3 targetPos;

	CCustomExplosionGenerator ceg;
	std::string cegTag;

protected:
	float3 startpos;
	int ttl;
	int colorTeam;
	int bounces;
	
	virtual void UpdateGroundBounce();
	bool keepBouncing;

	bool TraveledRange();
	CWeaponProjectile* interceptTarget;
public:
	void DependentDied(CObject* o);
	void PostLoad();
};


#endif /* WEAPONPROJECTILE_H */
