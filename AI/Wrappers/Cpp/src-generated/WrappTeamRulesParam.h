/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPTEAMRULESPARAM_H
#define _CPPWRAPPER_WRAPPTEAMRULESPARAM_H

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
class WrappTeamRulesParam : public TeamRulesParam {

private:
	int skirmishAIId;
	int teamId;
	int teamRulesParamId;

	WrappTeamRulesParam(int skirmishAIId, int teamId, int teamRulesParamId);
	virtual ~WrappTeamRulesParam();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetTeamId() const;
public:
	// @Override
	virtual int GetTeamRulesParamId() const;
public:
	static TeamRulesParam* GetInstance(int skirmishAIId, int teamId, int teamRulesParamId);

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
}; // class WrappTeamRulesParam

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPTEAMRULESPARAM_H

