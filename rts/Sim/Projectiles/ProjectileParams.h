/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_PARAMS_H
#define PROJECTILE_PARAMS_H

#include "System/float3.h"

class CWorldObject;
class CUnit;
struct S3DModel;
struct WeaponDef;

// parameters used to spawn weapon-projectiles
struct ProjectileParams {
	ProjectileParams()
		: target(NULL)
		, owner(NULL)
		, model(NULL)
		, weaponDef(NULL)

		, ownerID(-1u)
		, teamID(-1u)
		, weaponNum(-1u)
		, cegID(-1u)

		, ttl(0)
		, gravity(0.0f)
		, tracking(0.0f)
		, maxRange(0.0f)
		, upTime(-1.0f)

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
	unsigned int weaponNum;
	unsigned int cegID;

	int ttl;
	float gravity;
	float tracking;
	float maxRange;
	float upTime;

	// BeamLaser-specific junk
	float startAlpha;
	float endAlpha;
};

#endif

