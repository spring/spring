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

	@author	Robin Vobruba <hoijui.quaero@gmail.com>
 */

#ifndef _SKIRMISHAIDATA_H
#define _SKIRMISHAIDATA_H

#include <string>
#include <vector>
#include <map>

/**
 * Contains everything needed to initialize a Skirmish AI instance.
 * @see Game/GameSetup
 */
struct SkirmishAIData {
	/**
	 * This name is purely inforamtive.
	 * It will be shown in statistics eg.,
	 * and everywhere else where normal players
	 * have their name displayed.
	 */
	std::string name;
	/** Id of the team this Skirmish AI is controlling. */
	int team;
	/** Number of the player whose computer this AI runs on. */
	int hostPlayerNum;
	std::string shortName;
	std::string version;
	std::vector<std::string> optionKeys;
	std::map<std::string, std::string> options;
};

#endif // _SKIRMISHAIDATA_H
