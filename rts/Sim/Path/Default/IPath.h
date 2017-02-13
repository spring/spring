/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IPATH_H
#define IPATH_H

#include <vector>
#include "System/float3.h"
#include "System/type2.h"

namespace IPath {
	// note: ordered from best to worst
	enum SearchResult {
		Ok,
		CantGetCloser,
		GoalOutOfRange,
		Error
	};

	typedef std::vector<float3> path_list_type;
	typedef std::vector<int2> square_list_type;

	struct Path {
		Path()
			: goalRadius(-1.0f)
			, pathCost(-1.0f)
		{}
		Path(const Path& p) { *this = p; }
		Path(Path&& p) { *this = std::move(p); }

		Path& operator = (const Path& p) {
			path    = p.path;
			squares = p.squares;

			desiredGoal = p.desiredGoal;
			pathGoal    = p.pathGoal;

			goalRadius = p.goalRadius;
			pathCost   = p.pathCost;
			return *this;
		}
		Path& operator = (Path&& p) {
			path    = std::move(p.path);
			squares = std::move(p.squares);

			desiredGoal = p.desiredGoal;
			pathGoal    = p.pathGoal;

			goalRadius = p.goalRadius;
			pathCost   = p.pathCost;
			return *this;
		}

		path_list_type path;
		square_list_type squares;

		float3 desiredGoal; // requested goal-position
		float3 pathGoal; // generated goal-position

		float goalRadius;
		float pathCost;
	};
}

#endif
