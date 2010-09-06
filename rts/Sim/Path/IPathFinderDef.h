/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_PATH_FINDER_DEF_H
#define I_PATH_FINDER_DEF_H

#include "float3.h"

class IPathFinderDef {
	public:
		IPathFinderDef() {}
		IPathFinderDef(float3 goalCenter, float goalRadius);
		virtual ~IPathFinderDef() {}

	protected:
		float3 goal;
		float sqGoalRadius;
};

#endif
