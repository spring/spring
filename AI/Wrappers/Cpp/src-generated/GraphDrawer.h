/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_GRAPHDRAWER_H
#define _CPPWRAPPER_GRAPHDRAWER_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class GraphDrawer {

public:
	virtual ~GraphDrawer(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual bool IsEnabled() = 0;

public:
	virtual void SetPosition(float x, float y) = 0;

public:
	virtual void SetSize(float w, float h) = 0;

public:
	virtual springai::GraphLine* GetGraphLine() = 0;

}; // class GraphDrawer

}  // namespace springai

#endif // _CPPWRAPPER_GRAPHDRAWER_H

