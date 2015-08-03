/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_RESOURCE_H
#define _CPPWRAPPER_RESOURCE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Resource {

public:
	virtual ~Resource(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetResourceId() const = 0;

public:
	virtual const char* GetName() = 0;

public:
	virtual float GetOptimum() = 0;

}; // class Resource

}  // namespace springai

#endif // _CPPWRAPPER_RESOURCE_H

