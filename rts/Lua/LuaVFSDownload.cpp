/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/EventHandler.h"

#include "LuaVFSDownload.h"
#include "System/Util.h"
#include "System/EventHandler.h"
#include "System/Platform/Threading.h"

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

struct dlStarted {
	int ID;
};
dlStarted* dls = NULL;
struct dlFinished {
	int ID;
};
dlFinished* dlfi = NULL;
struct dlFailed {
	int ID;
	int errorID;
};
dlFailed* dlfa = NULL;
struct dlProgress {
	int ID;
	long downloaded;
	long total;
};
dlProgress* dlp = NULL;

// TODO: all these functions need to lock the appropriate variables.
void QueueDownloadStarted(int ID) //queue from other thread download started event
{
	dls = new dlStarted(); dls->ID = ID;
}

void QueueDownloadFinished(int ID) //queue from other thread download started event
{
	dlfi = new dlFinished(); dlfi->ID = ID;

}

void QueueDownloadFailed(int ID, int errorID) //queue from other thread download started event
{
	dlfa = new dlFailed(); dlfa->ID = ID; dlfa->errorID = errorID;
}

void QueueDownloadProgress(int ID, long downloaded, long total) //queue from other thread download started event
{
	dlp = new dlProgress(); dlp->ID = ID; dlp->downloaded = downloaded; dlp->total = total;
}



bool LuaVFSDownload::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(DownloadArchive);
	REGISTER_LUA_CFUNC(CalcMd5);
	return true;
}

struct DownloadItem {
	int ID;
	std::string filename;
	category cat;
	DownloadItem(int ID, const std::string& filename, category& cat) : ID(ID), filename(filename), cat(cat) {
	}
};

static int queueIDCount = -1;
static int currentDownloadID = -1;
static std::list<DownloadItem> queue;
static bool isDownloading = false;

void StartDownload();

void UpdateProgress(int done, int size) {
	QueueDownloadProgress(currentDownloadID, done, size);
}

int Download(int ID, const std::string& filename, category cat)
{
	currentDownloadID = ID;
	// FIXME: Progress is incorrectly updated when rapid is queried to check for existing packages.
	SetDownloadListener(UpdateProgress);
	LOG_L(L_DEBUG, "Going to download %s", filename.c_str());
	DownloadInit();
	const int count = DownloadSearch(DL_ANY, cat, filename.c_str());
	for (int i = 0; i < count; i++) {
		DownloadAdd(i);
		struct downloadInfo dl;
		if (DownloadGetSearchInfo(i, dl)) {
			LOG_L(L_DEBUG, "Download info: %s %d %d", dl.filename, dl.type, dl.cat);
		}
	}
	int result;
	LOG_L(L_DEBUG, "Download count: %d", count);
	// TODO: count will be 1 when archives already exist. We should instead return a different result with DownloadFailed/DownloadFinished?
	if (count == 1) { // there's nothing to download
		result = 2;
		QueueDownloadFailed(ID, result);
	} else {
		QueueDownloadStarted(ID);
		result = DownloadStart();
		LOG_L(L_DEBUG, "Download finished %s", filename.c_str());
	}
	DownloadShutdown();

	return result;
}

void StartDownload() {
	isDownloading = true;
	const DownloadItem& downloadItem = queue.front();
	queue.pop_front();
	const std::string filename = downloadItem.filename;
	category cat = downloadItem.cat;
	const int ID = downloadItem.ID;
	if (filename.c_str() != nullptr) {
		LOG_L(L_DEBUG, "DOWNLOADING: %s", filename.c_str());
	}
	std::thread {[ID, filename, cat]() {
			int result = Download(ID, filename, cat);
			if (result == 0) {
				QueueDownloadFinished(ID);
			} else {
				QueueDownloadFailed(ID, result);
			}

			// TODO: needs to lock the queue/isDownloading variables before checking
			if (!queue.empty()) {
				StartDownload();
			} else {
				isDownloading = false;
			}
		}
	}.detach();
}

/******************************************************************************/

int LuaVFSDownload::DownloadArchive(lua_State* L)
{
	const std::string filename = luaL_checkstring(L, 1);
	std::string categoryStr = luaL_optstring(L, 2, "any");
	if (filename.empty()) {
		luaL_error(L, "Missing download archive name.");
	}

	category cat;
	if (categoryStr == "map") {
		cat = CAT_MAP;
	} else if (categoryStr == "game") {
		cat = CAT_GAME;
	} else if (categoryStr == "engine") {
		cat = CAT_ENGINE;
	} else if (categoryStr == "any") {
		cat = CAT_ANY;
	} else {
		luaL_error(L, "If specified, category must be one of: map, game, engine, any.");
	}

	queueIDCount++;
	// TODO: needs to lock queue variable
	queue.push_back(DownloadItem(queueIDCount, filename, cat));
	eventHandler.DownloadQueued(queueIDCount, filename, categoryStr);
	if (!isDownloading) {
		if (queue.size() == 1) {
			StartDownload();
		}
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
	lua_pushsstring(L, encoded);
	return 1;
}

void LuaVFSDownload::ProcessDownloads()
{
	assert(Threading::IsMainThread() || Threading::IsGameLoadThread());
	// FIXME: These events might not be executed in the order they were received, which could cause for weird things to happen as Lua will probably expect no DownloadProgress events to be issued after DownloadFailed/DownloadFinished, and also no such events before DownloadStarted.
	if (dls != nullptr) {
		// TODO: lock here
		int ID = dls->ID;
		SafeDelete(dls);
		// TODO: unlock here
		eventHandler.DownloadStarted(ID);
	}
	if (dlfi != nullptr) {
		// TODO: lock here
		int ID = dlfi->ID;
		SafeDelete(dlfi);
		// TODO: unlock here
		eventHandler.DownloadFinished(ID);
	}
	if (dlfa != nullptr) {
		// TODO: lock here
		int ID = dlfa->ID;
		int errorID = dlfa->errorID;
		SafeDelete(dlfa);
		// TODO: unlock here
		eventHandler.DownloadFailed(ID, errorID);
	}
	if (dlp != nullptr) {
		// TODO: lock here
		int ID = dlp->ID;
		int downloaded = dlp->downloaded;
		int total = dlp->total;
		SafeDelete(dlp);
		// TODO: unlock here
		eventHandler.DownloadProgress(ID, downloaded, total);
	}
}

