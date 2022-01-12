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
	float3 pos;
	float3 end;
	float3 speed;
	float3 spread;
	float3 error;

	// unit, feature or weapon projectile to intercept
	CWorldObject* target = nullptr;
	CUnit* owner = nullptr;
	S3DModel* model = nullptr;

	const WeaponDef* weaponDef = nullptr;

	unsigned int ownerID = -1u;
	unsigned int teamID = -1u;
	unsigned int weaponNum = -1u;
	unsigned int cegID = -1u;

	int ttl = 0;

	float gravity = 0.0f;
	float tracking = 0.0f;
	float maxRange = 0.0f;
	float upTime = -1.0f;

	// BeamLaser-specific junk
	float startAlpha = 0.0f;
	float endAlpha = 1.0f;
};

#endif

