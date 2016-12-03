/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef THREADPOOL

#include "ThreadPool.h"
#include "Exceptions.h"
#include "myMath.h"
#include "Platform/Threading.h"
#include "Threading/SpringThreading.h"
#include "TimeProfiler.h"
#include "Util.h"

#if !defined(UNITSYNC) && !defined(UNIT_TEST)
	#include "OffscreenGLContext.h"
#endif

#include <deque>
#include <utility>
#include <functional>


static std::deque<void*> thread_group; //the worker threads



template<typename T>
struct SingleConsumerQueue {
	static constexpr unsigned ringSize = 256;
	typedef typename std::array<std::shared_ptr<T>, ringSize> container_type; //FIME remove shared_ptr

	unsigned ourGen = 0; //< always <= curGen+1
	static std::atomic_uint curGen; //< position in the ringbuffer is ringBuffer[curGen % 256]
	static container_type ringBuffer;

	T* pop() {
		auto v = ringBuffer[ourGen % ringSize].get();

		if (v == nullptr)
			return nullptr;

		if (v->GetId() < ourGen)
			return nullptr;

		++ourGen;
		return v;
	}

	static void push(std::shared_ptr<T>& v) {
		const unsigned id = v->GetId();

		ringBuffer[id % ringSize] = v;

		const unsigned nextGen = id + 1;
		unsigned cg = nextGen - 1;
		while (!curGen.compare_exchange_weak(cg, nextGen)) {
			// note: compare_exchange_weak updates 'cg' var
		}
	}

	bool empty() const {
		auto v = ringBuffer[ourGen % ringSize].get();
		return (v == nullptr) || (v->GetId() < ourGen);
	}

	void reset() {
		// has to be called after a thread is created
		ourGen = curGen;
	}
};
template<typename T>
typename SingleConsumerQueue<T>::container_type SingleConsumerQueue<T>::ringBuffer;
template<typename T>
std::atomic_uint SingleConsumerQueue<T>::curGen = {0};







static std::array<SingleConsumerQueue<ITaskGroup>,ThreadPool::MAX_THREADS> perThreadQueue;
static spring::mutex newTasksMutex;
static spring::signal newTasks;
static _threadlocal int threadnum(0);
static std::array<bool, ThreadPool::MAX_THREADS> exitThread;


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
	return (thread_group.size() + 1);
}

bool HasThreads()
{
	return !thread_group.empty();
}



/// returns false, when no further tasks were found
static bool DoTask(SingleConsumerQueue<ITaskGroup>& queue)
{
	if (queue.empty())
		return false;

	// inform other workers that there is work to do
	// wakeing is a kernel-syscall and so eats time,
	// it's better to shift this task to the workers
	// (the main thread only wakes when _all_ workers are sleeping)
	NotifyWorkerThreads(true);

	// start with the work
	SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
	while (auto tg = queue.pop()) {
		while (tg->ExecuteTask()) {
		}
	}

	return true;
}


__FORCE_ALIGN_STACK__
static void WorkerLoop(int id)
{
	SetThreadNum(id);
#ifndef UNIT_TEST
	Threading::SetThreadName(IntToString(id, "worker%i"));
#endif
	auto& queue = perThreadQueue[id];
	queue.reset();
	static const auto maxSleepTime = spring_time::fromMilliSecs(30);

	// make first worker spin a while before sleeping/waiting on the thread signal
	// this increase the chance that at least one worker is awake when a new TP-Task is inserted
	// and this worker can then take over the job of waking up the sleeping workers (see NotifyWorkerThreads)
	const auto ourSpinTime = spring_time::fromMilliSecs((id == 1) ? 1 : 0);

	while (!exitThread[id]) {
		const auto spinlockEnd = spring_now() + ourSpinTime;
		      auto sleepTime   = spring_time::fromMicroSecs(1);

		while (!DoTask(queue) && !exitThread[id]) {
			if (spring_now() < spinlockEnd)
				continue;

			sleepTime = std::min(sleepTime * 1.25f, maxSleepTime);
			newTasks.wait_for(sleepTime);
		}
	}
}


void WaitForFinished(std::shared_ptr<ITaskGroup>&& taskgroup)
{
	{
		SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
		while (taskgroup->ExecuteTask()) {
		}
	}


	if (!taskgroup->IsFinished()) {
		// the task hasn't completed yet
		// use the waiting time to execute other tasks
		NotifyWorkerThreads(true);
		const auto id = ThreadPool::GetThreadNum();
		auto& queue = perThreadQueue[id];

		do {
			const auto spinlockEnd = spring_now() + spring_time::fromMilliSecs(500);

			while (!DoTask(queue) && !taskgroup->IsFinished() && !exitThread[id]) {
				if (spring_now() < spinlockEnd)
					continue;

				//LOG_L(L_DEBUG, "Hang in ThreadPool");
				NotifyWorkerThreads(true);
				break;
			}
		} while (!taskgroup->IsFinished() && !exitThread[id]);
	}

	//LOG("WaitForFinished %i", taskgroup->GetExceptions().size());
	//for (auto& ep: taskgroup->GetExceptions())
	//	std::rethrow_exception(ep);
}


void PushTaskGroup(std::shared_ptr<ITaskGroup>&& taskgroup)
{
	SingleConsumerQueue<ITaskGroup>::push(taskgroup);
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
	const int minSleepers = (force) ? 0 : ThreadPool::GetNumThreads() - 1;

	newTasks.notify_all(minSleepers);
}









static void SpawnThreads(int num, int curThreads)
{
#ifndef UNITSYNC
	if (hasOGLthreads) {
		try {
			for (int i = curThreads; i < num; ++i) {
				exitThread[i] = false;
				thread_group.push_back(new COffscreenGLThread(std::bind(&WorkerLoop, i)));
			}
		} catch (const opengl_error& gle) {
			// shared gl context creation failed :<
			ThreadPool::SetThreadCount(0);

			hasOGLthreads = false;
			curThreads = ThreadPool::GetNumThreads();
		}
	} else
#endif
	{
		for (int i = curThreads; i<num; ++i) {
			exitThread[i] = false;
			thread_group.push_back(new spring::thread(std::bind(&WorkerLoop, i)));
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
		assert(!thread_group.empty());
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

	assert((num != 0) || thread_group.empty());
}


void SetThreadCount(int numWantedThreads)
{
	int curThreads = ThreadPool::GetNumThreads();
	LOG("[ThreadPool::%s][1] #wanted=%d #current=%d #max=%d", __FUNCTION__, numWantedThreads, curThreads, ThreadPool::GetMaxThreads());
	numWantedThreads = Clamp(numWantedThreads, 1, ThreadPool::GetMaxThreads());

	if (curThreads < numWantedThreads) {
		SpawnThreads(numWantedThreads, curThreads);
	} else {
		KillThreads(numWantedThreads, curThreads);
	}

	LOG("[ThreadPool::%s][2] #threads=%u", __FUNCTION__, (unsigned) thread_group.size());
}

}

#endif
