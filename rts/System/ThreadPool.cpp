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
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

static std::deque<std::shared_ptr<ITaskGroup>> taskGroups;
static std::deque<void*> thread_group;

static boost::shared_mutex taskMutex;
static boost::condition_variable newTasks;
static std::atomic<bool> waitForLock(false);

#if !defined(UNITSYNC) && !defined(UNIT_TEST)
static bool hasOGLthreads = false; // disable for now (not used atm)
#else
static bool hasOGLthreads = false;
#endif

#if defined(WIN32)
static __declspec(thread) int threadnum(0);
static __declspec(thread) bool exitThread(false);
#else
static __thread int threadnum(0); 
static __thread bool exitThread(false);
#endif

static int spinlockMs = 5;

static struct do_once {
	do_once() {
		//ThreadPool::SetThreadCount(ThreadPool::GetMaxThreads());
	}
	~do_once() {
		ThreadPool::SetThreadCount(0);
	}
} doOnce;


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
	return Threading::GetAvailableCores();
}


int GetNumThreads()
{
	//FIXME mutex/atomic?
	return thread_group.size() + 1; // +1 cause we also count mainthread
}


bool HasThreads()
{
	return !thread_group.empty() + 1;
}


/// returns false, when no further tasks were found
static bool DoTask(boost::shared_lock<boost::shared_mutex>& lk_)
{
	if (waitForLock)
		return true;

#ifndef __MINGW32__
	auto& lk = lk_;
#else
	boost::unique_lock<boost::shared_mutex> lk(taskMutex, boost::defer_lock);
#endif

	if (!taskGroups.empty()) {
		if (lk.try_lock()) {
			bool foundEmpty = false;

			for(auto tg: taskGroups) {
				auto p = tg->GetTask();

				if (p) {
					lk.unlock();
					SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
					(*p)();
					return true;
				}

				if (tg->IsEmpty()) {
					foundEmpty = true;
				}
			}
			lk.unlock();

			if (foundEmpty) {
				//FIXME this could be made lock-free too, but is it worth it?
				waitForLock = true;
				boost::unique_lock<boost::shared_mutex> ulk(taskMutex, boost::defer_lock);
				while (!ulk.try_lock()) {}
				for(auto it = taskGroups.begin(); it != taskGroups.end();) {
					if ((*it)->IsEmpty()) {
						it = taskGroups.erase(it);
					} else {
						++it;
					}
				}
				waitForLock = false;
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
	const bool f = p;
	if (f) {
		SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
		(*p)();
	}
	return f;
}


__FORCE_ALIGN_STACK__
static void WorkerLoop(int id)
{
	SetThreadNum(id);
	Threading::SetThreadName(IntToString(id, "worker%i"));
	boost::shared_lock<boost::shared_mutex> lk(taskMutex, boost::defer_lock);
	boost::mutex m;
	boost::unique_lock<boost::mutex> lk2(m);

	while (!exitThread) {
		const auto spinlockStart = boost::chrono::high_resolution_clock::now() + boost::chrono::milliseconds(spinlockMs);

		while (!DoTask(lk) && !exitThread) {
			if (spinlockStart < boost::chrono::high_resolution_clock::now()) {
			#ifndef BOOST_THREAD_USES_CHRONO
				const boost::system_time timeout = boost::get_system_time() + boost::posix_time::microseconds(1);
				newTasks.timed_wait(lk2, timeout);
			#else
				newTasks.wait_for(lk2, boost::chrono::nanoseconds(100));
			#endif
			}
		}
	}
}


void WaitForFinished(std::shared_ptr<ITaskGroup> taskgroup)
{
	while (DoTask(taskgroup)) {
	}

	while (!taskgroup->wait_for(boost::chrono::seconds(5))) {
		LOG_L(L_WARNING, "Hang in ThreadPool");
	}

	//LOG("WaitForFinished %i", taskgroup->GetExceptions().size());
	//for (auto& r: taskgroup->results())
	//	r.get();
}


void PushTaskGroup(std::shared_ptr<ITaskGroup> taskgroup)
{
	waitForLock = true;
	boost::unique_lock<boost::shared_mutex> lk(taskMutex, boost::defer_lock);
	while (!lk.try_lock()) {}
	taskGroups.emplace_back(taskgroup);
	waitForLock = false;
	lk.unlock();
	newTasks.notify_all();
}


void NotifyWorkerThreads()
{
	newTasks.notify_all();
}


void SetThreadCount(int num)
{
	int curThreads = GetNumThreads();

	if (curThreads < num) {
#ifndef UNITSYNC
		if (hasOGLthreads) {
			try {
				for (int i = curThreads; i<num; ++i) {
					thread_group.push_back(new COffscreenGLThread(boost::bind(&WorkerLoop, i)));
				}
			} catch (const opengl_error& gle) {
				// shared gl context creation failed :<
				SetThreadCount(0);
				hasOGLthreads = false;
				curThreads = GetNumThreads();
			}
		}
#endif
		if (!hasOGLthreads) {
			for (int i = curThreads; i<num; ++i) {
				thread_group.push_back(new boost::thread(boost::bind(&WorkerLoop, i)));
			}
		}
	} else {
		for (int i = curThreads; i>num && i>1; --i) {
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
				auto th = reinterpret_cast<boost::thread*>(thread_group.back());
				th->join();
				delete th;
			}
			thread_group.pop_back();
		}
		if (num == 0) assert(thread_group.empty());
	}
}

void SetThreadSpinTime(int milliSeconds)
{
	spinlockMs = milliSeconds;
}

};

#endif

