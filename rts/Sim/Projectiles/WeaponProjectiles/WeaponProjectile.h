/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_PROJECTILE_H
#define WEAPON_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
#include "WeaponProjectileTypes.h"

struct WeaponDef;
class CPlasmaRepulser;
class CWeaponProjectile;

struct ProjectileParams {
	ProjectileParams()
		: target(NULL)
		, owner(NULL)
		, weaponDef(NULL)

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

	const WeaponDef* weaponDef;

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
	CWeaponProjectile(const ProjectileParams& params, const bool isRay = false);
	virtual ~CWeaponProjectile();

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

	void SetStartPos(const float3& newStartPos) { startpos = newStartPos; }
	void SetTargetPos(const float3& newTargetPos) { targetPos = newTargetPos; }

	const float3& GetStartPos() const { return startpos; }
	const float3& GetTargetPos() const { return targetPos; }

protected:
	void UpdateInterception();
	virtual void UpdateGroundBounce();
	bool TraveledRange();

public:
	/// true if we are a nuke and an anti is on the way
	bool targeted;
	const WeaponDef* weaponDef;

	unsigned int weaponDefID;
	unsigned int cegID;

	int colorTeam;

protected:
	CWorldObject* target;

	float3 startpos;
	float3 targetPos;

	int ttl;
	int bounces;
	bool keepBouncing;
};

#endif /* WEAPON_PROJECTILE_H */
