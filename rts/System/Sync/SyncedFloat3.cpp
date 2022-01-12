/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SyncedFloat3.h"
#include "System/SpringMath.h"

#if defined(SYNCDEBUG) || defined(SYNCCHECK)

CR_BIND(SyncedFloat3, )
CR_REG_METADATA(SyncedFloat3, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z)))


bool SyncedFloat3::IsInBounds() const
{
	assert(float3::maxxpos > 0.0f); // check if initialized

	return ((x >= 0.0f && x <= float3::maxxpos) && (z >= 0.0f && z <= float3::maxzpos));
}


void SyncedFloat3::ClampInBounds()
{
	assert(float3::maxxpos > 0.0f); // check if initialized

	x = Clamp((float)x, 0.0f, float3::maxxpos);
	z = Clamp((float)z, 0.0f, float3::maxzpos);

	//return *this;
}


void SyncedFloat3::ClampInMap()
{
	assert(float3::maxxpos > 0.0f); // check if initialized

	x = Clamp((float)x, 0.0f, float3::maxxpos + 1);
	z = Clamp((float)z, 0.0f, float3::maxzpos + 1);
}

#endif // defined(SYNCDEBUG) || defined(SYNCCHECK)
