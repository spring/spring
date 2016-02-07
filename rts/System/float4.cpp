/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/float4.h"

#include "System/creg/creg_cond.h"
#include <algorithm> // std::{min,max}

CR_BIND(float4, )
CR_REG_METADATA(float4,
		(CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)))

float4::float4(): float3(), w(0.0f)
{
	// x, y and z are default to 0.0f in float3()

	// ensure that we use a continuous block of memory
	// (required for passing to gl functions)
	assert(&y == &x + 1);
	assert(&z == &y + 1);
	assert(&w == &z + 1);
}

bool float4::operator == (const float4& f) const
{
	return math::fabs(x - f.x) <= float3::CMP_EPS * std::max(std::max(math::fabs(x), math::fabs(f.x)), 1.0f)
		&& math::fabs(y - f.y) <= float3::CMP_EPS * std::max(std::max(math::fabs(y), math::fabs(f.y)), 1.0f)
		&& math::fabs(z - f.z) <= float3::CMP_EPS * std::max(std::max(math::fabs(z), math::fabs(f.z)), 1.0f)
		&& math::fabs(w - f.w) <= float3::CMP_EPS * std::max(std::max(math::fabs(w), math::fabs(f.w)), 1.0f);
}
