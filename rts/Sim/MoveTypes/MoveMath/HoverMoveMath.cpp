/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveMath.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

/*
Calculate speed-multiplier for given height and slope data.
*/
float CMoveMath::HoverSpeedMod(const MoveDef& moveDef, float height, float slope)
{
	// no speed-penalty if on water (unless noWaterMove)
	if (height < 0.0f)
		return (1.0f * noHoverWaterMove);
	// slope too steep?
	if (slope > moveDef.maxSlope)
		return 0.0f;

	return (1.0f / (1.0f + slope * moveDef.slopeMod));
}

float CMoveMath::HoverSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeScale)
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

