#ifdef THREADPOOL

#include "ThreadPool.h"
#include "Exceptions.h"
#include "Platform/Threading.h"
#include "TimeProfiler.h"
#include "Util.h"
#if !defined(UNITSYNC) && !defined(UNIT_TEST)
	#include "OffscreenGLContext.h"
#endif

#include <deque>
#include <vector>
#include <utility>
#include <functional>

static std::deque<std::shared_ptr<ITaskGroup>> taskGroups;
static std::deque<void*> thread_group;

static spring::mutex taskMutex;
static spring::condition_variable_any newTasks;
static std::atomic<bool> waitForLock(false);

#if !defined(UNITSYNC) && !defined(UNIT_TEST)
static bool hasOGLthreads = false; // disable for now (not used atm)
#else
static bool hasOGLthreads = false;
#endif
#if defined(_MSC_VER)
static __declspec(thread) int threadnum(0);
static __declspec(thread) bool exitThread(false);
#else
static __thread int threadnum(0);
static __thread bool exitThread(false);
#endif


static int spinlockMs = 5;


namespace ThreadPool {

int GetThreadNum()
{
	//return omp_get_thread_num();
	return threadnum;
}

static void SetThreadNum(const int idx)
{
	threadnum = idx;
}


int GetMaxThreads()
{
#ifndef UNIT_TEST
	return std::min(MAX_THREADS, Threading::GetPhysicalCpuCores());
#else
	return 10;
#endif
}

int GetNumThreads()
{
	// FIXME: mutex/atomic?
	// NOTE: +1 cause we also count mainthread
	return (thread_group.size() + 1);
}

bool HasThreads()
{
	return !thread_group.empty();
}


/// returns false, when no further tasks were found
static bool DoTask(std::unique_lock<spring::mutex>& lk_)
{
	if (waitForLock.load(std::memory_order_acquire))
		return true;

	auto& lk = lk_;

	if (!taskGroups.empty()) {
		if (lk.try_lock()) {
			bool foundEmpty = false;

			for (auto tg: taskGroups) {
				if (tg->IsEmpty()) {
					foundEmpty = true;
					break;
				} else {
					lk.unlock();
					auto p = tg->GetTask();
					do {
						if (p) {
							SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
							(*p)();
						}
					} while (bool(p = tg->GetTask()));
					break;
				}
			}
			if (lk.owns_lock()) lk.unlock();

			if (foundEmpty) {
				//FIXME this could be made lock-free too, but is it worth it?
				waitForLock.store(true, std::memory_order_release);
				std::unique_lock<spring::mutex> ulk(taskMutex, std::defer_lock);
				while (!ulk.try_lock()) {}
				for(auto it = taskGroups.begin(); it != taskGroups.end();) {
					if ((*it)->IsEmpty()) {
						it = taskGroups.erase(it);
					} else {
						++it;
					}
				}
				waitForLock.store(false, std::memory_order_release);
				ulk.unlock();
			}

			return false;
		}

		// we didn't got a lock, so we couldn't see if there are further tasks
		return true;
	}

	return false;
}


static bool DoTask(std::shared_ptr<ITaskGroup> tg)
{
	auto p = tg->GetTask();
	if (p) {
		SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
		(*p)();
	}
	return static_cast<bool>(p);
}


__FORCE_ALIGN_STACK__
static void WorkerLoop(int id)
{
	SetThreadNum(id);
#ifndef UNIT_TEST
	Threading::SetThreadName(IntToString(id, "worker%i"));
#endif
	std::unique_lock<spring::mutex> lk(taskMutex, std::defer_lock);
	spring::mutex m;
	std::unique_lock<spring::mutex> lk2(m);

	while (!exitThread) {
		const auto spinlockStart = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(spinlockMs);

		while (!DoTask(lk) && !exitThread) {
			if (spinlockStart < std::chrono::high_resolution_clock::now()) {
				newTasks.wait_for(lk2, std::chrono::nanoseconds(100));
			}
		}
	}
}


void WaitForFinished(std::shared_ptr<ITaskGroup> taskgroup)
{
	while (DoTask(taskgroup)) {
	}

	while (!taskgroup->wait_for(std::chrono::seconds(5))) {
		LOG_L(L_WARNING, "Hang in ThreadPool");
	}

	//LOG("WaitForFinished %i", taskgroup->GetExceptions().size());
	//for (auto& r: taskgroup->results())
	//	r.get();
}


void PushTaskGroup(std::shared_ptr<ITaskGroup> taskgroup)
{
	waitForLock.store(true, std::memory_order_release);
	std::unique_lock<spring::mutex> lk(taskMutex, std::defer_lock);
	while (!lk.try_lock()) {}
	taskGroups.emplace_back(taskgroup);
	waitForLock.store(false, std::memory_order_release);
	lk.unlock();
	newTasks.notify_all();
}


void NotifyWorkerThreads()
{
	newTasks.notify_all();
}


void SetThreadCount(int num)
{
	int curThreads = ThreadPool::GetNumThreads();
	LOG("[ThreadPool::%s][1] #wanted=%d #current=%d #max=%d", __FUNCTION__, num, curThreads, ThreadPool::GetMaxThreads());
	num = std::min(num, ThreadPool::GetMaxThreads());

	if (curThreads < num) {
#ifndef UNITSYNC
		if (hasOGLthreads) {
			try {
				for (int i = curThreads; i < num; ++i) {
					thread_group.push_back(new COffscreenGLThread(std::bind(&WorkerLoop, i)));
				}
			} catch (const opengl_error& gle) {
				// shared gl context creation failed :<
				ThreadPool::SetThreadCount(0);

				hasOGLthreads = false;
				curThreads = ThreadPool::GetNumThreads();
			}
		}
#endif
		if (!hasOGLthreads) {
			for (int i = curThreads; i<num; ++i) {
				thread_group.push_back(new spring::thread(std::bind(&WorkerLoop, i)));
			}
		}
	} else {
		for (int i = curThreads; i > num && i > 1; --i) {
			assert(!thread_group.empty());

			auto taskgroup = std::make_shared<ParallelTaskGroup<const std::function<void()>>>();
			taskgroup->enqueue_unique(GetNumThreads() - 1, []{ exitThread = true; });
			ThreadPool::PushTaskGroup(taskgroup);
#ifndef UNITSYNC
			if (hasOGLthreads) {
				auto th = reinterpret_cast<COffscreenGLThread*>(thread_group.back());
				th->join();
				delete th;
			} else
#endif
			{
				auto th = reinterpret_cast<spring::thread*>(thread_group.back());
				th->join();
				delete th;
			}
			thread_group.pop_back();
		}

		if (num == 0)
			assert(thread_group.empty());
	}

	LOG("[ThreadPool::%s][2] #threads=%u", __FUNCTION__, (unsigned) thread_group.size());
}

void SetThreadSpinTime(int milliSeconds)
{
	spinlockMs = milliSeconds;
}

}

#endif

