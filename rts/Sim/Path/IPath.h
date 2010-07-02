/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IPATH_H
#define IPATH_H

#include <vector>
#include "float3.h"
#include "Vec2.h"

class IPath {
public:
	virtual ~IPath() {};

	enum SearchResult {
		Ok,
		GoalOutOfRange,
		CantGetCloser,
		Error
	};

	typedef std::vector<float3> path_list_type;
	typedef std::vector<int2> square_list_type;

	struct Path {
		//Information about the requested path.
		float3 desiredGoal;
		float goalRadius;
		//Information about the generated path.
		float3 pathGoal;
		path_list_type path;
		square_list_type squares;
		float pathCost;
	};
};

#endif
