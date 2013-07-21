
#include "ThreadPool.h"
#include "Platform/Threading.h"
#include "TimeProfiler.h"
#include "Util.h"
#include "OffscreenGLContext.h"

#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <utility>
#include <boost/optional.hpp>

//typedef std::thread thread_class;
typedef COffscreenGLThread thread_class;

static std::deque<std::shared_ptr<ITaskGroup>> taskGroups;
static std::deque<thread_class> thread_group;

static std::mutex taskMutex;
static std::condition_variable newTasks;

static __thread int threadnum(0); //FIXME __thread is gcc only, thread_local is c++11 but doesn't work in <4.8, and is also slower
static __thread bool exitThread(false);

static struct do_once {
	do_once() {
		//ThreadPool::SetThreadCount(ThreadPool::GetMaxThreads() - 1);
	}
	~do_once() {
		//ThreadPool::SetThreadCount(0);
	}
} doOnce;


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
	return Threading::GetAvailableCores();
}


int GetNumThreads()
{
	//return omp_get_num_threads();

	//FIXME mutex/atomic?
	return thread_group.size() + 1; // +1 cause we also count mainthread
}


static void DoTask(std::unique_lock<std::mutex>& lk)
{
	if (lk.try_lock()) {
		for(auto it = taskGroups.begin(); it != taskGroups.end();) {
			auto& tg = **it;
			auto p = tg.GetTask();
			if (tg.IsEmpty()) {
				it = taskGroups.erase(it);
			} else {
				++it;
			}
			if (p) {
				lk.unlock();
				SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
				try {
					(*p)();
				} catch(...) {
					tg.PushException(std::current_exception());
				}
				return;
			}
		}
		lk.unlock();
	}
}


static void DoTask(std::shared_ptr<ITaskGroup> tg, std::unique_lock<std::mutex>& lk)
{
	if (lk.try_lock()) {
		auto p = tg->GetTask();
		lk.unlock();
		if (p) {
			SCOPED_MT_TIMER("::ThreadWorkers (accumulated)");
			(*p)();
			return;
		}
	}
}


static void WorkerLoop(int id)
{
	SetThreadNum(id);
	Threading::SetThreadName(IntToString(id, "worker%i"));
	std::unique_lock<std::mutex> lk(taskMutex, std::defer_lock);
	std::mutex m;
	std::unique_lock<std::mutex> lk2(m, std::defer_lock);

	while (!exitThread) {
		if (taskGroups.empty()) {
			//std::this_thread::yield();
			newTasks.wait_for(lk2, std::chrono::nanoseconds(1));
		}

		DoTask(lk);
	}
}


void WaitForFinished(std::shared_ptr<ITaskGroup> taskgroup)
{
	std::unique_lock<std::mutex> lk(taskMutex, std::defer_lock);

	while (!taskgroup->IsEmpty()) {
		DoTask(taskgroup, lk);
	}

	while (!taskgroup->IsFinished()) { }

	for (auto& ep: taskgroup->GetExceptions())
		std::rethrow_exception(ep);
}


void PushTaskGroup(std::shared_ptr<ITaskGroup> taskgroup)
{
	std::unique_lock<std::mutex> lk(taskMutex, std::defer_lock);
	while (!lk.try_lock()) {}
	taskGroups.emplace_back(taskgroup);
	lk.unlock();
	newTasks.notify_all();
}


void SetThreadCount(int num)
{
	int curThreads = GetNumThreads();

	if (curThreads < num) {
		for (int i = curThreads; i<num; ++i) {
			thread_group.emplace_back(boost::bind(&WorkerLoop, i));
		}
	} else {
		for (int i = curThreads; i>num && i>1; --i) {
			assert(!thread_group.empty());

			auto taskgroup = std::make_shared<ParallelTaskGroup<const std::function<void()>>>();
			taskgroup->enqueue_unique(GetNumThreads() - 1, []{ exitThread = true; });
			ThreadPool::PushTaskGroup(taskgroup);

			thread_group.back().join();
			thread_group.pop_back();
		}
		if (num == 0) assert(thread_group.empty());
	}
}

};
