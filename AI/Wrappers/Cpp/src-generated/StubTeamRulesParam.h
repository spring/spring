/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBTEAMRULESPARAM_H
#define _CPPWRAPPER_STUBTEAMRULESPARAM_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "TeamRulesParam.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubTeamRulesParam : public TeamRulesParam {

protected:
	virtual ~StubTeamRulesParam();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int teamId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTeamId(int teamId);
	// @Override
	virtual int GetTeamId() const;
private:
	int teamRulesParamId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTeamRulesParamId(int teamRulesParamId);
	// @Override
	virtual int GetTeamRulesParamId() const;
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
}; // class StubTeamRulesParam

}  // namespace springai

#endif // _CPPWRAPPER_STUBTEAMRULESPARAM_H

