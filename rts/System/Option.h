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

// The structs in this files relate to *Options.lua files
// They are used for Mods and Skirmish AIs for example;
// This file is used (at least) by unitsync and the engine

#ifndef _OPTION_H
#define	_OPTION_H

#include "System/FileSystem/VFSModes.h"
#include "LogOutput.h"

#include <string>
#include <vector>
#include <set>

/**
 * @brief Available mod/map/ai option types
 * @sa GetOptionType
 */
enum OptionType {
	opt_error   = 0, ///< error
	opt_bool    = 1, ///< boolean option
	opt_list    = 2, ///< list option (e.g. combobox)
	opt_number  = 3, ///< numeric option (e.g. spinner / slider)
	opt_string  = 4, ///< string option (e.g. textbox)
	opt_section = 5  ///< option section (e.g. groupbox)
};


struct OptionListItem {
	std::string key;
	std::string name;
	std::string desc;
};


struct Option {
	Option() : typeCode(opt_error) {}

	std::string key;
	std::string scope;
	std::string name;
	std::string desc;
	std::string section;
	std::string style;

	std::string type; // "bool", "number", "string", "list", "section"

	OptionType typeCode;

	bool   boolDef;

	float  numberDef;
	float  numberMin;
	float  numberMax;
	float  numberStep; // aligned to numberDef

	std::string stringDef;
	int         stringMaxLen;

	std::string listDef;
	std::vector<OptionListItem> list;
};

void parseOptions(
		std::vector<Option>& options,
		const std::string& fileName,
		const std::string& fileModes = SPRING_VFS_RAW,
		const std::string& accessModes = SPRING_VFS_RAW,
		const std::string& mapName = "",
		std::set<std::string>* optionsSet = NULL,
		CLogSubsystem* logSubsystem = &(CLogOutput::GetDefaultLogSubsystem()));

std::vector<Option> parseOptions(
		const std::string& fileName,
		const std::string& fileModes = SPRING_VFS_RAW,
		const std::string& accessModes = SPRING_VFS_RAW,
		const std::string& mapName = "",
		std::set<std::string>* optionsSet = NULL,
		CLogSubsystem* logSubsystem = &(CLogOutput::GetDefaultLogSubsystem()));

#endif // _OPTION_H
