/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBTEAM_H
#define _CPPWRAPPER_STUBTEAM_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Team.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubTeam : public Team {

protected:
	virtual ~StubTeam();
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
	// @Override
	virtual bool HasAIController();
	/**
	 * This is a set of parameters that is created by SetTeamRulesParam() and may change during the game.
	 * Each parameter is uniquely identified only by its id (which is the index in the vector).
	 * Parameters may or may not have a name.
	 * @return visible to skirmishAIId parameters.
	 * If cheats are enabled, this will return all parameters.
	 */
private:
	std::vector<springai::TeamRulesParam*> teamRulesParams;/* = std::vector<springai::TeamRulesParam*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTeamRulesParams(std::vector<springai::TeamRulesParam*> teamRulesParams);
	// @Override
	virtual std::vector<springai::TeamRulesParam*> GetTeamRulesParams();
	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
	// @Override
	virtual springai::TeamRulesParam* GetTeamRulesParamByName(const char* rulesParamName);
	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
	// @Override
	virtual springai::TeamRulesParam* GetTeamRulesParamById(int rulesParamId);
}; // class StubTeam

}  // namespace springai

#endif // _CPPWRAPPER_STUBTEAM_H

