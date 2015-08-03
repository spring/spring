/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_TEAM_H
#define _CPPWRAPPER_TEAM_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Team {

public:
	virtual ~Team(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetTeamId() const = 0;

public:
	virtual bool HasAIController() = 0;

	/**
	 * This is a set of parameters that is created by SetTeamRulesParam() and may change during the game.
	 * Each parameter is uniquely identified only by its id (which is the index in the vector).
	 * Parameters may or may not have a name.
	 * @return visible to skirmishAIId parameters.
	 * If cheats are enabled, this will return all parameters.
	 */
public:
	virtual std::vector<springai::TeamRulesParam*> GetTeamRulesParams() = 0;

	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
public:
	virtual springai::TeamRulesParam* GetTeamRulesParamByName(const char* rulesParamName) = 0;

	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
public:
	virtual springai::TeamRulesParam* GetTeamRulesParamById(int rulesParamId) = 0;

}; // class Team

}  // namespace springai

#endif // _CPPWRAPPER_TEAM_H

