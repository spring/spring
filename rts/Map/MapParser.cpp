#include "StdAfx.h"

#include <string>
#include <ctype.h>
using namespace std;

#include "mmgr.h"

#include "MapParser.h"
#include "Lua/LuaSyncedRead.h"
#include "FileSystem/FileHandler.h"


string MapParser::GetMapConfigName(const string& mapName)
{
	if (mapName.length() < 3) {
		return "";
	}

	const string extension = mapName.substr(mapName.length() - 3);

	if (extension == "sm3") {
		return string("maps/") + mapName;
	}
	else if (extension == "smf") {
		return string("maps/") +
		       mapName.substr(0, mapName.find_last_of('.')) + ".smd";
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
	parser->AddString("fileName", mapName);
	parser->AddString("fullName", "maps/" + mapName);
	parser->AddString("configFile", mapConfig);
	parser->EndTable();
#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
	// this should not be included with unitsync:
	// 1. avoids linkage with LuaSyncedRead
	// 2. MapOptions are not valid during unitsync map parsing
	parser->GetTable("Spring");
	parser->AddFunc("GetMapOptions", LuaSyncedRead::GetMapOptions);
	parser->EndTable();
#endif // !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
	if (!parser->Execute()) {
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
