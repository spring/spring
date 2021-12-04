/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPONTARGET_H
#define WEAPONTARGET_H

#include "System/float3.h"
#include "System/creg/creg_cond.h"


class CUnit;
class CWeaponProjectile;


enum TargetType {
	Target_None,
	Target_Unit,
	Target_Pos,
	Target_Intercept
};


struct SWeaponTarget {
	CR_DECLARE_STRUCT(SWeaponTarget)
	SWeaponTarget();

	// explicit ctors
	SWeaponTarget(const CUnit* unit, bool isUserTarget = false);
	SWeaponTarget(float3 groundPos, bool isUserTarget = false);
	SWeaponTarget(CWeaponProjectile* intercept, bool isUserTarget = false);

	// conditional ctors
	SWeaponTarget(const CUnit* unit, float3 groundPos, bool isUserTarget = false);

	// operators
	bool operator!=(const SWeaponTarget& other) const;
	bool operator==(const SWeaponTarget& other) const { return !(*this != other); }

public:
	TargetType type;    // indicates if we have a target and what type
	bool isUserTarget;
	bool isManualFire;

	CUnit* unit;                  // if targettype=unit: the targeted unit
	CWeaponProjectile* intercept; // if targettype=intercept: projectile that we currently target for interception
	float3 groundPos;             // if targettype=ground: the ground position
};

#endif // WEAPONTARGET_H
