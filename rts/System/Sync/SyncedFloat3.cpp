/**
 * @file SyncedFloat3.cpp
 * @brief SyncedFloat3 source
 *
 * Implementation of SyncedFloat3 class
 */

#include "StdAfx.h"
#include "SyncedFloat3.h"

#if defined(SYNCDEBUG) || defined(SYNCCHECK)

CR_BIND(SyncedFloat3, );
CR_REG_METADATA(SyncedFloat3, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z)));

/**
 * @return whether or not it's in bounds
 * 
 * Tests whether this vector is in
 * the bounds of the maximum x and z positions.
 */
bool SyncedFloat3::CheckInBounds()
{
	bool in=true;
	if(x<1){
		x=1;
		in=false;
	}
	if(z<1){
		z=1;
		in=false;
	}
	if(x>float3::maxxpos){
		x=float3::maxxpos;
		in=false;
	}
	if(z>float3::maxzpos){
		z=float3::maxzpos;
		in=false;
	}

	return in;
}

#endif // SYNCDEBUG || SYNCCHECK
