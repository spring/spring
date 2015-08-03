/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPRESOURCE_H
#define _CPPWRAPPER_WRAPPRESOURCE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Resource.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappResource : public Resource {

private:
	int skirmishAIId;
	int resourceId;

	WrappResource(int skirmishAIId, int resourceId);
	virtual ~WrappResource();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetResourceId() const;
public:
	static Resource* GetInstance(int skirmishAIId, int resourceId);

public:
	// @Override
	virtual const char* GetName();

public:
	// @Override
	virtual float GetOptimum();
}; // class WrappResource

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPRESOURCE_H

