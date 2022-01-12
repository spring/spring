/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_ARCHIVE_H
#define LUA_ARCHIVE_H

struct lua_State;


class LuaArchive {
	friend class CLuaIntro;

	public:
		static bool PushEntries(lua_State* L);

	private:
		static int GetMaps(lua_State* L);
		static int GetGames(lua_State* L);
		static int GetAllArchives(lua_State* L);
		static int HasArchive(lua_State* L);
		static int GetLoadedArchives(lua_State* L);

		static int GetArchivePath(lua_State* L);
		static int GetArchiveInfo(lua_State* L);
		static int GetArchiveDependencies(lua_State* L);
		static int GetArchiveReplaces(lua_State* L);

		static int GetArchiveChecksum(lua_State* L);

		static int GetNameFromRapidTag(lua_State* L);

		static int GetAvailableAIs(lua_State* L);
};


#endif /* LUA_ARCHIVE_H */
