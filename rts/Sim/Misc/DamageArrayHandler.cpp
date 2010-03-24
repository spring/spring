/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <algorithm>
#include <locale>
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

using namespace std;


CR_BIND(CDamageArrayHandler, );

CR_REG_METADATA(CDamageArrayHandler, (
		CR_MEMBER(numTypes),
		CR_MEMBER(name2type),
		CR_MEMBER(typeList),
		CR_RESERVED(16)
		));


CDamageArrayHandler* damageArrayHandler;


CDamageArrayHandler::CDamageArrayHandler(void)
{
	try {
		const LuaTable rootTable = game->defsParser->GetRoot().SubTable("ArmorDefs");
		if (!rootTable.IsValid()) {
			throw content_error("Error loading ArmorDefs");
		}

		rootTable.GetKeys(typeList);

		numTypes = typeList.size() + 1;
		typeList.insert(typeList.begin(), "default");
		name2type["default"] = 0;

		logOutput.Print("Number of damage types: %d", numTypes);

		for (int armorID = 1; armorID < (int)typeList.size(); armorID++) {
			const string armorName = StringToLower(typeList[armorID]);
			if (armorName == "default") {
				throw content_error("Tried to define the \"default\" armor type\n");
			}
			name2type[armorName] = armorID;

			LuaTable armorTable = rootTable.SubTable(typeList[armorID]);
			vector<string> units; // the values are not used (afaict)
			armorTable.GetKeys(units);

			vector<string>::const_iterator ui;
			for (ui = units.begin(); ui != units.end(); ++ui) {
				const string unitName = StringToLower(*ui); // NOTE: not required
				name2type[unitName] = armorID;
			}
		}
	}
	catch (content_error) {
		numTypes = 1;
		name2type.clear();
		name2type["default"] = 0;
		typeList.clear();
		typeList.push_back("default");
	}
}


CDamageArrayHandler::~CDamageArrayHandler(void)
{
}


int CDamageArrayHandler::GetTypeFromName(string name)
{
	StringToLowerInPlace(name);
	if (name2type.find(name) != name2type.end()) {
		return name2type[name];
	}
	return 0; // 'default' armor index
}
