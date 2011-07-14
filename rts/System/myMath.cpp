/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/myMath.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "Sim/Units/Scripts/CobInstance.h" // for TAANG2RAD (ugh)

float2 CMyMath::headingToVectorTable[NUM_HEADINGS];

void CMyMath::Init()
{
	const unsigned int sseBits = proc::GetProcSSEBits();
		LOG("[CMyMath::Init] CPU SSE mask: %u, flags:\n", sseBits);
		LOG("\tSSE 1.0:  %d,  SSE 2.0:  %d\n", (sseBits >> 5) & 1, (sseBits >> 4) & 1);
		LOG("\tSSE 3.0:  %d, SSSE 3.0:  %d\n", (sseBits >> 3) & 1, (sseBits >> 2) & 1);
		LOG("\tSSE 4.1:  %d,  SSE 4.2:  %d\n", (sseBits >> 1) & 1, (sseBits >> 0) & 1);
		LOG("\tSSE 4.0A: %d,  SSE 5.0A: %d\n", (sseBits >> 8) & 1, (sseBits >> 7) & 1);

#ifdef STREFLOP_H
	// SSE 1.0 is mandatory in synced context
	if (((sseBits >> 5) & 1) == 0) {
		#ifdef STREFLOP_SSE
		handleerror(0, "CPU is missing SSE instruction support", "Sync Error", 0);
		#elif STREFLOP_X87
		LOG_L(L_WARNING, "\tStreflop floating-point math is not SSE-enabled");
		LOG_L(L_WARNING, "\tThis may cause desyncs during multi-player games");
		LOG_L(L_WARNING, "\tThis CPU is not SSE-capable; it can only use X87 mode");
		#else
		handleerror(0, "streflop FP-math mode must be either SSE or X87", "Sync Error", 0);
		#endif
	} else {
		#ifdef STREFLOP_SSE
		LOG("\tusing streflop SSE FP-math mode, CPU supports SSE instructions");
		#elif STREFLOP_X87
		LOG_L(L_WARNING, "\tStreflop floating-point math is set to X87 mode");
		LOG_L(L_WARNING, "\tThis may cause desyncs during multi-player games");
		LOG_L(L_WARNING, "\tThis CPU is SSE-capable; consider recompiling");
		#else
		handleerror(0, "streflop FP-math mode must be either SSE or X87", "Sync Error", 0);
		#endif
	}

	// Set single precision floating point math.
	streflop_init<streflop::Simple>();
#else
	// probably should check if SSE was enabled during
	// compilation and issue a warning about illegal
	// instructions if so (or just die with an error)
	LOG_L(L_WARNING, "Floating-point math is not controlled by streflop");
	LOG_L(L_WARNING, "This makes keeping multi-player sync 99% impossible");
#endif


	for (int a = 0; a < NUM_HEADINGS; ++a) {
		float ang = (a - (NUM_HEADINGS / 2)) * 2 * PI / NUM_HEADINGS;
		float2 v;
			v.x = sin(ang);
			v.y = cos(ang);
		headingToVectorTable[a] = v;
	}

	unsigned checksum = 0;
	for (int a = 0; a < NUM_HEADINGS; ++a) {
		checksum = 33 * checksum + *(unsigned*) &headingToVectorTable[a].x;
		checksum *= 33;
		checksum = 33 * checksum + *(unsigned*) &headingToVectorTable[a].y;
	}

#ifdef STREFLOP_H
	if (checksum != HEADING_CHECKSUM) {
		handleerror(0,
			"Invalid headingToVectorTable checksum. Most likely"
			" your streflop library was not compiled with the correct"
			" options, or you are not using streflop at all.",
			"Sync Error", 0);
	}
#endif
}



float3 GetVectorFromHAndPExact(short int heading, short int pitch)
{
	float3 ret;
	float h = heading * TAANG2RAD;
	float p = pitch * TAANG2RAD;
	ret.x = sin(h) * cos(p);
	ret.y = sin(p);
	ret.z = cos(h) * cos(p);
	return ret;
}

float LinePointDist(const float3& l1, const float3& l2, const float3& p)
{
	float3 dir(l2 - l1);
	float length = dir.Length();
	if (length == 0)
		length = 0.1f;
	dir /= length;

	float a = (p - l1).dot(dir);
	if (a <      0) a =      0;
	if (a > length) a = length;

	float3 p2 = p - dir * a;
	return p2.distance(l1);
}

/**
 * @brief calculate closest point on linepiece from l1 to l2
 * Note, this clamps the returned point to a position between l1 and l2.
 */
float3 ClosestPointOnLine(const float3& l1, const float3& l2, const float3& p)
{
	float3 dir(l2-l1);
	float3 pdir(p-l1);
	float length = dir.Length();
	if (fabs(length) < 1e-4f)
		return l1;
	float c = dir.dot(pdir) / length;
	if (c < 0) c = 0;
	if (c > length) c = length;
	return l1 + dir * (c / length);
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
std::pair<float, float> GetMapBoundaryIntersectionPoints(const float3& start, const float3& dir)
{
	const float rcpdirx = 1.0f / dir.x;
	const float rcpdirz = 1.0f / dir.z;
	float l1, l2, far, near;

	//! x component
	l1 = (           0.0f - start.x) * rcpdirx;
	l2 = (float3::maxxpos - start.x) * rcpdirx;
	near = std::min(l1, l2);
	far = std::max(l1, l2);

	//! z component
	l1 = (           0.0f - start.z) * rcpdirz;
	l2 = (float3::maxzpos - start.z) * rcpdirz;
	near = std::max(std::min(l1, l2), near);
	far = std::min(std::max(l1, l2), far);

	if (far < 0.0f || far < near) {
		//! outside of boundary
		return std::pair<float, float>(-1.0f, -1.0f);
	}
	return std::pair<float, float>(near, far);
}


bool ClampLineInMap(float3& start, float3& end)
{
	const float3 dir = end - start;
	const std::pair<float, float>& interp = GetMapBoundaryIntersectionPoints(start, dir);
	const float& near = interp.first;
	const float& far  = interp.second;

	if (far < 0.0f) {
		//! outside of map!
		start = float3(-1.0f, -1.0f, -1.0f);
		end   = float3(-1.0f, -1.0f, -1.0f);
		return true;
	}

	if (far < 1.0f || near > 0.0f) {
		end   = start + dir * std::min(far, 1.0f);
		start = start + dir * std::max(near, 0.0f);

		//! precision of near,far are limited, so better clamp it afterwards
		end.CheckInBounds();
		start.CheckInBounds();
		return true;
	}
	return false;
}


bool ClampRayInMap(const float3& start, float3& end)
{
	const float3 dir = end - start;
	std::pair<float, float> interp = GetMapBoundaryIntersectionPoints(start, dir);
	const float& near = interp.first;
	const float& far  = interp.second;

	if (far < 0.0f) {
		//! outside of map!
		end = start;
		return true;
	}

	if (far < 1.0f || near > 0.0f) {
		end = start + dir * std::min(far, 1.0f);

		//! precision of near,far are limited, so better clamp it afterwards
		end.CheckInBounds();
		return true;
	}
	return false;
}


float smoothstep(const float edge0, const float edge1, const float value)
{
	if (value<=edge0) return 0.0f;
	if (value>=edge1) return 1.0f;
	float t = (value - edge0) / (edge1 - edge0);
	t = std::min(1.0f,std::max(0.0f, t ));
	return t * t * (3.0f - 2.0f * t);
}


float3 smoothstep(const float edge0, const float edge1, float3 vec)
{
	vec.x = smoothstep(edge0, edge1, vec.x);
	vec.y = smoothstep(edge0, edge1, vec.y);
	vec.z = smoothstep(edge0, edge1, vec.z);
	return vec;
}
