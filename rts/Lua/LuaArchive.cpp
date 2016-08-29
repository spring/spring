/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaArchive.h"

#include "LuaInclude.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "ExternalAI/LuaAIImplHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Util.h"

#include <string>

/******************************************************************************/
/******************************************************************************/

bool LuaArchive::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

#define REGISTER_NAMED_LUA_CFUNC(x,name) \
	lua_pushstring(L, #name);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(GetMaps);
	REGISTER_LUA_CFUNC(GetGames);
	REGISTER_LUA_CFUNC(GetAllArchives);
	REGISTER_LUA_CFUNC(HasArchive);

	REGISTER_LUA_CFUNC(GetArchiveInfo);
	REGISTER_LUA_CFUNC(GetArchiveDependencies);
	REGISTER_LUA_CFUNC(GetArchiveReplaces);

	REGISTER_LUA_CFUNC(GetArchiveChecksum);

	REGISTER_LUA_CFUNC(GetAvailableAIs);

	return true;
}



/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetMaps(lua_State* L)
{
	const auto& list = archiveScanner->GetMaps();
	lua_createtable(L, list.size(), 0);

	unsigned int count = 0;
	for (const std::string& mapName: list) {
		lua_pushsstring(L, mapName);
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}


int LuaArchive::GetGames(lua_State* L)
{
	const auto& list = archiveScanner->GetPrimaryMods();
	lua_createtable(L, list.size(), 0);

	unsigned int count = 0;
	for (const CArchiveScanner::ArchiveData& gameArchData: list) {
		lua_pushsstring(L, gameArchData.GetNameVersioned());
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}


int LuaArchive::GetAllArchives(lua_State* L)
{
	const auto& list = archiveScanner->GetAllArchives();
	lua_createtable(L, list.size(), 0);

	unsigned int count = 0;
	for (const CArchiveScanner::ArchiveData& archName: list) {
		lua_pushsstring(L, archName.GetNameVersioned());
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::HasArchive(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);
	CArchiveScanner::ArchiveData archiveData = archiveScanner->GetArchiveData(archiveName);
	lua_pushboolean(L, !archiveData.IsEmpty());
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetArchiveInfo(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);
	CArchiveScanner::ArchiveData archiveData = archiveScanner->GetArchiveData(archiveName);
	if (archiveData.IsEmpty()) {
		return 0;
	}

	lua_createtable(L, 0, archiveData.GetInfo().size());

	for (const auto& pair: archiveData.GetInfo()) {
		const std::string& itemName = pair.first;
		const InfoItem&    itemData = pair.second;

		lua_pushsstring(L, itemName);
		switch (itemData.valueType) {
			case INFO_VALUE_TYPE_STRING: {
				lua_pushsstring(L, itemData.valueTypeString);
			} break;
			case INFO_VALUE_TYPE_INTEGER: {
				lua_pushinteger(L, itemData.value.typeInteger);
			} break;
			case INFO_VALUE_TYPE_FLOAT: {
				lua_pushnumber(L,  itemData.value.typeFloat);
			} break;
			case INFO_VALUE_TYPE_BOOL: {
				lua_pushboolean(L, itemData.value.typeBool);
			} break;
			default: assert(false);
		}
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaArchive::GetArchiveDependencies(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);
	CArchiveScanner::ArchiveData archiveData = archiveScanner->GetArchiveData(archiveName);
	if (archiveData.IsEmpty()) {
		return 0;
	}

	const std::vector<std::string>& dependencies = archiveData.GetDependencies();

	unsigned int count = 0;
	lua_createtable(L, dependencies.size(), 0);

	for (const std::string& dependency: dependencies) {
		lua_pushsstring(L, dependency);
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}


int LuaArchive::GetArchiveReplaces(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);
	CArchiveScanner::ArchiveData archiveData = archiveScanner->GetArchiveData(archiveName);
	if (archiveData.IsEmpty()) {
		return 0;
	}

	const std::vector<std::string>& replaces = archiveData.GetReplaces();

	unsigned int count = 0;
	lua_createtable(L, replaces.size(), 0);

	for (const std::string& replace: replaces) {
		lua_pushsstring(L, replace);
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetArchiveChecksum(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);
	const unsigned int checksumSingl = archiveScanner->GetSingleArchiveChecksum(archiveName);
	const unsigned int checksumAccum = archiveScanner->GetArchiveCompleteChecksum(archiveName);
	lua_pushsstring(L, IntToString(checksumSingl, "%X"));
	lua_pushsstring(L, IntToString(checksumAccum, "%X"));
	return 2;
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetAvailableAIs(lua_State* L)
{
	const std::string gameArchiveName = luaL_optsstring(L, 1, "");
	const std::string mapArchiveName = luaL_optsstring(L, 2, "");

	// load selected archives to get lua ais
	if (!gameArchiveName.empty()) {
		vfsHandler->AddArchive(gameArchiveName, false);
	}
	if (!mapArchiveName.empty()) {
		vfsHandler->AddArchive(mapArchiveName, false);
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
	if (!mapArchiveName.empty()) {
		vfsHandler->RemoveArchive(mapArchiveName);
	}
	if (!gameArchiveName.empty()) {
		vfsHandler->RemoveArchive(gameArchiveName);
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
