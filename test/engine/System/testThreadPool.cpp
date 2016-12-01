/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/ThreadPool.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"
#include "System/Misc/SpringTime.h"
#include "System/myMath.h"
#include "System/UnsyncedRNG.h"
#include <vector>
#include <atomic>
#include <boost/thread/future.hpp>

#define BOOST_TEST_MODULE ThreadPool
#include <boost/test/unit_test.hpp>
BOOST_GLOBAL_FIXTURE(InitSpringTime);

static int NUM_THREADS = std::min(ThreadPool::GetMaxThreads(), 10);

// !!! BOOST.TEST IS NOT THREADSAFE !!! (facepalms)
#define SAFE_BOOST_CHECK( P )           do { std::lock_guard<spring::mutex> _(m); BOOST_CHECK( ( P ) );               } while( 0 );
static spring::mutex m;


BOOST_AUTO_TEST_CASE( testThreadPool1 )
{
	LOG_L(L_WARNING, "for_mt");

	static const int RUNS = 5000;
	std::atomic<int> cnt(0);
	BOOST_CHECK(cnt.is_lock_free());
	std::vector<int> nums(RUNS,0);
	std::vector<int> runs(NUM_THREADS,0);
	ThreadPool::SetThreadCount(NUM_THREADS);
	BOOST_CHECK(ThreadPool::GetNumThreads() == NUM_THREADS);
	for_mt(0, RUNS, 2, [&](const int i) {
		const int threadnum = ThreadPool::GetThreadNum();
		SAFE_BOOST_CHECK(threadnum < NUM_THREADS);
		SAFE_BOOST_CHECK(threadnum >= 0);
		SAFE_BOOST_CHECK(i < RUNS);
		SAFE_BOOST_CHECK(i >= 0);
		assert(i < RUNS);
		runs[threadnum]++;
		nums[i] = 1;
		++cnt;
	});
	BOOST_CHECK(cnt == RUNS / 2);

	int rounds = 0;
	for(int i=0; i<runs.size(); i++) {
		rounds += runs[i];
	}
	BOOST_CHECK(rounds == (RUNS/2));

	for(int i=0; i<RUNS; i++) {
		BOOST_CHECK((i % 2) == (1 - nums[i]));
	}
}

BOOST_AUTO_TEST_CASE( testFor_mt )
{
	LOG_L(L_WARNING, "for_mt with stepSize");
	static const int RUNS = 5000;

	std::atomic_int hashMT = {0}, hash = {0};
	for_mt(0, RUNS, 2, [&](const int i) {
		hashMT += i;
	});
	for (int i = 0; i < RUNS; i += 2) {
		hash += i;
	}

	BOOST_CHECK(hash == hashMT);
	assert(hash == hashMT);
}

BOOST_AUTO_TEST_CASE( testParallel )
{
	LOG_L(L_WARNING, "parallel");

	std::vector<int> runs(NUM_THREADS);
	parallel([&]{
		const int threadnum = ThreadPool::GetThreadNum();
		SAFE_BOOST_CHECK(threadnum >= 0);
		SAFE_BOOST_CHECK(threadnum < NUM_THREADS);
		runs[threadnum]++;
	});

	for(int i=0; i<NUM_THREADS; i++) {
		BOOST_CHECK(runs[i] == 1);
	}
}

BOOST_AUTO_TEST_CASE( testThreadPool3 )
{
	LOG_L(L_WARNING, "parallel_reduce");

	int result = parallel_reduce([]() -> int {
		const int threadnum = ThreadPool::GetThreadNum();
		SAFE_BOOST_CHECK(threadnum >= 0);
		SAFE_BOOST_CHECK(threadnum < NUM_THREADS);
		return threadnum;
	}, [](int a, std::future<int>& b) -> int { return a + b.get(); });
	BOOST_CHECK(result == ((NUM_THREADS-1)*((NUM_THREADS-1) + 1))/2);
}

BOOST_AUTO_TEST_CASE( testThreadPool4 )
{
	LOG_L(L_WARNING, "nested for_mt");

	for_mt(0, 10, [&](const int y) {
		for_mt(0, 100, [&](const int x) {
			const int threadnum = ThreadPool::GetThreadNum();
			SAFE_BOOST_CHECK(threadnum < NUM_THREADS);
			SAFE_BOOST_CHECK(threadnum >= 0);
		});
	});
}

BOOST_AUTO_TEST_CASE( testThreadPool5 )
{
	/*parallel([&]{
		parallel([&]{
			const int threadnum = ThreadPool::GetThreadNum();
			SAFE_BOOST_CHECK(threadnum >= 0);
			SAFE_BOOST_CHECK(threadnum < NUM_THREADS);
		});
	});*/
}

BOOST_AUTO_TEST_CASE( testThreadPool6 )
{
	//FIXME FAILS ATM
	/*try {
		for_mt(0, 100, [&](const int i) {
			throw std::exception();
		});
	} catch(std::exception ex) {
		LOG_L(L_WARNING, "exception handling: fine");
	}*/
}

BOOST_AUTO_TEST_CASE( testThreadPoolNullLoop )
{
	for_mt(0, -100, [&](const int i) {
		SAFE_BOOST_CHECK(false); // shouldn't be called once
	});
	for_mt(0, 0, [&](const int i) {
		SAFE_BOOST_CHECK(false); // shouldn't be called once
	});
	for_mt(100, 10, -1, [&](const int i) {
		SAFE_BOOST_CHECK(false); // shouldn't be called once
	});
}


BOOST_AUTO_TEST_CASE( testThreadPoolSSE )
{
	LOG_L(L_WARNING, "SSE in for_mt");

	std::vector<UnsyncedRNG> rngs(NUM_THREADS);

	for (unsigned int n = 0; n < rngs.size(); n++)
		rngs[n].Seed(n);

	for_mt(0, 1000000, [&](const int i) {
		const float r = rngs[ThreadPool::GetThreadNum()].RandFloat() * 1000.0f;
		const float s = math::sqrt(r); // test SSE intrinsics (should be 100% reentrant)

		SAFE_BOOST_CHECK(!std::isinf(s));
		SAFE_BOOST_CHECK(!std::isnan(s));
	});
}





//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Performance Benchmarks below
///


static void HeatAir(const spring_time t)
{
	spring_time finish = spring_now() + t;
	while (spring_now() < finish) {}
}


static void ForVsForMt(const int RUNS, const spring_time kernelLoad)
{
	LOG("testing for vs. for_mt with a %.3fms kernel and %i runs (:= %.0fms total runtime):", kernelLoad.toMilliSecsf(), RUNS, (kernelLoad * RUNS).toMilliSecsf());
	spring_time t_for, t_formt;
	{
		const spring_time start = spring_now();
		for(int i=0; i<RUNS; ++i) {
			HeatAir(kernelLoad);
		}
		t_for = (spring_now() - start);

	}
	{
		const spring_time start = spring_now();
		for_mt(0, RUNS, [&](const int i) {
			HeatAir(kernelLoad);
		});
		t_formt = (spring_now() - start);
	}

	LOG("for    took %.4fms", t_for.toMilliSecsf());
	LOG("for_mt took %.4fms", t_formt.toMilliSecsf());
	LOG("mt runtime: %.0f%%", (t_formt.toMilliSecsf() / t_for.toMilliSecsf()) * 100.f);
}

BOOST_AUTO_TEST_CASE( testThreadPool )
{
	ForVsForMt(100,  spring_time::fromMicroSecs(10));
	ForVsForMt(1000, spring_time::fromMicroSecs(5));
	ForVsForMt(100,  spring_time::fromMicroSecs(50));
	ForVsForMt(10,   spring_time::fromMicroSecs(500));
	ForVsForMt(10,   spring_time::fromMicroSecs(1000));
}


static void TestReactionTimes(int RUNS)
{
	std::vector<float> totalWakeupTimes(ThreadPool::MAX_THREADS,0);

	parallel([&]() {}); // just wakeup the threads (to force reloading of the new spin timer)
	spring_time::fromSecs(1).sleep(); // make them sleep again

	for (int i = 0; i < RUNS; ++i) {
		auto start = spring_now();

		parallel([&]() {
			auto reactionTime = (spring_now() - start).toMilliSecsf();
			totalWakeupTimes[ThreadPool::GetThreadNum()] += reactionTime;
		});
	}

	for (int i = 0; i < ThreadPool::GetNumThreads(); ++i) {
		LOG("parallel %i reacted in avg %.6fms", i, totalWakeupTimes[i] / RUNS);
	}
}


BOOST_AUTO_TEST_CASE( testThreadPool8 )
{
	static const int RUNS = 10000;

	LOG_L(L_WARNING, "reaction times");
	//ThreadPool::SetThreadCount(ThreadPool::GetMaxThreads());

	for (int spin: {0, 1, 5, 10, 0}) {
		LOG_L(L_WARNING, "reaction times with %ims spin", spin);
		ThreadPool::SetThreadSpinTime(spin);
		TestReactionTimes(RUNS);
	}
}


BOOST_AUTO_TEST_CASE( testThreadPool9 )
{
	static const int RUNS = 10;
	std::vector<float> wakeupTimes(RUNS,0);
	std::vector<int  > wakeupThread(RUNS,0);
	spring_time start;

	ThreadPool::SetThreadCount(ThreadPool::GetMaxThreads());
	ThreadPool::SetThreadSpinTime(5);

	for (const char* s: {"cold","hot"}) {
		LOG_L(L_WARNING, "reaction times %s", s);
		start = spring_now();
		for_mt(0, RUNS, [&](const int i) {
			auto reactionTime = (spring_now() - start).toMilliSecsf();
			wakeupTimes[i] = reactionTime;
			wakeupThread[i] = ThreadPool::GetThreadNum();
		});

		for (int i = 0; i < RUNS; ++i) {
			LOG("for_mt %i reacted in %.6fms (handled on thread %i)", i, wakeupTimes[i], wakeupThread[i]);
		}
	}
}


BOOST_AUTO_TEST_CASE( testThreadPool10 )
{
	std::vector<float> perThread(NUM_THREADS);
	std::atomic<int> threads(0);
	parallel([&]() {
		spring_time start = spring_now();
		int i = ThreadPool::GetThreadNum();
		float dt = (spring_now() - start).toMilliSecsf();
		perThread[i] = dt;
		threads++;
	});

	float totalCost = 0.0f;
	for (float f: perThread) {
		totalCost += f;
	}

	LOG("cost of ThreadPool::GetThreadNum() : %.6fms", totalCost / threads);
}

struct do_once {
	do_once()   {}
	~do_once()  {
		// workarround boost::condition_variable::~condition_variable(): Assertion `!ret' failed.
		ThreadPool::SetThreadCount(1);
	}
};
BOOST_GLOBAL_FIXTURE(do_once);

