/**
 * @file float4.cpp
 * @brief float4 source
 *
 * Implementation of float4 class
 */

#include "float4.h"

CR_BIND(float4, );
CR_REG_METADATA(float4, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)));

float4::float4() : w(0.0f)
{
	// x, y and z are default to 0 in float3()
	assert(&y == &x+1);
	assert(&z == &y+1);
	assert(&w == &z+1);
}