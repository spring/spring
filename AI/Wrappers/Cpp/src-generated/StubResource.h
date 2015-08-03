/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBRESOURCE_H
#define _CPPWRAPPER_STUBRESOURCE_H

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
class StubResource : public Resource {

protected:
	virtual ~StubResource();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int resourceId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetResourceId(int resourceId);
	// @Override
	virtual int GetResourceId() const;
private:
	const char* name;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetName(const char* name);
	// @Override
	virtual const char* GetName();
private:
	float optimum;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetOptimum(float optimum);
	// @Override
	virtual float GetOptimum();
}; // class StubResource

}  // namespace springai

#endif // _CPPWRAPPER_STUBRESOURCE_H

