/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaArchive.h"

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "System/FileSystem/ArchiveScanner.h"
#include "System/Util.h"


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
