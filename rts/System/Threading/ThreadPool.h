/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#ifndef THREADPOOL
#include  <functional>
#include "System/Threading/SpringThreading.h"

namespace ThreadPool {
	template<class F, class... Args>
	static inline void Enqueue(F&& f, Args&&... args)
	{
		f(args ...);
	}

	static inline void AddExtJob(spring::thread&& t) { t.join(); }
	static inline void AddExtJob(std::future<void>&& f) { f.get(); }
	static inline void ClearExtJobs() {}

	static inline void SetMaximumThreadCount() {}
	static inline void SetDefaultThreadCount() {}
	static inline void SetThreadCount(int num) {}
	static inline int GetThreadNum() { return 0; }
	static inline int GetMaxThreads() { return 1; }
	static inline int GetNumThreads() { return 1; }
	static inline void NotifyWorkerThreads(bool force, bool async) {}
	static inline bool HasThreads() { return false; }

	static constexpr int MAX_THREADS = 1;
}


static inline void for_mt(int start, int end, int step, const std::function<void(const int i)>&& f)
{
	for (int i = start; i < end; i += step) {
		f(i);
	}
}

static inline void for_mt(int start, int end, const std::function<void(const int i)>&& f)
{
	for_mt(start, end, 1, std::move(f));
}

static inline void for_mt2(int start, int end, unsigned worksize, const std::function<void(const int i)>&& f)
{
	for_mt(start, end, 1, std::move(f));
}


static inline void parallel(const std::function<void()>&& f)
{
	f();
}

template<class F, class G>
static inline auto parallel_reduce(F&& f, G&& g) -> typename std::result_of<F()>::type
{
	return f();
}

#else

#include "System/TimeProfiler.h"
#include "System/Platform/Threading.h"
#include "System/Threading/SpringThreading.h"

#include  <array>
#include <vector>
#include <numeric>
#include <atomic>

#undef gt
#include <memory>

#ifdef UNITSYNC
	#undef SCOPED_MT_TIMER
	#define SCOPED_MT_TIMER(x)
#endif

class ITaskGroup;
namespace ThreadPool {
	template<class F, class... Args>
	static auto Enqueue(F&& f, Args&&... args)
	-> std::shared_ptr<std::future<typename std::result_of<F(Args...)>::type>>;

	void AddExtJob(spring::thread&& t);
	void AddExtJob(std::future<void>&& f);
	void ClearExtJobs();

	void PushTaskGroup(ITaskGroup* taskGroup);
	void PushTaskGroup(std::shared_ptr<ITaskGroup>&& taskGroup);
	void WaitForFinished(std::shared_ptr<ITaskGroup>&& taskGroup);

	template<typename T>
	inline void PushTaskGroup(std::shared_ptr<T>& taskGroup) { PushTaskGroup(std::move(std::static_pointer_cast<ITaskGroup>(taskGroup))); } //FIXME std::move? doesn't it delete the original arg?
	template<typename T>
	inline void WaitForFinished(std::shared_ptr<T>& taskGroup) { WaitForFinished(std::move(std::static_pointer_cast<ITaskGroup>(taskGroup))); }

	void SetMaximumThreadCount();
	void SetDefaultThreadCount();
	void SetThreadCount(int num);
	int GetThreadNum();
	bool HasThreads();
	int GetMaxThreads();
	int GetNumThreads();
	void NotifyWorkerThreads(bool force, bool async);

	static constexpr int MAX_THREADS = 16;
}




class ITaskGroup
{
public:
	ITaskGroup(const bool getid = true, const bool pooled = false): id(getid ? lastId.fetch_add(1) : -1u), ts(0) {
		ResetState(!pooled, pooled, false);
	}

	virtual ~ITaskGroup() {
		assert(AllowDelete());
	}

	virtual bool IsAsyncTask() const { return false; }
	virtual bool IsSliceTask() const { return false; }
	virtual bool ExecuteStep() = 0;
	virtual bool SelfDelete() const { return false; }

	uint64_t ExecuteLoop(int tid, bool wffCall) {
		const spring_time t0 = spring_now();

		while (ExecuteStep());

		const spring_time t1 = spring_now();
		const spring_time dt = t1 - t0;

		if (IsSliceTask()) {
			// inTaskQueue would be set to false prematurely by the
			// first slice to finish, let it be handled by WFF which
			// blocks until all threads are
			if (!wffCall)
				return (dt.toNanoSecsi());

			inTaskQueue.store(false);
		} else {
			// do not set this to false from WFF, defeats the purpose
			if (!wffCall) {
				assert(inTaskQueue.load());
				inTaskQueue.store(wffCall);
			}
		}

		if (SelfDelete()) {
			// store *after* the check in both branches, avoids UAF
			// async-tasks can never have a parent waiting for them
			execLoopDone.store(true);
			delete this;
		} else {
			execLoopDone.store(ExecLoopDone() || !wffCall);
		}

		return (dt.toNanoSecsi());
	}

	bool IsFinished() const { assert(remainingTasks.load() >= 0); return (remainingTasks.load(std::memory_order_relaxed) == 0); }
	bool IsInJobQueue() const { return (inTaskQueue.load(std::memory_order_relaxed)); }
	bool IsInTaskPool() const { return ((taskPoolMask.load(std::memory_order_relaxed) & (1 << 0)) != 0); }
	bool IsInPoolUse() const { return ((taskPoolMask.load(std::memory_order_relaxed) & (1 << 1)) != 0); }

	bool ExecLoopDone() const { return (execLoopDone.load(std::memory_order_relaxed)); }
	// pooled tasks are deleted only when their pool dies (on exit) which is always allowed
	bool AllowDelete() const { return (IsFinished() && ((!IsInJobQueue() && ExecLoopDone()) || IsInTaskPool())); }

	int RemainingTasks() const { return remainingTasks; }
	int WantedThread() const { return wantedThread; }

	bool WaitFor(const spring_time& rel_time) const {
		const auto end = spring_now() + rel_time;
		while (!IsFinished() && (spring_now() < end));
		return IsFinished();
	}

	uint32_t GetId() const { return id; }
	uint64_t GetDeltaTime(const spring_time t) const { return (std::max(ts.load(), uint64_t(t.toNanoSecsi())) - ts); }

	void UpdateId() { id = lastId.fetch_add(1); }
	void SetTimeStamp(const spring_time t) { ts = t.toNanoSecsi(); }

	void ResetState(bool queued, bool pooled, bool inuse) {
		remainingTasks.store(0);
		wantedThread.store(0);
		taskPoolMask.store(((1 * pooled) << 0) + ((1 * inuse) << 1));

		inTaskQueue.store(queued);
		execLoopDone.store(false);
	}

public:
	std::atomic_int remainingTasks;
	std::atomic_int wantedThread; // if 0 (default), task will be executed by an arbitrary thread
	std::atomic_int taskPoolMask; // whether this task is managed (owned) and in use by a TaskPool

	std::atomic_bool inTaskQueue; // whether this task is still in a thread's queue
	std::atomic_bool execLoopDone; // whether the thread running this task is about to exit ExecLoop

private:
	static std::atomic_uint lastId;

	std::atomic<uint32_t> id;
	std::atomic<uint64_t> ts; // timestamp (ns)
};


template<class F, class... Args>
class AsyncTask: public ITaskGroup
{
public:
	typedef  typename std::result_of<F(Args...)>::type  return_type;

	AsyncTask(F f, Args... args) : selfDelete(true) {
		task = std::make_shared<std::packaged_task<return_type()>>(std::bind(f, std::forward<Args>(args)...));
		result = std::make_shared<std::future<return_type>>(task->get_future());

		remainingTasks += 1;
	}

	bool IsAsyncTask() const override { return true; }
	bool SelfDelete() const override { return (selfDelete.load()); }
	bool ExecuteStep() override {
		// note: *never* called from WaitForFinished
		(*task)();
		remainingTasks -= 1;
		return false;
	}

	// FIXME: rethrow exceptions some time
	std::shared_ptr<std::future<return_type>> GetFuture() { assert(result->valid()); return std::move(result); }

public:
	// if true, we are not managed by a shared_ptr
	std::atomic<bool> selfDelete;

	std::shared_ptr<std::packaged_task<return_type()>> task;
	std::shared_ptr<std::future<return_type>> result;
};



template<class F, typename R = int, class... Args>
class TTaskGroup: public ITaskGroup
{
public:
	TTaskGroup(const int num = 0) : curtask(0) {
		results.reserve(num);
		tasks.reserve(num);
	}

	typedef R return_type;

	void Enqueue(F f, Args... args)
	{
		auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(f, std::forward<Args>(args)...));
		results.emplace_back(task->get_future());
		tasks.emplace_back(task);
		remainingTasks.fetch_add(1, std::memory_order_release);
	}


	bool ExecuteStep() override
	{
		const int pos = curtask.fetch_add(1, std::memory_order_relaxed);

		if (pos < tasks.size()) {
			tasks[pos]();
			remainingTasks.fetch_sub(1, std::memory_order_release);
			return true;
		}

		return false;
	}

	template<typename G>
	return_type GetResult(const G&& g) {
		return std::accumulate(results.begin(), results.end(), 0, g);
	}

public:
	std::atomic<int> curtask;
	std::vector<std::function<void()>> tasks;
	std::vector<std::future<return_type>> results;
};




template<class F, typename ...Args>
class TTaskGroup<F, void, Args...>: public ITaskGroup
{
public:
	TTaskGroup(const int num = 0) : curtask(0) {
		tasks.reserve(num);
	}

	void Enqueue(F f, Args... args)
	{
		tasks.emplace_back(std::bind(f, args...));
		remainingTasks.fetch_add(1, std::memory_order_release);
	}

	bool ExecuteStep() override
	{
		const int pos = curtask.fetch_add(1, std::memory_order_relaxed);

		if (pos < tasks.size()) {
			tasks[pos]();
			remainingTasks.fetch_sub(1, std::memory_order_release);
			return true;
		}

		return false;
	}

public:
	std::atomic<int> curtask;
	std::vector<std::function<void()>> tasks;
};


template<class F>
class TTaskGroup<F, void>: public ITaskGroup
{
public:
	TTaskGroup(const int num = 0) : curtask(0) {
		tasks.reserve(num);
	}

	void Enqueue(F f)
	{
		tasks.emplace_back(f);
		remainingTasks.fetch_add(1, std::memory_order_release);
	}

	bool ExecuteStep() override
	{
		const int pos = curtask.fetch_add(1, std::memory_order_relaxed);

		if (pos < tasks.size()) {
			tasks[pos]();
			remainingTasks.fetch_sub(1, std::memory_order_release);
			return true;
		}

		return false;
	}

public:
	std::atomic<int> curtask;
	std::vector<std::function<void()>> tasks;
};


template<typename F, typename ...Args>
class TaskGroup: public TTaskGroup<F, decltype(std::declval<F>()((std::declval<Args>())...)), Args...> {
public:
	typedef decltype(std::declval<F>()((std::declval<Args>())...)) R;

	TaskGroup(const int num = 0) : TTaskGroup<F, R, Args...>(num) {}
};




#if 0
template<typename F, typename return_type = int, typename... Args>
class TParallelTaskGroup: public TTaskGroup<F, return_type, Args...>
{
public:
	TParallelTaskGroup(const int num = 0) : TTaskGroup<F, return_type, Args...>(num) {
		uniqueTasks.fill(nullptr);
	}

	void EnqueueUnique(const int threadNum, F& f, Args... args)
	{
		auto task = std::make_shared< std::packaged_task<return_type()> >(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		this->results.emplace_back(task->get_future());
		this->remainingTasks += 1;

		// NOTE:
		//   here we want task <task> to be executed by thread <threadNum>
		//   this does not actually happen, thread i can call ExecuteStep
		//   multiple times while thread j never calls it (because any TG
		//   is pulled from the queue *once*, by a random thread) meaning
		//   we will have leftover unexecuted tasks and hang
		uniqueTasks[threadNum] = [=](){ (*task)(); };
	}

	bool ExecuteStep() override
	{
		auto& func = uniqueTasks[ThreadPool::GetThreadNum()];

		// does nothing when num=0 except return false (no change to remainingTasks)
		if (func == nullptr)
			return TTaskGroup<F, return_type, Args...>::ExecuteStep();

		// no need to make threadsafe; each thread has its own container
		func();
		func = nullptr;

		if (!this->IsFinished()) {
			this->remainingTasks -= 1;
			return true;
		}

		return false;
	}

public:
	std::array<std::function<void()>, ThreadPool::MAX_THREADS> uniqueTasks;
};
#endif


#if 0
template<typename F, typename ...Args>
class ParallelTaskGroup: public TParallelTaskGroup<F, decltype(std::declval<F>()((std::declval<Args>())...)), Args...> {
public:
	typedef decltype(std::declval<F>()((std::declval<Args>())...)) R;

	ParallelTaskGroup(const int num = 0) : TParallelTaskGroup<F, R, Args...>(num) {}
};
#endif




#if 0
template<typename F>
class Parallel2TaskGroup: public ITaskGroup
{
public:
	Parallel2TaskGroup(bool pooled) : ITaskGroup(false, pooled) {
		// uniqueTasks.fill(false);
	}


	void Enqueue(F& func)
	{
		f = func;
		remainingTasks = ThreadPool::GetNumThreads();

		uniqueTasks.fill(false);
	}

	// NOTE:
	//   as written, this can only work *by accident* if 1) the thread
	//   that executes it in DoTask has a different id than the one in
	//   WaitForFinished and 2) the pool contains at most two threads
	//   (three or more will inevitably cause a hang, same conditions
	//   as TParallelTaskGroup)
	bool ExecuteStep() override
	{
		auto& ut = uniqueTasks[ThreadPool::GetThreadNum()];

		if (!ut) {
			// no need to make threadsafe; each thread has its own container
			ut = true;
			f();

			remainingTasks -= 1;
			return (!IsFinished());
		}

		return false;
	}

public:
	std::array<bool, ThreadPool::MAX_THREADS> uniqueTasks;
	std::function<void()> f;
};

#else

template<typename F>
class Parallel2TaskGroup: public ITaskGroup
{
public:
	typedef  TTaskGroup<F, void>  ChildTaskType;

	Parallel2TaskGroup(bool pooled) : ITaskGroup(false, pooled) {}

	void Enqueue(F& func)
	{
		// note: GNT counts main so we would be short one worker
		// (final task would never be executed and hang the pool)
		remainingTasks.store(ThreadPool::GetNumThreads() - 1);

		childTasks.clear();
		childTasks.reserve(remainingTasks);

		for (int i = 0; i < remainingTasks; i++) {
			auto task = std::make_shared<ChildTaskType>(1);

			task->Enqueue(func);
			task->wantedThread.store(1 + i % (ThreadPool::GetNumThreads() - 1));

			childTasks.push_back(task);
			ThreadPool::PushTaskGroup(task);
		}
	}

	bool ExecuteStep() override
	{
		bool isFinished = true;

		for (size_t n = 0; n < childTasks.size(); n++) {
			isFinished &= childTasks[n]->AllowDelete();
		}

		if (!isFinished)
			return true;

		// P2TG's are never actually in the queue, let WFF terminate
		inTaskQueue.store(false);
		remainingTasks.store(0);

		childTasks.clear();
		return false;
	}

private:
	std::vector< std::shared_ptr<ITaskGroup> > childTasks;
};
#endif



#if 0
template<typename F>
class ForTaskGroup: public ITaskGroup
{
public:
	typedef  TTaskGroup<F, void, int>  ChildTaskType;

	ForTaskGroup(bool pooled) : ITaskGroup(false, pooled) {}

	void Enqueue(const int from, const int to, const int step, F& func)
	{
		assert(to >= from);

		remainingTasks.store((step == 1) ? (to - from) : ((to - from + step - 1) / step));

		childTasks.clear();
		childTasks.reserve(remainingTasks);

		for (int i = 0; i < remainingTasks; i++) {
			auto task = std::make_shared<ChildTaskType>(1);

			task->Enqueue(func, from + (step * i));
			task->wantedThread.store(1 + i % (ThreadPool::GetNumThreads() - 1));

			childTasks.push_back(task);
			ThreadPool::PushTaskGroup(task);
		}
	}


	bool ExecuteStep() override
	{
		bool isFinished = true;

		for (size_t n = 0; n < childTasks.size(); n++) {
			isFinished &= (childTasks[n]->IsFinished() && !childTasks[n]->IsInJobQueue());
		}

		if (!isFinished)
			return true;

		remainingTasks.store(0);
		childTasks.clear();
		return false;
	}

private:
	std::vector< std::shared_ptr<ITaskGroup> > childTasks;
};

#else

template<typename F>
class ForTaskGroup: public ITaskGroup
{
public:
	typedef  TTaskGroup<F, void, int>  ChildTaskType;

	ForTaskGroup(bool pooled) : ITaskGroup(false, pooled) {}

	void Enqueue(const int from, const int to, const int step, F& func)
	{
		assert(to >= from);

		remainingTasks.store((step == 1) ? (to - from) : ((to - from + step - 1) / step));
		ctr.store(0);

		this->from = from;
		this->to   = to;
		this->step = step;
		this->func = func;
	}

	bool IsSliceTask() const override { return true; }
	bool ExecuteStep() override
	{
		const int i = from + (step * ctr.fetch_add(1, std::memory_order_relaxed));

		if (i < to) {
			func(i);
			remainingTasks -= 1;
			return true;
		}

		return false;
	}

private:
	std::atomic<int> ctr;
	std::function<void(const int)> func;

	int from;
	int to;
	int step;
};
#endif








template <template<typename> class TG, typename F>
struct TaskPool {
	typedef TG<F> FuncTaskGroup;
	typedef std::shared_ptr<FuncTaskGroup> FuncTaskGroupPtr;

	// more than 256 nested for_mt's or parallel's should be uncommon
	std::array<FuncTaskGroupPtr, 256> tgPool;
	std::atomic_int pos = {0};

	TaskPool() {
		for (size_t i = 0; i < tgPool.size(); ++i) {
			tgPool[i] = FuncTaskGroupPtr(new FuncTaskGroup(true));
		}
	}


	FuncTaskGroupPtr GetTaskGroup() {
		auto tg = tgPool[pos.fetch_add(1) % tgPool.size()];

		assert(tg->IsFinished());
		assert(tg->IsInTaskPool());
		assert(!tg->IsInJobQueue());
		assert(!tg->IsInPoolUse());

		tg->ResetState(true, true, true);
		return tg;
	}
};








template <typename F>
static inline void for_mt(int start, int end, int step, F&& f)
{
	if (!ThreadPool::HasThreads() || ((end - start) < step)) {
		for (int i = start; i < end; i += step) {
			f(i);
		}
		return;
	}

	SCOPED_MT_TIMER("ThreadPool::AddTask");

	// static, so TaskGroup's are recycled
	static TaskPool<ForTaskGroup, F> pool;
	auto taskGroup = pool.GetTaskGroup();

	taskGroup->Enqueue(start, end, step, f);
	taskGroup->UpdateId();

	assert(taskGroup->IsInJobQueue());

	#if 0
	ThreadPool::PushTaskGroup(taskGroup);
	#else
	// store the group in all worker queues s.t. each executes a slice
	for (size_t i = 1; i < ThreadPool::GetNumThreads(); ++i) {
		taskGroup->wantedThread.store(i);
		ThreadPool::PushTaskGroup(taskGroup);
	}
	#endif

	// make calling thread also run ExecuteLoop
	ThreadPool::WaitForFinished(taskGroup);

}

template <typename F>
static inline void for_mt(int start, int end, F&& f)
{
	for_mt(start, end, 1, f);
}


template <typename F>
static inline void parallel(F&& f)
{
	if (!ThreadPool::HasThreads())
		return f();

	SCOPED_MT_TIMER("ThreadPool::AddTask");

	// static, so TaskGroup's are recycled
	static TaskPool<Parallel2TaskGroup, F> pool;
	auto taskGroup = pool.GetTaskGroup();

	taskGroup->Enqueue(f);
	taskGroup->UpdateId();

	assert(taskGroup->IsInJobQueue());

	// note: child-tasks are pushed, parent itself should not be
	// ThreadPool::PushTaskGroup(taskGroup);
	ThreadPool::WaitForFinished(taskGroup);
}


template<class F, class G>
static inline auto parallel_reduce(F&& f, G&& g) -> typename std::result_of<F()>::type
{
	if (!ThreadPool::HasThreads())
		return f();

	SCOPED_MT_TIMER("ThreadPool::AddTask");

	typedef  typename std::result_of<F()>::type  RetType;
	// typedef  typename std::shared_ptr< AsyncTask<F> >  TaskType;
	typedef           std::shared_ptr< std::future<RetType> >  FoldType;

	// std::array<TaskType, ThreadPool::MAX_THREADS> tasks;
	std::array<AsyncTask<F>*, ThreadPool::MAX_THREADS> tasks;
	std::array<FoldType, ThreadPool::MAX_THREADS> results;

	// NOTE:
	//   results become available in AsyncTask::ExecuteStep, and can allow
	//   accumulate to return (followed by tasks going out of scope) before
	//   ExecuteStep's themselves have returned --> premature task deletion
	//   if shared_ptr were used (all tasks *must* have exited ExecuteLoop)
	//
	// tasks[0] = std::move(std::make_shared<AsyncTask<F>>(std::forward<F>(f)));
	tasks[0] = new AsyncTask<F>(std::forward<F>(f));
	results[0] = std::move(tasks[0]->GetFuture());

	// first job in a reduction usually wants to run on the main thread
	tasks[0]->ExecuteLoop(0, false);

	// need to push N individual tasks; see NOTE in TParallelTaskGroup
	for (size_t i = 1, n = ThreadPool::GetNumThreads(); i < n; ++i) {
		// tasks[i] = std::move(std::make_shared<AsyncTask<F>>(std::forward<F>(f)));
		tasks[i] = new AsyncTask<F>(std::forward<F>(f));
		results[i] = std::move(tasks[i]->GetFuture());

		// tasks[i]->selfDelete.store(false);
		tasks[i]->wantedThread.store(i);

		ThreadPool::PushTaskGroup(tasks[i]);
	}

	return (std::accumulate(results.begin(), results.begin() + ThreadPool::GetNumThreads(), 0, g));
}




namespace ThreadPool {
	template<class F, class... Args>
	static inline auto Enqueue(F&& f, Args&&... args)
	-> std::shared_ptr<std::future<typename std::result_of<F(Args...)>::type>>
	{
		typedef typename std::result_of<F(Args...)>::type return_type;

		if (!ThreadPool::HasThreads()) {
			// directly process when there are no worker threads
			auto task = std::make_shared< std::packaged_task<return_type()> >(std::bind(f, args ...));
			auto fut = std::make_shared<std::future<return_type>>(task->get_future());
			(*task)();
			return fut;
		}

		// can not use shared_ptr here, make async tasks delete themselves instead
		// auto task = std::make_shared<AsyncTask<F, Args...>>(std::forward<F>(f), std::forward<Args>(args)...);
		auto task = new AsyncTask<F, Args...>(std::forward<F>(f), std::forward<Args>(args)...);
		auto fut = task->GetFuture();

		// minor hack: assume AsyncTask's will cause (heavy) disk IO
		// although these can never block the main thread, the async
		// workers might still be handed an uneven work distribution
		task->wantedThread.store(1 + task->GetId() % (ThreadPool::GetNumThreads() - 1));

		ThreadPool::PushTaskGroup(task);
		return fut;
	}
}

#endif
#endif

