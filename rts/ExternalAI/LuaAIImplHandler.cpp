/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaAIImplHandler.h"

#include "Lua/LuaParser.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
//#include "ExternalAI/SkirmishAIKey.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Exceptions.h"

#include <assert.h>

//CR_BIND(CLuaAIImplHandler,);
//
//CR_REG_METADATA(CLuaAIImplHandler, (
//	CR_RESERVED(64)
//));


CLuaAIImplHandler* CLuaAIImplHandler::mySingleton = NULL;

CLuaAIImplHandler& CLuaAIImplHandler::GetInstance() {

	if (mySingleton == NULL) {
		mySingleton = new CLuaAIImplHandler();
	}

	return *mySingleton;
}

CLuaAIImplHandler::CLuaAIImplHandler()
{
}

CLuaAIImplHandler::~CLuaAIImplHandler()
{
}

std::vector< std::vector<InfoItem> > CLuaAIImplHandler::LoadInfos() {

	std::vector< std::vector<InfoItem> > luaAIInfos;

	LuaParser luaParser("LuaAI.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	if (!luaParser.Execute()) {
		// It is not an error if the mod does not come with Lua AIs.
		return luaAIInfos;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	for (int i = 1; root.KeyExists(i); i++) {
		std::string shortName;
		std::string description;
		// Lua AIs can be specified in two different formats:
		shortName = root.GetString(i, "");
		if (!shortName.empty()) {
			// ... string format (name)

			description = "(please see mod description, forum or homepage)";
		} else {
			// ... table format  (name & desc)

			const LuaTable& optTbl = root.SubTable(i);
			if (!optTbl.IsValid()) {
				continue;
			}
			shortName = optTbl.GetString("name", "");
			if (shortName.empty()) {
				continue;
			}

			description = optTbl.GetString("desc", shortName);
		}

		struct InfoItem ii;
		std::vector<InfoItem> aiInfo;

		ii.key = SKIRMISH_AI_PROPERTY_SHORT_NAME;
		ii.value = shortName;
		ii.desc = "the short name of this Lua Skirmish AI";
		aiInfo.push_back(ii);

		ii.key = SKIRMISH_AI_PROPERTY_VERSION;
		ii.value = "<not-versioned>";
		ii.desc = "Lua Skirmish AIs do not have a version, "
				"because they are fully defined by the mods version already.";
		aiInfo.push_back(ii);

		ii.key = SKIRMISH_AI_PROPERTY_NAME;
		ii.value = shortName + " (Mod specific AI)";
		ii.desc = "the human readable name of this Lua Skirmish AI";
		aiInfo.push_back(ii);

		ii.key = SKIRMISH_AI_PROPERTY_DESCRIPTION;
		ii.value = description;
		ii.desc = "a short description of this Lua Skirmish AI";
		aiInfo.push_back(ii);

		luaAIInfos.push_back(aiInfo);
	}

	return luaAIInfos;
}
