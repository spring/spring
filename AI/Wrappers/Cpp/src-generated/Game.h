/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_GAME_H
#define _CPPWRAPPER_GAME_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Game {

public:
	virtual ~Game(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns the current game time measured in frames (the
	 * simulation runs at 30 frames per second at normal speed)
	 * 
	 * This should not be used, as we get the frame from the SUpdateEvent.
	 * @deprecated
	 */
public:
	virtual int GetCurrentFrame() = 0;

public:
	virtual int GetAiInterfaceVersion() = 0;

public:
	virtual int GetMyTeam() = 0;

public:
	virtual int GetMyAllyTeam() = 0;

public:
	virtual int GetPlayerTeam(int playerId) = 0;

	/**
	 * Returns the number of active teams participating
	 * in the currently running game.
	 * A return value of 6 for example, would mean that teams 0 till 5
	 * take part in the game.
	 */
public:
	virtual int GetTeams() = 0;

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
public:
	virtual const char* GetTeamSide(int otherTeamId) = 0;

	/**
	 * Returns the color of a team in the game.
	 * 
	 * This should only be used when drawing stuff,
	 * and not for team-identification.
	 * @return the RGB color of a team, with values in [0, 255]
	 */
public:
	virtual springai::AIColor GetTeamColor(int otherTeamId) = 0;

	/**
	 * Returns the income multiplier of a team in the game.
	 * All the teams resource income is multiplied by this factor.
	 * The default value is 1.0f, the valid range is [0.0, FLOAT_MAX].
	 */
public:
	virtual float GetTeamIncomeMultiplier(int otherTeamId) = 0;

	/**
	 * Returns the ally-team of a team
	 */
public:
	virtual int GetTeamAllyTeam(int otherTeamId) = 0;

	/**
	 * Returns the current level of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current level of the requested resource of the other team, or -1.0 on an invalid request
	 */
public:
	virtual float GetTeamResourceCurrent(int otherTeamId, int resourceId) = 0;

	/**
	 * Returns the current income of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current income of the requested resource of the other team, or -1.0 on an invalid request
	 */
public:
	virtual float GetTeamResourceIncome(int otherTeamId, int resourceId) = 0;

	/**
	 * Returns the current usage of a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return current usage of the requested resource of the other team, or -1.0 on an invalid request
	 */
public:
	virtual float GetTeamResourceUsage(int otherTeamId, int resourceId) = 0;

	/**
	 * Returns the storage capacity for a resource of another team.
	 * Allways works for allied teams.
	 * Works for all teams when cheating is enabled.
	 * @return storage capacity for the requested resource of the other team, or -1.0 on an invalid request
	 */
public:
	virtual float GetTeamResourceStorage(int otherTeamId, int resourceId) = 0;

public:
	virtual float GetTeamResourcePull(int otherTeamId, int resourceId) = 0;

public:
	virtual float GetTeamResourceShare(int otherTeamId, int resourceId) = 0;

public:
	virtual float GetTeamResourceSent(int otherTeamId, int resourceId) = 0;

public:
	virtual float GetTeamResourceReceived(int otherTeamId, int resourceId) = 0;

public:
	virtual float GetTeamResourceExcess(int otherTeamId, int resourceId) = 0;

	/**
	 * Returns true, if the two supplied ally-teams are currently allied
	 */
public:
	virtual bool IsAllied(int firstAllyTeamId, int secondAllyTeamId) = 0;

public:
	virtual bool IsExceptionHandlingEnabled() = 0;

public:
	virtual bool IsDebugModeEnabled() = 0;

public:
	virtual int GetMode() = 0;

public:
	virtual bool IsPaused() = 0;

public:
	virtual float GetSpeedFactor() = 0;

public:
	virtual const char* GetSetupScript() = 0;

	/**
	 * Returns the categories bit field value.
	 * @return the categories bit field value or 0,
	 *         in case of empty name or too many categories
	 * @see getCategoryName
	 */
public:
	virtual int GetCategoryFlag(const char* categoryName) = 0;

	/**
	 * Returns the bitfield values of a list of category names.
	 * @param categoryNames space delimited list of names
	 * @see Game#getCategoryFlag
	 */
public:
	virtual int GetCategoriesFlag(const char* categoryNames) = 0;

	/**
	 * Return the name of the category described by a category flag.
	 * @see Game#getCategoryFlag
	 */
public:
	virtual void GetCategoryName(int categoryFlag, char* name, int name_sizeMax) = 0;

	/**
	 * This is a set of parameters that is created by SetGameRulesParam() and may change during the game.
	 * Each parameter is uniquely identified only by its id (which is the index in the vector).
	 * Parameters may or may not have a name.
	 * @return visible to skirmishAIId parameters.
	 * If cheats are enabled, this will return all parameters.
	 */
public:
	virtual std::vector<springai::GameRulesParam*> GetGameRulesParams() = 0;

	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
public:
	virtual springai::GameRulesParam* GetGameRulesParamByName(const char* rulesParamName) = 0;

	/**
	 * @return only visible to skirmishAIId parameter.
	 * If cheats are enabled, this will return parameter despite it's losStatus.
	 */
public:
	virtual springai::GameRulesParam* GetGameRulesParamById(int rulesParamId) = 0;

	/**
	 * @brief Sends a chat/text message to other players.
	 * This text will also end up in infolog.txt.
	 */
public:
	virtual void SendTextMessage(const char* text, int zone) = 0;

	/**
	 * Assigns a map location to the last text message sent by the AI.
	 */
public:
	virtual void SetLastMessagePosition(const springai::AIFloat3& pos) = 0;

	/**
	 * @param pos_posF3  on this position, only x and z matter
	 */
public:
	virtual void SendStartPosition(bool ready, const springai::AIFloat3& pos) = 0;

	/**
	 * Pause or unpauses the game.
	 * This is meant for debugging purposes.
	 * Keep in mind that pause does not happen immediately.
	 * It can take 1-2 frames in single- and up to 10 frames in multiplayer matches.
	 * @param reason  reason for the (un-)pause, or NULL
	 */
public:
	virtual void SetPause(bool enable, const char* reason) = 0;

}; // class Game

}  // namespace springai

#endif // _CPPWRAPPER_GAME_H

