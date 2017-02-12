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
inline short int GetHeadingFromFacing(const int facing)
{
	switch (facing) {
		case FACING_SOUTH: return      0;
		case FACING_EAST:  return  16384;
		case FACING_NORTH: return  32767;
		case FACING_WEST:  return -16384;
		default: return 0;
	}
}

inline int GetFacingFromHeading(const short int heading)
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

inline float GetHeadingFromVectorF(const float dx, const float dz)
{
	float h = 0.0f;

	if (dz != 0.0f) {
		// ensure a minimum distance between dz and 0 such that
		// sqr(dx/dz) never exceeds abs(num_limits<float>::max)
		const float sz = dz * 2.0f - 1.0f;
		const float d  = dx / (dz + 0.000001f * sz);
		const float dd = d * d;

		if (d > 1.0f) {
			h = HALFPI - d / (dd + 0.28f);
		} else if (d < -1.0f) {
			h = -HALFPI - d / (dd + 0.28f);
		} else {
			h = d / (1.0f + 0.28f * dd);
		}

		// add PI (if dx > 0) or -PI (if dx < 0) when dz < 0
		h += ((PI * ((dx > 0.0f) * 2.0f - 1.0f)) * (dz < 0.0f));
	} else {
		h = HALFPI * ((dx > 0.0f) * 2.0f - 1.0f);
	}

	return h;
}

inline short int GetHeadingFromVector(const float dx, const float dz)
{
	constexpr float s = SHORTINT_MAXVALUE * INVPI;
	const     float h = GetHeadingFromVectorF(dx, dz) * s;

	// Prevents h from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following
	// conversion to a short int crashes.
	// if (h > SHORTINT_MAXVALUE) h = SHORTINT_MAXVALUE;
	// return (short int) h;
	int ih = (int) h;

	// if ih represents due-north the modulo operation
	// below would cause it to wrap around from -32768
	// to 0 (which means due-south), so add 1
	ih += (ih == -SHORTINT_MAXVALUE);
	ih %= SHORTINT_MAXVALUE;
	return (short int) ih;
}

// vec should be normalized
inline shortint2 GetHAndPFromVector(const float3 vec)
{
	shortint2 ret;

	// Prevents ret.y from going beyond SHORTINT_MAXVALUE.
	// If h goes beyond SHORTINT_MAXVALUE, the following
	// conversion to a short int crashes.
	// this change destroys the whole meaning with using short ints....
	int iy = (int) (math::asin(vec.y) * (SHORTINT_MAXVALUE * INVPI));
	iy %= SHORTINT_MAXVALUE;
	ret.y = (short int) iy;
	ret.x = GetHeadingFromVector(vec.x, vec.z);
	return ret;
}

// vec should be normalized
inline float2 GetHAndPFromVectorF(const float3 vec)
{
	float2 ret;
	ret.x = GetHeadingFromVectorF(vec.x, vec.z);
	ret.y = math::asin(vec.y);
	return ret;
}

inline float3 GetVectorFromHeading(const short int heading)
{
	const int idx = heading / ((SHORTINT_MAXVALUE/NUM_HEADINGS) * 2) + NUM_HEADINGS/2;
	const float2 vec = CMyMath::headingToVectorTable[idx];
	return float3(vec.x, 0.0f, vec.y);
}

inline float3 CalcBeizer(const float i, const float3 p1, const float3 p2, const float3 p3, const float3 p4)
{
	const float ni = 1.0f - i;
	const float  a =        ni * ni * ni;
	const float  b = 3.0f *  i * ni * ni;
	const float  c = 3.0f *  i *  i * ni;
	const float  d =         i  * i *  i;

	return float3((p1 * a) + (p2 * b) + (p3 * c) + (p4 * d));
}


inline int Round(const float f)
{
	return math::floor(f + 0.5f);
}


inline int2 IdxToCoord(unsigned x, unsigned array_width)
{
	int2 r;
	r.x = x % array_width;
	r.y = x / array_width;
	return r;
}


inline float ClampRad(float f)
{
	f = math::fmod(f, TWOPI);
	if (f < 0.0f) f += TWOPI;
	return f;
}

inline void ClampRad(float* f)
{
	*f = math::fmod(*f, TWOPI);
	if (*f < 0.0f) *f += TWOPI;
}

inline bool RadsAreEqual(const float f1, const float f2)
{
	return (math::fmod(f1 - f2, TWOPI) == 0.0f);
}

inline float GetRadFromXY(const float dx, const float dy)
{
	float a;
	if(dx != 0) {
		a = math::atan(dy / dx);
		if(dx < 0)
			a += PI;
		else if(dy < 0)
			a += 2.0f * PI;
		return a;
	}
	a = PI / 2.0f;
	if(dy < 0)
		a += PI;
	return a;
}
