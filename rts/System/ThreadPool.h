
#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#ifdef __MINGW32__
	#ifndef _GLIBCXX_HAS_GTHREADS
		#error "pthreads missing"
	#endif
#endif

#include "TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"

#include <deque>
#include <vector>
#include <list>
#include <boost/optional.hpp>
#include <numeric>

//#ifdef __MINGW32__
	#include <boost/atomic.hpp>
	#include <boost/thread/future.hpp>
	#include <boost/pointer_cast.hpp>
	#include <boost/chrono/include.hpp>
	#include <boost/utility.hpp>
	#include <boost/make_shared.hpp>
/*#else
	#include <atomic>
	#include <future>
	#include <memory>
	#include <chrono>
#endif*/


class ITaskGroup
{
public:
	virtual boost::optional<boost::function<void()>> GetTask() = 0;
	virtual bool IsFinished() const = 0;
	virtual bool IsEmpty() const = 0;

	const std::list<boost::exception_ptr>& GetExceptions() const { return exceptions; }
	void PushException(boost::exception_ptr excep) { exceptions.emplace_back(excep); }
private:
	//virtual void FinishedATask() = 0;

private:
	std::list<boost::exception_ptr> exceptions;
};


namespace ThreadPool {
	template<class F, class... Args>
	static auto enqueue(F&& f, Args&&... args)
	-> boost::shared_ptr<boost::unique_future<typename boost::result_of<F(Args...)>::type>>;

	void PushTaskGroup(boost::shared_ptr<ITaskGroup> taskgroup);
	void WaitForFinished(boost::shared_ptr<ITaskGroup> taskgroup);

	template<typename T>
	inline void PushTaskGroup(boost::shared_ptr<T> taskgroup) { PushTaskGroup(boost::static_pointer_cast<ITaskGroup>(taskgroup)); }
	template<typename T>
	inline void WaitForFinished(boost::shared_ptr<T> taskgroup) { WaitForFinished(boost::static_pointer_cast<ITaskGroup>(taskgroup)); }

	void SetThreadCount(int num);
	int GetThreadNum();
	int GetMaxThreads();
	int GetNumThreads();
};


template<class F, class... Args>
class SingleTask : public ITaskGroup
{
public:
	typedef typename boost::result_of<F(Args...)>::type return_type;

	SingleTask(F&& f, Args&&... args) : finished(false), done(false) {
		auto p = boost::make_shared<boost::packaged_task<return_type()>>(
			boost::bind(boost::forward<F>(f), boost::forward<Args>(args)...)
		);
		result = boost::make_shared<boost::unique_future<return_type>>(p->get_future());
		task = [&,p]{ (*p)(); finished = true; };
	}

	boost::optional<boost::function<void()>> GetTask() { done = true; return task; }

	bool IsEmpty() const    { return done; }
	bool IsFinished() const { return finished; }
	boost::shared_ptr<boost::unique_future<return_type>> GetFuture() { assert(result->valid()); return boost::move(result); } //FIXME rethrow exceptions some time

private:
	//void FinishedATask() { finished = true; }

public:
	boost::atomic<bool> finished;
	boost::atomic<bool> done;
	boost::function<void()> task;
	boost::shared_ptr<boost::unique_future<return_type>> result;
};


template<class F, class... Args>
class TaskGroup : public ITaskGroup
{
public:
	TaskGroup(const int num = 0) : remainingTasks(0), latency(0) {
		start = boost::chrono::high_resolution_clock::now();
		results.reserve(num);
	}

	typedef typename boost::result_of<F(Args...)>::type return_type;

	void enqueue(F& f, Args&... args)
	{
		auto task = boost::make_shared< boost::packaged_task<return_type()> >(
			boost::bind(f, args ...)
		);
		results.emplace_back(task->get_future());
		tasks.emplace_back([&,task]{ (*task)(); remainingTasks--; });
		remainingTasks++;
	}

	void enqueue(F&& f, Args&&... args)
	{
		auto task = boost::make_shared< boost::packaged_task<return_type()> >(
			boost::bind(boost::forward<F>(f), boost::forward<Args>(args)...)
		);
		results.emplace_back(task->get_future());
		tasks.emplace_back([&,task]{ (*task)(); remainingTasks--; });
		remainingTasks++;
	}


	boost::optional<boost::function<void()>> GetTask()
	{
		if (!tasks.empty()) {
			auto t = tasks.front();
			tasks.pop_front();
			/*if (latency.count() == 0) {
				auto now = boost::chrono::high_resolution_clock::now();
				latency = (now - start);
				LOG("latency %fms", latency.count() / 1000000.f);
			}*/
			return t;
		}
		return boost::optional<boost::function<void()>>();
	}

	bool IsEmpty() const    { return tasks.empty(); }
	bool IsFinished() const { return (remainingTasks == 0); }

	template<typename G>
	return_type GetResult(const G&& g) {
		return std::accumulate(results.begin(), results.end(), 0, g);
	}

private:
	//void FinishedATask() { remainingTasks--; }

public:
	boost::atomic<int> remainingTasks;
	std::deque<boost::function<void()>> tasks; //make vector?
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

	typedef typename boost::result_of<F(Args...)>::type return_type;

	void enqueue_unique(const int threadNum, F& f, Args&... args)
	{
		auto task = boost::make_shared< boost::packaged_task<return_type()> >(
			boost::bind(boost::forward<F>(f), boost::forward<Args>(args)...)
		);
		this->results.emplace_back(task->get_future());
		uniqueTasks[threadNum].emplace_back([&,task]{ (*task)(); this->remainingTasks--; });
		this->remainingTasks++;
	}

	void enqueue_unique(const int threadNum, F&& f, Args&&... args)
	{
		auto task = boost::make_shared< boost::packaged_task<return_type()> >(
			boost::bind(boost::forward<F>(f), boost::forward<Args>(args)...)
		);
		this->results.emplace_back(task->get_future());
		uniqueTasks[threadNum].emplace_back([&,task]{ (*task)(); this->remainingTasks--; });
		this->remainingTasks++;
	}


	boost::optional<boost::function<void()>> GetTask()
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

		return boost::optional<boost::function<void()>>();
	}

	bool IsEmpty() const {
		for(auto& ut: uniqueTasks) { if (!ut.empty()) return false; }
		return this->tasks.empty();
	}

public:
	std::vector<std::deque<boost::function<void()>>> uniqueTasks;
};



static inline void for_mt(int start, int end, int step, const boost::function<void(const int i)>&& f)
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

		static float foo = 0.0f;
		foo = foo * 0.8f + ((end-start)/step) * 0.2f;

		LOG("for_mt steps %f %i", foo, (end-start)/step);

		SCOPED_MT_TIMER("::ThreadWorkers (real)");
		auto taskgroup = boost::make_shared<TaskGroup<const boost::function<void(const int)>, const int>>((end-start)/step);
		for (int i = start; i < end; i+=step) { //FIXME optimize worksize (group tasks in bigger ones than 1-steps)
			taskgroup->enqueue(f, i);
		}
		ThreadPool::PushTaskGroup(taskgroup);
		ThreadPool::WaitForFinished(taskgroup);
	}
}


static inline void for_mt(int start, int end, const boost::function<void(const int i)>&& f)
{
	for_mt(start, end, 1, boost::move(f));
}


static inline void parallel(const boost::function<void()>&& f)
{
	SCOPED_MT_TIMER("::ThreadWorkers (real)");
	auto taskgroup = boost::make_shared<ParallelTaskGroup<const boost::function<void()>>>();
	for (int i = 0; i < ThreadPool::GetNumThreads(); ++i) {
		taskgroup->enqueue_unique(i, f);
	}
	ThreadPool::PushTaskGroup(taskgroup);
	ThreadPool::WaitForFinished(taskgroup);
}


template<class F, class G>
static inline auto parallel_reduce(F&& f, G&& g) -> typename boost::result_of<F()>::type
{
	SCOPED_MT_TIMER("::ThreadWorkers (real)");
	auto taskgroup = boost::make_shared<ParallelTaskGroup<F>>();
	for (int i = 0; i < ThreadPool::GetNumThreads(); ++i) {
		taskgroup->enqueue_unique(i, f);
	}
	ThreadPool::PushTaskGroup(taskgroup);
	ThreadPool::WaitForFinished(taskgroup);
	return taskgroup->GetResult(boost::move(g));
}


namespace ThreadPool {
	template<class F, class... Args>
	static inline auto enqueue(F&& f, Args&&... args)
	-> boost::shared_ptr<boost::unique_future<typename boost::result_of<F(Args...)>::type>>
	{
		typedef typename boost::result_of<F(Args...)>::type return_type;

		if (ThreadPool::GetNumThreads() <= 1) {
			// directly process when there are no worker threads
			auto task = boost::make_shared< boost::packaged_task<return_type()> >(boost::bind(f, args ...));
			auto fut = boost::make_shared<boost::unique_future<return_type>>(task->get_future());
			(*task)();
			return fut;
		}

		auto singletask = boost::make_shared<SingleTask<F, Args...>>(boost::forward<F>(f), boost::forward<Args>(args)...);
		ThreadPool::PushTaskGroup(singletask);
		return singletask->GetFuture();
	}
};


#endif
