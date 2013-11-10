/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MYMATH_H
#define MYMATH_H

#include "Sim/Misc/GlobalConstants.h"
#include "System/type2.h"
#include "System/float3.h"

#include <algorithm> // std::{min,max}

#ifdef __GNUC__
	#define _const __attribute__((const))
	#define _pure __attribute__((pure))
	#define _warn_unused_result __attribute__((warn_unused_result))
#else
	#define _const
	#define _pure
	#define _warn_unused_result
#endif



static const float INVPI  = 1.0f / PI;
static const float TWOPI  = 2.0f * PI;
static const float HALFPI = 0.5f * PI;
static const int SHORTINT_MAXVALUE  = 32768;
static const int SPRING_CIRCLE_DIVS = (SHORTINT_MAXVALUE << 1);

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

class CMyMath {
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
std::pair<float,float> GetMapBoundaryIntersectionPoints(const float3 start, const float3 dir) _pure _warn_unused_result;

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

// template<class T> T mix(const T v1, const T v2, const float a) { return (v1 * (1.0f - a) + v2 * a); }
template<class T> T mix(const T v1, const T v2, const float a) { return (v1 + (v2 - v1) * a); }
template<class T> T Blend(const T v1, const T v2, const float a) { return mix(v1, v2, a); }

int Round(const float f) _const _warn_unused_result;

template<class T> T Square(const T x) { return x*x; }
template<class T> T Clamp(const T v, const T vmin, const T vmax) { return std::min(vmax, std::max(vmin, v)); }
// NOTE: '>' instead of '>=' s.t. Sign(int(true)) != Sign(int(false)) --> zero is negative!
template<class T> T Sign(const T v) { return ((v > T(0)) * T(2) - T(1)); }

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


#include "myMath.inl"

#undef _const

#endif // MYMATH_H
