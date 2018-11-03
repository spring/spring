/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaAIImplHandler.h"

#include "Lua/LuaParser.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
//#include "ExternalAI/SkirmishAIKey.h"
#include "System/Exceptions.h"

#include <cassert>

//CR_BIND(CLuaAIImplHandler,);
//
//CR_REG_METADATA(CLuaAIImplHandler, (
//	CR_RESERVED(64)
//));


CLuaAIImplHandler& CLuaAIImplHandler::GetInstance()
{
	static CLuaAIImplHandler mySingleton;
	return mySingleton;
}

CLuaAIImplHandler::InfoItemVector CLuaAIImplHandler::LoadInfoItems()
{
	InfoItemVector luaAIInfos;

	LuaParser luaParser("LuaAI.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);

	// It is not an error if the mod does not come with Lua AIs.
	if (!luaParser.Execute())
		return luaAIInfos;

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid())
		throw content_error("root table invalid");

	for (int i = 1; root.KeyExists(i); i++) {
		std::string shortName = root.GetString(i, "");
		std::string description;
		std::string version;

		// Lua AIs can be specified in two different formats, string (name) or table (name & desc)
		if (!shortName.empty()) {
			description = "(please see game description, forum or homepage)";
		} else {
			const LuaTable& optTbl = root.SubTable(i);

			if (!optTbl.IsValid())
				continue;

			shortName = optTbl.GetString("name", "");
			if (shortName.empty())
				continue;

			description = optTbl.GetString("desc", shortName);
			version = optTbl.GetString("version", "<not-versioned>");
		}

		luaAIInfos.emplace_back();
		auto& aiInfo = luaAIInfos.back();

		InfoItem ii;
		ii.key = SKIRMISH_AI_PROPERTY_SHORT_NAME;
		ii.valueType = INFO_VALUE_TYPE_STRING;
		ii.valueTypeString = shortName;
		ii.desc = "short name of this Lua Skirmish AI";
		aiInfo[0] = ii;

		ii.key = SKIRMISH_AI_PROPERTY_VERSION;
		ii.valueType = INFO_VALUE_TYPE_STRING;
		ii.valueTypeString = version;
		ii.desc = "version of this Lua Skirmish AI, normally defined by the game's version";
		aiInfo[1] = ii;

		ii.key = SKIRMISH_AI_PROPERTY_NAME;
		ii.valueType = INFO_VALUE_TYPE_STRING;
		ii.valueTypeString = shortName + " (game-specific AI)";
		ii.desc = "human-readable name of this Lua Skirmish AI";
		aiInfo[2] = ii;

		ii.key = SKIRMISH_AI_PROPERTY_DESCRIPTION;
		ii.valueType = INFO_VALUE_TYPE_STRING;
		ii.valueTypeString = description;
		ii.desc = "short description of this Lua Skirmish AI";
		aiInfo[3] = ii;
	}

	return luaAIInfos;
}
