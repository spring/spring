/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKIRMISH_AI_BASE_H
#define SKIRMISH_AI_BASE_H

#include "Game/TeamController.h"
#include "System/creg/creg_cond.h"

#include <string>
#include <map>


/**
 * Possible live-cycle stati of a Skirmish AI instance.
 */
enum ESkirmishAIStatus {
	SKIRMAISTATE_CONSTRUCTED  = 10,
	SKIRMAISTATE_INITIALIZING = 20,
	SKIRMAISTATE_RELOADING    = 30,
	SKIRMAISTATE_ALIVE        = 40,
	SKIRMAISTATE_DIEING       = 50,
	SKIRMAISTATE_DEAD         = 60
};

/**
 * Stores all sync relevant data about a Skirmish AI instance,
 * which is comparable to a human player, as in the thing that controlls a team.
 * This class is the AIs equivalent to the humans PlayerBase.
 * It is used on the Game-Server and on clients (as a base class).
 */
class SkirmishAIBase : public TeamController {
	CR_DECLARE(SkirmishAIBase);

public:
	typedef std::map<std::string, std::string> customOpts;

	/**
	 * @brief Constructor assigning default values.
	 */
	SkirmishAIBase()
		: TeamController()
		, hostPlayer(-1)
		, status(SKIRMAISTATE_CONSTRUCTED)
		{}

	int hostPlayer;
	ESkirmishAIStatus status;
};

/**
 * @brief Contains statistical data about a player concerning a single game.
 */
class SkirmishAIStatistics : public TeamControllerStatistics
{
	CR_DECLARE_STRUCT(SkirmishAIStatistics);

public:
	/**
	 * @brief Constructor assigning default values.
	 */
	SkirmishAIStatistics()
		: TeamControllerStatistics()
		, cpuTime(0)
		{}

	/// How much CPU time has been used by the AI (in ms?)
	int cpuTime;

	/// Change structure from host endian to little endian or vice versa.
	void swab();
};

#endif // SKIRMISH_AI_BASE_H
