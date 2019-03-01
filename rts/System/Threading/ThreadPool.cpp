/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef THREADPOOL

#include "ThreadPool.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#if (!defined(UNITSYNC) && !defined(UNIT_TEST))
	#include "System/OffscreenGLContext.h"
#endif
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"
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

#include <utility>
#include <functional>

#define USE_TASK_STATS_TRACKING

// not in mingwlibs
// #define USE_BOOST_LOCKFREE_QUEUE

#ifdef USE_BOOST_LOCKFREE_QUEUE
#include <boost/lockfree/queue.hpp>
#else
#include "System/ConcurrentQueue.h"
#endif



#ifndef UNIT_TEST
CONFIG(int, WorkerThreadCount).defaultValue(-1).safemodeValue(0).minimumValue(-1).description("Number of workers (including the main thread!) used by ThreadPool.");
#endif



struct ThreadStats {
	uint64_t numTasksRun;
	uint64_t sumExecTime;
	uint64_t minExecTime;
	uint64_t maxExecTime;
	uint64_t sumWaitTime;
	uint64_t minWaitTime;
	uint64_t maxWaitTime;
};



// external background threads which are only joined on exit
static std::vector< spring::thread > extThreads;
static std::vector< std::future<void> > extFutures;

// global [idx = 0] and smaller per-thread [idx > 0] queues; the latter are
// for tasks that want to execute on specific threads, e.g. parallel_reduce
// note: std::shared_ptr<T> can not be made atomic, queues must store T*'s
#ifdef USE_BOOST_LOCKFREE_QUEUE
static std::array<boost::lockfree::queue<ITaskGroup*>, ThreadPool::MAX_THREADS> taskQueues[2];
#else
static std::array<moodycamel::ConcurrentQueue<ITaskGroup*>, ThreadPool::MAX_THREADS> taskQueues[2];
#endif

static std::vector<void*> workerThreads[2];
static std::array<bool, ThreadPool::MAX_THREADS> exitFlags;
static std::array<ThreadStats, ThreadPool::MAX_THREADS> threadStats[2];
static spring::signal newTasksSignal[2];

static _threadlocal int threadnum(0);

#ifndef UNITSYNC
// if enabled, allows OpenGL calls from ThreadPool tasks
// so certain logic (e.g. loading models) can be written
// without forcing GL code to run within the main thread
// this is highly experimental, use at own risk
static bool glThreadSupport = false;
#endif



std::atomic_uint ITaskGroup::lastId(0);



namespace ThreadPool {

int GetThreadNum() { return threadnum; }
static void SetThreadNum(const int idx) { threadnum = idx; }


static int GetConfigNumWorkers() {
	#ifndef UNIT_TEST
	return configHandler->GetInt("WorkerThreadCount");
	#else
	return -1;
	#endif
}

static int GetDefaultNumWorkers() {
	const int maxNumThreads = GetMaxThreads(); // min(MAX_THREADS, cpuCores)
	const int cfgNumWorkers = GetConfigNumWorkers();

	// for latency reasons our worker threads yield rarely (busy-looping)
	// so we always leave 1 core free for our other threads, drivers & OS
	// if the user has not set WorkerThreadCount
	if (cfgNumWorkers < 0) {
		if (maxNumThreads == 2)
			return maxNumThreads;
		if (maxNumThreads < 6)
			return (maxNumThreads - 1);

		return (maxNumThreads / 2);
	}

	if (cfgNumWorkers > maxNumThreads) {
		LOG_L(L_WARNING, "[ThreadPool::%s] workers set to %i, but there are just %i cores!", __func__, cfgNumWorkers, maxNumThreads);
		return maxNumThreads;
	}

	return cfgNumWorkers;
}


// FIXME: mutex/atomic?
// NOTE: +1 because we also count the main thread, workers start at 1
int GetNumThreads() { return (workerThreads[false].size() + 1); }
int GetMaxThreads() { return std::min(MAX_THREADS, Threading::GetPhysicalCpuCores()); }

bool HasThreads() { return !workerThreads[false].empty(); }



static bool DoTask(int tid, bool async)
{
	#ifndef UNIT_TEST
	SCOPED_MT_TIMER("ThreadPool::RunTask");
	#endif

	ITaskGroup* tg = nullptr;

	// any external thread calling WaitForFinished will have
	// id=0 and *only* processes tasks from the global queue
	for (int idx = 0; idx <= tid; idx += std::max(tid, 1)) {
		auto& queue = taskQueues[async][idx];

		#ifdef USE_BOOST_LOCKFREE_QUEUE
		if (queue.pop(tg)) {
		#else
		if (queue.try_dequeue(tg)) {
		#endif
			// inform other workers when there is global work to do
			// waking is an expensive kernel-syscall, so better shift this
			// cost to the workers too (the main thread only wakes when ALL
			// workers are sleeping)
			if (idx == 0)
				NotifyWorkerThreads(true, async);

			assert(!async || tg->IsAsyncTask());

			#ifdef USE_TASK_STATS_TRACKING
			const uint64_t wdt = tg->GetDeltaTime(spring_now());
			const uint64_t edt = tg->ExecuteLoop(tid, false);

			threadStats[async][tid].numTasksRun += 1;
			threadStats[async][tid].sumExecTime += edt;
			threadStats[async][tid].sumWaitTime += wdt;
			threadStats[async][tid].minExecTime  = std::min(threadStats[async][tid].minExecTime, edt);
			threadStats[async][tid].maxExecTime  = std::max(threadStats[async][tid].maxExecTime, edt);
			threadStats[async][tid].minWaitTime  = std::min(threadStats[async][tid].minWaitTime, wdt);
			threadStats[async][tid].maxWaitTime  = std::max(threadStats[async][tid].maxWaitTime, wdt);
			#else
			tg->ExecuteLoop(tid, false);
			#endif
		}

		#ifdef USE_BOOST_LOCKFREE_QUEUE
		while (queue.pop(tg)) {
		#else
		while (queue.try_dequeue(tg)) {
		#endif
			assert(!async || tg->IsAsyncTask());

			#ifdef USE_TASK_STATS_TRACKING
			const uint64_t wdt = tg->GetDeltaTime(spring_now());
			const uint64_t edt = tg->ExecuteLoop(tid, false);

			threadStats[async][tid].numTasksRun += 1;
			threadStats[async][tid].sumExecTime += edt;
			threadStats[async][tid].sumWaitTime += wdt;
			threadStats[async][tid].minExecTime  = std::min(threadStats[async][tid].minExecTime, edt);
			threadStats[async][tid].maxExecTime  = std::max(threadStats[async][tid].maxExecTime, edt);
			threadStats[async][tid].minWaitTime  = std::min(threadStats[async][tid].minWaitTime, wdt);
			threadStats[async][tid].maxWaitTime  = std::max(threadStats[async][tid].maxWaitTime, wdt);
			#else
			tg->ExecuteLoop(tid, false);
			#endif
		}
	}

	// if true, queue contained at least one element
	return (tg != nullptr);
}


__FORCE_ALIGN_STACK__
static void WorkerLoop(int tid, bool async)
{
	assert(tid != 0);
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

		while (!DoTask(tid, async) && !exitFlags[tid]) {
			if (spring_now() < spinlockEnd)
				continue;

			newTasksSignal[async].wait_for(sleepTime = std::min(sleepTime * 1.25f, maxSleepTime));
		}
	}
}





void WaitForFinished(std::shared_ptr<ITaskGroup>&& taskGroup)
{
	// can be any worker-thread (for_mt inside another for_mt, etc)
	const int tid = GetThreadNum();

	{
		#ifndef UNIT_TEST
		SCOPED_MT_TIMER("ThreadPool::WaitFor");
		#endif

		assert(!taskGroup->IsAsyncTask());
		assert(!taskGroup->SelfDelete());
		taskGroup->ExecuteLoop(tid, true);
	}

	// NOTE:
	//   it is possible for the task-group to have been completed
	//   entirely by the loop above, before any worker thread has
	//   even had a chance to pop it from the queue (so returning
	//   under that condition could cause the group to be deleted
	//   or reassigned prematurely) --> wait
	if (taskGroup->IsFinished()) {
		while (taskGroup->IsInJobQueue()) {
			DoTask(tid, false);
		}

		taskGroup->ResetState(false, taskGroup->IsInTaskPool(), false);
		return;
	}


	// task hasn't completed yet, use waiting time to execute other tasks
	NotifyWorkerThreads(true, false);

	do {
		const auto spinlockEnd = spring_now() + spring_time::fromMilliSecs(500);

		while (!DoTask(tid, false) && !taskGroup->IsFinished() && !exitFlags[tid]) {
			if (spring_now() < spinlockEnd)
				continue;

			// avoid a hang if the task is still not finished
			NotifyWorkerThreads(true, false);
			break;
		}
	} while (!taskGroup->IsFinished() && !exitFlags[tid]);

	while (taskGroup->IsInJobQueue()) {
		DoTask(tid, false);
	}

	taskGroup->ResetState(false, taskGroup->IsInTaskPool(), false);
}


// WARNING:
//   leaking the raw pointer *forces* caller to WaitForFinished
//   otherwise task might get deleted while its pointer is still
//   in the queue
void PushTaskGroup(std::shared_ptr<ITaskGroup>&& taskGroup) { PushTaskGroup(taskGroup.get()); }
void PushTaskGroup(ITaskGroup* taskGroup)
{
	auto& queue = taskQueues[ taskGroup->IsAsyncTask() ][ taskGroup->WantedThread() ];

	#if 0
	// fake single-task group, handled by WaitForFinished to
	// avoid a (delete) race-condition between it and DoTask
	if (taskGroup->RemainingTasks() == 1 && !taskGroup->IsAsyncTask())
		return;
	#endif

	taskGroup->SetTimeStamp(spring_now());

	#ifdef USE_BOOST_LOCKFREE_QUEUE
	while (!queue.push(taskGroup));
	#else
	while (!queue.enqueue(taskGroup));
	#endif

	#if 1
	// AsyncTask's do not care about wakeup-latency as much
	if (taskGroup->IsAsyncTask())
		return;

	NotifyWorkerThreads(false, false);
	#else
	NotifyWorkerThreads(false, taskGroup->IsAsyncTask());
	#endif
}

void NotifyWorkerThreads(bool force, bool async)
{
	// OPTIMIZATION
	// if !force then only wake up threads when _all_ are sleeping
	// this is an optimization; waking up other threads is a kernel
	// syscall that costs a lot of time (up to 1000ms!) so we prefer
	// not to block the thread that called PushTaskGroup and instead
	// let the worker threads themselves inform each other
	newTasksSignal[async].notify_all((GetNumThreads() - 1) * (1 - force));
}




static void SpawnThreads(int wantedNumThreads, int curNumThreads)
{
#ifndef UNITSYNC
	if (glThreadSupport) {
		try {
			for (int i = curNumThreads; i < wantedNumThreads; ++i) {
				exitFlags[i] = false;

				workerThreads[false].push_back(new COffscreenGLThread(std::bind(&WorkerLoop, i, false)));
				workerThreads[ true].push_back(new COffscreenGLThread(std::bind(&WorkerLoop, i,  true)));
			}
		} catch (const opengl_error& gle) {
			// shared gl context creation failed
			ThreadPool::SetThreadCount(0);

			glThreadSupport = false;
			curNumThreads = ThreadPool::GetNumThreads();
		}
	} else
#endif
	{
		for (int i = curNumThreads; i < wantedNumThreads; ++i) {
			exitFlags[i] = false;

			workerThreads[false].push_back(new spring::thread(std::bind(&WorkerLoop, i, false)));
			workerThreads[ true].push_back(new spring::thread(std::bind(&WorkerLoop, i,  true)));
		}
	}
}

static void KillThreads(int wantedNumThreads, int curNumThreads)
{
	for (int i = curNumThreads - 1; i >= wantedNumThreads && i > 0; --i) {
		exitFlags[i] = true;
	}

	NotifyWorkerThreads(true, false);

	for (int i = curNumThreads - 1; i >= wantedNumThreads && i > 0; --i) {
		assert(!workerThreads[false].empty());
		assert(!workerThreads[ true].empty());

	#ifndef UNITSYNC
		if (glThreadSupport) {
			{ auto th = reinterpret_cast<COffscreenGLThread*>(workerThreads[false].back()); th->join(); delete th; }
			{ auto th = reinterpret_cast<COffscreenGLThread*>(workerThreads[ true].back()); th->join(); delete th; }
		} else
	#endif
		{
			{ auto th = reinterpret_cast<spring::thread*>(workerThreads[false].back()); th->join(); delete th; }
			{ auto th = reinterpret_cast<spring::thread*>(workerThreads[ true].back()); th->join(); delete th; }
		}

		workerThreads[false].pop_back();
		workerThreads[ true].pop_back();
	}

	// play it safe
	for (int i = curNumThreads - 1; i >= wantedNumThreads && i > 0; --i) {
		ITaskGroup* tg = nullptr;

		#ifdef USE_BOOST_LOCKFREE_QUEUE
		while (taskQueues[false][i].pop(tg));
		while (taskQueues[ true][i].pop(tg));
		#else
		while (taskQueues[false][i].try_dequeue(tg));
		while (taskQueues[ true][i].try_dequeue(tg));
		#endif
	}

	assert((wantedNumThreads != 0) || workerThreads[false].empty());
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
	return (~0u);
}




void SetThreadCount(int wantedNumThreads)
{
	const int curNumThreads = GetNumThreads(); // includes main
	const int wtdNumThreads = Clamp(wantedNumThreads, 1, GetMaxThreads());

	const char* fmts[4] = {
		"[ThreadPool::%s][1] wanted=%d current=%d maximum=%d (init=%d)",
		"[ThreadPool::%s][2] workers=%lu",
		"\t[async=%d] threads=%d tasks=%lu {sum,avg}{exec,wait}time={{%.3f, %.3f}, {%.3f, %.3f}}ms",
		"\t\tthread=%d tasks=%lu {sum,min,max,avg}{exec,wait}time={{%.3f, %.3f, %.3f, %.3f}, {%.3f, %.3f, %.3f, %.3f}}ms",
	};

	// total number of tasks executed by pool; total time spent in DoTask
	uint64_t pNumTasksRun [2] = {0lu, 0lu};
	uint64_t pSumExecTimes[2] = {0lu, 0lu};
	uint64_t pSumWaitTimes[2] = {0lu, 0lu};

	LOG(fmts[0], __func__, wantedNumThreads, curNumThreads, GetMaxThreads(), workerThreads[false].empty());

	if (workerThreads[false].empty()) {
		assert(workerThreads[true].empty());

		#ifdef USE_BOOST_LOCKFREE_QUEUE
		taskQueues[false][0].reserve(1024);
		taskQueues[ true][0].reserve(1024);
		#endif

		#ifdef USE_TASK_STATS_TRACKING
		for (bool async: {false, true}) {
			for (int i = 0; i < MAX_THREADS; i++) {
				threadStats[async][i].numTasksRun = std::numeric_limits<uint64_t>::min();
				threadStats[async][i].sumExecTime = std::numeric_limits<uint64_t>::min();
				threadStats[async][i].minExecTime = std::numeric_limits<uint64_t>::max();
				threadStats[async][i].maxExecTime = std::numeric_limits<uint64_t>::min();
				threadStats[async][i].sumWaitTime = std::numeric_limits<uint64_t>::min();
				threadStats[async][i].minWaitTime = std::numeric_limits<uint64_t>::max();
				threadStats[async][i].maxWaitTime = std::numeric_limits<uint64_t>::min();
			}
		}
		#endif
	}


	#if (!defined(UNITSYNC) && !defined(UNIT_TEST))
	if (wantedNumThreads != 0) {
		CTimeProfiler::RegisterTimer("ThreadPool::AddTask");
		CTimeProfiler::RegisterTimer("ThreadPool::RunTask");
		CTimeProfiler::RegisterTimer("ThreadPool::WaitFor");
	}
	#endif

	if (curNumThreads < wtdNumThreads) {
		SpawnThreads(wtdNumThreads, curNumThreads);
	} else {
		KillThreads(wtdNumThreads, curNumThreads);
	}

	#if (!defined(UNITSYNC) && !defined(UNIT_TEST))
	if (wantedNumThreads == 0) {
		CTimeProfiler::UnRegisterTimer("ThreadPool::AddTask");
		CTimeProfiler::UnRegisterTimer("ThreadPool::RunTask");
		CTimeProfiler::UnRegisterTimer("ThreadPool::WaitFor");
	}
	#endif


	#ifdef USE_TASK_STATS_TRACKING
	if (workerThreads[false].empty()) {
		assert(workerThreads[true].empty());

		for (bool async: {false, true}) {
			for (int i = 0; i < curNumThreads; i++) {
				pNumTasksRun [async] += threadStats[async][i].numTasksRun;
				pSumExecTimes[async] += threadStats[async][i].sumExecTime;
				pSumWaitTimes[async] += threadStats[async][i].sumWaitTime;
			}
		}

		for (bool async: {false, true}) {
			const float pSumExecTime =  pSumExecTimes[async] * 1e-6f;
			const float pSumWaitTime =  pSumWaitTimes[async] * 1e-6f;
			const float pAvgExecTime = (pSumExecTimes[async] * 1e-6f) / std::max(pNumTasksRun[async], uint64_t(1));
			const float pAvgWaitTime = (pSumWaitTimes[async] * 1e-6f) / std::max(pNumTasksRun[async], uint64_t(1));

			LOG(fmts[2], async, curNumThreads, pNumTasksRun[async],  pSumExecTime, pAvgExecTime,  pSumWaitTime, pAvgWaitTime);

			for (int i = 0; i < curNumThreads; i++) {
				const ThreadStats& ts = threadStats[async][i];

				if (ts.numTasksRun == 0)
					continue;

				const float tSumExecTime = ts.sumExecTime * 1e-6f; // ms
				const float tSumWaitTime = ts.sumWaitTime * 1e-6f; // ms
				const float tMinExecTime = ts.minExecTime * 1e-6f; // ms
				const float tMinWaitTime = ts.minWaitTime * 1e-6f; // ms
				const float tMaxExecTime = ts.maxExecTime * 1e-6f; // ms
				const float tMaxWaitTime = ts.maxWaitTime * 1e-6f; // ms
				const float tAvgExecTime = tSumExecTime / std::max(ts.numTasksRun, uint64_t(1));
				const float tAvgWaitTime = tSumWaitTime / std::max(ts.numTasksRun, uint64_t(1));

				LOG(fmts[3], i, ts.numTasksRun,  tSumExecTime, tMinExecTime, tMaxExecTime, tAvgExecTime,  tSumWaitTime, tMinWaitTime, tMaxWaitTime, tAvgWaitTime);
			}
		}
	}
	#endif

	LOG(fmts[1], __func__, workerThreads[false].size());
}

void SetMaximumThreadCount()
{
	if (workerThreads[false].empty()) {
		workerThreads[false].reserve(MAX_THREADS);
		workerThreads[ true].reserve(MAX_THREADS);

		// NOTE:
		//   do *not* remove, this makes sure the profiler instance
		//   exists before any thread creates a timer that accesses
		//   it on destruction
		#ifndef UNIT_TEST
		profiler.ResetState();
		#endif
	}

	if (GetConfigNumWorkers() <= 0)
		return;

	SetThreadCount(GetMaxThreads());
}

void SetDefaultThreadCount()
{
	std::uint32_t systemCores  = Threading::GetAvailableCoresMask();
	std::uint32_t mainAffinity = systemCores;

	#ifndef UNIT_TEST
	mainAffinity &= configHandler->GetUnsigned("SetCoreAffinity");
	#endif

	std::uint32_t workerAvailCores = systemCores & ~mainAffinity;

	SetThreadCount(GetDefaultNumWorkers());

	{
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

		const std::uint32_t poolCoreAffinity = parallel_reduce(AffinityFunc, ReduceFunc);
		const std::uint32_t mainCoreAffinity = ~poolCoreAffinity;

		if (mainAffinity == 0)
			mainAffinity = systemCores;

		Threading::SetAffinityHelper("Main", mainAffinity & mainCoreAffinity);
	}
}



void AddExtJob(spring::thread&& t) {
	for (auto& et: extThreads) {
		if (et.joinable())
			continue;

		et = std::move(t);
		return;
	}

	extThreads.emplace_back(std::move(t));
}

void AddExtJob(std::future<void>&& f) {
	#ifndef WIN32
	for (auto& ef: extFutures) {
		// find a future whose (void) result is already available, without blocking
		// FIXME: does not currently (august 2017) compile on Windows mingw buildbots
		if (ef.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(0)) != std::future_status::ready)
			continue;

		ef = std::move(f);
		return;
	}
	#endif

	extFutures.emplace_back(std::move(f));
}

static void JoinExtThreads() {
	for (auto& t: extThreads) {
		t.join();
	}

	extThreads.clear();
}

static void GetExtFutures() {
	for (auto& f: extFutures) {
		f.get();
	}

	extFutures.clear();
}

void ClearExtJobs() {
	JoinExtThreads();
	GetExtFutures();
}

}

#endif

