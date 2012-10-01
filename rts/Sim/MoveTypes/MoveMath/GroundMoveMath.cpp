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

float CMoveMath::GroundSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod)
{
	float speedMod = 0.0f;

	// NOTE:
	//    dirSlopeMod is always a value in [-1, 1], so <slope * mod>
	//    is never greater than <slope> and never less than <-slope>
	//
	//    any slope > (tolerance * 2) is always non-navigable (up or down)
	//    any slope < (tolerance    ) is always     navigable (up or down)
	//    for any in-between slope it depends on the directional modifier
	if (slope > (moveDef.maxSlope * 2.0f))
		return speedMod;

	// too steep downhill slope?
	if (dirSlopeMod <= 0.0f && (slope * dirSlopeMod) < (-moveDef.maxSlope * 2.0f))
		return speedMod;
	// too steep uphill slope?
	if (dirSlopeMod  > 0.0f && (slope * dirSlopeMod) > ( moveDef.maxSlope       ))
		return speedMod;

	// is this square below our maxWaterDepth and are we going further downhill?
	if ((dirSlopeMod < 0.0f) && (-height > moveDef.depth))
		return speedMod;

	// slope-mod (speedMod is not increased or decreased by downhill slopes)
	speedMod = 1.0f / (1.0f + std::max(0.0f, slope * dirSlopeMod) * moveDef.slopeMod);
	speedMod *= ((height < 0.0f)? waterDamageCost: 1.0f);
	speedMod *= moveDef.GetDepthMod(height);

	return speedMod;
}

