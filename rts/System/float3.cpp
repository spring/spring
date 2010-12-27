/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "float3.h"
#include "creg/creg_cond.h"

CR_BIND(float3, );
CR_REG_METADATA(float3, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z)));

//! gets initialized later when the map is loaded
float float3::maxxpos = -1.0f;
float float3::maxzpos = -1.0f;

#ifdef _MSC_VER
	const float float3::CMP_EPS = 1e-4f;
	const float float3::NORMALIZE_EPS = 1e-12f;
#endif

bool float3::IsInBounds() const
{
	assert(maxxpos > 0.0f); // check if initialized

	return ((x >= 0.0f && x <= maxxpos) && (z >= 0.0f && z <= maxzpos));
}

/**
 * @return whether or not it's in bounds
 *
 * Tests whether this vector is in the
 * bounds of the maximum x and z positions.
 */
bool float3::CheckInBounds()
{
	assert(maxxpos > 0.0f); // check if initialized

	bool in = true;

	if (x < 1.0f) {
		x = 1.0f;
		in = false;
	}
	if (z < 1.0f) {
		z = 1.0f;
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
