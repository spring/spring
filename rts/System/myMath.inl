/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//                  F(N=2) = H(-32768 / 32767)
// 
//                         ^
//                         |
//                         |
// F(W=3) = H(-16384)  <---o--->  F(E=1) = H(16384)
//                         |
//                         |
//                         v
// 
//                  F(S=0) = H(0)
inline short int GetHeadingFromFacing(int facing)
{
	switch (facing) {
		case FACING_SOUTH: return      0;
		case FACING_EAST:  return  16384;
		case FACING_NORTH: return  32767;
		case FACING_WEST:  return -16384;
		default: return 0;
	}
}

inline int GetFacingFromHeading(short int heading)
{
	if (heading >= 0) {
		if (heading <  8192) { return FACING_SOUTH; }
		if (heading < 24576) { return FACING_EAST; }
		return FACING_NORTH;
	} else {
		if (heading >=  -8192) { return FACING_SOUTH; }
		if (heading >= -24576) { return FACING_WEST; }
		return FACING_NORTH;
	}
}

inline float GetHeadingFromVectorF(float dx, float dz)
{
	float h = 0.0f;

	if (dz != 0.0f) {
		float d = dx / dz;

		if (d > 1.0f) {
			h = HALFPI - d / (d * d + 0.28f);
		} else if (d < -1.0f) {
			h = -HALFPI - d / (d * d + 0.28f);
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
			h = HALFPI;
		else
			h = -HALFPI;
	}

	return h;
}

inline short int GetHeadingFromVector(float dx, float dz)
{
	float h = GetHeadingFromVectorF(dx, dz);

	h *= SHORTINT_MAXVALUE * INVPI;

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

// vec should be normalized
inline shortint2 GetHAndPFromVector(const float3& vec)
{
	shortint2 ret;

	// Prevents ret.y from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following
	// conversion to a short int crashes.
	// this change destroys the whole meaning with using short ints....
  #if defined BUILDING_AI
	int iy = (int) (std::asin(vec.y) * (SHORTINT_MAXVALUE * INVPI));
  #else
	int iy = (int) (streflop::asin(vec.y) * (SHORTINT_MAXVALUE * INVPI));
  #endif
	iy %= SHORTINT_MAXVALUE;
	ret.y = (short int) iy;
	ret.x = GetHeadingFromVector(vec.x, vec.z);
	return ret;
}

// vec should be normalized
inline float2 GetHAndPFromVectorF(const float3& vec)
{
	float2 ret;

  #if defined BUILDING_AI
	ret.y = std::asin(vec.y);
  #else
	ret.y = streflop::asin(vec.y);
  #endif
	ret.x = GetHeadingFromVectorF(vec.x, vec.z);
	return ret;
}

inline float3 GetVectorFromHeading(short int heading)
{
	float2 v = CMyMath::headingToVectorTable[heading / ((SHORTINT_MAXVALUE/NUM_HEADINGS) * 2) + NUM_HEADINGS/2];
	return float3(v.x, 0.0f, v.y);
}

inline float3 CalcBeizer(float i, const float3& p1, const float3& p2, const float3& p3, const float3& p4)
{
	float ni = 1 - i;

	float3 res((p1 * ni * ni * ni) + (p2 * 3 * i * ni * ni) + (p3 * 3 * i * i * ni) + (p4 * i * i * i));
	return res;
}


inline float Square(const float x)
{
	return x * x;
}

#include <math.h>

inline int Round(const float f)
{
	return floor(f + 0.5f);
}

inline float Clamp(const float& v, const float& min, const float& max)
{
	return std::min(max, std::max(min, v));
}

template<class T>
inline T Clamp(const T& v, const T& min, const T& max)
{
	return v < min ? min : (v > max ? max : v);
}

inline float ClampRad(float f)
{
	f = fmod(f, TWOPI);
	if (f < 0.0f) f += TWOPI;
	return f;
}

inline void ClampRad(float* f)
{
	*f = fmod(*f, TWOPI);
	if (*f < 0.0f) *f += TWOPI;
}

inline bool RadsAreEqual(const float f1, const float f2)
{
	return (fmod(f1 - f2, TWOPI) == 0.0f);
}
