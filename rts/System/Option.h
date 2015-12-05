/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// The structs in this file relate to *Options.lua files
// They are used for Mods and Skirmish AIs for example.
// This file is used (at least) by unitsync and the engine

#ifndef _OPTION_H
#define	_OPTION_H

#include "System/FileSystem/VFSModes.h"

#include <string>
#include <vector>
#include <set>
#include <cfloat> // for FLT_MAX
#include <climits> // for INT_MAX

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
	Option()
		: typeCode(opt_error)
		, boolDef(false)
		, numberDef(0.0f)
		, numberMin(0.0f)
		, numberMax(FLT_MAX)
		, numberStep(1.0f)
		, stringMaxLen(INT_MAX)
	{}

	std::string key;
	std::string scope;
	std::string name;
	std::string desc;
	std::string section;
	std::string style; //deprecated, see unitsync GetOptionStyle()

	std::string type; ///< "bool", "number", "string", "list", "section"

	OptionType typeCode;

	bool   boolDef;

	float  numberDef;
	float  numberMin;
	float  numberMax;
	float  numberStep; ///< aligned to numberDef

	std::string stringDef;
	int         stringMaxLen;

	std::string listDef;
	std::vector<OptionListItem> list;
};

std::string option_getDefString(const Option& option);

void option_parseOptions(
		std::vector<Option>& options,
		const std::string& fileName,
		const std::string& fileModes = SPRING_VFS_RAW,
		const std::string& accessModes = SPRING_VFS_RAW,
		std::set<std::string>* optionsSet = NULL);

void option_parseOptionsLuaString(
		std::vector<Option>& options,
		const std::string& optionsLuaString,
		const std::string& accessModes = SPRING_VFS_RAW,
		std::set<std::string>* optionsSet = NULL);

void option_parseMapOptions(
		std::vector<Option>& options,
		const std::string& fileName,
		const std::string& mapName,
		const std::string& fileModes = SPRING_VFS_RAW,
		const std::string& accessModes = SPRING_VFS_RAW,
		std::set<std::string>* optionsSet = NULL);

#endif // _OPTION_H
