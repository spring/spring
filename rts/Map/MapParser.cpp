/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MapParser.h"

#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "System/float3.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

#include <cassert>
#include <cctype>


std::string MapParser::GetMapConfigName(const std::string& mapFileName)
{
	const std::string directory = FileSystem::GetDirectory(mapFileName);
	const std::string filename  = FileSystem::GetBasename(mapFileName);
	const std::string extension = FileSystem::GetExtension(mapFileName);

	if (extension == "smf")
		return directory + filename + ".smd";

	return mapFileName;
}


MapParser::MapParser(const std::string& mapFileName) : parser(nullptr)
{
	const std::string mapConfig = GetMapConfigName(mapFileName);

	if (CFileHandler::FileExists("mapinfo.lua", SPRING_VFS_MAP_BASE)) {
		// map supplies its own
		parser = new LuaParser("mapinfo.lua", SPRING_VFS_MAP_BASE, SPRING_VFS_MAP_BASE);
	} else {
		// rely on basecontent
		parser = new LuaParser("maphelper/mapinfo.lua", SPRING_VFS_MAP_BASE, SPRING_VFS_MAP_BASE);
	}

	parser->GetTable("Map");
	parser->AddString("fileName", FileSystem::GetFilename(mapFileName));
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

	if (parser->Execute())
		return;

	errorLog = parser->GetErrorLog();
}


MapParser::~MapParser()
{
	delete parser;
	parser = nullptr;
}


bool MapParser::GetStartPos(int team, float3& pos) const
{
	errorLog.clear();

	if (!parser->IsValid()) {
		errorLog = "[MapParser] can not get start-position for team " + IntToString(team) + ", reason: " + parser->GetErrorLog();
		return false;
	}

	const LuaTable teamsTable = parser->GetRoot().SubTable("teams");
	const LuaTable posTable = teamsTable.SubTable(team).SubTable("startPos");

	if (!posTable.IsValid()) {
		errorLog = "[MapParser] start-position for team " + IntToString(team) + " not defined in the map's config";
		return false;
	}

	pos.x = posTable.GetFloat("x", pos.x);
	pos.z = posTable.GetFloat("z", pos.z);
	return true;
}


LuaTable MapParser::GetRoot()
{
	// can never be non-null
	assert(parser != nullptr);
	errorLog.clear();
	return parser->GetRoot();
}


bool MapParser::IsValid() const
{
	assert(parser != nullptr);
	return parser->IsValid();
}


const std::string& MapParser::GetErrorLog() const
{
	assert(parser != nullptr);
	return errorLog;
}

