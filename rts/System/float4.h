/**
 * @file float4.h
 * @brief float4 header
 *
 * Declaration of float4 class
 */
#ifndef FLOAT4_H
#define FLOAT4_H

#include "float3.h"

/** Float3 with a fourth data member, which is basically unused but required
    to be able to pass data into e.g. OpenGL functions that expect
    an array of 4 floats. */
struct float4 : public float3
{
	CR_DECLARE_STRUCT(float4);

	float w;

	float4();
	float4(const float3& f, const float w = 0.0f) : float3(f), w(w) {}
	float4(const float* f) : float3(f[0],f[1],f[2]), w(f[3]) {}
	float4(const float x, const float y, const float z, const float w = 0.0f) : float3(x,y,z), w(w) {}

	inline void operator= (const float f[4]){
		x=f[0];
		y=f[1];
		z=f[2];
		w=f[3];
	}

	inline void operator= (const float3 f){
		x=f.x;
		y=f.y;
		z=f.z;
	}

	inline void operator= (const float4 f){
		x=f.x;
		y=f.y;
		z=f.z;
		w=f.w;
	}

	inline bool operator== (const float4 &f) const {
		return math::fabs(x-f.x) <= math::fabs(1.0E-4f*x) &&
			math::fabs(y-f.y) <= math::fabs(1.0E-4f*y) &&
			math::fabs(z-f.z) <= math::fabs(1.0E-4f*z) &&
			math::fabs(w-f.w) <= math::fabs(1.0E-4f*w);
	}

	inline bool operator!= (const float4 &f) const {
		return !(*this == f);
	}


	/// Allows implicit conversion to float* (for passing to gl functions)
	operator const float* () const { return &x; }
	operator float* () { return &x; }
};

#endif /* FLOAT4_H */
