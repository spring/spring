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



// global [idx = 0] and smaller per-thread [idx > 0] queues; the latter are
// for tasks that want to execute on specific threads, e.g. parallel_reduce
// note: std::shared_ptr<T> can not be made atomic, queues must store T*'s
#ifdef USE_BOOST_LOCKFREE_QUEUE
static std::array<boost::lockfree::queue<ITaskGroup*>, ThreadPool::MAX_THREADS> taskQueues;
#else
static std::array<moodycamel::ConcurrentQueue<ITaskGroup*>, ThreadPool::MAX_THREADS> taskQueues;
#endif

static std::deque<void*> workerThreads;
static std::array<bool, ThreadPool::MAX_THREADS> exitFlags;
static spring::signal newTasksSignal;

static _threadlocal int threadnum(0);


#if !defined(UNITSYNC) && !defined(UNIT_TEST)
static bool hasOGLthreads = false; // disable for now (not used atm)
#else
static bool hasOGLthreads = false;
#endif

std::atomic_uint ITaskGroup::lastId(0);



namespace ThreadPool {

int GetThreadNum() { return threadnum; }
static void SetThreadNum(const int idx) { threadnum = idx; }


// FIXME: mutex/atomic?
// NOTE: +1 cause we also count mainthread (workers start at 1)
int GetNumThreads() { return (workerThreads.size() + 1); }
int GetMaxThreads() { return std::min(MAX_THREADS, Threading::GetPhysicalCpuCores()); }

bool HasThreads() { return !workerThreads.empty(); }



static bool DoTask(int id)
{
	SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
	assert(id > 0);

	ITaskGroup* tg = nullptr;

	for (auto* queue: {&taskQueues[0], &taskQueues[id]}) {
		#ifdef USE_BOOST_LOCKFREE_QUEUE
		if (queue->pop(tg)) {
		#else
		if (queue->try_dequeue(tg)) {
		#endif
			// inform other workers when there is global work to do
			// waking is an expensive kernel-syscall, so better shift this
			// cost to the workers too (the main thread only wakes when ALL
			// workers are sleeping)
			if (queue == &taskQueues[0])
				NotifyWorkerThreads(true);

			while (tg->ExecuteTask());
		}

		#ifdef USE_BOOST_LOCKFREE_QUEUE
		while (queue->pop(tg)) {
		#else
		while (queue->try_dequeue(tg)) {
		#endif
			while (tg->ExecuteTask());
		}
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

	// make first worker spin a while before sleeping/waiting on the thread signal
	// this increases the chance that at least one worker is awake when a new TP-Task is inserted
	// and this worker can then take over the job of waking up the sleeping workers (see NotifyWorkerThreads)
	const auto ourSpinTime = spring_time::fromMilliSecs(1 * (id == 1));
	const auto maxSleepTime = spring_time::fromMilliSecs(30);

	while (!exitFlags[id]) {
		const auto spinlockEnd = spring_now() + ourSpinTime;
		      auto sleepTime   = spring_time::fromMicroSecs(1);

		while (!DoTask(id) && !exitFlags[id]) {
			if (spring_now() < spinlockEnd)
				continue;

			sleepTime = std::min(sleepTime * 1.25f, maxSleepTime);
			newTasksSignal.wait_for(sleepTime);
		}
	}
}


void WaitForFinished(std::shared_ptr<ITaskGroup>&& taskGroup)
{
	{
		SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
		while (taskGroup->ExecuteTask());
	}

	if (taskGroup->IsFinished())
		return;

	// task hasn't completed yet, use waiting time to execute other tasks
	NotifyWorkerThreads(true);

	const auto id = ThreadPool::GetThreadNum();

	do {
		const auto spinlockEnd = spring_now() + spring_time::fromMilliSecs(500);

		while (!DoTask(id) && !taskGroup->IsFinished() && !exitFlags[id]) {
			if (spring_now() < spinlockEnd)
				continue;

			//LOG_L(L_DEBUG, "Hang in ThreadPool");
			NotifyWorkerThreads(true);
			break;
		}
	} while (!taskGroup->IsFinished() && !exitFlags[id]);

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
	auto& queue = taskQueues[ taskGroup->WantedThread() ];

	#ifdef USE_BOOST_LOCKFREE_QUEUE
	while (!queue.push(taskGroup));
	#else
	while (!queue.enqueue(taskGroup));
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
	newTasksSignal.notify_all((ThreadPool::GetNumThreads() - 1) * (1 - force));
}






static void SpawnThreads(int wantedNumThreads, int curNumThreads)
{
#ifndef UNITSYNC
	if (hasOGLthreads) {
		try {
			for (int i = curNumThreads; i < wantedNumThreads; ++i) {
				exitFlags[i] = false;
				workerThreads.push_back(new COffscreenGLThread(std::bind(&WorkerLoop, i)));
			}
		} catch (const opengl_error& gle) {
			// shared gl context creation failed
			ThreadPool::SetThreadCount(0);

			hasOGLthreads = false;
			curNumThreads = ThreadPool::GetNumThreads();
		}
	} else
#endif
	{
		for (int i = curNumThreads; i < wantedNumThreads; ++i) {
			exitFlags[i] = false;
			workerThreads.push_back(new spring::thread(std::bind(&WorkerLoop, i)));
		}
	}
}

static void KillThreads(int wantedNumThreads, int curNumThreads)
{
	for (int i = curNumThreads - 1; i >= wantedNumThreads && i > 0; --i) {
		exitFlags[i] = true;
	}

	NotifyWorkerThreads(true);

	for (int i = curNumThreads - 1; i >= wantedNumThreads && i > 0; --i) {
		assert(!workerThreads.empty());
	#ifndef UNITSYNC
		if (hasOGLthreads) {
			auto th = reinterpret_cast<COffscreenGLThread*>(workerThreads.back());
			th->join();
			delete th;
		} else
	#endif
		{
			auto th = reinterpret_cast<spring::thread*>(workerThreads.back());
			th->join();
			delete th;
		}
		workerThreads.pop_back();
	}

	assert((wantedNumThreads != 0) || workerThreads.empty());
}


void SetThreadCount(int wantedNumThreads)
{
	const int curNumThreads = ThreadPool::GetNumThreads();
	const int wtdNumThreads = Clamp(wantedNumThreads, 1, ThreadPool::GetMaxThreads());

	LOG("[ThreadPool::%s][1] #wanted=%d #current=%d #max=%d", __FUNCTION__, wantedNumThreads, curNumThreads, ThreadPool::GetMaxThreads());

	#ifdef USE_BOOST_LOCKFREE_QUEUE
	taskQueues[0].reserve(1024);
	#endif

	if (curNumThreads < wtdNumThreads) {
		SpawnThreads(wtdNumThreads, curNumThreads);
	} else {
		KillThreads(wtdNumThreads, curNumThreads);
	}

	LOG("[ThreadPool::%s][2] #threads=%u", __FUNCTION__, (unsigned) workerThreads.size());
}

}

#endif

