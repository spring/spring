
#ifndef _THREADPOOL_H
#define _THREADPOOL_H


#ifndef THREADPOOL
#include  <functional>

namespace ThreadPool {
	template<class F, class... Args>
	static inline void enqueue(F&& f, Args&&... args)
	{
		f(args ...);
	}

	static inline void SetThreadCount(int num) {}
	static inline void SetThreadSpinTime(int milliSeconds) {}
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

#include "TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/Threading/SpringThreading.h"

#include <deque>
#include <vector>
#include <list>
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
	ITaskGroup(const bool getid = true) : remainingTasks(0), id(getid ? lastId.fetch_add(1) : -1) { }
	virtual ~ITaskGroup() {}

	virtual bool ExecuteTask() = 0;
	bool IsFinished() const      { assert(remainingTasks.load() >= 0); return (remainingTasks.load(std::memory_order_relaxed) == 0); }
	int RemainingTasks() const   { return remainingTasks; }

	bool wait_for(const spring_time& rel_time) const {
		const auto end = spring_now() + rel_time;
		while (!IsFinished() && (spring_now() < end)) {
		}
		return IsFinished();
	}

	unsigned GetId() const { return id; }
	void UpdateId() { id = lastId.fetch_add(1); }

public:
	std::atomic_int remainingTasks;

private:
	static std::atomic_uint lastId;
	unsigned id;
};


namespace ThreadPool {
	template<class F, class... Args>
	static auto enqueue(F&& f, Args&&... args)
	-> std::shared_ptr<std::future<typename std::result_of<F(Args...)>::type>>;

	void PushTaskGroup(std::shared_ptr<ITaskGroup>&& taskgroup);
	void WaitForFinished(std::shared_ptr<ITaskGroup>&& taskgroup);

	template<typename T>
	inline void PushTaskGroup(std::shared_ptr<T>& taskgroup) { PushTaskGroup(std::move(std::static_pointer_cast<ITaskGroup>(taskgroup))); } //FIME std::move? doesn't it delete the original arg?
	template<typename T>
	inline void WaitForFinished(std::shared_ptr<T>& taskgroup) { WaitForFinished(std::move(std::static_pointer_cast<ITaskGroup>(taskgroup))); }

	void SetThreadCount(int num);
	void SetThreadSpinTime(int milliSeconds);
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

	SingleTask(F& f, Args... args) : done(false) {
		auto p = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		result = std::make_shared<std::future<return_type>>(p->get_future());
		task = p;
		this->remainingTasks++;
	}

	bool ExecuteTask() {
		if (done.exchange(true, std::memory_order_relaxed))
			return false;

		task();
		this->remainingTasks--;
		return true;
	}

	std::shared_ptr<std::future<return_type>> GetFuture() { assert(result->valid()); return std::move(result); } //FIXME rethrow exceptions some time

public:
	std::atomic<bool> done;
	std::function<void()> task;
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

	void enqueue(F& f, Args... args)
	{
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(f, args ...)
		);
		results.emplace_back(task->get_future());
		tasks.emplace_back(task);
		this->remainingTasks.fetch_add(1, std::memory_order_release);
	}


	bool ExecuteTask()
	{
		const int pos = curtask.fetch_add(1, std::memory_order_relaxed);
		if (pos < tasks.size()) {
			tasks[pos]();
			this->remainingTasks.fetch_sub(1, std::memory_order_release);
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
class TTaskGroup<F,void,Args...> : public ITaskGroup
{
public:
	TTaskGroup(const int num = 0) : curtask(0) {
		tasks.reserve(num);
	}

	void enqueue(F f, Args... args)
	{
		tasks.emplace_back([=]{ f(args...); });
		this->remainingTasks.fetch_add(1, std::memory_order_release);
	}

	bool ExecuteTask()
	{
		const int pos = curtask.fetch_add(1, std::memory_order_relaxed);
		if (pos < tasks.size()) {
			tasks[pos]();
			this->remainingTasks.fetch_sub(1, std::memory_order_release);
			return true;
		}
		return false;
	}

public:
	std::atomic<int> curtask;
	std::vector<std::function<void()>> tasks;
};


template<class F>
class TTaskGroup<F,void> : public ITaskGroup
{
public:
	TTaskGroup(const int num = 0) : curtask(0) {
		tasks.reserve(num);
	}

	void enqueue(F f)
	{
		tasks.emplace_back(f);
		this->remainingTasks.fetch_add(1, std::memory_order_release);
	}

	bool ExecuteTask()
	{
		const int pos = curtask.fetch_add(1, std::memory_order_relaxed);
		if (pos < tasks.size()) {
			tasks[pos]();
			this->remainingTasks.fetch_sub(1, std::memory_order_release);
			return true;
		}
		return false;
	}

public:
	std::atomic<int> curtask;
	std::vector<std::function<void()>> tasks;
};



template<typename F, typename return_type = int, typename... Args>
class TParallelTaskGroup : public TTaskGroup<F,return_type,Args...>
{
public:
	TParallelTaskGroup(const int num = 0) : TTaskGroup<F,return_type,Args...>(num) {}

	void enqueue_unique(const int threadNum, F& f, Args... args)
	{
		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		this->results.emplace_back(task->get_future());
		uniqueTasks[threadNum] = [=](){ (*task)(); };
		this->remainingTasks++;
	}

	bool ExecuteTask()
	{
		auto& func = uniqueTasks[ThreadPool::GetThreadNum()];
		if (!func)
			return TTaskGroup<F,return_type,Args...>::ExecuteTask();

		// no need to make threadsafe cause each thread got its own container
		func();
		func = nullptr;
		this->remainingTasks--;
		return true;
	}

public:
	std::array<std::function<void()>, ThreadPool::MAX_THREADS> uniqueTasks;
};


template<typename F, typename ...Args>
class TaskGroup : public TTaskGroup<F,decltype(std::declval<F>()((std::declval<Args>())...)),Args...> {
public:
	typedef decltype(std::declval<F>()((std::declval<Args>())...)) R;

	TaskGroup(const int num = 0) : TTaskGroup<F,R,Args...>(num) {}
};

template<typename F, typename ...Args>
class ParallelTaskGroup : public TParallelTaskGroup<F,decltype(std::declval<F>()((std::declval<Args>())...)),Args...> {
public:
	typedef decltype(std::declval<F>()((std::declval<Args>())...)) R;

	ParallelTaskGroup(const int num = 0) : TParallelTaskGroup<F,R,Args...>(num) {}
};




template<typename F>
class Parallel2TaskGroup : public ITaskGroup
{
public:
	Parallel2TaskGroup() : ITaskGroup(false) {
		//uniqueTasks.fill(false);
	}


	void enqueue(F& func)
	{
		f = func;
		this->remainingTasks = ThreadPool::GetNumThreads();
		uniqueTasks.fill(false);
	}


	bool ExecuteTask()
	{
		auto& ut = uniqueTasks[ThreadPool::GetThreadNum()];
		if (ut) {
			return false;
		}

		// no need to make threadsafe cause each thread got its own container
		ut = true;
		f();
		this->remainingTasks--;
		assert(this->remainingTasks.load() >= 0);
		return true;
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


	void enqueue(const int from, const int to, const int step, F& func)
	{
		assert(to >= from);
		this->remainingTasks = (step == 1) ? (to - from) : ((to - from) / step);
		this->curtask = {0};
		this->from = from;
		this->to   = to;
		this->step = step;
		this->f    = func;
	}


	bool ExecuteTask()
	{
		const int i = from + (step * curtask.fetch_add(1, std::memory_order_relaxed));
		if (i < to) {
			f(i);
			this->remainingTasks--;
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
	typedef TG<F> I;
	typedef std::shared_ptr<I> P;

	std::array<P, 256> cp;
	std::atomic_int pos = {0};

	TaskPool() {
		for (int i = 0; i<256; ++i) {
			cp[i] = P(new I);
		}
	}


	P Get() {
		auto v = cp[pos.fetch_add(1) % 256];
		assert(!v || v->IsFinished());
		return v;
	}
};




























template <typename F>
static inline void for_mt(int start, int end, int step, F&& f)
{
	const bool singleIteration = (end - start) < step;
	if (!ThreadPool::HasThreads() || singleIteration) {
		for (int i = start; i < end; i += step) {
			f(i);
		}
		return;
	}

	SCOPED_MT_TIMER("::ThreadWorkers (real)");
	static TaskPool<ForTaskGroup,F> pool;
	auto taskgroup = pool.Get();
	taskgroup->enqueue(start, end, step, f);
	taskgroup->UpdateId();
	ThreadPool::PushTaskGroup(taskgroup);
	ThreadPool::WaitForFinished(taskgroup);
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
	static TaskPool<Parallel2TaskGroup,F> pool;
	auto taskgroup = pool.Get();
	taskgroup->enqueue(f);
	taskgroup->UpdateId();
	ThreadPool::PushTaskGroup(taskgroup);
	ThreadPool::WaitForFinished(taskgroup);
}


template<class F, class G>
static inline auto parallel_reduce(F&& f, G&& g) -> typename std::result_of<F()>::type
{
	if (!ThreadPool::HasThreads())
		return f();

	SCOPED_MT_TIMER("::ThreadWorkers (real)");

	auto taskgroup = std::make_shared<ParallelTaskGroup<F>>(); //FIXME
	for (int i = 0; i < ThreadPool::GetNumThreads(); ++i) {
		taskgroup->enqueue_unique(i, f);
	}

	ThreadPool::PushTaskGroup(taskgroup);
	ThreadPool::WaitForFinished(taskgroup);
	return taskgroup->GetResult(std::move(g));
}


namespace ThreadPool {
	template<class F, class... Args>
	static inline auto enqueue(F&& f, Args&&... args)
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

		auto singletask = std::make_shared<SingleTask<F, Args...>>(std::forward<F>(f), std::forward<Args>(args)...);
		ThreadPool::PushTaskGroup(singletask);
		return singletask->GetFuture();
	}
}

#endif
#endif
