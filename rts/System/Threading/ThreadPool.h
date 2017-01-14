/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADPOOL_H
#define _THREADPOOL_H


#ifndef THREADPOOL
#include  <functional>

namespace ThreadPool {
	template<class F, class... Args>
	static inline void Enqueue(F&& f, Args&&... args)
	{
		f(args ...);
	}

	static inline void SetMaxThreadCount() {}
	static inline void SetDefaultThreadCount() {}
	static inline void SetThreadCount(int num) {}
	static inline int GetThreadNum() { return 0; }
	static inline int GetMaxThreads() { return 1; }
	static inline int GetNumThreads() { return 1; }
	static inline void NotifyWorkerThreads() {}
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




class ITaskGroup
{
public:
	ITaskGroup(const bool getid = true) : id(getid ? lastId.fetch_add(1) : -1) {
		ResetState();
	}

	virtual ~ITaskGroup() {
		assert(IsFinished());
		assert(!IsInQueue());
	}

	virtual void ResetState() {
		remainingTasks = 0;
		wantedThread = 0;
		inTaskQueue = 1;
	}
	virtual bool IsSingleTask() const { return false; }
	virtual bool ExecuteStep() = 0;
	virtual bool SelfDelete() const { return false; }

	spring_time ExecuteLoop(bool wffCall) {
		const spring_time t0 = spring_now();

		while (ExecuteStep());

		const spring_time t1 = spring_now();
		const spring_time dt = t1 - t0;

		if (!wffCall) {
			// do not set this from WFF, defeats the purpose
			assert(inTaskQueue.load() == 1);
			inTaskQueue.store(0);
		}

		if (SelfDelete())
			delete this;

		return dt;
	}

	bool IsFinished() const { assert(remainingTasks.load() >= 0); return (remainingTasks.load(std::memory_order_relaxed) == 0); }
	bool IsInQueue() const { return (inTaskQueue.load(std::memory_order_relaxed) == 1); }

	int RemainingTasks() const { return remainingTasks; }
	int WantedThread() const { return wantedThread; }

	bool wait_for(const spring_time& rel_time) const {
		const auto end = spring_now() + rel_time;
		while (!IsFinished() && (spring_now() < end));
		return IsFinished();
	}

	unsigned GetId() const { return id; }
	void UpdateId() { id = lastId.fetch_add(1); }

public:
	std::atomic_int remainingTasks;
	// if 0 (default), task will be executed by an arbitrary thread
	std::atomic_int wantedThread;
	std::atomic_int inTaskQueue;

private:
	static std::atomic_uint lastId;
	unsigned id;
};


namespace ThreadPool {
	template<class F, class... Args>
	static auto Enqueue(F&& f, Args&&... args)
	-> std::shared_ptr<std::future<typename std::result_of<F(Args...)>::type>>;

	void PushTaskGroup(ITaskGroup* taskGroup);
	void PushTaskGroup(std::shared_ptr<ITaskGroup>&& taskGroup);
	void WaitForFinished(std::shared_ptr<ITaskGroup>&& taskGroup);

	template<typename T>
	inline void PushTaskGroup(std::shared_ptr<T>& taskGroup) { PushTaskGroup(std::move(std::static_pointer_cast<ITaskGroup>(taskGroup))); } //FIXME std::move? doesn't it delete the original arg?
	template<typename T>
	inline void WaitForFinished(std::shared_ptr<T>& taskGroup) { WaitForFinished(std::move(std::static_pointer_cast<ITaskGroup>(taskGroup))); }

	void SetMaxThreadCount();
	void SetDefaultThreadCount();
	void SetThreadCount(int num);
	int GetThreadNum();
	bool HasThreads();
	int GetMaxThreads();
	int GetNumThreads();
	void NotifyWorkerThreads(const bool force = false);

	static constexpr int MAX_THREADS = 16;
}


template<class F, class... Args>
class SingleTask : public ITaskGroup
{
public:
	typedef typename std::result_of<F(Args...)>::type return_type;

	SingleTask(F f, Args... args) : selfDelete(true) {
		task = std::make_shared<std::packaged_task<return_type()>>(std::bind(f, std::forward<Args>(args)...));
		result = std::make_shared<std::future<return_type>>(task->get_future());

		remainingTasks += 1;
	}

	bool IsSingleTask() const override { return true; }
	bool SelfDelete() const { return (selfDelete.load()); }
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
class TTaskGroup : public ITaskGroup
{
public:
	TTaskGroup(const int num = 0) : curtask(0) {
		results.reserve(num);
		tasks.reserve(num);
	}

	typedef R return_type;

	void Enqueue(F f, Args... args)
	{
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(f, std::forward<Args>(args)...)
		);
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
class TTaskGroup<F, void, Args...> : public ITaskGroup
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
class TTaskGroup<F, void> : public ITaskGroup
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




template<typename F, typename return_type = int, typename... Args>
class TParallelTaskGroup : public TTaskGroup<F, return_type, Args...>
{
public:
	TParallelTaskGroup(const int num = 0) : TTaskGroup<F, return_type, Args...>(num) {
		uniqueTasks.fill(nullptr);
	}

	void EnqueueUnique(const int threadNum, F& f, Args... args)
	{
		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

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


template<typename F, typename ...Args>
class TaskGroup : public TTaskGroup<F, decltype(std::declval<F>()((std::declval<Args>())...)), Args...> {
public:
	typedef decltype(std::declval<F>()((std::declval<Args>())...)) R;

	TaskGroup(const int num = 0) : TTaskGroup<F, R, Args...>(num) {}
};

template<typename F, typename ...Args>
class ParallelTaskGroup : public TParallelTaskGroup<F, decltype(std::declval<F>()((std::declval<Args>())...)), Args...> {
public:
	typedef decltype(std::declval<F>()((std::declval<Args>())...)) R;

	ParallelTaskGroup(const int num = 0) : TParallelTaskGroup<F, R, Args...>(num) {}
};




template<typename F>
class Parallel2TaskGroup : public ITaskGroup
{
public:
	Parallel2TaskGroup() : ITaskGroup(false) {
		//uniqueTasks.fill(false);
	}


	void Enqueue(F& func)
	{
		f = func;
		remainingTasks = ThreadPool::GetNumThreads();
		uniqueTasks.fill(false);
	}


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


template<typename F>
class ForTaskGroup : public ITaskGroup
{
public:
	ForTaskGroup() : ITaskGroup(false), curtask(0) {
	}


	void Enqueue(const int from, const int to, const int step, F& func)
	{
		assert(to >= from);
		remainingTasks = (step == 1) ? (to - from) : ((to - from + step - 1) / step);
		curtask = {0};

		this->from = from;
		this->to   = to;
		this->step = step;
		this->f    = func;
	}


	bool ExecuteStep() override
	{
		const int i = from + (step * curtask.fetch_add(1, std::memory_order_relaxed));

		if (i < to) {
			f(i);
			remainingTasks -= 1;
			return true;
		}

		return false;
	}

public:
	std::atomic<int> curtask;
	int from;
	int to;
	int step;
	std::function<void(const int)> f;
};








template <template<typename> class TG, typename F>
struct TaskPool {
	typedef TG<F> FuncTaskGroup;
	typedef std::shared_ptr<FuncTaskGroup> FuncTaskGroupPtr;

	// more than 256 nested for_mt's or parallel's should be uncommon
	std::array<FuncTaskGroupPtr, 256> tgPool;
	std::atomic_int pos = {0};

	TaskPool() {
		for (int i = 0; i < tgPool.size(); ++i) {
			tgPool[i] = FuncTaskGroupPtr(new FuncTaskGroup());
		}
	}


	FuncTaskGroupPtr GetTaskGroup() {
		auto tg = tgPool[pos.fetch_add(1) % tgPool.size()];

		assert(tg->IsFinished());
		tg->ResetState();
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

	SCOPED_MT_TIMER("::ThreadWorkers (real)");

	// static, so TaskGroup's are recycled
	static TaskPool<ForTaskGroup, F> pool;
	auto taskGroup = pool.GetTaskGroup();

	taskGroup->Enqueue(start, end, step, f);
	taskGroup->UpdateId();
	ThreadPool::PushTaskGroup(taskGroup);
	ThreadPool::WaitForFinished(taskGroup); // make calling thread also run ExecuteLoop
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

	SCOPED_MT_TIMER("::ThreadWorkers (real)");

	// static, so TaskGroup's are recycled
	static TaskPool<Parallel2TaskGroup, F> pool;
	auto taskGroup = pool.GetTaskGroup();

	taskGroup->Enqueue(f);
	taskGroup->UpdateId();
	ThreadPool::PushTaskGroup(taskGroup);
	ThreadPool::WaitForFinished(taskGroup);
}


template<class F, class G>
static inline auto parallel_reduce(F&& f, G&& g) -> typename std::result_of<F()>::type
{
	if (!ThreadPool::HasThreads())
		return f();

	SCOPED_MT_TIMER("::ThreadWorkers (real)");

	typedef  typename std::result_of<F()>::type  RetType;
	// typedef  typename std::shared_ptr< SingleTask<F> >  TaskType;
	typedef           std::shared_ptr< std::future<RetType> >  FoldType;

	// std::vector<TaskType> tasks(ThreadPool::GetNumThreads());
	std::vector<SingleTask<F>*> tasks(ThreadPool::GetNumThreads(), nullptr);
	std::vector<FoldType> results(ThreadPool::GetNumThreads());

	// NOTE:
	//   results become available in SingleTask::ExecuteStep, and can allow
	//   accumulate to return (followed by tasks going out of scope) before
	//   ExecuteStep's themselves have returned --> premature task deletion
	//   if shared_ptr were used (all tasks *must* have exited ExecuteLoop)
	//
	// tasks[0] = std::move(std::make_shared<SingleTask<F>>(std::forward<F>(f)));
	tasks[0] = new SingleTask<F>(std::forward<F>(f));
	results[0] = std::move(tasks[0]->GetFuture());

	// first job usually wants to run on the main thread
	tasks[0]->ExecuteLoop(false);

	// need to push N individual tasks; see NOTE in TParallelTaskGroup
	for (size_t i = 1; i < results.size(); ++i) {
		// tasks[i] = std::move(std::make_shared<SingleTask<F>>(std::forward<F>(f)));
		tasks[i] = new SingleTask<F>(std::forward<F>(f));
		results[i] = std::move(tasks[i]->GetFuture());

		// tasks[i]->selfDelete.store(false);
		tasks[i]->wantedThread.store(i);

		ThreadPool::PushTaskGroup(tasks[i]);
	}

	return (std::accumulate(results.begin(), results.end(), 0, g));
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
		// auto task = std::make_shared<SingleTask<F, Args...>>(std::forward<F>(f), std::forward<Args>(args)...);
		auto task = new SingleTask<F, Args...>(std::forward<F>(f), std::forward<Args>(args)...);
		auto fut = task->GetFuture();

		ThreadPool::PushTaskGroup(task);
		return fut;
	}
}

#endif
#endif

