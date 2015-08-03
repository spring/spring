/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPDEBUG_H
#define _CPPWRAPPER_WRAPPDEBUG_H

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
class WrappDebug : public Debug {

private:
	int skirmishAIId;

	WrappDebug(int skirmishAIId);
	virtual ~WrappDebug();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Debug* GetInstance(int skirmishAIId);

public:
	// @Override
	virtual int AddOverlayTexture(const float* texData, int w, int h);

public:
	// @Override
	virtual springai::GraphDrawer* GetGraphDrawer();
}; // class WrappDebug

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPDEBUG_H

