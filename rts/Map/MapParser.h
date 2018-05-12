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
		~MapParser();

		// no-copy
		MapParser(const MapParser&) = delete;

		LuaParser* GetParser() { return parser; }
		LuaTable GetRoot();

		bool IsValid() const;
		bool GetStartPos(int team, float3& pos) const;

		const std::string& GetErrorLog() const;

	private:
		LuaParser* parser;
		mutable std::string errorLog;
};

#endif // MAP_PARSER_H
