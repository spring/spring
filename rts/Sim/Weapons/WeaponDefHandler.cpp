/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include "WeaponDefHandler.h"

#include "Lua/LuaParser.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/Log/ILog.h"


CWeaponDefHandler* weaponDefHandler = NULL;


CWeaponDefHandler::CWeaponDefHandler(LuaParser* defsParser)
{
	const LuaTable rootTable = defsParser->GetRoot().SubTable("WeaponDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading WeaponDefs");
	}

	vector<string> weaponNames;
	rootTable.GetKeys(weaponNames);

	weaponDefs.reserve(weaponNames.size());

	for (int wid = 0; wid < weaponNames.size(); wid++) {
		const std::string& name = weaponNames[wid];
		const LuaTable wdTable = rootTable.SubTable(name);
		weaponDefs.emplace_back(wdTable, name, wid);
		weaponID[name] = wid;
	}
}


CWeaponDefHandler::~CWeaponDefHandler()
{
}



const WeaponDef* CWeaponDefHandler::GetWeaponDef(std::string weaponname) const
{
	StringToLowerInPlace(weaponname);

	std::map<std::string,int>::const_iterator ii = weaponID.find(weaponname);
	if (ii == weaponID.end())
		return NULL;

	return &weaponDefs[ii->second];
}


const WeaponDef* CWeaponDefHandler::GetWeaponDefByID(int weaponDefId) const
{
	if ((weaponDefId < 0) || (weaponDefId >= weaponDefs.size())) {
		return NULL;
	}
	return &weaponDefs[weaponDefId];
}
