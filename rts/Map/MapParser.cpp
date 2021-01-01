/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MapParser.h"

#include "Lua/LuaParser.h"
#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
#include "Lua/LuaSyncedRead.h"
#endif
#include "System/float3.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

#include <cassert>
#include <cctype>

static const char* mapInfos[] = {"maphelper/mapinfo.lua", "mapinfo.lua"};
static const char* vfsModes   = SPRING_VFS_MAP_BASE;


std::string MapParser::GetMapConfigName(const std::string& mapFileName)
{
	const std::string directory = FileSystem::GetDirectory(mapFileName);
	const std::string filename  = FileSystem::GetBasename(mapFileName);
	const std::string extension = FileSystem::GetExtension(mapFileName);

	if (extension == "smf")
		return directory + filename + ".smd";

	return mapFileName;
}


// check if map supplies its own info, otherwise rely on basecontent
MapParser::MapParser(const std::string& mapFileName): parser(mapInfos[CFileHandler::FileExists(mapInfos[1], vfsModes)], vfsModes, vfsModes)
{
	parser.GetTable("Map");
	parser.AddString("fileName", FileSystem::GetFilename(mapFileName));
	parser.AddString("fullName", mapFileName);
	parser.AddString("configFile", GetMapConfigName(mapFileName));
	parser.EndTable();

#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
	// this should not be included with unitsync:
	// 1. avoids linkage with LuaSyncedRead
	// 2. MapOptions are not valid during unitsync map parsing
	parser.GetTable("Spring");
	parser.AddFunc("GetMapOptions", LuaSyncedRead::GetMapOptions);
	parser.EndTable();
#endif

	if (parser.Execute())
		return;

	errorLog = parser.GetErrorLog();
}


bool MapParser::GetStartPos(int team, float3& pos)
{
	errorLog.clear();

	if (!parser.IsValid()) {
		errorLog = "[MapParser] can not get start-position for team " + IntToString(team) + ": " + parser.GetErrorLog();
		return false;
	}

	const LuaTable&  rootTable = parser.GetRoot();
	const LuaTable& teamsTable =  rootTable.SubTable("teams");
	const LuaTable&  teamTable = teamsTable.SubTable(team);
	const LuaTable&   posTable =  teamTable.SubTable("startPos");

	if (!posTable.IsValid()) {
		errorLog = "[MapParser] start-position for team " + IntToString(team) + " not defined in the map's config";
		return false;
	}

	pos.x = posTable.GetFloat("x", pos.x);
	pos.z = posTable.GetFloat("z", pos.z);
	return true;
}

