/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __TRACE_RAY_H__
#define __TRACE_RAY_H__

class float3;
class CUnit;
class CFeature;
class CSolidObject;

namespace Collision {
	enum {
		NOENEMIES    = 1,
		NOFRIENDLIES = 2,
		NOFEATURES   = 4,
		NONEUTRALS   = 8,
		NOGROUND     = 16,
		BOOLEAN      = 32, //skip further testings after the first collision
		NOUNITS = NOENEMIES | NOFRIENDLIES | NONEUTRALS
	};
}

namespace TraceRay {
	float TraceRay(const float3& start, const float3& dir, float length, int collisionFlags, const CUnit* owner, CUnit*& hitUnit, CFeature*& hitFeature);
	float GuiTraceRay(const float3 &start, const float3 &dir, float length, bool useRadar, const CUnit* exclude, CUnit*& hitUnit, CFeature*& hitFeature);

	bool LineFeatureCol(const float3& start, const float3& dir, float length);

	bool TestAllyCone(const float3& from, const float3& weaponDir, float length, float spread, int allyteam, CUnit* owner);
	bool TestNeutralCone(const float3& from, const float3& weaponDir, float length, float spread, CUnit* owner);
	bool TestTrajectoryAllyCone(const float3& from, const float3& flatdir, float length, float linear, float quadratic, float spread, float baseSize, int allyteam, CUnit* owner);
	bool TestTrajectoryNeutralCone(const float3& from, const float3& flatdir, float length, float linear, float quadratic, float spread, float baseSize, CUnit* owner);
}

#endif // __TRACE_RAY_H__
