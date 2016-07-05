/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaAI.h"

#include "LuaInclude.h"
#include "LuaHashString.h"
#include "LuaUtils.h"


#include "ExternalAI/LuaAIImplHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"

#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"


/******************************************************************************/
/******************************************************************************/

bool LuaAI::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

#define REGISTER_NAMED_LUA_CFUNC(x,name) \
	lua_pushstring(L, #name);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(GetAvailableAIs);

	return true;
}

// returns absolute filename for given archive name, empty if not found
static const std::string GetFileName(const std::string& name){
	if (name.empty())
		return name;
	const std::string& filename = archiveScanner->ArchiveFromName(name);
	if (filename == name)
		return "";
	const std::string& path = archiveScanner->GetArchivePath(filename);
	return path + filename;
}

/******************************************************************************/
/******************************************************************************/

int LuaAI::GetAvailableAIs(lua_State* L)
{
	const std::string gameArchivePath = GetFileName(luaL_optsstring(L, 1, ""));
	const std::string mapArchivePath = GetFileName(luaL_optsstring(L, 2, ""));

	// load selected archives to get lua ais
	if (!gameArchivePath.empty()) {
		vfsHandler->AddArchive(gameArchivePath, true);
	}
	if (!mapArchivePath.empty()) {
		vfsHandler->AddArchive(mapArchivePath, true);
	}

	const IAILibraryManager::T_skirmishAIKeys& skirmishAIKeys = aiLibManager->GetSkirmishAIKeys();
	std::vector< std::vector<InfoItem> > luaAIInfos = luaAIImplHandler.LoadInfos();	

	lua_createtable(L, skirmishAIKeys.size() + luaAIInfos.size(), 0);
	unsigned int count = 0;

	for(int i=0; i<luaAIInfos.size(); i++) {
		lua_newtable(L); {
			for (int j=0; j<luaAIInfos[i].size(); j++) {
				if (luaAIInfos[i][j].key==SKIRMISH_AI_PROPERTY_SHORT_NAME) {
					HSTR_PUSH_STRING(L, "shortName", luaAIInfos[i][j].GetValueAsString());
				} else if (luaAIInfos[i][j].key==SKIRMISH_AI_PROPERTY_VERSION) {
					HSTR_PUSH_STRING(L, "version", luaAIInfos[i][j].GetValueAsString());
				}
			}
		}
		lua_rawseti(L, -2, ++count);
	}
	
	// close archives
	if (!mapArchivePath.empty()) {
		vfsHandler->RemoveArchive(mapArchivePath);
	}
	if (!gameArchivePath.empty()) {
		vfsHandler->RemoveArchive(gameArchivePath);
	}
	
	IAILibraryManager::T_skirmishAIKeys::const_iterator i = skirmishAIKeys.begin();
	IAILibraryManager::T_skirmishAIKeys::const_iterator e = skirmishAIKeys.end();

	for (; i != e; ++i) {
		lua_newtable(L); {
			HSTR_PUSH_STRING(L, "shortName", i->GetShortName());
			HSTR_PUSH_STRING(L, "version", i->GetVersion());
		}
		lua_rawseti(L, -2, ++count);
	}
	
	return 1;
}

/******************************************************************************/
/******************************************************************************/
