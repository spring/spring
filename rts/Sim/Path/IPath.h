#ifndef IPATH_H
#define IPATH_H

#include "float3.h"
#include <vector>

class IPath {
public:
	enum SearchResult {
		Ok,
		GoalOutOfRange,
		CantGetCloser,
		Error
	};

	typedef std::vector<float3> path_list_type;

	struct Path {
		//Information about the requested path.
		float3 desiredGoal;
		float goalRadius;
		//Information about the generated path.
		float3 pathGoal;
		path_list_type path;
		float pathCost;
	};
};

#endif
