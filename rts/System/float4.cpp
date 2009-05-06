/**
 * @file float4.cpp
 * @brief float4 source
 *
 * Implementation of float4 class
 */

#include "float4.h"

CR_BIND(float4, );
CR_REG_METADATA(float4, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z), CR_MEMBER(w)));

float4::float4()
{
	float tmp[4];

	// ensure alignment is correct to use it as array of floats
	(void) tmp;
	assert(&y - &x == &tmp[1] - &tmp[0]);
	assert(&z - &x == &tmp[2] - &tmp[0]);
	assert(&w - &x == &tmp[3] - &tmp[0]);

	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	w = 0.0f;
}