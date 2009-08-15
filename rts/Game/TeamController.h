/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __TEAM_CONTROLLER_H
#define __TEAM_CONTROLLER_H

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

#endif // __TEAM_CONTROLLER_H
