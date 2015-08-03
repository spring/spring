/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_ABSTRACTMOD_H
#define _CPPWRAPPER_ABSTRACTMOD_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Mod.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class AbstractMod : public Mod {

protected:
	virtual ~AbstractMod();
	// @Override
public:
	virtual int CompareTo(const Mod& other);
	// @Override
public:
	virtual bool Equals(const Mod& other);
	// @Override
public:
	virtual int HashCode();
	// @Override
public:
	virtual std::string ToString();
}; // class AbstractMod

}  // namespace springai

#endif // _CPPWRAPPER_ABSTRACTMOD_H

