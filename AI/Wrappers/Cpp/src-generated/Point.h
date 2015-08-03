/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_POINT_H
#define _CPPWRAPPER_POINT_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Point {

public:
	virtual ~Point(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetPointId() const = 0;

public:
	virtual springai::AIFloat3 GetPosition() = 0;

public:
	virtual springai::AIColor GetColor() = 0;

public:
	virtual const char* GetLabel() = 0;

}; // class Point

}  // namespace springai

#endif // _CPPWRAPPER_POINT_H

