/**
 * @file float3.cpp
 * @brief float3 source
 *
 * Implementation of float3 class
 */
#include "StdAfx.h"

CR_BIND_STRUCT(float3);
CR_REG_METADATA(float3, (CR_MEMBER(x), CR_MEMBER(y), CR_MEMBER(z)));

float float3::maxxpos=2048; /**< Maximum X position is 2048 */
float float3::maxzpos=2048; /**< Maximum z position is 2048 */

/**
 * @return whether or not it's in bounds
 * 
 * Tests whether this vector is in
 * the bounds of the maximum x and z positions.
 */
bool float3::CheckInBounds()
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
	if(x>maxxpos){
		x=maxxpos;
		in=false;
	}
	if(z>maxzpos){
		z=maxzpos;
		in=false;
	}

	return in;
}
