/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Threading/ThreadPool.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"
#include "System/Misc/SpringTime.h"
#include "System/SpringMath.h"
#include "System/GlobalRNG.h"

#include <vector>
#include <atomic>
#include <future>

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"


// Catch is not threadsafe
#define SAFE_CHECK( P )                \
	do {                                     \
		std::lock_guard<spring::mutex> _(m); \
		CHECK( (P) );                  \
	} while (0);

struct do_once {
	do_once() { printf("[%s]\n", __func__); Threading::DetectCores(); } // make GetMaxThreads() work
};

InitSpringTime ist;
do_once doonce;


// static do_once o;
static spring::mutex m;

static constexpr int NUM_RUNS = 5000;
static           int NUM_THREADS = 0;


TEST_CASE("test_for_mt")
{
	LOG("[%s::test_for_mt] {NUM,MAX}_THREADS={%d,%d}", __func__, NUM_THREADS = ThreadPool::GetMaxThreads(), ThreadPool::MAX_THREADS);

	std::atomic<int> cnt(0);

	CHECK(cnt.is_lock_free());

	std::vector<int> nums(NUM_RUNS, 0);
	std::vector<int> runs(NUM_THREADS, 0);

	ThreadPool::SetThreadCount(NUM_THREADS);
	CHECK(ThreadPool::GetNumThreads() == NUM_THREADS);

	for_mt(0, NUM_RUNS, 2, [&](const int i) {
		const int threadnum = ThreadPool::GetThreadNum();
		SAFE_CHECK(threadnum < NUM_THREADS);
		SAFE_CHECK(threadnum >= 0);
		SAFE_CHECK(i < NUM_RUNS);
		SAFE_CHECK(i >= 0);
		assert(i < NUM_RUNS);
		runs[threadnum]++;
		nums[i] = 1;
		++cnt;
	});
	CHECK(cnt == NUM_RUNS / 2);

	int rounds = 0;
	for (size_t i = 0; i < runs.size(); i++) {
		rounds += runs[i];
	}
	CHECK(rounds == (NUM_RUNS / 2));

	for (int i = 0; i < NUM_RUNS; i++) {
		CHECK((i % 2) == (1 - nums[i]));
	}
}



TEST_CASE("test_stepped_for_mt")
{
	LOG("[%s::test_stepped_for_mt]", __func__);

	std::atomic_int hashMT = {0};
	std::atomic_int hash = {0};

	for_mt(0, NUM_RUNS, 2, [&](const int i) {
		hashMT += i;
	});
	for (int i = 0; i < NUM_RUNS; i += 2) {
		hash += i;
	}

	CHECK(hash == hashMT);
	assert(hash == hashMT);
}

TEST_CASE("test_parallel")
{
	LOG("[%s::test_parallel]", __func__);
	CHECK(ThreadPool::GetNumThreads() == NUM_THREADS);

	std::vector<int> runs(NUM_THREADS, 0);

	// should be executed exactly once by each worker, and never by WaitForFinished (tid=0)
	// threadnum=0 only in the special case that the pool is actually empty (NUM_THREADS=1)
	parallel([&]{
		const int threadnum = ThreadPool::GetThreadNum();
		SAFE_CHECK(threadnum >           0 || runs.size() == 1);
		SAFE_CHECK(threadnum < NUM_THREADS                    );
		runs[threadnum]++;
	});

	for (int i = 0; i < NUM_THREADS; i++) {
		CHECK((runs[i] == 1 || (i == 0 && NUM_THREADS != 1)));
	}
}

TEST_CASE("test_parallel_reduce")
{
	LOG("[%s::test_parallel_reduce]", __func__);

	const auto ReduceFunc = [](int a, std::shared_ptr< std::future<int> >& b) -> int { return (a + (b.get())->get()); };
	const auto TestFunc = []() -> int {
		const int threadnum = ThreadPool::GetThreadNum();
		SAFE_CHECK(threadnum >= 0);
		SAFE_CHECK(threadnum < NUM_THREADS);
		return threadnum;
	};

	const int result = parallel_reduce(TestFunc, ReduceFunc);
	CHECK(result == ((NUM_THREADS - 1) * ((NUM_THREADS - 1) + 1)) / 2);
}

TEST_CASE("test_nested_for_mt")
{
	LOG("[%s::test_nested_for_mt]", __func__);

	for_mt(0, 100, [&](const int y) {
		for_mt(0, 100, [&](const int x) {
			const int threadnum = ThreadPool::GetThreadNum();
			SAFE_CHECK(threadnum < NUM_THREADS);
			SAFE_CHECK(threadnum >= 0);
		});
	});
}

TEST_CASE("test_nested_parallel")
{
	#if 0
	parallel([&] {
		parallel([&] {
			const int threadnum = ThreadPool::GetThreadNum();
			SAFE_CHECK(threadnum >= 0);
			SAFE_CHECK(threadnum < NUM_THREADS);
		});
	});
	#endif
}

TEST_CASE("test_throw_for_mt")
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

TEST_CASE("test_null_for_mt")
{
	for_mt(0, -100, [&](const int i) {
		SAFE_CHECK(false); // shouldn't be called once
	});
	for_mt(0, 0, [&](const int i) {
		SAFE_CHECK(false); // shouldn't be called once
	});
	for_mt(100, 10, -1, [&](const int i) {
		SAFE_CHECK(false); // shouldn't be called once
	});
}


TEST_CASE("test_sse_for_mt")
{
	LOG("[%s::test_sse_for_mt]", __func__);

	std::vector<CGlobalUnsyncedRNG> rngs(NUM_THREADS);

	for (size_t n = 0; n < rngs.size(); n++)
		rngs[n].Seed(n);

	for_mt(0, 1000000, [&](const int i) {
		const float r = rngs[ThreadPool::GetThreadNum()].NextFloat() * 1000.0f;
		const float s = math::sqrt(r); // test SSE intrinsics (should be 100% reentrant)

		SAFE_CHECK(!std::isinf(s));
		SAFE_CHECK(!std::isnan(s));
	});
}





//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Performance Benchmarks below
///

static void for_vs_for_mt_kernel(const int numRuns, const spring_time kernelLoad)
{
	LOG("\t[%s] running %.3fms kernel for %i runs (%.0fms total runtime):", __func__, kernelLoad.toMilliSecsf(), numRuns, (kernelLoad * numRuns).toMilliSecsf());

	spring_time t_for;
	spring_time t_formt;

	const auto& ExecKernel = [](const spring_time t) {
		const spring_time finish = spring_now() + t;
		while (spring_now() < finish) {}
	};

	{
		const spring_time start = spring_now();

		for (int i = 0; i < numRuns; ++i) {
			ExecKernel(kernelLoad);
		}

		t_for = (spring_now() - start);
	}
	{
		const spring_time start = spring_now();

		for_mt(0, numRuns, [&](const int i) {
			ExecKernel(kernelLoad);
		});

		t_formt = (spring_now() - start);
	}

	LOG("\t\tfor    took %.4fms", t_for.toMilliSecsf());
	LOG("\t\tfor_mt took %.4fms", t_formt.toMilliSecsf());
	LOG("\t\tmt runtime: %.0f%%", (t_formt.toMilliSecsf() / t_for.toMilliSecsf()) * 100.0f);
}

TEST_CASE("test_for_vs_for_mt")
{
	for_vs_for_mt_kernel(100,  spring_time::fromMicroSecs(10));
	for_vs_for_mt_kernel(1000, spring_time::fromMicroSecs(5));
	for_vs_for_mt_kernel(100,  spring_time::fromMicroSecs(50));
	for_vs_for_mt_kernel(10,   spring_time::fromMicroSecs(500));
	for_vs_for_mt_kernel(10,   spring_time::fromMicroSecs(1000));
}


static void test_parallel_reaction_times_aux(int numRuns)
{
	LOG("\t[%s]", __func__);

	std::vector<float> totalWakeupTimes(ThreadPool::MAX_THREADS, 0);

	parallel([&]() {}); // just wake up the threads (to force reloading of the new spin timer)
	spring_time::fromSecs(1).sleep(); // make them sleep again

	for (int i = 0; i < numRuns; ++i) {
		const auto start = spring_now();

		parallel([&]() {
			totalWakeupTimes[ThreadPool::GetThreadNum()] += (spring_now() - start).toMilliSecsf();
		});
	}

	for (int i = 0; i < ThreadPool::GetNumThreads(); ++i) {
		LOG("\t\tparallel-thread %i: %.6fms (%d runs)", i, totalWakeupTimes[i] / numRuns, numRuns);
	}
}


TEST_CASE("test_parallel_reaction_times")
{
	LOG("[%s::test_parallel_reaction_times]", __func__);

	for (int i: {0, 1, 2, 3, 4}) {
		test_parallel_reaction_times_aux(NUM_RUNS * 2);
	}
}


TEST_CASE("test_for_mt_reaction_times")
{
	constexpr int RUNS = 10;

	LOG("[%s::test_for_mt_reaction_times]", __func__);

	std::vector<float> wakeupTimes(RUNS, 0);
	std::vector<int  > wakeupThread(RUNS, 0);

	ThreadPool::SetThreadCount(ThreadPool::GetMaxThreads());

	for (const char* s: {"cold", "hot"}) {
		const spring_time start = spring_now();

		for_mt(0, RUNS, [&](const int i) {
			wakeupTimes[i] = (spring_now() - start).toMilliSecsf();
			wakeupThread[i] = ThreadPool::GetThreadNum();
		});

		LOG("\treaction times (%s)", s);

		for (int i = 0; i < RUNS; ++i) {
			LOG("\t\tfor_mt %i: %.6fms (handled on thread %i)", i, wakeupTimes[i], wakeupThread[i]);
		}
	}
}


TEST_CASE("test_parallel_gtn_cost")
{
	std::vector<float> costs(NUM_THREADS);
	std::atomic<int> threads(0);

	parallel([&]() {
		const spring_time start = spring_now();
		const int i = ThreadPool::GetThreadNum();

		costs[i] = (spring_now() - start).toMilliSecsf();
		threads++;
	});

	float totalCost = 0.0f;
	for (float f: costs) {
		totalCost += f;
	}

	LOG("[%s::test_parallel_gtn_cost] %.6fms (avg)", __func__, totalCost / threads);
}

TEST_CASE("Cleanup")
{
	ThreadPool::SetThreadCount(0);
	CHECK(ThreadPool::GetNumThreads() == 1);
}

