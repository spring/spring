/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_PROJECTILE_H
#define WEAPON_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
#include "WeaponProjectileTypes.h"

struct WeaponDef;
struct S3DModel;
class CVertexArray;
class CPlasmaRepulser;
class CWeaponProjectile;

struct ProjectileParams {
	ProjectileParams()
		: target(NULL)
		, owner(NULL)
		, model(NULL)
		, weaponDef(NULL)

		, ownerID(-1u)
		, teamID(-1u)
		, weaponID(-1u)
		, cegID(-1u)

		, ttl(0)
		, gravity(0.0f)
		, tracking(0.0f)
		, maxRange(0.0f)

		, startAlpha(0.0f)
		, endAlpha(1.0f)
	{
	}

	float3 pos;
	float3 end;
	float3 speed;
	float3 spread;
	float3 error;

	// unit, feature or weapon projectile to intercept
	CWorldObject* target;
	CUnit* owner;
	S3DModel* model;

	const WeaponDef* weaponDef;

	unsigned int ownerID;
	unsigned int teamID;
	unsigned int weaponID;
	unsigned int cegID;

	int ttl;
	float gravity;
	float tracking;
	float maxRange;

	// BeamLaser-specific junk
	float startAlpha;
	float endAlpha;
};


/**
 * Base class for all projectiles originating from a weapon or having
 * weapon-properties. Uses data from a weapon definition.
 */
class CWeaponProjectile : public CProjectile
{
	CR_DECLARE(CWeaponProjectile);
public:
	CWeaponProjectile();
	CWeaponProjectile(const ProjectileParams& params);
	virtual ~CWeaponProjectile() {}

	virtual void Explode(CUnit* hitUnit, CFeature* hitFeature, float3 impactPos, float3 impactDir);
	virtual void Collision();
	virtual void Collision(CFeature* feature);
	virtual void Collision(CUnit* unit);
	virtual void Update();
	/// @return 0=unaffected, 1=instant repulse, 2=gradual repulse
	virtual int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed) { return 0; }

	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);

	void DependentDied(CObject* o);
	void PostLoad();

	void SetTargetObject(CWorldObject* newTarget) {
		if (newTarget != NULL) {
			targetPos = newTarget->pos;
		}

		target = newTarget;
	}

	const CWorldObject* GetTargetObject() const { return target; }
	      CWorldObject* GetTargetObject()       { return target; }

	const WeaponDef* GetWeaponDef() const { return weaponDef; }

	void SetStartPos(const float3& newStartPos) { startPos = newStartPos; }
	void SetTargetPos(const float3& newTargetPos) { targetPos = newTargetPos; }

	const float3& GetStartPos() const { return startPos; }
	const float3& GetTargetPos() const { return targetPos; }

	void SetBeingIntercepted(bool b) { targeted = b; }
	bool IsBeingIntercepted() const { return targeted; }

	bool TraveledRange() const;

protected:
	void UpdateInterception();
	virtual void UpdateGroundBounce();

protected:
	const WeaponDef* weaponDef;

	CWorldObject* target;

	unsigned int weaponDefID;

	int ttl;
	int bounces;

	/// true if we are an interceptable projectile
	// and an interceptor projectile is on the way
	bool targeted;

	float3 startPos;
	float3 targetPos;
};

#endif /* WEAPON_PROJECTILE_H */
