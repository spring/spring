/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLOAT4_H
#define FLOAT4_H

#include "System/float3.h"
#include "System/creg/creg_cond.h"

/** Float3 with a fourth data member, which is basically unused but required
    to be able to pass data into e.g. OpenGL functions that expect
    an array of 4 floats. */
struct float4 : public float3
{
	CR_DECLARE_STRUCT(float4)

	union {
		struct { float w; };
		struct { float a; };
		struct { float y2; };
		struct { float q; };
		struct { float yend; };
	};

	float4();
	float4(const float3& f, const float w = 0.0f): float3(f), w(w) {}
	float4(const float* f): float3(f[0], f[1], f[2]), w(f[3]) {}
	float4(const float x, const float y, const float z, const float w = 0.0f): float3(x, y, z), w(w) {}

	inline float4& operator= (const float f[4]) {
		x = f[0]; y = f[1];
		z = f[2]; w = f[3];
		return *this;
	}

	inline void fromFloat3 (const float f[3]) {
		x = f[0];
		y = f[1];
		z = f[2];
	}

	inline float4& operator= (const float3& f) {
		x = f.x;
		y = f.y;
		z = f.z;
		return *this;
	}

	inline float4& operator += (const float4& f) {
		x += f.x; y += f.y;
		z += f.z; w += f.w;
		return *this;
	}
	inline float4& operator -= (const float4& f) {
		x -= f.x; y -= f.y;
		z -= f.z; w -= f.w;
		return *this;
	}
	inline float4& operator *= (const float4& f) {
		x *= f.x; y *= f.y;
		z *= f.z; w *= f.w;
		return *this;
	}
#if 0
	inline float4 operator + (const float3& f) const { return float4(x + f.x, y + f.y, z + f.z, w); }
	inline float4 operator - (const float3& f) const { return float4(x - f.x, y - f.y, z - f.z, w); }

	inline float4& operator += (const float3& f) {
		x += f.x;
		y += f.y;
		z += f.z;
		return *this;
	}
	inline float4& operator -= (const float3& f) {
		x -= f.x;
		y -= f.y;
		z -= f.z;
		return *this;
	}
#endif

	// (in)equality tests between float4 and float3 ignore the w-component
	inline bool operator == (const float3& f) const { return (this->float3::operator == (f)); }
	inline bool operator != (const float3& f) const { return (this->float3::operator != (f)); }

	bool operator == (const float4& f) const;

	inline bool operator != (const float4& f) const {
		return !(*this == f);
	}


	inline float dot4(const float4& f) const {
		return (x * f.x) + (y * f.y) + (z * f.z) + (w * f.w);
	}
	
	/// Allows implicit conversion to float* (for passing to gl functions)
	operator const float* () const { return reinterpret_cast<const float*>(&x); }
	operator float* () { return reinterpret_cast<float*>(&x); }
};

#endif /* FLOAT4_H */
