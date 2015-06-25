/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveMath.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

/*
Calculate speed-multiplier for given height and slope data.
*/
float CMoveMath::HoverSpeedMod(const MoveDef& moveDef, float height, float slope)
{
	// no speed-penalty if on water (unless noWaterMove)
	if (height < 0.0f)
		return (1.0f * !noHoverWaterMove);
	// slope too steep?
	if (slope > moveDef.maxSlope)
		return 0.0f;

	return (1.0f / (1.0f + slope * moveDef.slopeMod));
}

float CMoveMath::HoverSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod)
{
	if (!modInfo.allowDirectionalPathing) {
		return HoverSpeedMod(moveDef, height, slope);
	}

	// Only difference direction can have is making hills climbing slower.

	// no speed-penalty if on water
	if (height < 0.0f)
		return (1.0f * !noHoverWaterMove);

	if (slope > moveDef.maxSlope)
		return 0.0f;

	return (1.0f / (1.0f + std::max(0.0f, slope * dirSlopeMod) * moveDef.slopeMod));
}

