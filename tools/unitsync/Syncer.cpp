/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "Syncer.h"

#include "Lua/LuaParser.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>


CSyncer::CSyncer()
{
}

CSyncer::~CSyncer()
{
}


void CSyncer::LoadUnits()
{
	LuaParser luaParser("gamedata/defs.lua", SPRING_VFS_MOD_BASE,
			SPRING_VFS_ZIP);
	if (!luaParser.Execute()) {
		throw content_error(
				"luaParser.Execute() failed: " + luaParser.GetErrorLog());
	}

	LuaTable rootTable = luaParser.GetRoot().SubTable("UnitDefs");
	if (!rootTable.IsValid()) {
		throw content_error("root unitdef table invalid");
	}

	std::vector<std::string> unitDefNames;
	rootTable.GetKeys(unitDefNames);

	const int count = (int)unitDefNames.size();

	for (int i = 0; i < count; ++i) {
		const std::string& udName =  unitDefNames[i];
		LuaTable udTable = rootTable.SubTable(udName);

		Unit unit;

		unit.name = udName;
		unit.fullName = udTable.GetString("name", udName);

		units.push_back(unit);
	}
}


int CSyncer::ProcessUnits()
{
	LoadUnits();

	return 0;
}


int CSyncer::GetUnitCount()
{
	return units.size();
}


const std::string& CSyncer::GetUnitName(int unit)
{
	return units[unit].name;
}


const std::string& CSyncer::GetFullUnitName(int unit)
{
	return units[unit].fullName;
}
