/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_SKIRMISHAIS_H
#define _CPPWRAPPER_SKIRMISHAIS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class SkirmishAIs {

public:
	virtual ~SkirmishAIs(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns the number of skirmish AIs in this game
	 */
public:
	virtual int GetSize() = 0;

	/**
	 * Returns the maximum number of skirmish AIs in any game
	 */
public:
	virtual int GetMax() = 0;

}; // class SkirmishAIs

}  // namespace springai

#endif // _CPPWRAPPER_SKIRMISHAIS_H

