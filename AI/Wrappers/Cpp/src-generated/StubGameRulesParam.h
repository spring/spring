/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBGAMERULESPARAM_H
#define _CPPWRAPPER_STUBGAMERULESPARAM_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "GameRulesParam.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubGameRulesParam : public GameRulesParam {

protected:
	virtual ~StubGameRulesParam();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int gameRulesParamId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGameRulesParamId(int gameRulesParamId);
	// @Override
	virtual int GetGameRulesParamId() const;
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
}; // class StubGameRulesParam

}  // namespace springai

#endif // _CPPWRAPPER_STUBGAMERULESPARAM_H

