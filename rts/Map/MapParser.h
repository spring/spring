#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include <string>
#include "float3.h"
#include "Lua/LuaParser.h"


class MapParser
{
	public:
		static std::string GetMapConfigName(const std::string& mapName);

	public:
		MapParser(const std::string& mapName);
		~MapParser();

		LuaParser* GetParser() { return parser; }

		LuaTable GetRoot();
		bool IsValid() const;
		std::string GetErrorLog() const;

		bool GetStartPos(int team, float3& pos) const;

	private:
		LuaParser* parser;
};


#endif // MAP_PARSER_H
