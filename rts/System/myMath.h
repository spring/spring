#ifndef MYMATH_H
#define MYMATH_H

#include "Sim/Misc/GlobalConstants.h"
#include "Vec2.h"
#include "float3.h"

#ifndef __GNUC__
	#define __const
	#define __pure
#else
	#define __const __attribute__((const))
	#define __pure __attribute__((pure))
#endif


#define MaxByAbs(a,b) (abs((a)) > abs((b))) ? (a) : (b);

static const float TWOPI = 2*PI;

#define SHORTINT_MAXVALUE 32768

#define HEADING_CHECKSUM_1024 0x617a9968
#define HEADING_CHECKSUM_4096 0x3d51b476
#define NUM_HEADINGS 4096

#if (NUM_HEADINGS == 1024)
#  define HEADING_CHECKSUM  HEADING_CHECKSUM_1024
#elif (NUM_HEADINGS == 4096)
#  define HEADING_CHECKSUM  HEADING_CHECKSUM_4096
#else
#  error "HEADING_CHECKSUM not set, invalid NUM_HEADINGS?"
#endif


class CMyMath {
public:
	static void Init();
	static float2 headingToVectorTable[NUM_HEADINGS];
};



inline short int __const GetHeadingFromFacing(int facing)
{
	switch (facing) {
		case 0: return      0;	// south
		case 1: return  16384;	// east
		case 2: return  32767;	// north == -32768
		case 3: return -16384;	// west
		default: return 0;
	}
}

inline float __const GetHeadingFromVectorF(float dx, float dz)
{
	float h = 0.0f;

	if (dz != 0.0f) {
		float d = dx / dz;

		if (d > 1.0f) {
			h = (PI * 0.5f) - d / (d * d + 0.28f);
		} else if (d < -1.0f) {
			h = -(PI * 0.5f) - d / (d * d + 0.28f);
		} else {
			h = d / (1.0f + 0.28f * d * d);
		}

		if (dz < 0.0f) {
			if (dx > 0.0f)
				h += PI;
			else
				h -= PI;
		}
	} else {
		if (dx > 0.0f)
			h = PI * 0.5f;
		else
			h = -PI * 0.5f;
	}

	return h;
}

inline short int __const GetHeadingFromVector(float dx, float dz)
{
	float h = GetHeadingFromVectorF(dx, dz);

	h *= SHORTINT_MAXVALUE / PI;

	// Prevents h from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following
	// conversion to a short int crashes.
	// if (h > SHORTINT_MAXVALUE) h = SHORTINT_MAXVALUE;
	// return (short int) h;

	int ih = (int) h;
	if (ih == -SHORTINT_MAXVALUE) {
		// ih now represents due-north, but the modulo operation
		// below would cause it to wrap around from -32768 to 0
		// which means due-south, so add 1
		ih += 1;
	}
	ih %= SHORTINT_MAXVALUE;
	return (short int) ih;
}

struct shortint2 {
	short int x, y;
};

// vec should be normalized
inline shortint2 __const GetHAndPFromVector(const float3& vec)
{
	shortint2 ret;

	// Prevents ret.y from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following
	// conversion to a short int crashes.
	// this change destroys the whole meaning with using short ints....
	#if defined BUILDING_AI
	int iy = (int) (std::asin(vec.y) * (SHORTINT_MAXVALUE / PI));
	#else
	int iy = (int) (streflop::asin(vec.y) * (SHORTINT_MAXVALUE / PI));
	#endif // defined BUILDING_AI
	iy %= SHORTINT_MAXVALUE;
	ret.y = (short int) iy;
	ret.x = GetHeadingFromVector(vec.x, vec.z);
	return ret;
}

// vec should be normalized
inline float2 __const GetHAndPFromVectorF(const float3& vec)
{
	float2 ret;

	#if defined BUILDING_AI
	ret.y = std::asin(vec.y);
	#else
	ret.y = streflop::asin(vec.y);
	#endif // defined BUILDING_AI
	ret.x = GetHeadingFromVectorF(vec.x, vec.z);
	return ret;
}

inline float3 __pure GetVectorFromHeading(short int heading)
{
	float2 v = CMyMath::headingToVectorTable[heading / ((SHORTINT_MAXVALUE/NUM_HEADINGS) * 2) + NUM_HEADINGS/2];
	return float3(v.x, 0.0f, v.y);
}

float3 GetVectorFromHAndPExact(short int heading,short int pitch);

inline float3 CalcBeizer(float i, const float3& p1, const float3& p2, const float3& p3, const float3& p4)
{
	float ni = 1 - i;

	float3 res((p1 * ni * ni * ni) + (p2 * 3 * i * ni * ni) + (p3 * 3 * i * i * ni) + (p4 * i * i * i));
	return res;
}

float LinePointDist(const float3& l1, const float3& l2, const float3& p);
float3 ClosestPointOnLine(const float3& l1, const float3& l2, const float3& p);

inline float __const Square(const float x)
{
	return x * x;
}

float smoothstep(const float edge0, const float edge1, const float value);
float3 smoothstep(const float edge0, const float edge1, float3 vec);


inline float __const Clamp(const float& v, const float& min, const float& max)
{
	if (v>max) {
		return max;
	} else if (v<min) {
		return min;
	}
	return v;
}

/**
 * @brief Clamps an radian angle between 0 .. 2*pi
 * @param f float* value to clamp
 */
inline float __const ClampRad(float f)
{
	f = fmod(f, TWOPI);
	if (f < 0.0f) f += TWOPI;
	return f;
}



/**
 * @brief Clamps an radian angle between 0 .. 2*pi
 * @param f float* value to clamp
 */
inline void __const ClampRad(float* f)
{
	*f = fmod(*f, TWOPI);
	if (*f < 0.0f) *f += TWOPI;
}



/**
 * @brief Checks if 2 radian values discribe the same angle
 * @param f1 float* first compare value
 * @param f2 float* second compare value
 */
inline bool __const RadsAreEqual(const float f1, const float f2)
{
	return (fmod(f1 - f2, TWOPI) == 0.0f);
}

#endif // MYMATH_H
