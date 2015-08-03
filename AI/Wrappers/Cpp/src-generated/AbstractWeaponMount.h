/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_ABSTRACTWEAPONMOUNT_H
#define _CPPWRAPPER_ABSTRACTWEAPONMOUNT_H

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
class AbstractWeaponMount : public WeaponMount {

protected:
	virtual ~AbstractWeaponMount();
	// @Override
public:
	virtual int CompareTo(const WeaponMount& other);
	// @Override
public:
	virtual bool Equals(const WeaponMount& other);
	// @Override
public:
	virtual int HashCode();
	// @Override
public:
	virtual std::string ToString();
}; // class AbstractWeaponMount

}  // namespace springai

#endif // _CPPWRAPPER_ABSTRACTWEAPONMOUNT_H

