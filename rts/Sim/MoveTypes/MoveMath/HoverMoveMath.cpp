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

	//FIXME broken cause of:
	// 1. it doubles maxSlope -> the value NOT the angle, and so _massively_ increasing maxSlope angle
	// 2. by doing so it makes squares pathable that are blocked in classic SpeedMod w/o movedir.
	//    Not bad itself, problem is that GroundMoveType.cpp code is broken and sometimes uses it with movedir
	//    sometimes without. And so codes working against each other making units shake when going hills up etc.

	// no speed-penalty if on water
	if (height < 0.0f)
		return 1.0f;

	if (slope > (moveDef.maxSlope * 2.0f))
		return 0.0f;

	// too steep downhill slope?
	if (dirSlopeMod <= 0.0f && (slope * dirSlopeMod) < (-moveDef.maxSlope * 2.0f))
		return 0.0f;
	// too steep uphill slope?
	if (dirSlopeMod  > 0.0f && (slope * dirSlopeMod) > ( moveDef.maxSlope       ))
		return 0.0f;

	return (1.0f / (1.0f + std::max(0.0f, slope * dirSlopeMod) * moveDef.slopeMod));
}

