/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "float4.h"

#include "creg/creg_cond.h"

CR_BIND(float4, );
CR_REG_METADATA(float4,
		(CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)));

float4::float4() : w(0.0f)
{
	// x, y and z are default to 0.0f in float3()

	// ensure that we use a continuous block of memory
	// (required for passing to gl functions)
	assert(&y == &x + 1);
	assert(&z == &y + 1);
	assert(&w == &z + 1);
}
