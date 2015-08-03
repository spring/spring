/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPENGINE_H
#define _CPPWRAPPER_WRAPPENGINE_H

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
class WrappEngine : public Engine {

private:

	WrappEngine();
	virtual ~WrappEngine();
public:
public:
	static Engine* GetInstance();
}; // class WrappEngine

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPENGINE_H

