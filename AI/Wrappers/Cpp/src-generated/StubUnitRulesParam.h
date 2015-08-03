/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBUNITRULESPARAM_H
#define _CPPWRAPPER_STUBUNITRULESPARAM_H

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
class StubUnitRulesParam : public UnitRulesParam {

protected:
	virtual ~StubUnitRulesParam();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int unitId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitId(int unitId);
	// @Override
	virtual int GetUnitId() const;
private:
	int unitRulesParamId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitRulesParamId(int unitRulesParamId);
	// @Override
	virtual int GetUnitRulesParamId() const;
	/**
	 * Not every mod parameter has a name.
	 */
private:
	const char* name;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetName(const char* name);
	// @Override
	virtual const char* GetName();
	/**
	 * @return float value of parameter if it's set, 0.0 otherwise.
	 */
private:
	float valueFloat;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetValueFloat(float valueFloat);
	// @Override
	virtual float GetValueFloat();
	/**
	 * @return string value of parameter if it's set, empty string otherwise.
	 */
private:
	const char* valueString;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetValueString(const char* valueString);
	// @Override
	virtual const char* GetValueString();
}; // class StubUnitRulesParam

}  // namespace springai

#endif // _CPPWRAPPER_STUBUNITRULESPARAM_H

