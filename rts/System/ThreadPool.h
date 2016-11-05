
#ifndef _THREADPOOL_H
#define _THREADPOOL_H


#ifndef THREADPOOL

//#include <boost/thread/future.hpp>
#include  <functional>

namespace ThreadPool {
	template<class F, class... Args>
	static inline void enqueue(F&& f, Args&&... args)
	{
		f(args ...);
	}
	//-> std::shared_ptr<boost::unique_future<typename std::result_of<F(Args...)>::type>> {}

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

#include <deque>
#include <vector>
#include <list>
#include <boost/optional.hpp>
#include <numeric>
#include <atomic>

// mingw is missing c++11 thread support atm, so for KISS always prefer boost atm
#include <boost/thread/future.hpp>
#undef gt
#include <boost/chrono/include.hpp>
#include <memory>

#ifdef UNITSYNC
	#undef SCOPED_MT_TIMER
	#define SCOPED_MT_TIMER(x)
#endif



class ITaskGroup
{
public:
	virtual ~ITaskGroup() {}

	virtual boost::optional<std::function<void()>> GetTask() = 0;
	virtual bool IsFinished() const = 0;
	virtual bool IsEmpty() const = 0;

	virtual int RemainingTasks() const = 0;

	template< class Rep, class Period >
	bool wait_for(const boost::chrono::duration<Rep, Period>& rel_time) const {
		const auto end = boost::chrono::high_resolution_clock::now() + rel_time;
		while (!IsFinished() && (boost::chrono::high_resolution_clock::now() < end)) {
		}
		return IsFinished();
	}
private:
	//virtual void FinishedATask() = 0;
};


namespace ThreadPool {
	template<class F, class... Args>
	static auto enqueue(F&& f, Args&&... args)
	-> std::shared_ptr<boost::unique_future<typename std::result_of<F(Args...)>::type>>;

	void PushTaskGroup(std::shared_ptr<ITaskGroup> taskgroup);
	void WaitForFinished(std::shared_ptr<ITaskGroup> taskgroup);

	template<typename T>
	inline void PushTaskGroup(std::shared_ptr<T> taskgroup) { PushTaskGroup(std::static_pointer_cast<ITaskGroup>(taskgroup)); }
	template<typename T>
	inline void WaitForFinished(std::shared_ptr<T> taskgroup) { WaitForFinished(std::static_pointer_cast<ITaskGroup>(taskgroup)); }

	void SetThreadCount(int num);
	void SetThreadSpinTime(int milliSeconds);
	int GetThreadNum();
	bool HasThreads();
	int GetMaxThreads();
	int GetNumThreads();
	void NotifyWorkerThreads();

	static constexpr int MAX_THREADS = 16;
}


template<class F, class... Args>
class SingleTask : public ITaskGroup
{
public:
	typedef typename std::result_of<F(Args...)>::type return_type;

	SingleTask(F&& f, Args&&... args) : finished(false), done(false) {
		auto p = std::make_shared<boost::packaged_task<return_type>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		result = std::make_shared<boost::unique_future<return_type>>(p->get_future());
		task = [&,p]{ (*p)(); finished.store(true, std::memory_order_release); };
	}

	boost::optional<std::function<void()>> GetTask() { return (!done.exchange(true, std::memory_order_relaxed)) ? boost::optional<std::function<void()>>(task) : boost::optional<std::function<void()>>(); }

	bool IsEmpty() const    { return done.load(std::memory_order_relaxed); }
	bool IsFinished() const { return finished.load(std::memory_order_relaxed); }
	int RemainingTasks() const { return done ? 0 : 1; }
	std::shared_ptr<boost::unique_future<return_type>> GetFuture() { assert(result->valid()); return std::move(result); } //FIXME rethrow exceptions some time

private:
	//void FinishedATask() { finished = true; }

public:
	std::atomic<bool> finished;
	std::atomic<bool> done;
	std::function<void()> task;
	std::shared_ptr<boost::unique_future<return_type>> result;
};


template<class F, class... Args>
class TaskGroup : public ITaskGroup
{
public:
	TaskGroup(const int num = 0) : remainingTasks(0), curtask(0), latency(0) {
		//start = boost::chrono::high_resolution_clock::now();
		results.reserve(num);
		tasks.reserve(num);
	}

	virtual ~TaskGroup() {}

	typedef typename std::result_of<F(Args...)>::type return_type;

	void enqueue(F& f, Args&... args)
	{
		auto task = std::make_shared<boost::packaged_task<return_type>>(
			std::bind(f, args ...)
		);
		results.emplace_back(task->get_future());
		// workaround a Fedora gcc bug else it reports in the lambda below:
		// error: no 'operator--(int)' declared for postfix '--'
		auto* atomicCounter = &remainingTasks;
		tasks.emplace_back([task,atomicCounter]{ (*task)(); atomicCounter->fetch_sub(1, std::memory_order_release); });
		remainingTasks.fetch_add(1, std::memory_order_release);
	}

	void enqueue(F&& f, Args&&... args)
	{
		auto task = std::make_shared< boost::packaged_task<return_type> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		results.emplace_back(task->get_future());
		// workaround a Fedora gcc bug else it reports in the lambda below:
		// error: no 'operator--(int)' declared for postfix '--'
		auto* atomicCounter = &remainingTasks;
		tasks.emplace_back([task,atomicCounter]{ (*task)(); atomicCounter->fetch_sub(1, std::memory_order_release); });
		remainingTasks.fetch_add(1, std::memory_order_release);
	}


	virtual boost::optional<std::function<void()>> GetTask()
	{
		const int pos = curtask.fetch_add(1, std::memory_order_relaxed);
		if (pos < tasks.size()) {
			/*if (latency.count() == 0) {
				auto now = boost::chrono::high_resolution_clock::now();
				latency = (now - start);
				LOG("latency %fms", latency.count() / 1000000.f);
			}*/
			return tasks[pos];
		}
		return boost::optional<std::function<void()>>();
	}

	virtual bool IsEmpty() const { return curtask.load(std::memory_order_relaxed) >= tasks.size(); }
	bool IsFinished() const      { return (remainingTasks.load(std::memory_order_relaxed) == 0); }
	int RemainingTasks() const   { return remainingTasks; }

	template<typename G>
	return_type GetResult(const G&& g) {
		return std::accumulate(results.begin(), results.end(), 0, g);
	}

private:
	//void FinishedATask() { remainingTasks--; }

public:
	std::atomic<int> remainingTasks;
	std::atomic<int> curtask;
	std::vector<std::function<void()>> tasks;
	std::vector<boost::unique_future<return_type>> results;

	boost::chrono::time_point<boost::chrono::high_resolution_clock> start; // use for latency profiling!
	boost::chrono::nanoseconds latency;
};


template<class F, class... Args>
class ParallelTaskGroup : public TaskGroup<F,Args...>
{
public:
	ParallelTaskGroup(const int num = 0) : TaskGroup<F,Args...>(num) {
		uniqueTasks.resize(ThreadPool::GetNumThreads());
	}

	typedef typename std::result_of<F(Args...)>::type return_type;

	void enqueue_unique(const int threadNum, F& f, Args&... args)
	{
		auto task = std::make_shared< boost::packaged_task<return_type> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		this->results.emplace_back(task->get_future());
		uniqueTasks[threadNum].emplace_back([&,task]{ (*task)(); (this->remainingTasks)--; });
		this->remainingTasks++;
	}

	void enqueue_unique(const int threadNum, F&& f, Args&&... args)
	{
		auto task = std::make_shared< boost::packaged_task<return_type> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		this->results.emplace_back(task->get_future());
		uniqueTasks[threadNum].emplace_back([&,task]{ (*task)(); (this->remainingTasks)--; });
		this->remainingTasks++;
	}


	boost::optional<std::function<void()>> GetTask()
	{
		auto& ut = uniqueTasks[ThreadPool::GetThreadNum()];
		if (!ut.empty()) {
			// no need to make threadsafe cause each thread got its own container
			auto t = ut.front();
			ut.pop_front();
			return t;
		}

		return TaskGroup<F,Args...>::GetTask();
	}

	bool IsEmpty() const {
		for(auto& ut: uniqueTasks) { if (!ut.empty()) return false; }
		return TaskGroup<F,Args...>::IsEmpty();
	}

public:
	std::vector<std::deque<std::function<void()>>> uniqueTasks;
};



static inline void for_mt(int start, int end, int step, const std::function<void(const int i)>&& f)
{
	if (end <= start)
		return;

	const bool singleIteration = (end - start) < step;

	// do not use HasThreads because that counts main as a worker
	if (!ThreadPool::HasThreads() || singleIteration) {
		for (int i = start; i < end; i += step) {
			f(i);
		}
		return;
	}

	ThreadPool::NotifyWorkerThreads();
	SCOPED_MT_TIMER("::ThreadWorkers (real)");
	auto taskgroup = std::make_shared<TaskGroup<const std::function<void(const int)>, const int>>((end-start)/step);
	for (int i = start; i < end; i += step) { //FIXME optimize worksize (group tasks in bigger ones than 1-steps)
		taskgroup->enqueue(f, i);
	}
	ThreadPool::PushTaskGroup(taskgroup);
	ThreadPool::WaitForFinished(taskgroup);
}


static inline void for_mt(int start, int end, const std::function<void(const int i)>&& f)
{
	for_mt(start, end, 1, std::move(f));
}


static inline void parallel(const std::function<void()>&& f)
{
	if (!ThreadPool::HasThreads())
		return f();

	ThreadPool::NotifyWorkerThreads();
	SCOPED_MT_TIMER("::ThreadWorkers (real)");

	auto taskgroup = std::make_shared<ParallelTaskGroup<const std::function<void()>>>();
	for (int i = 0; i < ThreadPool::GetNumThreads(); ++i) {
		taskgroup->enqueue_unique(i, f);
	}
	ThreadPool::PushTaskGroup(taskgroup);
	ThreadPool::WaitForFinished(taskgroup);
}


template<class F, class G>
static inline auto parallel_reduce(F&& f, G&& g) -> typename std::result_of<F()>::type
{
	if (!ThreadPool::HasThreads())
		return f();

	ThreadPool::NotifyWorkerThreads();
	SCOPED_MT_TIMER("::ThreadWorkers (real)");

	auto taskgroup = std::make_shared<ParallelTaskGroup<F>>();
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
	-> std::shared_ptr<boost::unique_future<typename std::result_of<F(Args...)>::type>>
	{
		typedef typename std::result_of<F(Args...)>::type return_type;

		if (!ThreadPool::HasThreads()) {
			// directly process when there are no worker threads
			auto task = std::make_shared< boost::packaged_task<return_type> >(std::bind(f, args ...));
			auto fut = std::make_shared<boost::unique_future<return_type>>(task->get_future());
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
