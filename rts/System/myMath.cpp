/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef USE_VALGRIND
	#include <valgrind/valgrind.h>
#endif

#include "System/myMath.h"
#include "System/Exceptions.h"
#include "System/Sync/FPUCheck.h"
#include "System/Log/ILog.h"
#include "Sim/Units/Scripts/CobInstance.h" // for TAANG2RAD (ugh)

#undef far
#undef near

float2 CMyMath::headingToVectorTable[NUM_HEADINGS];

void CMyMath::Init()
{
	good_fpu_init();

	for (int a = 0; a < NUM_HEADINGS; ++a) {
		float ang = (a - (NUM_HEADINGS / 2)) * math::TWOPI / NUM_HEADINGS;
		float2 v;
			v.x = math::sin(ang);
			v.y = math::cos(ang);
		headingToVectorTable[a] = v;
	}

	unsigned checksum = 0;
	for (int a = 0; a < NUM_HEADINGS; ++a) {
		checksum = 33 * checksum + *(unsigned*) &headingToVectorTable[a].x;
		checksum *= 33;
		checksum = 33 * checksum + *(unsigned*) &headingToVectorTable[a].y;
	}

#ifdef USE_VALGRIND
	if (RUNNING_ON_VALGRIND) {
		// Valgrind doesn't allow us setting the FPU, so syncing is impossible
		LOG_L(L_WARNING, "Valgrind detected sync checking disabled!");
		return;
	}
#endif

#if STREFLOP_ENABLED
	if (checksum != HEADING_CHECKSUM) {
		throw unsupported_error(
			"Invalid headingToVectorTable checksum. Most likely"
			" your streflop library was not compiled with the correct"
			" options, or you are not using streflop at all.");
	}
#endif
}



float3 GetVectorFromHAndPExact(const short int heading, const short int pitch)
{
	float3 ret;
	float h = heading * TAANG2RAD;
	float p = pitch * TAANG2RAD;
	ret.x = math::sin(h) * math::cos(p);
	ret.y = math::sin(p);
	ret.z = math::cos(h) * math::cos(p);
	return ret;
}

float LinePointDist(const float3 l1, const float3 l2, const float3 p)
{
	const float3 dir = (l2 - l1).SafeNormalize();
	const float3 vec = dir * Clamp(dir.dot(p - l1), 0.0f, dir.dot(l2 - l1));
	const float3  p2 = p - vec;
	return (p2.distance(l1));
}

/**
 * @brief calculate closest point on linepiece from l1 to l2
 * Note, this clamps the returned point to a position between l1 and l2.
 */
float3 ClosestPointOnLine(const float3 l1, const float3 l2, const float3 p)
{
	const float3 ldir(l2 - l1);
	const float3 pdir( p - l1);

	const float length = ldir.Length();

	if (length < 1e-4f)
		return l1;

	const float pdist = ldir.dot(pdir) / length;
	const float cdist = Clamp(pdist, 0.0f, length);

	return (l1 + ldir * (cdist / length));
}


/**
 * How does it works?
 * We calculate the two intersection points ON the
 * ray as multiple of `dir` starting from `start`.
 * So we get 2 scalars, whereupon `near` defines the
 * new startpoint and `far` defines the new endpoint.
 *
 * credits:
 * http://ompf.org/ray/ray_box.html
 */
float2 GetMapBoundaryIntersectionPoints(const float3 start, const float3 dir)
{
	const float rcpdirx = (dir.x != 0.0f)? (1.0f / dir.x): 10000.0f;
	const float rcpdirz = (dir.z != 0.0f)? (1.0f / dir.z): 10000.0f;

	const float mapwidth  = float3::maxxpos + 1.0f;
	const float mapheight = float3::maxzpos + 1.0f;

	// x-component
	float xl1 = (    0.0f - start.x) * rcpdirx;
	float xl2 = (mapwidth - start.x) * rcpdirx;
	float xnear = std::min(xl1, xl2);
	float xfar  = std::max(xl1, xl2);

	// z-component
	float zl1 = (     0.0f - start.z) * rcpdirz;
	float zl2 = (mapheight - start.z) * rcpdirz;
	float znear = std::min(zl1, zl2);
	float zfar  = std::max(zl1, zl2);

	// both
	float near = std::max(xnear, znear);
	float far  = std::min(xfar, zfar);

	if (far < 0.0f || far < near) {
		// outside of boundary
		near = -1.0f;
		far = -1.0f;
	}

	return {near, far};
}


bool ClampLineInMap(float3& start, float3& end)
{
	const float3 dir = end - start;
	const float2 ips = GetMapBoundaryIntersectionPoints(start, dir);

	const float near = ips.x;
	const float far  = ips.y;

	if (far < 0.0f) {
		//! outside of map!
		start = -OnesVector;
		end   = -OnesVector;
		return true;
	}

	if (far < 1.0f || near > 0.0f) {
		end   = start + dir * std::min(far, 1.0f);
		start = start + dir * std::max(near, 0.0f);

		//! precision of near,far are limited, so better clamp it afterwards
		end.ClampInMap();
		start.ClampInMap();
		return true;
	}

	return false;
}


bool ClampRayInMap(const float3 start, float3& end)
{
	const float3 dir = end - start;
	const float2 ips = GetMapBoundaryIntersectionPoints(start, dir);

	const float near = ips.x;
	const float far  = ips.y;

	if (far < 0.0f) {
		//! outside of map!
		end = start;
		return true;
	}

	if (far < 1.0f || near > 0.0f) {
		end = start + dir * std::min(far, 1.0f);

		//! precision of near,far are limited, so better clamp it afterwards
		end.ClampInMap();
		return true;
	}

	return false;
}


float smoothstep(const float edge0, const float edge1, const float value)
{
	if (value <= edge0) return 0.0f;
	if (value >= edge1) return 1.0f;
	const float x = (value - edge0) / (edge1 - edge0);
	const float t = Clamp(x, 0.0f, 1.0f);
	return (t * t * (3.0f - 2.0f * t));
}

float3 smoothstep(const float edge0, const float edge1, float3 vec)
{
	vec.x = smoothstep(edge0, edge1, vec.x);
	vec.y = smoothstep(edge0, edge1, vec.y);
	vec.z = smoothstep(edge0, edge1, vec.z);
	return vec;
}

float linearstep(const float edge0, const float edge1, const float value)
{
	if (value <= edge0) return 0.0f;
	if (value >= edge1) return 1.0f;
	const float x = (value - edge0) / (edge1 - edge0);
	const float t = Clamp(x, 0.0f, 1.0f);
	return t;
}


float3 hs2rgb(float h, float s)
{
	// FIXME? ignores saturation completely
	s = 1.0f;

	const float invSat = 1.0f - s;

	if (h > 0.5f) { h += 0.1f; }
	if (h > 1.0f) { h -= 1.0f; }

	float3 col(invSat / 2.0f, invSat / 2.0f, invSat / 2.0f);

	if (h < (1.0f / 6.0f)) {
		col.x += s;
		col.y += s * (h * 6.0f);
	} else if (h < (1.0f / 3.0f)) {
		col.y += s;
		col.x += s * ((1.0f / 3.0f - h) * 6.0f);
	} else if (h < (1.0f / 2.0f)) {
		col.y += s;
		col.z += s * ((h - (1.0f / 3.0f)) * 6.0f);
	} else if (h < (2.0f / 3.0f)) {
		col.z += s;
		col.y += s * ((2.0f / 3.0f - h) * 6.0f);
	} else if (h < (5.0f / 6.0f)) {
		col.z += s;
		col.x += s * ((h - (2.0f / 3.0f)) * 6.0f);
	} else {
		col.x += s;
		col.z += s * ((3.0f / 3.0f - h) * 6.0f);
	}

	return col;
}

