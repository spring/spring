/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBWEAPONMOUNT_H
#define _CPPWRAPPER_STUBWEAPONMOUNT_H

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
class StubWeaponMount : public WeaponMount {

protected:
	virtual ~StubWeaponMount();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int unitDefId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitDefId(int unitDefId);
	// @Override
	virtual int GetUnitDefId() const;
private:
	int weaponMountId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWeaponMountId(int weaponMountId);
	// @Override
	virtual int GetWeaponMountId() const;
private:
	const char* name;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetName(const char* name);
	// @Override
	virtual const char* GetName();
private:
	springai::WeaponDef* weaponDef;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWeaponDef(springai::WeaponDef* weaponDef);
	// @Override
	virtual springai::WeaponDef* GetWeaponDef();
private:
	int slavedTo;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSlavedTo(int slavedTo);
	// @Override
	virtual int GetSlavedTo();
private:
	springai::AIFloat3 mainDir;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMainDir(springai::AIFloat3 mainDir);
	// @Override
	virtual springai::AIFloat3 GetMainDir();
private:
	float maxAngleDif;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxAngleDif(float maxAngleDif);
	// @Override
	virtual float GetMaxAngleDif();
	/**
	 * How many seconds of fuel it costs for the owning unit to fire this weapon.
	 */
private:
	float fuelUsage;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFuelUsage(float fuelUsage);
	// @Override
	virtual float GetFuelUsage();
	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * not target.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
private:
	int badTargetCategory;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBadTargetCategory(int badTargetCategory);
	// @Override
	virtual int GetBadTargetCategory();
	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * target, excluding all others.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
private:
	int onlyTargetCategory;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetOnlyTargetCategory(int onlyTargetCategory);
	// @Override
	virtual int GetOnlyTargetCategory();
}; // class StubWeaponMount

}  // namespace springai

#endif // _CPPWRAPPER_STUBWEAPONMOUNT_H

