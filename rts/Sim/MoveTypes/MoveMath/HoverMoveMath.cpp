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

float CMoveMath::HoverSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod)
{
	// no speed-penalty if on water
	if (height < 0.0f)
		return 1.0f;

	// too steep downhill slope?
	if ((slope * dirSlopeMod) < (-moveDef.maxSlope * 2.0f))
		return 0.0f;
	#if 0
	// too steep uphill slope?
	if ((slope * dirSlopeMod) > ( moveDef.maxSlope       ))
		return 0.0f;
	#endif
	// too steep uphill slope? (we ignore direction here)
	if (slope > moveDef.maxSlope)
		return 0.0f;

	return (1.0f / (1.0f + std::max(0.0f, slope * dirSlopeMod) * moveDef.slopeMod));
}

