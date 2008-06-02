#include "StdAfx.h"

#include <string>
#include <ctype.h>
using namespace std;

#include "MapParser.h"
#include "Lua/LuaSyncedRead.h"
#include "System/FileSystem/FileHandler.h"


static bool IsTdfFormat(CFileHandler& fh)
{

	while (!fh.Eof()) {
		char c = ' ';
		fh.Read(&c, 1);
		if (!isspace(c)) {
			// TDF files should start with:
			//   [name] - section header
			//   /*     - comment
			//   //     - comment
			return ((c == '[') || (c == '/'));
		}
	}
	return false;		
}


string MapParser::GetMapConfigName(const string& mapName)
{
	if (mapName.length() < 3) {
		throw runtime_error("CMapInfo::GetMapConfigName(): mapName '"
		                         + mapName + "' too short");
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
		throw runtime_error("MapParser::GetMapConfigName(): Unknown extension: "
		                    + extension);
	}
}


MapParser::MapParser(const string& mapName) : parser(NULL)
{
	const string mapConfig = GetMapConfigName(mapName);

	CFileHandler fh(mapConfig, SPRING_VFS_MAP);
	if (!fh.FileExists()) {
		throw runtime_error("MapParser() missing file: " + mapConfig);
	}

	const bool isTDF =  IsTdfFormat(fh);

	const string configScript = isTDF ? "maphelper/load_tdf_map.lua" : mapConfig;
	
	parser = SAFE_NEW LuaParser(configScript,
															SPRING_VFS_MAP_BASE, SPRING_VFS_MAP_BASE);
	parser->GetTable("Map");
	parser->AddString("fileName", mapName);
	parser->AddString("fullName", "maps/" + mapName);
	parser->AddString("configFile", mapConfig);
#ifndef UNITSYNC
	// this should not be included with unitsync:
	// 1. avoids linkage with LuaSyncedRead
	// 2. MapOptions are not valid during unitsync map parsing
	parser->AddFunc("GetMapOptions", LuaSyncedRead::GetMapOptions);
#endif
	parser->EndTable();
	parser->Execute();
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
