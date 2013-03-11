/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VEC2_H
#define VEC2_H

#include "lib/streflop/streflop_cond.h"
#include "System/creg/creg_cond.h"
#include "System/FastMath.h"

// can't easily use templates because of creg

struct int2
{
	CR_DECLARE_STRUCT(int2);

	int2() : x(0), y(0) {}
	int2(const int nx, const int ny) : x(nx), y(ny) {}

	int x;
	int y;
};

struct float2
{
	CR_DECLARE_STRUCT(float2);

	float2() : x(0.0f), y(0.0f) {}
	float2(const float nx, const float ny) : x(nx), y(ny) {}

	float2& f3_to_f2_xz(const float* f3) { x = f3[0]; y = f3[2]; return *this; }
	float2& f3_to_f2_xy(const float* f3) { x = f3[0]; y = f3[1]; return *this; }
	float2& f3_to_f2_yz(const float* f3) { x = f3[1]; y = f3[2]; return *this; }

	float distance(const float2& f) const {
		const float dx = x - f.x;
		const float dy = y - f.y;
		return math::sqrt(dx*dx + dy*dy);
	}

	float x;
	float y;
};

#endif
