/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBENGINE_H
#define _CPPWRAPPER_STUBENGINE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Engine.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubEngine : public Engine {

protected:
	virtual ~StubEngine();
}; // class StubEngine

}  // namespace springai

#endif // _CPPWRAPPER_STUBENGINE_H

