/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TEAM_CONTROLLER_H
#define _TEAM_CONTROLLER_H

#include "Platform/byteorder.h" // for swabdword

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

	/**
	 * @brief Constructor assigning default values.
	 */
	TeamController() :
		team(0),
		name("no name") {}

	/**
	 * Id of the controlled team.
	 */
	int team;
	/**
	 * The purely informative name of the controlling instance.
	 * This is either the human players nick or the Skirmish AIs instance nick.
	 */
	std::string name;

	virtual bool operator == (const TeamController& tc) const {
		return (name == tc.name);
	}
};

/**
 * Contains statistical data about a team controlled,
 * concerning a single game.
 *
 * Note: This class should be seen as abstract,
 * even though it is not, C++ technically speaking.
 */
class TeamControllerStatistics {
public:

	int numCommands;
	/**
	 * The Total amount of units affected by commands.
	 * Divide by numCommands for average units/command.
	 */
	int unitCommands;

protected:
	/// Change structure from host endian to little endian or vice versa.
	void swabTC() {
		numCommands  = swabdword(numCommands);
		unitCommands = swabdword(unitCommands);
	}
};

#endif // _TEAM_CONTROLLER_H
