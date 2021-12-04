/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/EventHandler.h"

#include "LuaVFSDownload.h"
#include "System/SafeUtil.h"
#include "System/EventHandler.h"
#include "System/Platform/Threading.h" // Is{Main,GameLoad}Thread
#include "System/Threading/SpringThreading.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/DataDirLocater.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

/******************************************************************************/
/******************************************************************************/

struct dlEvent {
	int id;
	virtual void processEvent() = 0;
	virtual ~dlEvent(){}
};

struct dlStarted : public dlEvent {
	void processEvent() {
		eventHandler.DownloadStarted(id);
	}
};
struct dlFinished : public dlEvent {
	void processEvent() {
		eventHandler.DownloadFinished(id);
	}
};
struct dlFailed : public dlEvent {
	int errorID;
	void processEvent() {
		eventHandler.DownloadFailed(id, errorID);
	}
};
struct dlProgress : public dlEvent {
	long downloaded;
	long total;
	void processEvent() {
		eventHandler.DownloadProgress(id, downloaded, total);
	}
};

struct DownloadItem {
	int id;
	std::string filename;
	DownloadEnum::Category cat;
	DownloadItem() { }
	DownloadItem(int id_, const std::string& filename, DownloadEnum::Category& cat) : id(id_), filename(filename), cat(cat) { }
};


struct DownloadQueue {
public:
	DownloadQueue(): thread(nullptr), breakLoop(false) {}
	~DownloadQueue() { Join(); }

	void Pump();
	void Push(const DownloadItem& downloadItem);
	bool Remove(int id);

	void Join();
private:
	std::deque<DownloadItem> queue;
	spring::mutex mutex;
	spring::thread* thread;
	bool breakLoop;
} downloadQueue;

std::deque<dlEvent*> dlEventQueue;
spring::mutex dlEventQueueMutex;

void AddQueueEvent(dlEvent* ev) {
	std::lock_guard<spring::mutex> lck(dlEventQueueMutex);
	dlEventQueue.push_back(ev);
}

void QueueDownloadStarted(int id) //queue from other thread download started event
{
	dlStarted* ev = new dlStarted();
	ev->id = id;
	AddQueueEvent(ev);
}

void QueueDownloadFinished(int id) //queue from other thread download started event
{
	dlFinished* ev = new dlFinished();
	ev->id = id;
	AddQueueEvent(ev);
}

void QueueDownloadFailed(int id, int errorID) //queue from other thread download started event
{
	dlFailed* ev = new dlFailed();
	ev->id = id;
	ev->errorID = errorID;
	AddQueueEvent(ev);
}

void QueueDownloadProgress(int id, long downloaded, long total) //queue from other thread download started event
{
	dlProgress* ev = new dlProgress();
	ev->id = id;
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


int Download(int id, const std::string& filename, DownloadEnum::Category cat)
{
	currentDownloadID = id;
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
		QueueDownloadFailed(id, result);
	} else {
		LOG_L(L_DEBUG, "Download finished %s", filename.c_str());
		QueueDownloadStarted(id);
		result = DownloadStart();
		// TODO: This works but there are errors spammed as it's trying to clear timers in the main thread, which this is not:
		// Error: [Watchdog::ClearTimer(id)] Invalid thread 4 (_threadId=(nil))
		archiveScanner->ScanAllDirs();
	}

	return result;
}



void DownloadQueue::Join()
{
	breakLoop = true;
	if (thread != nullptr) {
		thread->join();
		spring::SafeDelete(thread);
	}
	breakLoop = false;
}

__FORCE_ALIGN_STACK__
void DownloadQueue::Pump()
{
	while (!breakLoop) {
		DownloadItem downloadItem;
		{
			std::lock_guard<spring::mutex> lck(mutex);
			assert(queue.size() > 0);
			downloadItem = queue.front();
		}
		const std::string& filename = downloadItem.filename;
		DownloadEnum::Category cat = downloadItem.cat;
		int id = downloadItem.id;

		if (!filename.empty()) {
			LOG_L(L_DEBUG, "DOWNLOADING: %s", filename.c_str());
			const int result = Download(id, filename, cat);
			if (result == 0) {
				QueueDownloadFinished(id);
			} else {
				QueueDownloadFailed(id, result);
			}
		}

		{
			std::lock_guard<spring::mutex> lck(mutex);
			queue.pop_front();
			if(queue.empty())
				break;
		}
	}
}

void DownloadQueue::Push(const DownloadItem& downloadItem)
{
	std::unique_lock<spring::mutex> lck(mutex);

	if (queue.empty()) {
		if (thread != nullptr) {
			lck.unlock();

			thread->join();
			spring::SafeDelete(thread);

			lck.lock();
		}

		// mutex is still locked, thread will block if it gets
		// to queue.front() before we get to queue.push_back()
		thread = new spring::thread(std::bind(&DownloadQueue::Pump, this));
	}

	queue.push_back(downloadItem);
}

bool DownloadQueue::Remove(int id)
{
	std::unique_lock<spring::mutex> lck(mutex);

	for (auto it = queue.begin(); it != queue.end(); ++it) {
		if (it->id != id) {
			continue;
		}

		if (it == queue.begin()) {
			SetAbortDownloads(true);
			lck.unlock();
			Join();
			lck.lock();
			SetAbortDownloads(false);
			thread = new spring::thread(std::bind(&DownloadQueue::Pump, this));
		} else {
			queue.erase(it);
		}
		return true;
	}
	return false;
}



/******************************************************************************/

bool LuaVFSDownload::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(DownloadArchive);
	REGISTER_LUA_CFUNC(AbortDownload);
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
		SetAbortDownloads(true);
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

int LuaVFSDownload::AbortDownload(lua_State* L)
{
	const int id = luaL_checkint(L, 1);
	bool removed = downloadQueue.Remove(id);
	lua_pushboolean(L, removed);
	return 1;
}

void LuaVFSDownload::Update()
{
	assert(Threading::IsMainThread() || Threading::IsGameLoadThread());
	// only locks the mutex if the queue is not empty
	std::unique_lock<spring::mutex> lck(dlEventQueueMutex);
	while (!dlEventQueue.empty()) {
		dlEvent* ev = dlEventQueue.front();
		dlEventQueue.pop_front();

		{
			dlEventQueueMutex.unlock();
			ev->processEvent();
			spring::SafeDelete(ev);
			dlEventQueueMutex.lock();
		}
	}
}

