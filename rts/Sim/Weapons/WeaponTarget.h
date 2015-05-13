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
	bool operator!=(const SWeaponTarget& other) const;
	bool operator==(const SWeaponTarget& other) const { return !(*this != other); }

	TargetType type;              // indicates if we have a target and what type
	CUnit* unit;                  // if targettype=unit: the targeted unit
	CWeaponProjectile* intercept; // if targettype=intercept: projectile that we currently target for interception
	float3 groundPos;             // if targettype=ground: the ground position
	bool isUserTarget;
	bool isManualFire;
};

#endif // WEAPONTARGET_H
