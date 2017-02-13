/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/float4.h"
#include "System/creg/creg_cond.h"
#include "System/myMath.h"

CR_BIND(float4, )
CR_REG_METADATA(float4, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)))

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
	#define eps float3::cmp_eps()
	return (epscmp(x, f.x, eps) && epscmp(y, f.y, eps) && epscmp(z, f.z, eps) && epscmp(w, f.w, eps));
	#undef eps
}

