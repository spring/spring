/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IPATH_H
#define IPATH_H

#include <vector>
#include "System/float3.h"
#include "System/Vec2.h"

namespace IPath {
	enum SearchResult {
		Ok,
		GoalOutOfRange,
		CantGetCloser,
		Error
	};

	typedef std::vector<float3> path_list_type;
	typedef std::vector<int2> square_list_type;

	struct Path {
		Path()
			: desiredGoal(ZeroVector)
			, goalRadius(-1.0f)
			, pathGoal(ZeroVector)
			, pathCost(-1.0f)
		{}
		// Information about the requested path.
		float3 desiredGoal;
		float goalRadius;
		// Information about the generated path.
		float3 pathGoal;
		path_list_type path;
		square_list_type squares;
		float pathCost;
	};
};

#endif
