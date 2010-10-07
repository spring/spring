/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "MapParser.h"

#include <string>
#include <ctype.h>

#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "FileSystem/FileSystem.h"


std::string MapParser::GetMapConfigName(const std::string& mapFileName)
{
	const std::string directory = filesystem.GetDirectory(mapFileName);
	const std::string filename  = filesystem.GetBasename(mapFileName);
	const std::string extension = filesystem.GetExtension(mapFileName);

	if (extension == "sm3") {
		return mapFileName;
	}
	else if (extension == "smf") {
		return directory + filename + ".smd";
	}
	else {
		return mapFileName;
	}
}


MapParser::MapParser(const std::string& mapFileName) : parser(NULL)
{
	const std::string mapConfig = GetMapConfigName(mapFileName);

	parser = new LuaParser("maphelper/mapinfo.lua", SPRING_VFS_MAP_BASE, SPRING_VFS_MAP_BASE);
	parser->GetTable("Map");
	parser->AddString("fileName", filesystem.GetFilename(mapFileName));
	parser->AddString("fullName", mapFileName);
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
