/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_TEAMS_H
#define _CPPWRAPPER_TEAMS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Teams {

public:
	virtual ~Teams(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns the number of teams in this game
	 */
public:
	virtual int GetSize() = 0;

}; // class Teams

}  // namespace springai

#endif // _CPPWRAPPER_TEAMS_H

