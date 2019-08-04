/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaArchive.h"

#include "LuaInclude.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "ExternalAI/LuaAIImplHandler.h"
#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/RapidHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"

#include <string>

/******************************************************************************/
/******************************************************************************/

bool LuaArchive::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetMaps);
	REGISTER_LUA_CFUNC(GetGames);
	REGISTER_LUA_CFUNC(GetAllArchives);
	REGISTER_LUA_CFUNC(HasArchive);
	REGISTER_LUA_CFUNC(GetLoadedArchives);

	REGISTER_LUA_CFUNC(GetArchivePath);
	REGISTER_LUA_CFUNC(GetArchiveInfo);
	REGISTER_LUA_CFUNC(GetArchiveDependencies);
	REGISTER_LUA_CFUNC(GetArchiveReplaces);

	REGISTER_LUA_CFUNC(GetArchiveChecksum);

	REGISTER_LUA_CFUNC(GetNameFromRapidTag);

	REGISTER_LUA_CFUNC(GetAvailableAIs);

	return true;
}



/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetMaps(lua_State* L)
{
	LuaUtils::PushStringVector(L, archiveScanner->GetMaps());
	return 1;
}


int LuaArchive::GetGames(lua_State* L)
{
	const auto& archives = archiveScanner->GetPrimaryMods();

	lua_createtable(L, archives.size(), 0);

	for (const auto& archiveData: archives) {
		lua_pushsstring(L, archiveData.GetNameVersioned());
		lua_rawseti(L, -2, 1 + (&archiveData - &archives[0]));
	}

	return 1;
}

int LuaArchive::GetAllArchives(lua_State* L)
{
	const auto& archives = archiveScanner->GetAllArchives();

	lua_createtable(L, archives.size(), 0);

	for (const auto& archiveData: archives) {
		lua_pushsstring(L, archiveData.GetNameVersioned());
		lua_rawseti(L, -2, 1 + (&archiveData - &archives[0]));
	}

	return 1;
}


int LuaArchive::HasArchive(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);
	const CArchiveScanner::ArchiveData& archiveData = archiveScanner->GetArchiveData(archiveName);

	lua_pushboolean(L, !archiveData.IsEmpty());
	return 1;
}


int LuaArchive::GetLoadedArchives(lua_State* L)
{
	LuaUtils::PushStringVector(L, vfsHandler->GetAllArchiveNames());
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetArchivePath(lua_State* L)
{
	const auto archive = archiveScanner->ArchiveFromName(luaL_checksstring(L, 1));
	const std::string archivePath = archiveScanner->GetArchivePath(archive) + archive;

	if (archivePath.empty())
		return 0;

	lua_pushsstring(L, archivePath);
	return 1;
}


int LuaArchive::GetArchiveInfo(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);
	const auto archiveData = archiveScanner->GetArchiveData(archiveName);

	if (archiveData.IsEmpty())
		return 0;

	lua_createtable(L, 0, archiveData.GetInfo().size());

	for (const auto& pair: archiveData.GetInfo()) {
		const std::string& itemName = pair.first;
		const InfoItem& itemData    = pair.second;

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
	const auto archiveData = archiveScanner->GetArchiveData(archiveName);

	if (archiveData.IsEmpty())
		return 0;

	LuaUtils::PushStringVector(L, archiveData.GetDependencies());
	return 1;
}


int LuaArchive::GetArchiveReplaces(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);
	const auto archiveData = archiveScanner->GetArchiveData(archiveName);

	if (archiveData.IsEmpty())
		return 0;

	LuaUtils::PushStringVector(L, archiveData.GetReplaces());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetArchiveChecksum(lua_State* L)
{
	const std::string archiveName = luaL_checksstring(L, 1);

	sha512::hex_digest asChecksumHexDigest;
	sha512::hex_digest acChecksumHexDigest;

	sha512::dump_digest(archiveScanner->GetArchiveSingleChecksumBytes(archiveName), asChecksumHexDigest);
	sha512::dump_digest(archiveScanner->GetArchiveCompleteChecksumBytes(archiveName), acChecksumHexDigest);

	lua_pushsstring(L, asChecksumHexDigest.data());
	lua_pushsstring(L, acChecksumHexDigest.data());
	return 2;
}


/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetNameFromRapidTag(lua_State* L)
{
	const std::string rapidName = luaL_checksstring(L, 1);
	const std::string archiveName = GetRapidPackageFromTag(rapidName);

	if (archiveName == rapidName)
		return 0;

	lua_pushsstring(L, archiveScanner->NameFromArchive(archiveName));
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaArchive::GetAvailableAIs(lua_State* L)
{
	const std::string gameArchiveName = luaL_optsstring(L, 1, "");
	const std::string  mapArchiveName = luaL_optsstring(L, 2, "");

	LOG("LuaArchive::%s] game=\"%s\" (cached=%d loaded=%d) map=\"%s\" (cached=%d loaded=%d)", __func__,
		gameArchiveName.c_str(), vfsHandler->HasTempArchive(gameArchiveName), vfsHandler->HasArchive(gameArchiveName),
		 mapArchiveName.c_str(), vfsHandler->HasTempArchive( mapArchiveName), vfsHandler->HasArchive( mapArchiveName)
	);

	vfsHandler->GrabLock();
	vfsHandler->SetName("LuaArchiveVFS");
	// usually only called from LuaMenu between (re)load and PreGame
	// NB: if called at actual game time, must stash current mod/map
	vfsHandler->UnMapArchives(false);

	// load selected archives to access Lua AI's
	// if names are empty, scan current archives
	//
	// in case either archive is already stashed
	// (from a previous reload) this will simply
	// repopulate files
	if (!gameArchiveName.empty())
		vfsHandler->AddArchive(gameArchiveName, false);

	if (!mapArchiveName.empty())
		vfsHandler->AddArchive(mapArchiveName, false);

	{
		// executing LuaAI.lua can not do harm to VFS
		const auto& luaAIInfoItems = luaAIImplHandler.LoadInfoItems();
		const auto& skirmishAIKeys = aiLibManager->GetSkirmishAIKeys();

		lua_createtable(L, skirmishAIKeys.size() + luaAIInfoItems.size(), 0);

		unsigned int count = 0;

		for (const auto& luaAIInfo: luaAIInfoItems) {
			lua_createtable(L, 0, luaAIInfo.size()); {
				for (const auto& luaAIInfoItem: luaAIInfo) {
					if (luaAIInfoItem.key == SKIRMISH_AI_PROPERTY_SHORT_NAME) {
						HSTR_PUSH_STRING(L, "shortName", luaAIInfoItem.GetValueAsString());
					} else if (luaAIInfoItem.key == SKIRMISH_AI_PROPERTY_VERSION) {
						HSTR_PUSH_STRING(L, "version", luaAIInfoItem.GetValueAsString());
					}
				}
			}
			lua_rawseti(L, -2, ++count);
		}

		for (const auto& aiKey: skirmishAIKeys) {
			lua_createtable(L, 0, 2); {
				HSTR_PUSH_STRING(L, "shortName", aiKey.GetShortName());
				HSTR_PUSH_STRING(L, "version", aiKey.GetVersion());
			}
			lua_rawseti(L, -2, ++count);
		}
	}

	vfsHandler->ReMapArchives(false);
	vfsHandler->SetName("SpringVFS");
	vfsHandler->FreeLock();
	return 1;
}
