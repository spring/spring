/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_UNITRULESPARAM_H
#define _CPPWRAPPER_UNITRULESPARAM_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class UnitRulesParam {

public:
	virtual ~UnitRulesParam(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetUnitId() const = 0;

public:
	virtual int GetUnitRulesParamId() const = 0;

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

}; // class UnitRulesParam

}  // namespace springai

#endif // _CPPWRAPPER_UNITRULESPARAM_H

