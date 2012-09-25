/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveMath.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

/*
Calculate speed-multiplier for given height and slope data.
*/
float CMoveMath::GroundSpeedMod(const MoveDef& moveDef, float height, float slope)
{
	float speedMod = 0.0f;

	// slope too steep or square too deep?
	if (slope > moveDef.maxSlope)
		return speedMod;
	if (-height > moveDef.depth)
		return speedMod;

	// slope-mod
	speedMod = 1.0f / (1.0f + slope * moveDef.slopeMod);
	speedMod *= ((height < 0.0f)? waterDamageCost: 1.0f);
	speedMod *= moveDef.GetDepthMod(height);

	return speedMod;
}

float CMoveMath::GroundSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeScale)
{
	float speedMod = 0.0f;

	// too steep downhill slope?
	if ((slope * dirSlopeScale) < (-moveDef.maxSlope * 2.0f))
		return speedMod;
	// too steep uphill slope?
	if ((slope * dirSlopeScale) > ( moveDef.maxSlope       ))
		return speedMod;
	// is this square below our maxWaterDepth and are we going further downhill?
	if ((dirSlopeScale < 0.0f) && (-height > moveDef.depth))
		return speedMod;

	// slope-mod (speedMod is not increased or decreased by downhill slopes)
	speedMod = 1.0f / (1.0f + std::max(0.0f, slope * dirSlopeScale) * moveDef.slopeMod);
	speedMod *= ((height < 0.0f)? waterDamageCost: 1.0f);
	speedMod *= moveDef.GetDepthMod(height);

	return speedMod;
}

