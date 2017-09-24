/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAM_CONTROLLER_H
#define TEAM_CONTROLLER_H

#include "System/creg/creg_cond.h"
#include "System/Platform/byteorder.h" // for swabDWord

#include <string>


/**
 * Acts as a base class for everything that can control a team,
 * which is either a human player or a Skirmish AI.
 *
 * Note: This class should be seen as abstract,
 * even though it is not, C++ technically speaking.
 */
class TeamController
{
public:
	CR_DECLARE(TeamController)

	/**
	 * @brief Constructor assigning default values.
	 */
	TeamController() :
		team(-1),
		name("no name") {}
	virtual ~TeamController() {}

	/**
	 * Id of the controlled team.
	 */
	int team;

	/**
	 * The purely informative name of the controlling instance.
	 * This is either the human players nick or the Skirmish AIs instance nick.
	 */
	std::string name;
};

/**
 * Contains statistical data about a team controlled,
 * concerning a single game.
 *
 * Note: This class should be seen as abstract,
 * even though it is not, C++ technically speaking.
 *
 * This should be the base class of PlayerStatistics and SkirmishAIStatistics,
 * which it is not yet.
 */
struct TeamControllerStatistics {
public:
	TeamControllerStatistics()
		: numCommands(0)
		, unitCommands(0) {}

	int numCommands;
	/**
	 * The Total amount of units affected by commands.
	 * Divide by numCommands for average units/command.
	 */
	int unitCommands;

protected:
	/// Change structure from host endian to little endian or vice versa.
	void swab() {
		swabDWordInPlace(numCommands);
		swabDWordInPlace(unitCommands);
	}
};

#endif // TEAM_CONTROLLER_H
