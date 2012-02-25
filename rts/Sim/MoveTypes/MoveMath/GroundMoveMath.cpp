/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GroundMoveMath.h"
#include "Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Features/Feature.h"

CR_BIND_DERIVED(CGroundMoveMath, CMoveMath, );

float CGroundMoveMath::waterDamageCost = 1.0f;

/*
Calculate speed-multiplier for given height and slope data.
*/
float CGroundMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) const
{
	float speedMod = 0.0f;

	// too steep or too deep?
	if (slope > moveData.maxSlope)
		return speedMod;
	if (-height > moveData.depth)
		return speedMod;

	// slope-mod
	speedMod = 1.0f / (1.0f + slope * moveData.slopeMod);
	speedMod *= ((height < 0.0f)? waterDamageCost: 1.0f);
	speedMod *= moveData.GetDepthMod(height);

	return speedMod;
}

float CGroundMoveMath::SpeedMod(const MoveData& moveData, float height, float slope, float moveSlope) const
{
	float speedMod = 0.0f;

	// too steep or too deep?
	if ((slope * moveSlope) > moveData.maxSlope)
		return speedMod;
	if ((moveSlope < 0) && (-height > moveData.depth))
		return speedMod;

	// slope-mod
	speedMod = 1.0f / (1.0f + std::max(0.0f, slope * moveSlope) * moveData.slopeMod);
	speedMod *= ((height < 0.0f)? waterDamageCost: 1.0f);
	speedMod *= moveData.GetDepthMod(height);

	return speedMod;
}



/*
Gives the ground-level of given square.
*/
float CGroundMoveMath::yLevel(int xSquare, int zSquare) const
{
	return readmap->GetCenterHeightMapSynced()[xSquare + zSquare * gs->mapx];
}

float CGroundMoveMath::yLevel(const float3& pos) const
{
	return (ground->GetHeightReal(pos.x, pos.z) + 10.0f);
}
