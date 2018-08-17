/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Misc/RectangleOverlapHandler.h"
#include "System/Log/ILog.h"
#include <vector>
#include <stdlib.h>
#include <time.h>

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"

static inline float randf()
{
	return rand() / float(RAND_MAX);
}

static const int testRuns = 1000;
static const int size = 100;
static const int count_rects = 15;


int Test(std::vector<int>& testMap, CRectangleOverlapHandler& ro)
{
	//! clear testMap
	testMap.resize(size * size, 0);

	//! create random rectangles
	ro.clear();
	for (int i=0; i<count_rects; ++i) {
		SRectangle r(0,0,0,0);
		r.x1 = randf() * (size-1);
		r.z1 = randf() * (size-1);
		r.x2 = randf() * (size-1);
		r.z2 = randf() * (size-1);
		if (r.x1 > r.x2) std::swap(r.x1, r.x2);
		if (r.z1 > r.z2) std::swap(r.z1, r.z2);
		ro.push_back(r);
	}

	//! fill testMap with original areas
	for (CRectangleOverlapHandler::iterator it = ro.begin(); it != ro.end(); ++it) {
		const SRectangle& rect = *it;
		for (int z=rect.z1; z<rect.z2; ++z) { //FIXME <=
			for (int x=rect.x1; x<rect.x2; ++x) { //FIXME <=
				testMap[z * size + x] = 1;
			}
		}
	}

	//! optimize
	ro.Process();

	//! fill testMap with optimized
	for (CRectangleOverlapHandler::iterator it = ro.begin(); it != ro.end(); ++it) {
		const SRectangle& rect = *it;
		for (int z=rect.z1; z<rect.z2; ++z) { //FIXME <=
			for (int x=rect.x1; x<rect.x2; ++x) { //FIXME <=
				testMap[z * size + x] -= 1;
			}
		}
	}

	//! check if we have overlapping or missing areas
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


static inline bool TestArea(std::vector<int>& testMap, CRectangleOverlapHandler& ro)
{
	for (int i=0; i<testRuns; ++i) {
		if (Test(testMap, ro) > 0)
			return false;
	}
	return true;
}


static inline bool TestOverlapping(std::vector<int>& testMap, CRectangleOverlapHandler& ro)
{
	for (int i=0; i<testRuns; ++i) {
		if (Test(testMap, ro) < 0)
			return false;
	}
	return true;
}


TEST_CASE("RectangleOverlapHandler")
{
	srand( time(NULL) );

	std::vector<int> testMap(size * size, 0);
	CRectangleOverlapHandler ro;

	INFO("Optimized rectangles don't cover the same area!");
	CHECK(TestArea(testMap, ro));
	INFO("Optimized rectangles still overlap!");
	CHECK(TestOverlapping(testMap, ro));
}
