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
		case FACING_EAST : return  16384;
		case FACING_NORTH: return  32767;
		case FACING_WEST : return -16384;
		default          : return      0;
	}
}

inline int GetFacingFromHeading(const short int heading)
{
	if (heading >= 0) {
		if (heading <  8192) { return FACING_SOUTH; }
		if (heading < 24576) { return FACING_EAST ; }
		return FACING_NORTH;
	} else {
		if (heading >=  -8192) { return FACING_SOUTH; }
		if (heading >= -24576) { return FACING_WEST ; }
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

		if (math::fabs(d) > 1.0f) {
			h = std::copysign(1.0f, d) * math::HALFPI - d / (dd + 0.28f);
		} else {
			h = d / (1.0f + 0.28f * dd);
		}

		// add PI (if dx > 0) or -PI (if dx < 0) when dz < 0
		h += ((math::PI * ((dx > 0.0f) * 2.0f - 1.0f)) * (dz < 0.0f));
	} else {
		h = math::HALFPI * ((dx > 0.0f) * 2.0f - 1.0f);
	}

	return h;
}

inline short int GetHeadingFromVector(const float dx, const float dz)
{
	constexpr float s = SPRING_MAX_HEADING * math::INVPI;
	const     float h = GetHeadingFromVectorF(dx, dz) * s;

	// Prevents h from going beyond SPRING_MAX_HEADING.
	// If h goes beyond SPRING_MAX_HEADING, the following
	// conversion to a short int crashes.
	// if (h > SPRING_MAX_HEADING) h = SPRING_MAX_HEADING;
	// return (short int) h;
	int ih = (int) h;

	// if ih represents due-north the modulo operation
	// below would cause it to wrap around from -32768
	// to 0 (which means due-south), so add 1
	ih += (ih == -SPRING_MAX_HEADING);
	ih %= SPRING_MAX_HEADING;
	return (short int) ih;
}

// vec should be normalized
inline shortint2 GetHAndPFromVector(const float3 vec)
{
	shortint2 ret;

	// Prevents ret.y from going beyond SPRING_MAX_HEADING.
	// If h goes beyond SPRING_MAX_HEADING, the following
	// conversion to a short int crashes.
	// this change destroys the whole meaning with using short ints....
	int iy = (int) (math::asin(vec.y) * (SPRING_MAX_HEADING * math::INVPI));
	iy %= SPRING_MAX_HEADING;
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
	constexpr int div = (SPRING_MAX_HEADING / NUM_HEADINGS) * 2;
	const     int idx = heading / div + NUM_HEADINGS / 2;

	const float2 vec = SpringMath::headingToVectorTable[idx];
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
	f  = math::fmod(f, math::TWOPI);
	f += (math::TWOPI * (f < 0.0f));
	return f;
}

inline void ClampRad(float* f) { *f = ClampRad(*f); }


inline bool RadsAreEqual(const float f1, const float f2)
{
	return (math::fmod(f1 - f2, math::TWOPI) == 0.0f);
}

inline float GetRadFromXY(const float dx, const float dy)
{
	if (dx != 0.0f) {
		const float a = math::atan(dy / dx);

		if (dx < 0.0f)
			return (a + math::PI);
		if (dy < 0.0f)
			return (a + math::TWOPI);

		return a;
	}

	return (math::HALFPI + (math::PI * (dy < 0.0f)));
}
