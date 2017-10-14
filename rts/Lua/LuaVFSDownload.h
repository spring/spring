/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DOWNLOADER_H
#define LUA_DOWNLOADER_H

#include "System/EventClient.h"

struct lua_State;

class LuaVFSDownload: public CEventClient {
public:
	static LuaVFSDownload* GetInstance() {
		static LuaVFSDownload instance;
		return &instance;
	}

	static void Init();
	static void Free(bool stopDownloads = false);

	static bool PushEntries(lua_State* L);


	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "Update");
	}

	// checks if events have arrived from download-threads and processes them
	void Update() override;

private:
	LuaVFSDownload();
	~LuaVFSDownload();

	static int DownloadArchive(lua_State* L);
	static int AbortDownload(lua_State* L);
	static int ScanAllDirs(lua_State* L);
};

#define luaVFSDownload (LuaVFSDownload::GetInstance())

#endif /* LUA_DOWNLOADER_H */
