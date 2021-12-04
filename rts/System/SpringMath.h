/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_MATH_H
#define SPRING_MATH_H

#include "Sim/Misc/GlobalConstants.h"
#include "System/type2.h"
#include "System/float3.h"
#include "System/MathConstants.h"

#include <cmath> // std::fabs
#include <algorithm> // std::{min,max}

static constexpr int SPRING_MAX_HEADING = 32768;
static constexpr int SPRING_CIRCLE_DIVS = (SPRING_MAX_HEADING << 1);

#define HEADING_CHECKSUM_1024 0x617a9968
#define HEADING_CHECKSUM_4096 0x3d51b476
#define NUM_HEADINGS 4096

#if (NUM_HEADINGS == 1024)
	#define HEADING_CHECKSUM  HEADING_CHECKSUM_1024
#elif (NUM_HEADINGS == 4096)
	#define HEADING_CHECKSUM  HEADING_CHECKSUM_4096
#else
	#error "HEADING_CHECKSUM not set, invalid NUM_HEADINGS?"
#endif

enum FacingMap {
	FACING_NORTH = 2,
	FACING_SOUTH = 0,
	FACING_EAST  = 1,
	FACING_WEST  = 3,
	NUM_FACINGS  = 4,
};

class SpringMath {
public:
	static void Init();
	static float2 headingToVectorTable[NUM_HEADINGS];
};


struct shortint2 {
	short int x, y;
};


short int GetHeadingFromFacing(const int facing) _pure _warn_unused_result;
int GetFacingFromHeading(const short int heading) _pure _warn_unused_result;
float GetHeadingFromVectorF(const float dx, const float dz) _pure _warn_unused_result;
short int GetHeadingFromVector(const float dx, const float dz) _pure _warn_unused_result;
shortint2 GetHAndPFromVector(const float3 vec) _pure _warn_unused_result; // vec should be normalized
float2 GetHAndPFromVectorF(const float3 vec) _pure _warn_unused_result; // vec should be normalized
float3 GetVectorFromHeading(const short int heading) _pure _warn_unused_result;
float3 GetVectorFromHAndPExact(const short int heading, const short int pitch) _pure _warn_unused_result;

float3 CalcBeizer(const float i, const float3 p1, const float3 p2, const float3 p3, const float3 p4) _pure _warn_unused_result;

float LinePointDist(const float3 l1, const float3 l2, const float3 p) _pure _warn_unused_result;
float3 ClosestPointOnLine(const float3 l1, const float3 l2, const float3 p) _pure _warn_unused_result;

/**
 * @brief Returns the intersection points of a ray with the map boundary (2d only)
 * @param start float3 the start point of the line
 * @param dir float3 direction of the ray
 * @return <near,far> std::pair<float,float> distance to the intersection points in mulitples of `dir`
 */
float2 GetMapBoundaryIntersectionPoints(const float3 start, const float3 dir) _pure _warn_unused_result;

/**
 * @brief clamps a line (start & end points) to the map boundaries
 * @param start float3 the start point of the line
 * @param end float3 the end point of the line
 * @return true if either `end` or `start` was changed
 */
bool ClampLineInMap(float3& start, float3& end);

/**
 * @brief clamps a ray (just the end point) to the map boundaries
 * @param start const float3 the start point of the line
 * @param end float3 the `end` point of the line
 * @return true if changed
 */
bool ClampRayInMap(const float3 start, float3& end);


float smoothstep(const float edge0, const float edge1, const float value) _pure _warn_unused_result;
float3 smoothstep(const float edge0, const float edge1, float3 vec) _pure _warn_unused_result;

float linearstep(const float edge0, const float edge1, const float value) _pure _warn_unused_result;


#ifndef FAST_EPS_CMP
template<class T> inline bool epscmp(const T a, const T b, const T eps) {
	return ((a == b) || (math::fabs(a - b) <= (eps * std::max(std::max(math::fabs(a), math::fabs(b)), T(1)))));
}
#else
template<class T> inline bool epscmp(const T a, const T b, const T eps) {
	return ((a == b) || (std::fabs(a - b) <= (eps * (T(1) + std::fabs(a) + std::fabs(b)))));
}
#endif


// inlined to avoid multiple definitions due to the specializing templates
template<class T> inline T argmin(const T v1, const T v2) { return std::min(v1, v2); }
template<class T> inline T argmax(const T v1, const T v2) { return std::max(v1, v2); }
template<> inline float3 argmin(const float3 v1, const float3 v2) { return float3::min(v1, v2); }
template<> inline float3 argmax(const float3 v1, const float3 v2) { return float3::max(v1, v2); }

// template<class T> T mix(const T v1, const T v2, const float a) { return (v1 * (1.0f - a) + v2 * a); }
template<class T, typename T2> constexpr T mix(const T v1, const T v2, const T2 a) { return (v1 + (v2 - v1) * a); }
template<class T> constexpr T Blend(const T v1, const T v2, const float a) { return mix(v1, v2, a); }

int Round(const float f) _const _warn_unused_result;

template<class T> constexpr T Square(const T x) { return x*x; }
template<class T> constexpr T Clamp(const T v, const T vmin, const T vmax) { return std::min(vmax, std::max(vmin, v)); }
// NOTE: '>' instead of '>=' s.t. Sign(int(true)) != Sign(int(false)) --> zero is negative!
template<class T> constexpr T Sign(const T v) { return ((v > T(0)) * T(2) - T(1)); }

/**
 * @brief does a division and returns additionally the remnant
 */
int2 IdxToCoord(unsigned x, unsigned array_width) _const _warn_unused_result;


/**
 * @brief Clamps an radian angle between 0 .. 2*pi
 * @param f float* value to clamp
 */
float ClampRad(float f) _const _warn_unused_result;


/**
 * @brief Clamps an radian angle between 0 .. 2*pi
 * @param f float* value to clamp
 */
void ClampRad(float* f);


/**
 * @brief Checks if 2 radian values discribe the same angle
 * @param f1 float* first compare value
 * @param f2 float* second compare value
 */
bool RadsAreEqual(const float f1, const float f2) _const;


/**
 */
float GetRadFromXY(const float dx, const float dy) _const;


/**
 * convert a color in HS(V) space to RGB
 */
float3 hs2rgb(float h, float s) _pure _warn_unused_result;


#include "SpringMath.inl"

#undef _const

#endif // MYMATH_H
