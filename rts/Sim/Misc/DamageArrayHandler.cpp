/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <algorithm>
#include <locale>
#include <string>
#include <vector>
#include <cctype>
#include "mmgr.h"

#include "DamageArrayHandler.h"
#include "DamageArray.h"
#include "LogOutput.h"
#include "Game/Game.h"
#include "Lua/LuaParser.h"
#include "creg/STL_Map.h"
#include "Util.h"
#include "Exceptions.h"


CR_BIND(CDamageArrayHandler, );

CR_REG_METADATA(CDamageArrayHandler, (
		CR_MEMBER(name2type),
		CR_MEMBER(typeList),
		CR_RESERVED(16)
		));


CDamageArrayHandler* damageArrayHandler;


CDamageArrayHandler::CDamageArrayHandler()
{
	try {
		const LuaTable rootTable = game->defsParser->GetRoot().SubTable("ArmorDefs");
		if (!rootTable.IsValid()) {
			throw content_error("Error loading ArmorDefs");
		}

		rootTable.GetKeys(typeList);

		typeList.insert(typeList.begin(), "default");
		name2type["default"] = 0;

		logOutput.Print("Number of damage types: "_STPF_, typeList.size());

		for (int armorID = 1; armorID < (int)typeList.size(); armorID++) {
			const std::string armorName = StringToLower(typeList[armorID]);
			if (armorName == "default") {
				throw content_error("Tried to define the \"default\" armor type\n");
			}
			name2type[armorName] = armorID;

			LuaTable armorTable = rootTable.SubTable(typeList[armorID]);
			std::vector<std::string> units; // the values are not used (afaict)
			armorTable.GetKeys(units);

			std::vector<std::string>::const_iterator ui;
			for (ui = units.begin(); ui != units.end(); ++ui) {
				const std::string unitName = StringToLower(*ui); // NOTE: not required
				name2type[unitName] = armorID;
			}
		}
	}
	catch (content_error) {
		name2type.clear();
		name2type["default"] = 0;
		typeList.clear();
		typeList.push_back("default");
	}
}


CDamageArrayHandler::~CDamageArrayHandler()
{
}


int CDamageArrayHandler::GetTypeFromName(std::string name) const
{
	StringToLowerInPlace(name);
	const std::map<std::string, int>::const_iterator it = name2type.find(name);
	if (it != name2type.end()) {
		return it->second;
	}
	return 0; // 'default' armor index
}
