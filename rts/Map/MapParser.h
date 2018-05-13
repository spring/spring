/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include "Lua/LuaParser.h"

#include <string>

class float3;


class MapParser
{
public:
	static std::string GetMapConfigName(const std::string& mapFileName);

public:
	MapParser(const std::string& mapFileName);

	// no-copy
	MapParser(const MapParser&) = delete;

	LuaTable GetRoot() {
		errorLog.clear();
		return parser.GetRoot();
	}

	bool IsValid() const { return parser.IsValid(); }
	bool GetStartPos(int team, float3& pos);

	const std::string& GetErrorLog() const { return errorLog; }

private:
	LuaParser parser;

	std::string errorLog;
};

#endif // MAP_PARSER_H
