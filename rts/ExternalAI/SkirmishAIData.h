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

#ifndef __SKIRMISH_AI_DATA_H
#define __SKIRMISH_AI_DATA_H

#include "ExternalAI/SkirmishAIBase.h"

#include <string>
#include <vector>
#include <map>

/**
 * Contains everything needed to initialize a Skirmish AI instance.
 * @see Game/GameSetup
 */
class SkirmishAIData : public SkirmishAIBase {
public:
	std::string shortName;
	std::string version;
	std::vector<std::string> optionKeys;
	std::map<std::string, std::string> options;
};

#endif // __SKIRMISH_AI_DATA_H
