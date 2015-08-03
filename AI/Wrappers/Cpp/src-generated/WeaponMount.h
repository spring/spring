/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WEAPONMOUNT_H
#define _CPPWRAPPER_WEAPONMOUNT_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WeaponMount {

public:
	virtual ~WeaponMount(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetUnitDefId() const = 0;

public:
	virtual int GetWeaponMountId() const = 0;

public:
	virtual const char* GetName() = 0;

public:
	virtual springai::WeaponDef* GetWeaponDef() = 0;

public:
	virtual int GetSlavedTo() = 0;

public:
	virtual springai::AIFloat3 GetMainDir() = 0;

public:
	virtual float GetMaxAngleDif() = 0;

	/**
	 * How many seconds of fuel it costs for the owning unit to fire this weapon.
	 */
public:
	virtual float GetFuelUsage() = 0;

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * not target.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	virtual int GetBadTargetCategory() = 0;

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * target, excluding all others.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	virtual int GetOnlyTargetCategory() = 0;

}; // class WeaponMount

}  // namespace springai

#endif // _CPPWRAPPER_WEAPONMOUNT_H

