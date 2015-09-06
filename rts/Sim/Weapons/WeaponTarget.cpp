/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponTarget.h"
#include "Sim/Units/Unit.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"

CR_BIND(SWeaponTarget, )
CR_REG_METADATA(SWeaponTarget, (
	CR_MEMBER(isUserTarget),
	CR_MEMBER(isManualFire),
	CR_MEMBER(type),
	CR_MEMBER(unit),
	CR_MEMBER(intercept),
	CR_MEMBER(groundPos)
))



SWeaponTarget::SWeaponTarget()
: type(Target_None)
, isUserTarget(false)
, isManualFire(false)
, unit(nullptr)
, intercept(nullptr)
{}


SWeaponTarget::SWeaponTarget(const CUnit* u, bool userTarget)
: type(Target_Unit)
, isUserTarget(userTarget)
, isManualFire(false)
, unit(const_cast<CUnit*>(u)) //XXX evil but better than having two structs for const & non-const version
, intercept(nullptr)
{}


SWeaponTarget::SWeaponTarget(float3 p, bool userTarget)
: type(Target_Pos)
, isUserTarget(userTarget)
, isManualFire(false)
, unit(nullptr)
, intercept(nullptr)
, groundPos(p)
{}


SWeaponTarget::SWeaponTarget(CWeaponProjectile* i, bool userTarget)
: type(Target_Intercept)
, isUserTarget(userTarget)
, isManualFire(false)
, unit(nullptr)
, intercept(i)
{}


SWeaponTarget::SWeaponTarget(const CUnit* u, float3 p, bool userTarget)
: type(u ? Target_Unit: Target_Pos)
, isUserTarget(userTarget)
, isManualFire(false)
, unit(const_cast<CUnit*>(u))
, intercept(nullptr)
, groundPos(u ? p : ZeroVector)
{}


bool SWeaponTarget::operator!=(const SWeaponTarget& other) const
{
	if (type != other.type) return true;
	if (isUserTarget != other.isUserTarget) return true;
	if (isManualFire != other.isManualFire) return true;

	switch (type) {
		case Target_None: return false;
		case Target_Unit: return (unit != other.unit);
		case Target_Pos:  return (groundPos != other.groundPos);
		case Target_Intercept: return (intercept != other.intercept);
	}
	return false;
}

