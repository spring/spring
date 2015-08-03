/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_SKIRMISHAI_H
#define _CPPWRAPPER_SKIRMISHAI_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class SkirmishAI {

public:
	virtual ~SkirmishAI(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns the ID of the team controled by this Skirmish AI.
	 */
public:
	virtual int GetTeamId() = 0;

public:
	virtual springai::OptionValues* GetOptionValues() = 0;

public:
	virtual springai::Info* GetInfo() = 0;

}; // class SkirmishAI

}  // namespace springai

#endif // _CPPWRAPPER_SKIRMISHAI_H

