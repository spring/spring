/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathFinderDef.h"

IPathFinderDef::IPathFinderDef(float3 goalCenter, float goalRadius) {
	goal = goalCenter;
	sqGoalRadius = goalRadius;
}
