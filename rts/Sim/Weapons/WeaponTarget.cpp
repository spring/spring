/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponTarget.h"
#include "Sim/Units/Unit.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"

CR_BIND(SWeaponTarget, )
CR_REG_METADATA(SWeaponTarget, (
	CR_MEMBER(isUserTarget),
	CR_MEMBER(type),
	CR_MEMBER(unit),
	CR_MEMBER(intercept),
	CR_MEMBER(groundPos)
))



SWeaponTarget::SWeaponTarget()
: type(Target_None)
, unit(nullptr)
, intercept(nullptr)
, isUserTarget(false)
{}


bool SWeaponTarget::operator!=(const SWeaponTarget& other) const
{
	if (type != other.type) return true;
	if (isUserTarget != other.isUserTarget) return true;

	switch (type) {
		case Target_None: return false;
		case Target_Unit: return (unit != other.unit);
		case Target_Pos:  return (groundPos != other.groundPos);
		case Target_Intercept: return (intercept != other.intercept);
	}
	return false;
}


bool SWeaponTarget::operator==(const SWeaponTarget& other) const
{
	if (type != other.type) return false;
	if (isUserTarget != other.isUserTarget) return false;

	switch (type) {
		case Target_None: return false;
		case Target_Unit: return (unit == other.unit);
		case Target_Pos:  return (groundPos == other.groundPos);
		case Target_Intercept: return (intercept == other.intercept);
	}
	return false;
}
