/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MYMATH_H
#define MYMATH_H

#include "Sim/Misc/GlobalConstants.h"
#include "Vec2.h"
#include "float3.h"

#ifdef __GNUC__
	#define _const __attribute__((const))
#else
	#define _const 
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
	FACING_WEST  = 3
};

class CMyMath {
public:
	static void Init();
	static float2 headingToVectorTable[NUM_HEADINGS];
};


struct shortint2 {
	short int x, y;
};


short int GetHeadingFromFacing(int facing);
int GetFacingFromHeading(short int heading);
float GetHeadingFromVectorF(float dx, float dz);
short int GetHeadingFromVector(float dx, float dz);
shortint2 GetHAndPFromVector(const float3& vec); // vec should be normalized
float2 GetHAndPFromVectorF(const float3& vec); // vec should be normalized
float3 GetVectorFromHeading(short int heading);
float3 GetVectorFromHAndPExact(short int heading,short int pitch);

float3 CalcBeizer(float i, const float3& p1, const float3& p2, const float3& p3, const float3& p4);

float LinePointDist(const float3& l1, const float3& l2, const float3& p);
float3 ClosestPointOnLine(const float3& l1, const float3& l2, const float3& p);

/**
 * @brief Returns the intersection points of a ray with the map boundary (2d only)
 * @param start float3 the start point of the line
 * @param dir float3 direction of the ray
 * @return <near,far> std::pair<float,float> distance to the intersection points in mulitples of `dir`
 */
std::pair<float,float> GetMapBoundaryIntersectionPoints(const float3& start, const float3& dir);

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
bool ClampRayInMap(const float3& start, float3& end);


float smoothstep(const float edge0, const float edge1, const float value);
float3 smoothstep(const float edge0, const float edge1, float3 vec);

float Square(const float x) _const;
int Round(const float f) _const;
float Clamp(const float& v, const float& min, const float& max);
template<class T> T Clamp(const T& v, const T& min, const T& max);


/**
 * @brief Clamps an radian angle between 0 .. 2*pi
 * @param f float* value to clamp
 */
float ClampRad(float f) _const;


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


#include "myMath.inl"

#undef _const

#endif // MYMATH_H
