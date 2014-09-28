/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaArchive.h"

#include "alphanum.hpp"

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "System/FileSystem/ArchiveScanner.h"

#include <set>
#include <list>
#include <cctype>


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

	REGISTER_LUA_CFUNC(HasArchive);
	REGISTER_LUA_CFUNC(GetArchiveInfo);

	REGISTER_LUA_CFUNC(GetArchiveDependencies);
	REGISTER_LUA_CFUNC(GetArchiveReplaces);

	return true;
}



/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetMaps(lua_State* L)
{
	const std::vector<std::string> &arFound = archiveScanner->GetMaps();
	std::set<std::string, doj::alphanum_less<std::string> > mapSet; // use a set to sort them
	for (const auto& mapStr: arFound) {
		mapSet.insert(mapStr.c_str());
	}

	unsigned int count = 0;

	lua_createtable(L, mapSet.size(), 0);

	for (const auto& mapStr: mapSet) {
		lua_pushstring(L, mapStr.c_str());
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetGames(lua_State* L)
{
	const std::vector<CArchiveScanner::ArchiveData> &found = archiveScanner->GetPrimaryMods();
	std::set<std::string, doj::alphanum_less<std::string> > modSet; // use a set to sort them
	for (const auto& modStr: found) {
		modSet.insert(modStr.GetNameVersioned());
	}

	unsigned int count = 0;

	lua_createtable(L, modSet.size(), 0);

	for (const auto& modStr: modSet) {
		lua_pushstring(L, modStr.c_str());
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}

/******************************************************************************/
/******************************************************************************/
bool LuaArchive::_HasArchive(const std::string& archiveName)
{
	CArchiveScanner::ArchiveData archiveData = archiveScanner->GetArchiveData(archiveName);
	return (archiveName != "" && archiveName == archiveData.GetNameVersioned());
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::HasArchive(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);

	lua_pushboolean(L, _HasArchive(archiveName));

	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetArchiveInfo(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);

	if (!_HasArchive(archiveName)) {
		return 0;
	}

	CArchiveScanner::ArchiveData archiveData = archiveScanner->GetArchiveData(archiveName);

	const std::string name = archiveData.GetName();
	const std::string nameVersioned = archiveData.GetNameVersioned();
	const std::string shortName = archiveData.GetShortName();
	const std::string version = archiveData.GetVersion();
	const std::string mutator = archiveData.GetMutator();
	const std::string game = archiveData.GetGame();
	const std::string shortGame = archiveData.GetShortGame();
	const std::string description = archiveData.GetDescription();
	const std::string mapFile = archiveData.GetMapFile();
	const int modType = archiveData.GetModType();
	const int modHash = archiveScanner->GetArchiveCompleteChecksum(nameVersioned);

	lua_pushstring(L, name.c_str());
	lua_pushstring(L, nameVersioned.c_str());
	lua_pushstring(L, shortName.c_str());
	lua_pushstring(L, version.c_str());
	lua_pushstring(L, mutator.c_str());
	lua_pushstring(L, game.c_str());
	lua_pushstring(L, shortGame.c_str());
	lua_pushstring(L, description.c_str());
	lua_pushstring(L, mapFile.c_str());
	lua_pushnumber(L, modType);
	lua_pushnumber(L, modHash);

	return 11;
}


/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetArchiveDependencies(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);

	CArchiveScanner::ArchiveData archiveData = archiveScanner->GetArchiveData(archiveName);

	const std::vector<std::string> dependencies = archiveData.GetDependencies();

	unsigned int count = 0;
	lua_createtable(L, dependencies.size(), 0);

	for (const auto& dependency: dependencies) {
		lua_pushstring(L, dependency.c_str());
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetArchiveReplaces(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);

	CArchiveScanner::ArchiveData archiveData = archiveScanner->GetArchiveData(archiveName);

	const std::vector<std::string> replaces = archiveData.GetReplaces();

	unsigned int count = 0;
	lua_createtable(L, replaces.size(), 0);

	for (const auto& dependency: replaces) {
		lua_pushstring(L, dependency.c_str());
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}
