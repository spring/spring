#include "StdAfx.h"

#include <string>
#include <ctype.h>
using namespace std;

#include "MapParser.h"
#include "Lua/LuaSyncedRead.h"
#include "System/FileSystem/FileHandler.h"
#include "LogOutput.h"

#include "mmgr.h"


static CLogSubsystem LOG_MAPPARSER("MapParser");


string MapParser::GetMapConfigName(const string& mapName)
{
	if (mapName.length() < 3) {
		logOutput.Print(LOG_MAPPARSER, "map name too short");
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
		logOutput.Print(LOG_MAPPARSER, "unknown extension");
		return "";
	}
}


MapParser::MapParser(const string& mapName) : parser(NULL)
{
	logOutput.Print(LOG_MAPPARSER, "MapParser::MapParser(mapName = \"%s\")", mapName.c_str());

	const string mapConfig = GetMapConfigName(mapName);

	parser = SAFE_NEW LuaParser("maphelper/mapinfo.lua", SPRING_VFS_MAP_BASE, SPRING_VFS_MAP_BASE);
	parser->GetTable("Map");
	parser->AddString("fileName", mapName);
	parser->AddString("fullName", "maps/" + mapName);
	parser->AddString("configFile", mapConfig);
	parser->EndTable();
#if !defined UNITSYNC && !defined DEDICATED
	// this should not be included with unitsync:
	// 1. avoids linkage with LuaSyncedRead
	// 2. MapOptions are not valid during unitsync map parsing
	parser->GetTable("Spring");
	parser->AddFunc("GetMapOptions", LuaSyncedRead::GetMapOptions);
	parser->EndTable();
#endif
	if (!parser->Execute()) {
		logOutput.Print(LOG_MAPPARSER, "parser->Execute() failed: " + parser->GetErrorLog());
		// do nothing
	}
}


MapParser::~MapParser()
{
	logOutput.Print(LOG_MAPPARSER, "MapParser::~MapParser");
	delete parser;
}


bool MapParser::GetStartPos(int team, float3& pos) const
{
	if (!IsValid()) {
		logOutput.Print(LOG_MAPPARSER, "GetStartPos: parser invalid");
		return false;
	}
	const LuaTable teamsTable = parser->GetRoot().SubTable("teams");
	const LuaTable posTable = teamsTable.SubTable(team + 1).SubTable("startPos");
	if (!posTable.IsValid()) {
		logOutput.Print(LOG_MAPPARSER, "GetStartPos: posTable invalid for team %d", team);
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
