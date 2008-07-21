/**
 * @file float3.cpp
 * @brief float3 source
 *
 * Implementation of float3 class
 */

#include "float3.h"
#include "FastMath.h"

CR_BIND(float3, );
CR_REG_METADATA(float3, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z)));

float float3::maxxpos = 2048.0f; /**< Maximum x position is 2048 */
float float3::maxzpos = 2048.0f; /**< Maximum z position is 2048 */

/**
 * @return whether or not it's in bounds
 *
 * Tests whether this vector is in the
 * bounds of the maximum x and z positions.
 */
bool float3::CheckInBounds()
{
	bool in = true;

	if (x < 1) {
		x = 1;
		in = false;
	}
	if (z < 1) {
		z = 1;
		in = false;
	}
	if (x > maxxpos) {
		x = maxxpos;
		in = false;
	}
	if (z > maxzpos) {
		z = maxzpos;
		in = false;
	}

	return in;
}

float3& float3::ANormalize()
{
	float invL = fastmath::isqrt(SqLength());
	if (invL != 0.f) {
		x *= invL;
		y *= invL;
		z *= invL;
	}
	return *this;
}

SAIFloat3 float3::toSAIFloat3() const {

	SAIFloat3 sAIFloat3;

	sAIFloat3.x = x;
	sAIFloat3.y = y;
	sAIFloat3.z = z;

	return  sAIFloat3;
}
