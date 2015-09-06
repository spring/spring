/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DOWNLOADER_H
#define LUA_DOWNLOADER_H

struct lua_State;

class LuaVFSDownload {
	public:
		static bool PushEntries(lua_State* L);
		//checks if some events have arrived from other threads and then fires the events
		static void ProcessDownloads();

	private:
		static int DownloadArchive(lua_State* L);
		static int CalcMd5(lua_State* L);

};


#endif /* LUA_DOWNLOADER_H */
