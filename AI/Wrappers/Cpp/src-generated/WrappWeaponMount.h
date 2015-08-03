/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPWEAPONMOUNT_H
#define _CPPWRAPPER_WRAPPWEAPONMOUNT_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "WeaponMount.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappWeaponMount : public WeaponMount {

private:
	int skirmishAIId;
	int unitDefId;
	int weaponMountId;

	WrappWeaponMount(int skirmishAIId, int unitDefId, int weaponMountId);
	virtual ~WrappWeaponMount();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetUnitDefId() const;
public:
	// @Override
	virtual int GetWeaponMountId() const;
public:
	static WeaponMount* GetInstance(int skirmishAIId, int unitDefId, int weaponMountId);

public:
	// @Override
	virtual const char* GetName();

public:
	// @Override
	virtual springai::WeaponDef* GetWeaponDef();

public:
	// @Override
	virtual int GetSlavedTo();

public:
	// @Override
	virtual springai::AIFloat3 GetMainDir();

public:
	// @Override
	virtual float GetMaxAngleDif();

	/**
	 * How many seconds of fuel it costs for the owning unit to fire this weapon.
	 */
public:
	// @Override
	virtual float GetFuelUsage();

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * not target.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	// @Override
	virtual int GetBadTargetCategory();

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * target, excluding all others.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	// @Override
	virtual int GetOnlyTargetCategory();
}; // class WrappWeaponMount

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPWEAPONMOUNT_H

