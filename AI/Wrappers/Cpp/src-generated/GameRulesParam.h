/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_GAMERULESPARAM_H
#define _CPPWRAPPER_GAMERULESPARAM_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class GameRulesParam {

public:
	virtual ~GameRulesParam(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetGameRulesParamId() const = 0;

	/**
	 * Not every mod parameter has a name.
	 */
public:
	virtual const char* GetName() = 0;

	/**
	 * @return float value of parameter if it's set, 0.0 otherwise.
	 */
public:
	virtual float GetValueFloat() = 0;

	/**
	 * @return string value of parameter if it's set, empty string otherwise.
	 */
public:
	virtual const char* GetValueString() = 0;

}; // class GameRulesParam

}  // namespace springai

#endif // _CPPWRAPPER_GAMERULESPARAM_H

