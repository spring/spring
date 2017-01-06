/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef THREADPOOL

#include "ThreadPool.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#if !defined(UNITSYNC) && !defined(UNIT_TEST)
	#include "System/OffscreenGLContext.h"
#endif
#include "System/TimeProfiler.h"
#include "System/Util.h"
#ifndef UNIT_TEST
	#include "System/Config/ConfigHandler.h"
#endif
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/Threading/SpringThreading.h"

#ifdef   likely
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



#ifndef UNIT_TEST
CONFIG(int, WorkerThreadCount).defaultValue(-1).safemodeValue(0).minimumValue(-1).description("Count of worker threads (including mainthread!) used in parallel sections.");
#endif



struct ThreadStats {
	uint64_t numTasks;
	uint64_t sumTime;
	uint64_t minTime;
	uint64_t maxTime;
};



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
static std::array<ThreadStats, ThreadPool::MAX_THREADS> threadStats;
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



static bool DoTask(int tid)
{
	SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");

	ITaskGroup* tg = nullptr;

	// any external thread calling WaitForFinished will have
	// id=0 and *only* processes tasks from the global queue
	for (int idx = 0; idx <= tid; idx += std::max(tid, 1)) {
		auto& queue = taskQueues[idx];

		#ifdef USE_BOOST_LOCKFREE_QUEUE
		if (queue.pop(tg)) {
		#else
		if (queue.try_dequeue(tg)) {
		#endif
			const spring_time t0 = spring_now();

			// inform other workers when there is global work to do
			// waking is an expensive kernel-syscall, so better shift this
			// cost to the workers too (the main thread only wakes when ALL
			// workers are sleeping)
			if (idx == 0)
				NotifyWorkerThreads(true);

			while (tg->ExecuteTask());

			const spring_time t1 = spring_now();
			const spring_time dt = t1 - t0;

			threadStats[tid].numTasks += 1;
			threadStats[tid].sumTime  += dt.toNanoSecsi();
			threadStats[tid].minTime   = std::min(threadStats[tid].minTime, uint64_t(dt.toNanoSecsi()));
			threadStats[tid].maxTime   = std::max(threadStats[tid].maxTime, uint64_t(dt.toNanoSecsi()));
		}

		#ifdef USE_BOOST_LOCKFREE_QUEUE
		while (queue.pop(tg)) {
		#else
		while (queue.try_dequeue(tg)) {
		#endif
			const spring_time t0 = spring_now();

			while (tg->ExecuteTask());

			const spring_time t1 = spring_now();
			const spring_time dt = t1 - t0;

			threadStats[tid].numTasks += 1;
			threadStats[tid].sumTime  += dt.toNanoSecsi();
			threadStats[tid].minTime   = std::min(threadStats[tid].minTime, uint64_t(dt.toNanoSecsi()));
			threadStats[tid].maxTime   = std::max(threadStats[tid].maxTime, uint64_t(dt.toNanoSecsi()));
		}
	}

	// if true, queue contained at least one element
	return (tg != nullptr);
}


__FORCE_ALIGN_STACK__
static void WorkerLoop(int tid)
{
	SetThreadNum(tid);
#ifndef UNIT_TEST
	Threading::SetThreadName(IntToString(tid, "worker%i"));
#endif

	// make first worker spin a while before sleeping/waiting on the thread signal
	// this increases the chance that at least one worker is awake when a new task
	// is inserted, which can then take over the job of waking up sleeping workers
	// (see NotifyWorkerThreads)
	// NOTE: the spin-time has to be *short* to avoid biasing thread 1's workload
	const auto ourSpinTime = spring_time::fromMicroSecs(30 * (tid == 1));
	const auto maxSleepTime = spring_time::fromMilliSecs(30);

	while (!exitFlags[tid]) {
		const auto spinlockEnd = spring_now() + ourSpinTime;
		      auto sleepTime   = spring_time::fromMicroSecs(1);

		while (!DoTask(tid) && !exitFlags[tid]) {
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

	const auto tid = ThreadPool::GetThreadNum();

	do {
		const auto spinlockEnd = spring_now() + spring_time::fromMilliSecs(500);

		while (!DoTask(tid) && !taskGroup->IsFinished() && !exitFlags[tid]) {
			if (spring_now() < spinlockEnd)
				continue;

			//LOG_L(L_DEBUG, "Hang in ThreadPool");
			NotifyWorkerThreads(true);
			break;
		}
	} while (!taskGroup->IsFinished() && !exitFlags[tid]);

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


static std::uint32_t FindWorkerThreadCore(std::int32_t index, std::uint32_t availCores, std::uint32_t avoidCores)
{
	// find an unused core for worker-thread <index>
	const auto FindCore = [&index](std::uint32_t targetCores) {
		std::uint32_t workerCore = 1;
		std::int32_t n = index;

		while ((workerCore != 0) && !(workerCore & targetCores))
			workerCore <<= 1;

		// select n'th bit in targetCores
		// counts down because hyper-thread cores are appended to the end
		// and we prefer those for our worker threads (physical cores are
		// preferred for task specific threads)
		while (n--)
			do workerCore <<= 1; while ((workerCore != 0) && !(workerCore & targetCores));

		return workerCore;
	};

	const std::uint32_t threadAvailCore = FindCore(availCores);
	const std::uint32_t threadAvoidCore = FindCore(avoidCores);

	if (threadAvailCore != 0)
		return threadAvailCore;
	// select one of the main-thread cores if no others are available
	if (threadAvoidCore != 0)
		return threadAvoidCore;

	// fallback; use all
	return (~0);
}




void SetThreadCount(int wantedNumThreads)
{
	const int curNumThreads = ThreadPool::GetNumThreads();
	const int wtdNumThreads = Clamp(wantedNumThreads, 1, ThreadPool::GetMaxThreads());

	const char* fmts[3] = {
		"[ThreadPool::%s][1] wanted=%d current=%d maximum=%d",
		"[ThreadPool::%s][2] workers=%u tasks=%lu {sum,avg}time={%.3f, %.3f}ms",
		"\tthread=%d tasks=%lu (%3.3f%%) {sum,min,max,avg}time={%.3f, %.3f, %.3f, %.3f}ms",
	};

	// total number of tasks executed by pool; total time spent in DoTask
	uint64_t pNumTasks = 0lu;
	uint64_t pSumTime  = 0lu;

	LOG(fmts[0], __func__, wantedNumThreads, curNumThreads, ThreadPool::GetMaxThreads());

	if (workerThreads.empty()) {
		#ifdef USE_BOOST_LOCKFREE_QUEUE
		taskQueues[0].reserve(1024);
		#endif

		for (int i = 0; i < ThreadPool::MAX_THREADS; i++) {
			threadStats[i].numTasks =  0lu;
			threadStats[i].sumTime  =  0lu;
			threadStats[i].minTime  = -1lu;
			threadStats[i].maxTime  =  0lu;
		}
	}

	if (curNumThreads < wtdNumThreads) {
		SpawnThreads(wtdNumThreads, curNumThreads);
	} else {
		KillThreads(wtdNumThreads, curNumThreads);
	}

	if (workerThreads.empty()) {
		for (int i = 0; i < curNumThreads; i++) {
			pNumTasks += threadStats[i].numTasks;
			pSumTime  += threadStats[i].sumTime;
		}

		for (int i = 0; i < curNumThreads; i++) {
			const ThreadStats& ts = threadStats[i];

			const float tSumTime = ts.sumTime * 1e-6f; // ms
			const float tMinTime = ts.minTime * 1e-6f; // ms
			const float tMaxTime = ts.maxTime * 1e-6f; // ms
			const float tAvgTime = tSumTime / std::max(ts.numTasks, uint64_t(1));
			const float tRelFrac = (ts.numTasks * 1e2f) / std::max(pNumTasks, uint64_t(1));

			LOG(fmts[2], i, ts.numTasks, tRelFrac, tSumTime, tMinTime, tMaxTime, tAvgTime);
		}
	}

	LOG(fmts[1], __func__, (unsigned) workerThreads.size(), pNumTasks, pSumTime * 1e-6f, (pSumTime * 1e-6f) / std::max(pNumTasks, uint64_t(1)));
}


void InitWorkerThreads()
{
	std::uint32_t systemCores  = Threading::GetAvailableCoresMask();
	std::uint32_t mainAffinity = systemCores;

#ifndef UNIT_TEST
	mainAffinity &= configHandler->GetUnsigned("SetCoreAffinity");
#endif

	std::uint32_t workerAvailCores = systemCores & ~mainAffinity;

	{
#ifndef UNIT_TEST
		int workerCount = configHandler->GetInt("WorkerThreadCount");
#else
		int workerCount = -1;
#endif
		const int numCores = GetMaxThreads();

		// For latency reasons our worker threads yield rarely and so eat a lot cputime with idleing.
		// So it's better we always leave 1 core free for our other threads, drivers & OS
		if (workerCount < 0) {
			if (numCores == 2) {
				workerCount = numCores;
			} else if (numCores < 6) {
				workerCount = numCores - 1;
			} else {
				workerCount = numCores / 2;
			}
		}
		if (workerCount > numCores) {
			LOG_L(L_WARNING, "[ThreadPool::%s] workers set to %i, but there are just %i cores!", __func__, workerCount, numCores);
			workerCount = numCores;
		}

		SetThreadCount(workerCount);
	}

	// parallel_reduce now folds over shared_ptrs to futures
	// const auto ReduceFunc = [](std::uint32_t a, std::future<std::uint32_t>& b) -> std::uint32_t { return (a | b.get()); };
	const auto ReduceFunc = [](std::uint32_t a, std::shared_ptr< std::future<std::uint32_t> >& b) -> std::uint32_t { return (a | (b.get())->get()); };
	const auto AffinityFunc = [&]() -> std::uint32_t {
		const int i = ThreadPool::GetThreadNum();

		// 0 is the source thread, skip
		if (i == 0)
			return 0;

		const std::uint32_t workerCore = FindWorkerThreadCore(i - 1, workerAvailCores, mainAffinity);
		// const std::uint32_t workerCore = workerAvailCores;

		Threading::SetAffinity(workerCore);
		return workerCore;
	};

	const std::uint32_t    workerCoreAffin = parallel_reduce(AffinityFunc, ReduceFunc);
	const std::uint32_t nonWorkerCoreAffin = ~workerCoreAffin;

	if (mainAffinity == 0)
		mainAffinity = systemCores;

	Threading::SetAffinityHelper("Main", mainAffinity & nonWorkerCoreAffin);
}

}

#endif

