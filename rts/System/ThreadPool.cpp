/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef THREADPOOL

#include "ThreadPool.h"
#include "Exceptions.h"
#include "myMath.h"
#include "Log/ILog.h"
#include "Platform/Threading.h"
#include "Threading/SpringThreading.h"
#include "TimeProfiler.h"
#include "Util.h"

#if !defined(UNITSYNC) && !defined(UNIT_TEST)
	#include "OffscreenGLContext.h"
#endif

#ifdef likely
#undef   likely
#undef unlikely
#endif

#include <deque>
#include <utility>
#include <functional>

// not in mingwlibs
// #define USE_BOOST_LOCKFREE_QUEUE

#ifdef USE_BOOST_LOCKFREE_QUEUE
#include <boost/lockfree/queue.hpp>
#else
#include "System/ConcurrentQueue.h"
#endif



// std::shared_ptr<T> can not be made atomic, must store T*
#ifdef USE_BOOST_LOCKFREE_QUEUE
static boost::lockfree::queue<ITaskGroup*> taskQueue(1024);
#else
static moodycamel::ConcurrentQueue<ITaskGroup*> taskQueue;
#endif

static std::deque<void*> threadGroup; // worker threads
static spring::mutex newTasksMutex;
static spring::signal newTasks;
static std::array<bool, ThreadPool::MAX_THREADS> exitThread;

static _threadlocal int threadnum(0);


#if !defined(UNITSYNC) && !defined(UNIT_TEST)
static bool hasOGLthreads = false; // disable for now (not used atm)
#else
static bool hasOGLthreads = false;
#endif

std::atomic_uint ITaskGroup::lastId(0);



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
	return std::min(MAX_THREADS, Threading::GetPhysicalCpuCores());
}

int GetNumThreads()
{
	// FIXME: mutex/atomic?
	// NOTE: +1 cause we also count mainthread
	return (threadGroup.size() + 1);
}

bool HasThreads()
{
	return !threadGroup.empty();
}



static bool DoTask()
{
	SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");

	ITaskGroup* tg = nullptr;

	// inform other workers when there is work to do
	// waking is an expensive kernel-syscall, so better shift this
	// cost to the workers too (the main thread only wakes when ALL
	// workers are sleeping)
	#ifdef USE_BOOST_LOCKFREE_QUEUE
	if (taskQueue.pop(tg)) {
	#else
	if (taskQueue.try_dequeue(tg)) {
	#endif
		NotifyWorkerThreads(true);
		while (tg->ExecuteTask());
	}

	#ifdef USE_BOOST_LOCKFREE_QUEUE
	while (taskQueue.pop(tg)) {
	#else
	while (taskQueue.try_dequeue(tg)) {
	#endif
		while (tg->ExecuteTask());
	}

	// if true, queue contained at least one element
	return (tg != nullptr);
}


__FORCE_ALIGN_STACK__
static void WorkerLoop(int id)
{
	SetThreadNum(id);
#ifndef UNIT_TEST
	Threading::SetThreadName(IntToString(id, "worker%i"));
#endif
	static const auto maxSleepTime = spring_time::fromMilliSecs(30);

	// make first worker spin a while before sleeping/waiting on the thread signal
	// this increase the chance that at least one worker is awake when a new TP-Task is inserted
	// and this worker can then take over the job of waking up the sleeping workers (see NotifyWorkerThreads)
	const auto ourSpinTime = spring_time::fromMilliSecs((id == 1) ? 1 : 0);

	while (!exitThread[id]) {
		const auto spinlockEnd = spring_now() + ourSpinTime;
		      auto sleepTime   = spring_time::fromMicroSecs(1);

		while (!DoTask() && !exitThread[id]) {
			if (spring_now() < spinlockEnd)
				continue;

			sleepTime = std::min(sleepTime * 1.25f, maxSleepTime);
			newTasks.wait_for(sleepTime);
		}
	}
}


void WaitForFinished(std::shared_ptr<ITaskGroup>&& taskGroup)
{
	{
		SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
		while (taskGroup->ExecuteTask()) {
		}
	}


	if (!taskGroup->IsFinished()) {
		// task hasn't completed yet, use waiting time to execute other tasks
		NotifyWorkerThreads(true);
		const auto id = ThreadPool::GetThreadNum();

		do {
			const auto spinlockEnd = spring_now() + spring_time::fromMilliSecs(500);

			while (!DoTask() && !taskGroup->IsFinished() && !exitThread[id]) {
				if (spring_now() < spinlockEnd)
					continue;

				//LOG_L(L_DEBUG, "Hang in ThreadPool");
				NotifyWorkerThreads(true);
				break;
			}
		} while (!taskGroup->IsFinished() && !exitThread[id]);
	}

	//LOG("WaitForFinished %i", taskGroup->GetExceptions().size());
	//for (auto& ep: taskGroup->GetExceptions())
	//	std::rethrow_exception(ep);
}


// WARNING:
//   leaking the raw pointer *forces* caller to WaitForFinished
//   otherwise task might get deleted while its pointer is still
//   in the queue
void PushTaskGroup(std::shared_ptr<ITaskGroup>&& taskGroup) { PushTaskGroup(taskGroup.get()); }
void PushTaskGroup(ITaskGroup* taskGroup)
{
	#ifdef USE_BOOST_LOCKFREE_QUEUE
	while (!taskQueue.push(taskGroup)) {}
	#else
	while (!taskQueue.enqueue(taskGroup)) {}
	#endif

	NotifyWorkerThreads();
}


void NotifyWorkerThreads(const bool force)
{
	// OPTIMIZATION
	// if !force only wake up threads when _all_ are sleeping,
	// this is an optimization cause wakeing up other threads
	// is a kernel-syscall that costs a lot of time, we prefer
	// to not do so on the thread that added the ThreadPool-Task
	// and instead let the worker threads themselves inform each other.
	newTasks.notify_all((ThreadPool::GetNumThreads() - 1) * (1 - force));
}









static void SpawnThreads(int num, int curThreads)
{
#ifndef UNITSYNC
	if (hasOGLthreads) {
		try {
			for (int i = curThreads; i < num; ++i) {
				exitThread[i] = false;
				threadGroup.push_back(new COffscreenGLThread(std::bind(&WorkerLoop, i)));
			}
		} catch (const opengl_error& gle) {
			// shared gl context creation failed
			ThreadPool::SetThreadCount(0);

			hasOGLthreads = false;
			curThreads = ThreadPool::GetNumThreads();
		}
	} else
#endif
	{
		for (int i = curThreads; i<num; ++i) {
			exitThread[i] = false;
			threadGroup.push_back(new spring::thread(std::bind(&WorkerLoop, i)));
		}
	}
}


static void KillThreads(int num, int curThreads)
{
	for (int i = curThreads - 1; i >= num && i > 0; --i) {
		exitThread[i] = true;
	}

	NotifyWorkerThreads(true);

	for (int i = curThreads - 1; i >= num && i > 0; --i) {
		assert(!threadGroup.empty());
	#ifndef UNITSYNC
		if (hasOGLthreads) {
			auto th = reinterpret_cast<COffscreenGLThread*>(threadGroup.back());
			th->join();
			delete th;
		} else
	#endif
		{
			auto th = reinterpret_cast<spring::thread*>(threadGroup.back());
			th->join();
			delete th;
		}
		threadGroup.pop_back();
	}

	assert((num != 0) || threadGroup.empty());
}


void SetThreadCount(int numWantedThreads)
{
	const int curThreads = ThreadPool::GetNumThreads();

	LOG("[ThreadPool::%s][1] #wanted=%d #current=%d #max=%d", __FUNCTION__, numWantedThreads, curThreads, ThreadPool::GetMaxThreads());
	numWantedThreads = Clamp(numWantedThreads, 1, ThreadPool::GetMaxThreads());

	if (curThreads < numWantedThreads) {
		SpawnThreads(numWantedThreads, curThreads);
	} else {
		KillThreads(numWantedThreads, curThreads);
	}

	LOG("[ThreadPool::%s][2] #threads=%u", __FUNCTION__, (unsigned) threadGroup.size());
}

}

#endif

