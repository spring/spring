/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TRACE_RAY_H
#define _TRACE_RAY_H

class float3;
class CUnit;
class CFeature;
class CSolidObject;
struct CollisionQuery;

namespace Collision {
	enum {
		NOENEMIES    = 1,
		NOFRIENDLIES = 2,
		NOFEATURES   = 4,
		NONEUTRALS   = 8,
		NOGROUND     = 16,
		BOOLEAN      = 32, // skip further testings after the first collision
		NOUNITS = NOENEMIES | NOFRIENDLIES | NONEUTRALS
	};
}

namespace TraceRay {
	float TraceRay(
		const float3& start,
		const float3& dir,
		float length,
		int avoidFlags,
		const CUnit* owner,
		CUnit*& hitUnit,
		CFeature*& hitFeature,
		CollisionQuery* hitColQuery = 0x0
	);
	float GuiTraceRay(
		const float3& start,
		const float3& dir,
		const float length,
		const CUnit* exclude,
		CUnit*& hitUnit,
		CFeature*& hitFeature,
		bool useRadar,
		bool groundOnly = false,
		bool ignoreWater = false
	);

	/**
	 * @return true if there is an object (allied/neutral unit, feature)
	 * within the firing cone of \<owner\> (that might be hit)
	 */
	bool TestCone(
		const float3& from,
		const float3& dir,
		float length,
		float spread,
		int allyteam,
		int avoidFlags,
		CUnit* owner);

	/**
	 * @return true if there is an object (allied/neutral unit, feature)
	 *  within the firing trajectory of \<owner\> (that might be hit)
	 */
	bool TestTrajectoryCone(
		const float3& from,
		const float3& dir,
		float length,
		float linear,
		float quadratic,
		float spread,
		int allyteam,
		int avoidFlags,
		CUnit* owner);
}

#endif // _TRACE_RAY_H
