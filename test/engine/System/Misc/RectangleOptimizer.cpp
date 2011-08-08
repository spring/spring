/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Misc/RectangleOptimizer.h"
#include "System/Log/ILog.h"
#include <vector>
#include <stdlib.h>
#include <time.h>

#define BOOST_TEST_MODULE RectangleOptimizer
#include <boost/test/unit_test.hpp>

float randf()
{
	return rand() / float(RAND_MAX);
}

static const int testRuns = 2000;
static const int size = 100;
static const int count_rects = 15;

static CRectangleOptimizer ro;
static std::vector<int> testMap(size * size, 0);

int Test()
{
	//! clear testMap
	testMap.resize(size * size, 0);

	//! create random rectangles
	ro.clear();
	for (int i=0; i<count_rects; ++i) {
		Rectangle r(0,0,0,0);
		r.x1 = randf() * (size-1);
		r.z1 = randf() * (size-1);
		r.x2 = randf() * (size-1);
		r.z2 = randf() * (size-1);
		if (r.x1 > r.x2) std::swap(r.x1, r.x2);
		if (r.z1 > r.z2) std::swap(r.z1, r.z2);
		ro.push_back(r);
	}

	//! fill testMap with original areas
	for (CRectangleOptimizer::iterator it = ro.begin(); it != ro.end(); ++it) {
		const Rectangle& rect = *it;
		for (int z=rect.z1; z<rect.z2; ++z) { //FIXME <=
			for (int x=rect.x1; x<rect.x2; ++x) { //FIXME <=
				testMap[z * size + x] = 1;
			}
		}
	}

	//! optimize
	ro.Update();

	//! fill testMap with optimized
	for (CRectangleOptimizer::iterator it = ro.begin(); it != ro.end(); ++it) {
		const Rectangle& rect = *it;
		for (int z=rect.z1; z<rect.z2; ++z) { //FIXME <=
			for (int x=rect.x1; x<rect.x2; ++x) { //FIXME <=
				testMap[z * size + x] -= 1;
			}
		}
	}

	//! check if we have overlapping or any missing areas
	int sum = 0;
	for (int y=0; y<size; ++y) {
		for (int x=0; x<size; ++x) {
			sum += testMap[y * size + x];
		}
	}

	//! sum should be zero
	//! in case of <0: the optimized rectangles still overlap
	//! in case of >0: the optimized rectangles don't cover the same area as the unoptimized did
	return sum;
}


static inline bool TestArea()
{
	srand( time(NULL) );

	for (int i=0; i<testRuns; ++i) {
		if (Test() > 0)
			return false;
	}
	return true;
}


static inline bool TestOverlapping()
{
	srand( time(NULL) );

	for (int i=0; i<testRuns; ++i) {
		if (Test() < 0)
			return false;
	}
	return true;
}


BOOST_AUTO_TEST_CASE( RectangleOptimizer )
{
	BOOST_CHECK_MESSAGE(TestArea(), "Optimized rectangles don't cover the same area!");
	BOOST_CHECK_MESSAGE(TestOverlapping(), "Optimized rectangles still overlap!");
}
