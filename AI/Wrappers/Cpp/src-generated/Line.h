/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_LINE_H
#define _CPPWRAPPER_LINE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Line {

public:
	virtual ~Line(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetLineId() const = 0;

public:
	virtual springai::AIFloat3 GetFirstPosition() = 0;

public:
	virtual springai::AIFloat3 GetSecondPosition() = 0;

public:
	virtual springai::AIColor GetColor() = 0;

}; // class Line

}  // namespace springai

#endif // _CPPWRAPPER_LINE_H

