/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_GRAPHLINE_H
#define _CPPWRAPPER_GRAPHLINE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class GraphLine {

public:
	virtual ~GraphLine(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual void AddPoint(int lineId, float x, float y) = 0;

public:
	virtual void DeletePoints(int lineId, int numPoints) = 0;

public:
	virtual void SetColor(int lineId, const springai::AIColor& color) = 0;

public:
	virtual void SetLabel(int lineId, const char* label) = 0;

}; // class GraphLine

}  // namespace springai

#endif // _CPPWRAPPER_GRAPHLINE_H

