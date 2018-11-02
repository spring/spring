/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaVFSDownload.h"
#include "System/SafeUtil.h"
#include "System/EventHandler.h"
#include "System/Platform/Threading.h" // Is{Main,GameLoad}Thread
#include "System/Threading/SpringThreading.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/DataDirLocater.h"
#include "../tools/pr-downloader/src/pr-downloader.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include <deque>
#include <memory>


struct DLEvent {
	DLEvent(int _id): id(_id) {}
	virtual ~DLEvent() = default;
	virtual void Process() const = 0;

	int id;
};

struct DLStartedEvent: public DLEvent {
	DLStartedEvent(int _id): DLEvent(_id) {}
	void Process() const override {
		// handled by DLFinishedEvent
		// archiveScanner->ScanAllDirs();
		eventHandler.DownloadStarted(id);
	}
};

struct DLFinishedEvent: public DLEvent {
	DLFinishedEvent(int _id): DLEvent(_id) {}
	void Process() const override {
		// rescan before notifying clients; typically blocks less than ~50ms
		archiveScanner->ScanAllDirs();
		eventHandler.DownloadFinished(id);
	}
};

struct DLFailedEvent: public DLEvent {
	DLFailedEvent(int _id, int _errorID): DLEvent(_id), errorID(_errorID) {}
	void Process() const override { eventHandler.DownloadFailed(id, errorID); }

	int errorID;
};

struct DLProgressEvent: public DLEvent {
	DLProgressEvent(int _id, long _downloaded, long _total): DLEvent(_id), downloaded(_downloaded), total(_total) {}
	void Process() const override { eventHandler.DownloadProgress(id, downloaded, total); }

	long downloaded;
	long total;
};



struct DownloadItem {
	int id;
	std::string filename;
	DownloadEnum::Category cat;

	DownloadItem() = default;
	DownloadItem(int id_, const std::string& filename_, DownloadEnum::Category& cat_) : id(id_), filename(filename_), cat(cat_) {}
};


struct DownloadQueue {
public:
	DownloadQueue() = default;
	~DownloadQueue() { Join(); }

	void Pump();
	void Push(const DownloadItem& downloadItem);
	void Join();

	bool Remove(int id);

private:
	std::deque<DownloadItem> queue;
	spring::mutex mutex;
	spring::thread thread;

	bool breakLoop = false;
};


static DownloadQueue downloadQueue;

static std::deque< std::shared_ptr<DLEvent> > dlEventQueue;
static spring::mutex dlEventQueueMutex;

static int queueIDCount = -1;
static int currentDownloadID = -1;



static void AddQueueEvent(std::shared_ptr<DLEvent> ev) {
	std::lock_guard<spring::mutex> lck(dlEventQueueMutex);
	dlEventQueue.push_back(ev);
}

static void QueueDownloadStarted(int id) { AddQueueEvent(std::make_shared<DLStartedEvent>(id)); }
static void QueueDownloadFinished(int id) { AddQueueEvent(std::make_shared<DLFinishedEvent>(id)); }
static void QueueDownloadFailed(int id, int errorID) { AddQueueEvent(std::make_shared<DLFailedEvent>(id, errorID)); }
static void QueueDownloadProgress(int id, long downloaded, long total) { AddQueueEvent(std::make_shared<DLProgressEvent>(id, downloaded, total)); }

static void UpdateProgress(int done, int size) { QueueDownloadProgress(currentDownloadID, done, size); }



static int StartDownloadJob(int id, const std::string& filename, DownloadEnum::Category cat)
{
	currentDownloadID = id;

	// FIXME: progress is incorrectly updated when rapid is queried to check for existing packages
	SetDownloadListener(UpdateProgress);

	LOG_L(L_DEBUG, "[%s] downloading \"%s\"", __func__, filename.c_str());

	const int count = DownloadSearch(cat, filename.c_str());

	LOG_L(L_DEBUG, "[%s] download count: %d", __func__, count);

	for (int i = 0; i < count; i++) {
		DownloadAdd(i);

		struct downloadInfo dl;
		if (!DownloadGetInfo(i, dl))
			continue;

		LOG_L(L_DEBUG, "\tinfo=%d name=%s cat=%d", i, dl.filename, dl.cat);
	}

	// nothing to download
	// TODO:
	//   count will be 1 when an archive already exists; should
	//   instead return a different result with DownloadFailed
	//   or DownloadFinished?
	if (count == 0) {
		QueueDownloadFailed(id, 2);
		return 2;
	}

	QueueDownloadStarted(id);

	// FIXME:
	//   many functions in ArchiveScanner do not lock, and are called at
	//   various stages during loading (e.g. ArchiveFromName in PreGame)
	//   a call to VFS.DownloadArchive (say from LuaMenu just prior to a
	//   Spring.Reload) pushes an item into the queue which might at any
	//   point be consumed by the dl-pump thread running StartDownloadJob
	//   so this is problematic
	//   does not even make sense to rescan until download is *finished*
	// archiveScanner->ScanAllDirs();
	return (DownloadStart());
}



void DownloadQueue::Join()
{
	breakLoop = true;

	if (thread.joinable())
		thread.join();

	breakLoop = false;
}

__FORCE_ALIGN_STACK__
void DownloadQueue::Pump()
{
	while (!breakLoop) {
		DownloadItem downloadItem;
		{
			std::lock_guard<spring::mutex> lck(mutex);
			assert(!queue.empty());
			downloadItem = queue.front();
		}

		const std::string& filename = downloadItem.filename;

		if (!filename.empty()) {
			const int result = StartDownloadJob(downloadItem.id, filename, downloadItem.cat);

			if (result == 0) {
				QueueDownloadFinished(downloadItem.id);
			} else {
				QueueDownloadFailed(downloadItem.id, result);
			}
		}

		{
			std::lock_guard<spring::mutex> lck(mutex);
			queue.pop_front();

			if (queue.empty())
				break;
		}
	}
}

void DownloadQueue::Push(const DownloadItem& downloadItem)
{
	std::unique_lock<spring::mutex> lck(mutex);

	if (queue.empty()) {
		// keep only one concurrent download-thread
		if (thread.joinable()) {
			lck.unlock();
			thread.join();
			lck.lock();
		}

		// mutex is still locked, thread will block if it gets
		// to queue.front() before we get to queue.push_back()
		thread = std::move(spring::thread(&DownloadQueue::Pump, this));
	}

	queue.push_back(downloadItem);
}

bool DownloadQueue::Remove(int id)
{
	std::unique_lock<spring::mutex> lck(mutex);

	for (auto it = queue.begin(); it != queue.end(); ++it) {
		if (it->id != id)
			continue;

		if (it == queue.begin()) {
			SetAbortDownloads(true);
			lck.unlock();
			Join();
			lck.lock();
			SetAbortDownloads(false);

			thread = std::move(spring::thread(&DownloadQueue::Pump, this));
		} else {
			queue.erase(it);
		}

		return true;
	}

	return false;
}






LuaVFSDownload::LuaVFSDownload(): CEventClient("[LuaVFSDownload]", 314161, false)
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


void LuaVFSDownload::Update()
{
	assert(Threading::IsMainThread() || Threading::IsGameLoadThread());
	// only locks the mutex if the queue is not empty
	std::unique_lock<spring::mutex> lck(dlEventQueueMutex);

	while (!dlEventQueue.empty()) {
		std::shared_ptr<DLEvent> ev = dlEventQueue.front();
		dlEventQueue.pop_front();

		{
			dlEventQueueMutex.unlock();
			ev->Process();
			dlEventQueueMutex.lock();
		}
	}
}



bool LuaVFSDownload::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(DownloadArchive);
	REGISTER_LUA_CFUNC(AbortDownload);
	REGISTER_LUA_CFUNC(ScanAllDirs);
	return true;
}

int LuaVFSDownload::DownloadArchive(lua_State* L)
{
	const std::string& filename = luaL_checkstring(L, 1);
	const std::string& categoryStr = luaL_checkstring(L, 2);

	if (filename.empty())
		return luaL_error(L, "Missing download archive name.");

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
	lua_pushboolean(L, downloadQueue.Remove(luaL_checkint(L, 1)));
	return 1;
}

int LuaVFSDownload::ScanAllDirs(lua_State* L)
{
	archiveScanner->ScanAllDirs();
	return 0;
}
