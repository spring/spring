/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPUNITRULESPARAM_H
#define _CPPWRAPPER_WRAPPUNITRULESPARAM_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "UnitRulesParam.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappUnitRulesParam : public UnitRulesParam {

private:
	int skirmishAIId;
	int unitId;
	int unitRulesParamId;

	WrappUnitRulesParam(int skirmishAIId, int unitId, int unitRulesParamId);
	virtual ~WrappUnitRulesParam();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetUnitId() const;
public:
	// @Override
	virtual int GetUnitRulesParamId() const;
public:
	static UnitRulesParam* GetInstance(int skirmishAIId, int unitId, int unitRulesParamId);

	/**
	 * Not every mod parameter has a name.
	 */
public:
	// @Override
	virtual const char* GetName();

	/**
	 * @return float value of parameter if it's set, 0.0 otherwise.
	 */
public:
	// @Override
	virtual float GetValueFloat();

	/**
	 * @return string value of parameter if it's set, empty string otherwise.
	 */
public:
	// @Override
	virtual const char* GetValueString();
}; // class WrappUnitRulesParam

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPUNITRULESPARAM_H

