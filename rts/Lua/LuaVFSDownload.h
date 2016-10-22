/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DOWNLOADER_H
#define LUA_DOWNLOADER_H

struct lua_State;

#include <deque>
#include "System/Threading/SpringThreading.h"
#include "../tools/pr-downloader/src/pr-downloader.h"


class LuaVFSDownload: public CEventClient {
	public:
		static LuaVFSDownload* GetInstance();
		static void Free(bool stopDownloads = false);
		static void Init();

		bool WantsEvent(const std::string& eventName) override {
			return (eventName == "Update");
		}

		static bool PushEntries(lua_State* L);
		//checks if some events have arrived from other threads and then fires the events
		void Update() override;

	private:
		LuaVFSDownload();
		virtual ~LuaVFSDownload();
		static int DownloadArchive(lua_State* L);
};

#define luaVFSDownload (LuaVFSDownload::GetInstance())

#endif /* LUA_DOWNLOADER_H */
