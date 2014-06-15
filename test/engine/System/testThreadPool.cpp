/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/ThreadPool.h"
#include "System/UnsyncedRNG.h"
#include <vector>

#define BOOST_TEST_MODULE ThreadPool
#include <boost/test/unit_test.hpp>

static int NUM_THREADS = std::min(ThreadPool::GetMaxThreads(), 10);

// !!! BOOST.TEST IS NOT THREADSAFE !!! (facepalms)
#define SAFE_BOOST_CHECK( P )           do { boost::lock_guard<boost::mutex> _(m); BOOST_CHECK( ( P ) );               } while( 0 );
static boost::mutex m;


BOOST_AUTO_TEST_CASE( testThreadPool1 )
{
	LOG_L(L_WARNING, "testThreadPool1");

	#define RUNS 5000
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

BOOST_AUTO_TEST_CASE( testThreadPool2 )
{
	LOG_L(L_WARNING, "testThreadPool2");

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
	LOG_L(L_WARNING, "testThreadPool3");

	int result = parallel_reduce([]() -> int {
		const int threadnum = ThreadPool::GetThreadNum();
		SAFE_BOOST_CHECK(threadnum >= 0);
		SAFE_BOOST_CHECK(threadnum < NUM_THREADS);
		return threadnum;
	}, [](int a, boost::unique_future<int>& b) -> int { return a + b.get(); });
	BOOST_CHECK(result == ((NUM_THREADS-1)*((NUM_THREADS-1) + 1))/2);
}

BOOST_AUTO_TEST_CASE( testThreadPool4 )
{
	LOG_L(L_WARNING, "testThreadPool4");

	for_mt(0, 100, [&](const int y) {
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
	/*for_mt(0, 100, [&](const int i) {
		throw std::exception();
	});*/
}

BOOST_AUTO_TEST_CASE( testThreadPool7 )
{
	LOG_L(L_WARNING, "testThreadPool7");

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

struct do_once {
	do_once()   {}
	~do_once()  {
		// workarround boost::condition_variable::~condition_variable(): Assertion `!ret' failed.
		ThreadPool::SetThreadCount(1);
	}
};
BOOST_GLOBAL_FIXTURE(do_once);

