/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/EventHandler.h"

#include "LuaVFSDownload.h"

#include <future>

#include "LuaInclude.h"
#include "../tools/pr-downloader/src/pr-downloader.h"
#include "../tools/pr-downloader/src/lib/md5/md5.h"
#include "base64.h"

/******************************************************************************/
/******************************************************************************/
#define REGISTER_LUA_CFUNC(x) \
        lua_pushstring(L, #x);      \
        lua_pushcfunction(L, x);    \
        lua_rawset(L, -3)

bool LuaVFSDownload::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(DownloadArchive);
	REGISTER_LUA_CFUNC(CalcMd5);
	return true;
}

struct DownloadItem {
	std::string filename;
	std::string category;
	int ID;
	DownloadItem(const std::string& filename, const std::string& category, int ID) : filename(filename), category(category), ID(ID) {
	}
};
static std::future<int> result;
static int downloadID = -1;
static int queuedID = -1;
static std::list<DownloadItem> queue;

void StartDownload();

void UpdateProgress(int done, int size) {
	//eventHandler.ownloadProgress(downloadID, done, size);
}

int Download(const std::string& filename)
{
//	SetDownloadListener(UpdateProgress);
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
	//eventBatchHandler.DownloadStarted(downloadID);
	int result = DownloadStart();
	DownloadShutdown();
	LOG_L(L_DEBUG, "download finished %s", filename.c_str());
	if (!queue.empty()) {
		StartDownload();
	}
	return result;
}

void StartDownload() {
	const DownloadItem& downloadItem = queue.front();
	queue.pop_front();
	const std::string filename = downloadItem.filename;
	const std::string category = downloadItem.category;
	const int ID = downloadItem.ID;
	result = std::async(std::launch::async, [filename, category, ID]() {
			int result = Download(filename);
			if (result == 0) {
				//eventBatchHandler.DownloadFinished(ID);
			} else {
				//eventBatchHandler.DownloadFailed(ID, result);
			}
			return result;
		}
	);
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
	queuedID++;
	queue.push_back(DownloadItem(filename, category, queuedID));	
	//eventHandler.DownloadQueued(downloadID, filename, category);
	if (queue.size() == 1) {
		StartDownload();
	}
	return 0;
}

int LuaVFSDownload::CalcMd5(lua_State* L)
{
	const std::string sstr = luaL_checkstring(L, 1);
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*) sstr.c_str(), sstr.size());
	MD5Final(&ctx);
	const unsigned char* md5sum = ctx.digest;
	std::string encoded = base64_encode(md5sum, 16); 
	//const char* md5sum = CalculateMd5((unsigned char*) str.c_str());
	printf("input: %s digest: %s", sstr.c_str(), encoded.c_str());
	lua_pushsstring(L, encoded);
	return 1;
}
