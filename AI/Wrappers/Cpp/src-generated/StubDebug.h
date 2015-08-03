/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBDEBUG_H
#define _CPPWRAPPER_STUBDEBUG_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Debug.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubDebug : public Debug {

protected:
	virtual ~StubDebug();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	// @Override
	virtual int AddOverlayTexture(const float* texData, int w, int h);
private:
	springai::GraphDrawer* graphDrawer;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGraphDrawer(springai::GraphDrawer* graphDrawer);
	// @Override
	virtual springai::GraphDrawer* GetGraphDrawer();
}; // class StubDebug

}  // namespace springai

#endif // _CPPWRAPPER_STUBDEBUG_H

