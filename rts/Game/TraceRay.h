/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TRACE_RAY_H
#define _TRACE_RAY_H

#include <vector>

class float3;
class CUnit;
class CFeature;
class CWeapon;
class CPlasmaRepulser;
class CSolidObject;
struct CollisionQuery;

namespace Collision {
	enum {
		NOENEMIES    = 1,
		NOFRIENDLIES = 2,
		NOFEATURES   = 4,
		NONEUTRALS   = 8,
		NOGROUND     = 16,
		NOCLOAKED    = 32,
		BOOLEAN      = 64, // skip further tests after the first collision (unused)
		NOUNITS = NOENEMIES | NOFRIENDLIES | NONEUTRALS
	};
}

namespace TraceRay {
	struct SShieldDist {
		CPlasmaRepulser* rep;
		float dist;
	};

	// TODO: extend with allyTeam param s.t. we can add Collision::NO{LOS,RADAR}, etc.
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

	void TraceRayShields(
		const CWeapon* emitter,
		const float3& start,
		const float3& dir,
		float length,
		std::vector<SShieldDist>& hitShields
	);

	float GuiTraceRay(
		const float3& start,
		const float3& dir,
		const float length,
		const CUnit* exclude,
		const CUnit*& hitUnit,
		const CFeature*& hitFeature,
		bool useRadar,
		bool groundOnly = false,
		bool ignoreWater = true
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
