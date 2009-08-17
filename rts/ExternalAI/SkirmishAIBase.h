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

#ifndef __SKIRMISH_AI_BASE_H
#define __SKIRMISH_AI_BASE_H

#include "Game/TeamController.h"

#include <string>
#include <map>

/**
 * Possible live-cycle stati of a Skirmish AI instance.
 */
enum ESkirmishAIStatus {
	CONSTRUCTED = 2,
	INITIALIZING = 4,
	ALIVE = 6,
	DIEING = 8
};

/**
 * Stores all sync relevant data about a Skirmish AI instance,
 * which is comparable to a human player, as in the thing that controlls a team.
 * This class is the AIs equivalent to the humans PlayerBase.
 * It is used on the Game-Server and on clients (as a base class).
 */
class SkirmishAIBase : public TeamController {

public:
	typedef std::map<std::string, std::string> customOpts;

	/**
	 * @brief Constructor assigning default values.
	 */
	SkirmishAIBase() :
		  TeamController()
		, hostPlayer(-1)
		, status(CONSTRUCTED)
		{}

	int hostPlayer;
	ESkirmishAIStatus status;
};

/**
 * @brief Contains statistical data about a player concerning a single game.
 */
class SkirmishAIStatistics : public TeamControllerStatistics
{
public:
	/**
	 * @brief Constructor assigning default values.
	 */
	SkirmishAIStatistics() :
		  TeamControllerStatistics()
		, cpuTime(0)
		{}

	/// How much CPU time has been used by the AI (in ms?)
	int cpuTime;

	/// Change structure from host endian to little endian or vice versa.
	void swab();
};

#endif // __SKIRMISH_AI_BASE_H
