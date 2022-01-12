/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_LUA_GARBAGE_COLLECT_CTRL_H
#define SPRING_LUA_GARBAGE_COLLECT_CTRL_H

#include <limits>

struct SLuaGarbageCollectCtrl {
	// maximum number of lua_gc calls made in each CollectGarbage loop
	int itersPerBatch = std::numeric_limits<int>::max();

	// number of steps executed by a single lua_gc call
	int numStepsPerIter =    10;
	int minStepsPerIter =     1;
	int maxStepsPerIter = 10000;

	// CollectGarbage loop runtime bounds, in milliseconds
	float minLoopRunTime =   0.0f;
	float maxLoopRunTime = 100.0f;

	float baseRunTimeMult = 0.0f;
	float baseMemLoadMult = 0.0f;
};

#endif

