/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DOWNLOADER_H
#define LUA_DOWNLOADER_H

struct lua_State;

class LuaVFSDownload: public CEventClient {
	public:
		LuaVFSDownload();
		virtual ~LuaVFSDownload();

		bool WantsEvent(const std::string& eventName) override {
			return (eventName == "Update");
		 }

		static bool PushEntries(lua_State* L);
		//checks if some events have arrived from other threads and then fires the events
		void Update() override;

	private:
		static int DownloadArchive(lua_State* L);

};

extern LuaVFSDownload* luavfsdownload;

#endif /* LUA_DOWNLOADER_H */
