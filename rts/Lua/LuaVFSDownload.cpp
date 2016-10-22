/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/EventHandler.h"

#include "LuaVFSDownload.h"
#include "System/Util.h"
#include "System/EventHandler.h"
#include "System/Platform/Threading.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/DataDirLocater.h"

#include "LuaInclude.h"

/******************************************************************************/
/******************************************************************************/
#define REGISTER_LUA_CFUNC(x) \
        lua_pushstring(L, #x);      \
        lua_pushcfunction(L, x);    \
        lua_rawset(L, -3)

struct dlEvent {
	int ID;
	virtual void processEvent() = 0;
	virtual ~dlEvent(){}
};

struct dlStarted : public dlEvent {
	void processEvent() {
		eventHandler.DownloadStarted(ID);
	}
};
struct dlFinished : public dlEvent {
	void processEvent() {
		eventHandler.DownloadFinished(ID);
	}
};
struct dlFailed : public dlEvent {
	int errorID;
	void processEvent() {
		eventHandler.DownloadFailed(ID, errorID);
	}
};
struct dlProgress : public dlEvent {
	long downloaded;
	long total;
	void processEvent() {
		eventHandler.DownloadProgress(ID, downloaded, total);
	}
};

struct DownloadItem {
	int ID;
	std::string filename;
	DownloadEnum::Category cat;
	DownloadItem() { }
	DownloadItem(int ID, const std::string& filename, DownloadEnum::Category& cat) : ID(ID), filename(filename), cat(cat) { }
};


struct DownloadQueue {
public:
	DownloadQueue(): thread(nullptr), shutdown(false) {}
	~DownloadQueue() { Join(); }

	void Pump();
	void Push(const DownloadItem& downloadItem);

	void GrabLock() { mutex.lock(); }
	void FreeLock() { mutex.unlock(); }

	void Join();
private:
	std::deque<DownloadItem> queue;
	spring::mutex mutex;
	spring::thread* thread;
	bool shutdown;
} downloadQueue;

std::deque<dlEvent*> dlEventQueue;
spring::mutex dlEventQueueMutex;

void AddQueueEvent(dlEvent* ev) {
	dlEventQueueMutex.lock();
	dlEventQueue.push_back(ev);
	dlEventQueueMutex.unlock();
}

void QueueDownloadStarted(int ID) //queue from other thread download started event
{
	dlStarted* ev = new dlStarted();
	ev->ID = ID;
	AddQueueEvent(ev);
}

void QueueDownloadFinished(int ID) //queue from other thread download started event
{
	dlFinished* ev = new dlFinished();
	ev->ID = ID;
	AddQueueEvent(ev);
}

void QueueDownloadFailed(int ID, int errorID) //queue from other thread download started event
{
	dlFailed* ev = new dlFailed();
	ev->ID = ID;
	ev->errorID = errorID;
	AddQueueEvent(ev);
}

void QueueDownloadProgress(int ID, long downloaded, long total) //queue from other thread download started event
{
	dlProgress* ev = new dlProgress();
	ev->ID = ID;
	ev->downloaded = downloaded;
	ev->total = total;
	AddQueueEvent(ev);
}


static int queueIDCount = -1;
static int currentDownloadID = -1;

void StartDownload();

void UpdateProgress(int done, int size) {
	QueueDownloadProgress(currentDownloadID, done, size);
}


int Download(int ID, const std::string& filename, DownloadEnum::Category cat)
{
	currentDownloadID = ID;
	// FIXME: Progress is incorrectly updated when rapid is queried to check for existing packages.
	SetDownloadListener(UpdateProgress);
	LOG_L(L_DEBUG, "Going to download %s", filename.c_str());
	const int count = DownloadSearch(cat, filename.c_str());
	for (int i = 0; i < count; i++) {
		DownloadAdd(i);
		struct downloadInfo dl;
		if (DownloadGetInfo(i, dl)) {
			LOG_L(L_DEBUG, "Download info: %s %d", dl.filename, dl.cat);
		}
	}
	int result;
	LOG_L(L_DEBUG, "Download count: %d", count);
	// TODO: count will be 1 when archives already exist. We should instead return a different result with DownloadFailed/DownloadFinished?
	if (count == 0) { // there's nothing to download
		result = 2;
		QueueDownloadFailed(ID, result);
	} else {
		LOG_L(L_DEBUG, "Download finished %s", filename.c_str());
		QueueDownloadStarted(ID);
		result = DownloadStart();
		// TODO: This works but there are errors spammed as it's trying to clear timers in the main thread, which this is not:
		// Error: [Watchdog::ClearTimer(id)] Invalid thread 4 (_threadId=(nil))
		archiveScanner->ScanAllDirs();
	}

	return result;
}



void DownloadQueue::Join()
{
	shutdown = true;
	if (thread != nullptr) {
		thread->join();
		SafeDelete(thread);
	}
}

__FORCE_ALIGN_STACK__
void DownloadQueue::Pump()
{
	while (!shutdown) {
		DownloadItem downloadItem;

		{
			GrabLock();
			assert(queue.size() > 0);
			downloadItem = queue.front();
			FreeLock();
		}
		const std::string& filename = downloadItem.filename;
		DownloadEnum::Category cat = downloadItem.cat;
		int ID = downloadItem.ID;

		if (!filename.empty()) {
			LOG_L(L_DEBUG, "DOWNLOADING: %s", filename.c_str());
			const int result = Download(ID, filename, cat);
			if (result == 0) {
				QueueDownloadFinished(ID);
			} else {
				QueueDownloadFailed(ID, result);
			}
		}

		{
			GrabLock();
			queue.pop_front();
			const bool empty = queue.empty();
			FreeLock();
			if (empty)
				break;
		}
	}
}

void DownloadQueue::Push(const DownloadItem& downloadItem)
{
	GrabLock();

	if (queue.empty()) {
		if (thread != nullptr) {
			FreeLock();

			thread->join();
			SafeDelete(thread);

			GrabLock();
		}

		// mutex is still locked, thread will block if it gets
		// to queue.front() before we get to queue.push_back()
		thread = new spring::thread(std::bind(&DownloadQueue::Pump, this));
	}

	queue.push_back(downloadItem);

	FreeLock();
}



/******************************************************************************/

bool LuaVFSDownload::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(DownloadArchive);
	return true;
}


LuaVFSDownload::LuaVFSDownload():
	CEventClient("[LuaVFSDownload]", 314161, false)
{
	DownloadInit();
	DownloadSetConfig(CONFIG_FILESYSTEM_WRITEPATH, dataDirLocater.GetWriteDirPath().c_str());
}

LuaVFSDownload::~LuaVFSDownload()
{
	DownloadShutdown();
}

void LuaVFSDownload::Init()
{
	eventHandler.AddClient(luaVFSDownload);
}

void LuaVFSDownload::Free(bool stopDownloads)
{
	eventHandler.RemoveClient(luaVFSDownload);
	if (stopDownloads) {
		SetAbortDownloads();
		downloadQueue.Join();
	}
}


LuaVFSDownload* LuaVFSDownload::GetInstance()
{
	static LuaVFSDownload instance;
	return &instance;
}

int LuaVFSDownload::DownloadArchive(lua_State* L)
{
	const std::string filename = luaL_checkstring(L, 1);
	const std::string categoryStr = luaL_checkstring(L, 2);
	if (filename.empty()) {
		return luaL_error(L, "Missing download archive name.");
	}

	DownloadEnum::Category cat;
	if (categoryStr == "map") {
		cat = DownloadEnum::CAT_MAP;
	} else if (categoryStr == "game") {
		cat = DownloadEnum::CAT_GAME;
	} else if (categoryStr == "engine") {
		cat = DownloadEnum::CAT_ENGINE;
	} else {
		return luaL_error(L, "Category must be one of: map, game, engine.");
	}

	queueIDCount++;
	downloadQueue.Push(DownloadItem(queueIDCount, filename, cat));
	eventHandler.DownloadQueued(queueIDCount, filename, categoryStr);
	return 0;
}

void LuaVFSDownload::Update()
{
	assert(Threading::IsMainThread() || Threading::IsGameLoadThread());
	// only locks the mutex if the queue is not empty
	if (!dlEventQueue.empty()) {
		dlEventQueueMutex.lock();
		dlEvent* ev;
		while (!dlEventQueue.empty()) {
			ev = dlEventQueue.front();
			dlEventQueue.pop_front();
			ev->processEvent();
			SafeDelete(ev);
		}
		dlEventQueueMutex.unlock();
	}
}

