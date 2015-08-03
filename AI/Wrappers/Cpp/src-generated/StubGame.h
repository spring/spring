/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBGAME_H
#define _CPPWRAPPER_STUBGAME_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Game.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubGame : public Game {

protected:
	virtual ~StubGame();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the current game time measured in frames (the
	 * simulation runs at 30 frames per second at normal speed)
	 * 
	 * This should not be used, as we get the frame from the SUpdateEvent.
	 * @deprecated
	 */
private:
	int currentFrame;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCurrentFrame(int currentFrame);
	// @Override
	virtual int GetCurrentFrame();
private:
	int aiInterfaceVersion;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAiInterfaceVersion(int aiInterfaceVersion);
	// @Override
	virtual int GetAiInterfaceVersion();
private:
	int myTeam;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMyTeam(int myTeam);
	// @Override
	virtual int GetMyTeam();
private:
	int myAllyTeam;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMyAllyTeam(int myAllyTeam);
	// @Override
	virtual int GetMyAllyTeam();
	// @Override
	virtual int GetPlayerTeam(int playerId);
	/**
	 * Returns the number of active teams participating
	 * in the currently running game.
	 * A return value of 6 for example, would mean that teams 0 till 5
	 * take part in the game.
	 */
private:
	int teams;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTeams(int teams);
	// @Override
	virtual int GetTeams();
	/**
	 * Returns the name of the side of a team in the game.
	 * 
	 * This should not be used, as it may be "",
	 * and as the AI should rather rely on the units it has,
	 * which will lead to a more stable and versatile AI.
	 * @deprecated
	 * 
	 * @return eg. "ARM" or "CORE"; may be "", depending on how the game was setup
	 */
	// @Override
	virtual const char* GetTeamSide(int otherTeamId);
	/**
	 * Returns the color of a team in the game.
	 * 
	 * This should only be used when drawing stuff,
	 * and not for team-identification.
	 * @return the RGB color of a team, with values in [0, 255]
	 */
	// @Override
	virtual springai::AIColor GetTeamColor(int otherTeamId);
	/**
	 * Returns the income multiplier of a team in the game.
	 * All the teams resource income is multiplied by this factor.
	 * The default value is 1.0f, the valid range is [0.0, FLOAT_MAX].
	 */
	// @Override
	virtual float GetTeamIncomeMultiplier(int otherTeamId);
	/**
	 * Returns the ally-team of a team
	 */
	// @Override
	virtual int GetTeamAllyTeam(int otherTeamId);
	/**
	 * Returns the current level of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current level of the requested resource of the other team, or -1.0 on an invalid request
	 */
	// @Override
	virtual float GetTeamResourceCurrent(int otherTeamId, int resourceId);
	/**
	 * Returns the current income of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current income of the requested resource of the other team, or -1.0 on an invalid request
	 */
	// @Override
	virtual float GetTeamResourceIncome(int otherTeamId, int resourceId);
	/**
	 * Returns the current usage of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current usage of the requested resource of the other team, or -1.0 on an invalid request
	 */
	// @Override
	virtual float GetTeamResourceUsage(int otherTeamId, int resourceId);
	/**
	 * Returns the storage capacity for a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return storage capacity for the requested resource of the other team, or -1.0 on an invalid request
	 */
	// @Override
	virtual float GetTeamResourceStorage(int otherTeamId, int resourceId);
	// @Override
	virtual float GetTeamResourcePull(int otherTeamId, int resourceId);
	// @Override
	virtual float GetTeamResourceShare(int otherTeamId, int resourceId);
	// @Override
	virtual float GetTeamResourceSent(int otherTeamId, int resourceId);
	// @Override
	virtual float GetTeamResourceReceived(int otherTeamId, int resourceId);
	// @Override
	virtual float GetTeamResourceExcess(int otherTeamId, int resourceId);
	/**
	 * Returns true, if the two supplied ally-teams are currently allied
	 */
	// @Override
	virtual bool IsAllied(int firstAllyTeamId, int secondAllyTeamId);
private:
	bool isExceptionHandlingEnabled;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetExceptionHandlingEnabled(bool isExceptionHandlingEnabled);
	// @Override
	virtual bool IsExceptionHandlingEnabled();
private:
	bool isDebugModeEnabled;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDebugModeEnabled(bool isDebugModeEnabled);
	// @Override
	virtual bool IsDebugModeEnabled();
private:
	int mode;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMode(int mode);
	// @Override
	virtual int GetMode();
private:
	bool isPaused;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPaused(bool isPaused);
	// @Override
	virtual bool IsPaused();
private:
	float speedFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSpeedFactor(float speedFactor);
	// @Override
	virtual float GetSpeedFactor();
private:
	const char* setupScript;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSetupScript(const char* setupScript);
	// @Override
	virtual const char* GetSetupScript();
	/**
	 * Returns the categories bit field value.
	 * @return the categories bit field value or 0,
	 *         in case of empty name or too many categories
	 * @see getCategoryName
	 */
	// @Override
	virtual int GetCategoryFlag(const char* categoryName);
	/**
	 * Returns the bitfield values of a list of category names.
	 * @param categoryNames space delimited list of names
	 * @see Game#getCategoryFlag
	 */
	// @Override
	virtual int GetCategoriesFlag(const char* categoryNames);
	/**
	 * Return the name of the category described by a category flag.
	 * @see Game#getCategoryFlag
	 */
	// @Override
	virtual void GetCategoryName(int categoryFlag, char* name, int name_sizeMax);
	/**
	 * This is a set of parameters that is created by SetGameRulesParam() and may change during the game.
	 * Each parameter is uniquely identified only by its id (which is the index in the vector).
	 * Parameters may or may not have a name.
	 * @return visible to skirmishAIId parameters.
	 * If cheats are enabled, this will return all parameters.
	 */
private:
	std::vector<springai::GameRulesParam*> gameRulesParams;/* = std::vector<springai::GameRulesParam*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGameRulesParams(std::vector<springai::GameRulesParam*> gameRulesParams);
	// @Override
	virtual std::vector<springai::GameRulesParam*> GetGameRulesParams();
	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
	// @Override
	virtual springai::GameRulesParam* GetGameRulesParamByName(const char* rulesParamName);
	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
	// @Override
	virtual springai::GameRulesParam* GetGameRulesParamById(int rulesParamId);
	/**
	 * @brief Sends a chat/text message to other players.
	 * This text will also end up in infolog.txt.
	 */
	// @Override
	virtual void SendTextMessage(const char* text, int zone);
	/**
	 * Assigns a map location to the last text message sent by the AI.
	 */
	// @Override
	virtual void SetLastMessagePosition(const springai::AIFloat3& pos);
	/**
	 * @param pos_posF3  on this position, only x and z matter
	 */
	// @Override
	virtual void SendStartPosition(bool ready, const springai::AIFloat3& pos);
	/**
	 * Pause or unpauses the game.
	 * This is meant for debugging purposes.
	 * Keep in mind that pause does not happen immediately.
	 * It can take 1-2 frames in single- and up to 10 frames in multiplayer matches.
	 * @param reason  reason for the (un-)pause, or NULL
	 */
	// @Override
	virtual void SetPause(bool enable, const char* reason);
}; // class StubGame

}  // namespace springai

#endif // _CPPWRAPPER_STUBGAME_H

