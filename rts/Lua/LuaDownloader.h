/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DOWNLOADER_H
#define LUA_DOWNLOADER_H

struct lua_State;

class LuaDownloader {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int Download(lua_State* L);

};


#endif /* LUA_DOWNLOADER_H */
