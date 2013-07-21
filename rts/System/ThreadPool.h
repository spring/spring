
#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include <atomic>
#include <future>
#include <memory>

#include <deque>
#include <vector>
#include <list>
#include <boost/optional.hpp>
#include <numeric>
#include <chrono>

class ITaskGroup
{
public:
	virtual boost::optional<std::function<void()>> GetTask() = 0;
	virtual bool IsFinished() const = 0;
	virtual bool IsEmpty() const = 0;

	const std::list<std::exception_ptr>& GetExceptions() const { return exceptions; }
	void PushException(std::exception_ptr excep) { exceptions.emplace_back(excep); }
private:
	//virtual void FinishedATask() = 0;

private:
	std::list<std::exception_ptr> exceptions;
};


namespace ThreadPool {
	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args)
	-> std::shared_ptr<std::future<typename std::result_of<F(Args...)>::type>>;

	void PushTaskGroup(std::shared_ptr<ITaskGroup> taskgroup);
	void WaitForFinished(std::shared_ptr<ITaskGroup> taskgroup);

	template<typename T>
	inline void PushTaskGroup(std::shared_ptr<T> taskgroup) { PushTaskGroup(std::static_pointer_cast<ITaskGroup>(taskgroup)); }
	template<typename T>
	inline void WaitForFinished(std::shared_ptr<T> taskgroup) { WaitForFinished(std::static_pointer_cast<ITaskGroup>(taskgroup)); }

	void SetThreadCount(int num);
	int GetThreadNum();
	int GetMaxThreads();
	int GetNumThreads();
};


template<class F, class... Args>
class SingleTask : public ITaskGroup
{
public:
	typedef typename std::result_of<F(Args...)>::type return_type;

	SingleTask(F&& f, Args&&... args) : finished(false), done(false) {
		auto p = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		result = std::make_shared<std::future<return_type>>(p->get_future());
		task = [&,p]{ (*p)(); finished = true; };
	}

	boost::optional<std::function<void()>> GetTask() { done = true; return task; }

	bool IsEmpty() const    { return done; }
	bool IsFinished() const { return finished; }
	std::shared_ptr<std::future<return_type>> GetFuture() { assert(result->valid()); return std::move(result); } //FIXME rethrow exceptions some time

private:
	//void FinishedATask() { finished = true; }

public:
	std::atomic<bool> finished;
	std::atomic<bool> done;
	std::function<void()> task;
	std::shared_ptr<std::future<return_type>> result;
};


template<class F, class... Args>
class TaskGroup : public ITaskGroup
{
public:
	TaskGroup(const int num = 0) : remainingTasks(0), latency(0) {
		start = std::chrono::high_resolution_clock::now();
		results.reserve(num);
	}

	typedef typename std::result_of<F(Args...)>::type return_type;

	void enqueue(F& f, Args&... args)
	{
		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(f, args ...)
		);
		results.emplace_back(task->get_future());
		tasks.emplace_back([&,task]{ (*task)(); --remainingTasks; });
		remainingTasks++;
	}

	void enqueue(F&& f, Args&&... args)
	{
		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		results.emplace_back(task->get_future());
		tasks.emplace_back([&,task]{ (*task)(); --remainingTasks; });
		remainingTasks++;
	}


	boost::optional<std::function<void()>> GetTask()
	{
		if (!tasks.empty()) {
			auto t = tasks.front();
			tasks.pop_front();
			/*if (latency.count() == 0) {
				auto now = std::chrono::high_resolution_clock::now();
				latency = (now - start);
				LOG("latency %fms", latency.count() / 1000000.f);
			}*/
			return t;
		}
		return boost::optional<std::function<void()>>();
	}

	bool IsEmpty() const    { return tasks.empty(); }
	bool IsFinished() const { return (remainingTasks == 0); }

	template<typename G>
	return_type GetResult(const G&& g) {
		return std::accumulate(results.begin(), results.end(), 0, g);
	}

private:
	//void FinishedATask() { --remainingTasks; }

public:
	std::atomic<int> remainingTasks;
	std::deque<std::function<void()>> tasks; //make vector?
	std::vector<std::future<return_type>> results;

	std::chrono::time_point<std::chrono::high_resolution_clock> start; // use for latency profiling!
	std::chrono::nanoseconds latency;
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
		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		this->results.emplace_back(task->get_future());
		uniqueTasks[threadNum].emplace_back([&,task]{ (*task)(); --this->remainingTasks; });
		this->remainingTasks++;
	}

	void enqueue_unique(const int threadNum, F&& f, Args&&... args)
	{
		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		this->results.emplace_back(task->get_future());
		uniqueTasks[threadNum].emplace_back([&,task]{ (*task)(); --this->remainingTasks; });
		this->remainingTasks++;
	}


	boost::optional<std::function<void()>> GetTask()
	{
		auto& ut = uniqueTasks[ThreadPool::GetThreadNum()];
		if (!ut.empty()) {
			auto t = ut.front();
			ut.pop_front();
			return t;
		}

		if (!this->tasks.empty()) {
			return TaskGroup<F,Args...>::GetTask();
		}

		return boost::optional<std::function<void()>>();
	}

	bool IsEmpty() const {
		for(auto& ut: uniqueTasks) { if (!ut.empty()) return false; }
		return this->tasks.empty();
	}

public:
	std::vector<std::deque<std::function<void()>>> uniqueTasks;
};



static inline void for_mt(int start, int end, int step, const std::function<void(const int i)>&& f)
{
/*
	Threading::OMPCheck();
	#pragma omp parallel for
	for (int i = start; i < end; i+=step) {
		f(i);
	}
*/

	if (end > start) {
		if ((end - start) < step) {
			// single iteration -> directly process
			f(start);
			return;
		}

		SCOPED_MT_TIMER("::ThreadWorkers (real)");
		auto taskgroup = std::make_shared<TaskGroup<const std::function<void(const int)>, const int>>((end-start)/step);
		for (int i = start; i < end; i+=step) { //FIXME optimize worksize (group tasks in bigger ones than 1-steps)
			taskgroup->enqueue(f, i);
		}
		ThreadPool::PushTaskGroup(taskgroup);
		ThreadPool::WaitForFinished(taskgroup);
	}
}


static inline void for_mt(int start, int end, const std::function<void(const int i)>&& f)
{
	for_mt(start, end, 1, std::move(f));
}


static inline void parallel(const std::function<void()>&& f)
{
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
	-> std::shared_ptr<std::future<typename std::result_of<F(Args...)>::type>>
	{
		typedef typename std::result_of<F(Args...)>::type return_type;

		if (ThreadPool::GetNumThreads() <= 1) {
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
};


#endif
