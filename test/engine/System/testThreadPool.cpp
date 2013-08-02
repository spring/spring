/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/ThreadPool.h"
#include <vector>

#define BOOST_TEST_MODULE ThreadPool
#include <boost/test/unit_test.hpp>

#define NUM_THREADS 10

BOOST_AUTO_TEST_CASE( ThreadPoolTest )
{
	std::vector<int> nums(1000);
	std::vector<int> runs(NUM_THREADS);
	ThreadPool::SetThreadCount(NUM_THREADS);
	BOOST_CHECK(ThreadPool::GetNumThreads() == NUM_THREADS);
	for_mt(0, nums.size(), [&](const int y) {
		const int threadnum = ThreadPool::GetThreadNum();
		BOOST_CHECK(threadnum >= 0);
		BOOST_CHECK(threadnum < NUM_THREADS);
		for(int i=0; i<=y; i++) {
			nums[y] = i;
		}

		runs[threadnum]++;
	});

	for(int i=0; i<nums.size(); i++) {
		BOOST_CHECK(nums[i] == i);
	}

	int rounds = 0;
	for(int i=0; i<runs.size(); i++) {
		printf("Thread %d executed %d times\n", i, runs[i]);
		rounds += runs[i];
	}
	BOOST_CHECK(rounds = nums.size());
}


