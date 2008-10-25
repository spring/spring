#include "StdAfx.h"
#include "Syncer.h"
#include "Exceptions.h"
#include "FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include "LogOutput.h"
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

using namespace std;

CSyncer::CSyncer()
{
}


CSyncer::~CSyncer()
{
}


void CSyncer::LoadUnits()
{
	LuaParser luaParser("gamedata/defs.lua",
	                    SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!luaParser.Execute()) {
		throw content_error("luaParser.Execute() failed: " + luaParser.GetErrorLog());
	}

	LuaTable rootTable = luaParser.GetRoot().SubTable("UnitDefs");
	if (!rootTable.IsValid()) {
		throw content_error("root unitdef table invalid");
	}

	vector<string> unitDefNames;
	rootTable.GetKeys(unitDefNames);

	const int count = (int)unitDefNames.size();

	for (int i = 0; i < count; ++i) {
		const string& udName =  unitDefNames[i];
		LuaTable udTable = rootTable.SubTable(udName);

		Unit u;

		u.fullName = udTable.GetString("name", udName);

		units[udName] = u;
	}

	// map the unitIds
	map<string, Unit>::iterator mit;
	for (mit = units.begin(); mit != units.end(); ++mit) {
		unitIds.push_back(mit->first);
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


string CSyncer::GetUnitName(int unit)
{
	string unitName = unitIds[unit];
	return unitName;
}


string CSyncer::GetFullUnitName(int unit)
{
	const string& unitName = unitIds[unit];
	return units[unitName].fullName;
}
