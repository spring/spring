/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_DEBUG_H
#define _CPPWRAPPER_DEBUG_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Debug {

public:
	virtual ~Debug(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int AddOverlayTexture(const float* texData, int w, int h) = 0;

public:
	virtual springai::GraphDrawer* GetGraphDrawer() = 0;

}; // class Debug

}  // namespace springai

#endif // _CPPWRAPPER_DEBUG_H

