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

#ifndef _OPTION_CPP
#define _OPTION_CPP

#include "Option.h"

#include "System/Util.h"
#include "System/Exceptions.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"

#include <assert.h>

static const char* Option_badKeyChars = " =;\r\n\t";

static bool parseOption(const LuaTable& root, int index, Option& opt,
		std::set<string>& optionsSet, CLogSubsystem& logSubsystem) {

	const LuaTable& optTbl = root.SubTable(index);
	if (!optTbl.IsValid()) {
		logOutput.Print(logSubsystem,
				"parseOption: subtable %d invalid", index);
		return false;
	}

	// common options properties
	opt.key = optTbl.GetString("key", "");
	if (opt.key.empty()
			|| (opt.key.find_first_of(Option_badKeyChars) != string::npos)) {
		logOutput.Print(logSubsystem,
				"parseOption: empty key or key contains bad characters");
		return false;
	}
	opt.key = StringToLower(opt.key);

	opt.scope = optTbl.GetString("scope", "scope");
	if (opt.key.empty()
		   || (opt.key.find_first_of(Option_badKeyChars) != string::npos)) {
		logOutput.Print(logSubsystem,
						"parseOption: empty key or key contains bad characters");
		return false;
	}
	opt.scope = StringToLower(opt.scope);

	if (optionsSet.find(opt.key) != optionsSet.end()) {
		logOutput.Print(logSubsystem, "parseOption: key %s exists already",
				opt.key.c_str());
		return false;
	}
	opt.name = optTbl.GetString("name", opt.key);
	if (opt.name.empty()) {
		logOutput.Print(logSubsystem, "parseOption: %s: empty name",
				opt.key.c_str());
		return false;
	}
	opt.desc = optTbl.GetString("desc", opt.name);

	opt.section = optTbl.GetString("section", "");
	opt.style = optTbl.GetString("style", "");

	opt.type = optTbl.GetString("type", "");
	opt.type = StringToLower(opt.type);

	// option type specific properties
	if (opt.type == "bool") {
		opt.typeCode = opt_bool;
		opt.boolDef = optTbl.GetBool("def", false);
	}
	else if (opt.type == "number") {
		opt.typeCode = opt_number;
		opt.numberDef  = optTbl.GetFloat("def",  0.0f);
		opt.numberMin  = optTbl.GetFloat("min",  -1.0e30f);
		opt.numberMax  = optTbl.GetFloat("max",  +1.0e30f);
		opt.numberStep = optTbl.GetFloat("step", 0.0f);
	}
	else if (opt.type == "string") {
		opt.typeCode = opt_string;
		opt.stringDef    = optTbl.GetString("def", "");
		opt.stringMaxLen = optTbl.GetInt("maxlen", 0);
	}
	else if (opt.type == "list") {
		opt.typeCode = opt_list;

		const LuaTable& listTbl = optTbl.SubTable("items");
		if (!listTbl.IsValid()) {
			logOutput.Print(logSubsystem, "parseOption: %s: subtable items invalid", opt.key.c_str());
			return false;
		}

		for (int i = 1; listTbl.KeyExists(i); i++) {
			OptionListItem item;

			// string format
			item.key = listTbl.GetString(i, "");
			if (!item.key.empty() &&
			    (item.key.find_first_of(Option_badKeyChars) == string::npos)) {
				item.name = item.key;
				item.desc = item.name;
				opt.list.push_back(item);
				continue;
			}

			// table format  (name & desc)
			const LuaTable& itemTbl = listTbl.SubTable(i);
			if (!itemTbl.IsValid()) {
				logOutput.Print(logSubsystem,
						"parseOption: %s: subtable %d of subtable items invalid",
						opt.key.c_str(), i);
				break;
			}
			item.key = itemTbl.GetString("key", "");
			if (item.key.empty() || (item.key.find_first_of(Option_badKeyChars) != string::npos)) {
				logOutput.Print(logSubsystem,
						"parseOption: %s: empty key or key contains bad characters",
						opt.key.c_str());
				return false;
			}
			item.key = StringToLower(item.key);
			item.name = itemTbl.GetString("name", item.key);
			if (item.name.empty()) {
				logOutput.Print(logSubsystem, "parseOption: %s: empty name",
						opt.key.c_str());
				return false;
			}
			item.desc = itemTbl.GetString("desc", item.name);
			opt.list.push_back(item);
		}

		if (opt.list.size() <= 0) {
			logOutput.Print(logSubsystem, "parseOption: %s: empty list",
					opt.key.c_str());
			return false; // no empty lists
		}

		opt.listDef = optTbl.GetString("def", opt.list[0].name);
	}
	else if (opt.type == "section") {
		opt.typeCode = opt_section;
	}
	else {
		logOutput.Print(logSubsystem, "parseOption: %s: unknown type %s",
				opt.key.c_str(), opt.type.c_str());
		return false; // unknown type
	}

	optionsSet.insert(opt.key);

	return true;
}


void parseOptions(
		std::vector<Option>& options,
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		const std::string& mapName,
		std::set<std::string>* optionsSet,
		CLogSubsystem* logSubsystem) {

	if (!logSubsystem) {
		assert(logSubsystem);
	}

	LuaParser luaParser(fileName, fileModes, accessModes);

	const string configName = MapParser::GetMapConfigName(mapName);

	if (!mapName.empty() && !configName.empty()) {
		luaParser.GetTable("Map");
		luaParser.AddString("fileName", mapName);
		luaParser.AddString("fullName", "maps/" + mapName);
		luaParser.AddString("configFile", configName);
		luaParser.EndTable();
	}

	if (!luaParser.Execute()) {
		throw content_error("luaParser.Execute() failed: "
				+ luaParser.GetErrorLog());
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	std::set<std::string>* myOptionsSet = NULL;
	if (optionsSet == NULL) {
		myOptionsSet = new std::set<std::string>();
	} else {
		myOptionsSet = optionsSet;
	}
	for (int index = 1; root.KeyExists(index); index++) {
		Option opt;
		if (parseOption(root, index, opt, *myOptionsSet, *logSubsystem)) {
			options.push_back(opt);
		}
	}
	if (optionsSet == NULL) {
		delete myOptionsSet;
		myOptionsSet = NULL;
	}
}

std::vector<Option> parseOptions(
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		const std::string& mapName,
		std::set<std::string>* optionsSet,
		CLogSubsystem* logSubsystem) {

	std::vector<Option> options;

	parseOptions(options, fileName, fileModes, accessModes, mapName, optionsSet,
			logSubsystem);

	return options;
}

#endif // _OPTION_CPP
