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

#include "Info.h"

#include "System/Util.h"
#include "System/Exceptions.h"
#include "Lua/LuaParser.h"

#include <assert.h>

static const char* InfoItem_badKeyChars = " =;\r\n\t";

static bool parseInfoItem(const LuaTable& root, int index, InfoItem& inf,
		std::set<string>& infoSet, CLogSubsystem& logSubsystem)
{
	const LuaTable& infsTbl = root.SubTable(index);
	if (!infsTbl.IsValid()) {
		logOutput.Print(logSubsystem,
				"parseInfoItem: subtable %d invalid", index);
		return false;
	}

	// common info properties
	inf.key = infsTbl.GetString("key", "");
	if (inf.key.empty()
			|| (inf.key.find_first_of(InfoItem_badKeyChars) != string::npos)) {
		logOutput.Print(logSubsystem,
				"parseInfoItem: empty key or key contains bad characters");
		return false;
	}
	std::string lowerKey = StringToLower(inf.key);
	if (infoSet.find(inf.key) != infoSet.end()) {
		logOutput.Print(logSubsystem, "parseInfoItem: key toLowerCase(%s) exists already",
				inf.key.c_str());
		return false;
	}
	inf.value = infsTbl.GetString("value", inf.key);
	if (inf.value.empty()) {
		logOutput.Print(logSubsystem, "parseInfoItem: %s: empty value",
				inf.key.c_str());
		return false;
	}
	inf.desc = infsTbl.GetString("desc", "");

	infoSet.insert(lowerKey);

	return true;
}


void parseInfo(
		std::vector<InfoItem>& info,
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		std::set<std::string>* infoSet,
		CLogSubsystem* logSubsystem) {

	if (!logSubsystem) {
		assert(logSubsystem);
	}

	LuaParser luaParser(fileName, fileModes, accessModes);

	if (!luaParser.Execute()) {
		throw content_error("luaParser.Execute() failed: "
				+ luaParser.GetErrorLog());
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	std::set<std::string>* myInfoSet = NULL;
	if (infoSet == NULL) {
		myInfoSet = new std::set<std::string>();
	} else {
		myInfoSet = infoSet;
	}
	for (int index = 1; root.KeyExists(index); index++) {
		InfoItem inf;
		if (parseInfoItem(root, index, inf, *myInfoSet, *logSubsystem)) {
			info.push_back(inf);
		}
	}
	if (infoSet == NULL) {
		delete myInfoSet;
		myInfoSet = NULL;
	}
}

std::vector<InfoItem> parseInfo(
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		std::set<std::string>* infoSet,
		CLogSubsystem* logSubsystem) {

	std::vector<InfoItem> info;

	parseInfo(info, fileName, fileModes, accessModes, infoSet,
			logSubsystem);

	return info;
}
