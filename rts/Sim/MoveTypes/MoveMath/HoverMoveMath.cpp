/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "HoverMoveMath.h"
#include "Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
#include "Map/Ground.h"

CR_BIND_DERIVED(CHoverMoveMath, CMoveMath, );

bool CHoverMoveMath::noWaterMove = false;


/*
Calculate speed-multiplier for given height and slope data.
*/
float CHoverMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) const
{
	// no speed-penalty if on water (unless noWaterMove)
	if (height < 0.0f) {
		return (noWaterMove? 0.0f: 1.0f);
	}

	if (slope > moveData.maxSlope)
		return 0.0f;

	return (1.0f / (1.0f + slope * moveData.slopeMod));
}

float CHoverMoveMath::SpeedMod(const MoveData& moveData, float height, float slope, float moveSlope) const
{
	// no speed-penalty if on water
	if (height < 0.0f)
		return 1.0f;

	if ((slope * moveSlope) > moveData.maxSlope)
		return 0.0f;

	return (1.0f / (1.0f + std::max(0.0f, slope * moveSlope) * moveData.slopeMod));
}



/*
Gives a position slightly over ground and water level.
*/
float CHoverMoveMath::yLevel(int xSquare, int zSquare) const
{
	return (ground->GetHeightAboveWater(xSquare * SQUARE_SIZE, zSquare * SQUARE_SIZE) + 10.0f);
}

float CHoverMoveMath::yLevel(const float3& pos) const
{
	return (ground->GetHeightAboveWater(pos.x, pos.z) + 10.0f);
}
