/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include <string>

#include "Lua/LuaParser.h"
#include "System/float3.h"


class MapParser
{
	public:
		static std::string GetMapConfigName(const std::string& mapFileName);

	public:
		MapParser(const std::string& mapFileName);
		~MapParser();

		LuaParser* GetParser() { return parser; }

		LuaTable GetRoot();
		bool IsValid() const;
		std::string GetErrorLog() const;

		bool GetStartPos(int team, float3& pos) const;

	private:
		LuaParser* parser;
		mutable std::string errorLog;
};

#endif // MAP_PARSER_H
