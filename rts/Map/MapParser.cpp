/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "MapParser.h"

#include <string>
#include <ctype.h>

#include "Lua/LuaSyncedRead.h"
#include "FileSystem/FileSystem.h"

using namespace std;

string MapParser::GetMapConfigName(const string& mapName)
{
	if (mapName.length() < 3)
		return "";

	const string extension = filesystem.GetExtension(mapName);

	if (extension == "sm3") {
		return mapName;
	}
	else if (extension == "smf") {
		return mapName.substr(0, mapName.find_last_of('.')) + ".smd";
	}
	else {
		return "";
	}
}


MapParser::MapParser(const string& mapName) : parser(NULL)
{
	const string mapConfig = GetMapConfigName(mapName);

	parser = new LuaParser("maphelper/mapinfo.lua", SPRING_VFS_MAP_BASE, SPRING_VFS_MAP_BASE);
	parser->GetTable("Map");
	parser->AddString("fileName", filesystem.GetFilename(mapName));
	parser->AddString("fullName", mapName);
	parser->AddString("configFile", mapConfig);
	parser->EndTable();
#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
	// this should not be included with unitsync:
	// 1. avoids linkage with LuaSyncedRead
	// 2. MapOptions are not valid during unitsync map parsing
	parser->GetTable("Spring");
	parser->AddFunc("GetMapOptions", LuaSyncedRead::GetMapOptions);
	parser->EndTable();
#endif // !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
	if (!parser->Execute())
	{
		// do nothing
	}
}


MapParser::~MapParser()
{
	delete parser;
}


bool MapParser::GetStartPos(int team, float3& pos) const
{
	if (!parser->IsValid()) {
		return false;
	}
	const LuaTable teamsTable = parser->GetRoot().SubTable("teams");
	const LuaTable posTable = teamsTable.SubTable(team).SubTable("startPos");
	if (!posTable.IsValid()) {
		return false;
	}

	pos.x = posTable.GetFloat("x", pos.x);
	pos.z = posTable.GetFloat("z", pos.z);

	return true;
}


LuaTable MapParser::GetRoot()
{
	if (parser) {
		return parser->GetRoot();
	} else {
		return LuaTable();
	}
}


bool MapParser::IsValid() const
{
	if (parser) {
		return parser->IsValid();
	} else {
		return false;
	}
}


std::string MapParser::GetErrorLog() const
{
	if (parser) {
		return parser->GetErrorLog();
	} else {
		return "could not find file";
	}
}
