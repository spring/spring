/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SyncedFloat3.h"

#if defined(SYNCDEBUG) || defined(SYNCCHECK)

CR_BIND(SyncedFloat3, );
CR_REG_METADATA(SyncedFloat3, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z)));

bool SyncedFloat3::IsInBounds() const
{
	assert(float3::maxxpos > 0.0f); // check if initialized

	return ((x >= 0.0f && x <= float3::maxxpos) && (z >= 0.0f && z <= float3::maxzpos));
}

/**
 * @return whether or not it's in bounds
 *
 * Tests whether this vector is in the
 * bounds of the maximum x and z positions.
 */
bool SyncedFloat3::CheckInBounds()
{
	assert(float3::maxxpos > 0.0f); // check if initialized

	bool in = true;

	if (x < 1.0f) {
		x = 1.0f;
		in = false;
	}
	if (z < 1.0f) {
		z = 1.0f;
		in = false;
	}
	if (x > float3::maxxpos) {
		x = float3::maxxpos;
		in = false;
	}
	if (z > float3::maxzpos) {
		z = float3::maxzpos;
		in = false;
	}

	return in;
}

#endif // defined(SYNCDEBUG) || defined(SYNCCHECK)
