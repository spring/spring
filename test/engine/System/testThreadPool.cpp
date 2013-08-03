/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/ThreadPool.h"
#include <vector>

#define BOOST_TEST_MODULE ThreadPool
#include <boost/test/unit_test.hpp>

#define NUM_THREADS 10

// !!! BOOST.TEST IS NOT THREADSAFE !!! (facepalms)
#define SAFE_BOOST_CHECK( P )           do { boost::lock_guard<boost::mutex> _(m); BOOST_CHECK( ( P ) );               } while( 0 );
static boost::mutex m;


BOOST_AUTO_TEST_CASE( testThreadPool1 )
{
	#define RUNS 1000
	std::atomic<int> cnt(0);
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
		LOG("Thread %d executed %d times", i, runs[i]);
		rounds += runs[i];
	}
	BOOST_CHECK(rounds == (RUNS/2));

	for(int i=0; i<RUNS; i++) {
		BOOST_CHECK((i % 2) == (1 - nums[i]));
	}
}

BOOST_AUTO_TEST_CASE( testThreadPool2 )
{
	std::vector<int> runs(NUM_THREADS);
	ThreadPool::SetThreadCount(NUM_THREADS);
	BOOST_CHECK(ThreadPool::GetNumThreads() == NUM_THREADS);
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
	std::vector<int> runs(NUM_THREADS);
	ThreadPool::SetThreadCount(NUM_THREADS);
	BOOST_CHECK(ThreadPool::GetNumThreads() == NUM_THREADS);
	int result = parallel_reduce([]() -> int {
		return ThreadPool::GetThreadNum();
	}, [](int a, boost::unique_future<int>& b) -> int { return a + b.get(); });
	BOOST_CHECK(result == ((NUM_THREADS-1)*((NUM_THREADS-1) + 1))/2);
}

