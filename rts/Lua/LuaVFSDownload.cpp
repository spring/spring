/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaVFSDownload.h"

#include "LuaInclude.h"
#include "../tools/pr-downloader/src/pr-downloader.h"

/******************************************************************************/
/******************************************************************************/
#define REGISTER_LUA_CFUNC(x) \
        lua_pushstring(L, #x);      \
        lua_pushcfunction(L, x);    \
        lua_rawset(L, -3)


bool LuaVFSDownload::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(DownloadArchive);
	return true;
}

/******************************************************************************/

int LuaVFSDownload::DownloadArchive(lua_State* L)
{
	const std::string filename = luaL_checkstring(L, 1);
	const std::string category = luaL_checkstring(L, 2);
	if (filename.empty())
		return 0;
	if (category.empty())
		return 0;

//FIXME: make async!
	LOG_L(L_DEBUG, "going to download %s", filename.c_str());
	DownloadInit();
	const int count = DownloadSearch(DL_ANY, CAT_ANY, filename.c_str());
	for(int i=0; i<count; i++) {
		DownloadAdd(i);
		struct downloadInfo dl;
		if (DownloadGetSearchInfo(i, dl)) {
			LOG_L(L_DEBUG, "Download info: %s %d %d", dl.filename, dl.type, dl.cat);
		}
	}
	DownloadStart();
	DownloadShutdown();

	return 0;
}


