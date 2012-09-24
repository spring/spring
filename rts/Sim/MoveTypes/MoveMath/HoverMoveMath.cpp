/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "HoverMoveMath.h"
#include "Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
#include "Map/Ground.h"

CR_BIND_DERIVED(CHoverMoveMath, CMoveMath, );

bool CHoverMoveMath::noWaterMove = false;


/*
Calculate speed-multiplier for given height and slope data.
*/
float CHoverMoveMath::SpeedMod(const MoveDef& moveDef, float height, float slope) const
{
	// no speed-penalty if on water (unless noWaterMove)
	if (height < 0.0f)
		return (1.0f * noWaterMove);
	// slope too steep?
	if (slope > moveDef.maxSlope)
		return 0.0f;

	return (1.0f / (1.0f + slope * moveDef.slopeMod));
}

float CHoverMoveMath::SpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeScale) const
{
	// no speed-penalty if on water
	if (height < 0.0f)
		return 1.0f;

	// too steep downhill slope?
	if ((slope * dirSlopeScale) < (-moveDef.maxSlope * 2.0f))
		return 0.0f;
	// too steep uphill slope?
	if ((slope * dirSlopeScale) > ( moveDef.maxSlope       ))
		return 0.0f;

	return (1.0f / (1.0f + std::max(0.0f, slope * dirSlopeScale) * moveDef.slopeMod));
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
