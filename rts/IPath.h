#ifndef IPATH_H
#define IPATH_H

#include "float3.h"
#include <list>

class IPath {
public:
	enum SearchResult {
		Ok,
		GoalOutOfRange,
		CantGetCloser,
		Error
	};

	struct Path {
		//Information about the requested path.
		float3 desiredGoal;
		float goalRadius;
		//Information about the generated path.
		float3 pathGoal;
		std::list<float3> path;
		float pathCost;
	};
};

#endif
