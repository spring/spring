/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "HoverMoveMath.h"
#include "Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
#include "Map/Ground.h"

CR_BIND_DERIVED(CHoverMoveMath, CMoveMath, );

const float HOVERING_HEIGHT = 5;
bool CHoverMoveMath::noWaterMove;


/*
Calculate speed-multiplier for given height and slope data.
*/
float CHoverMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) const
{
	//On water?
	if(height < 0){
		if(noWaterMove)
			return 0.0f;
		return 1.0f;
	}
	//Too slope?
	if(slope > moveData.maxSlope)
		return 0.0f;
	//Slope-mod
	return 1 / (1 + slope * moveData.slopeMod);
}

float CHoverMoveMath::SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) const
{
	//On water?
	if(height < 0)
		return 1.0f;
	//Too slope?
	if(slope*moveSlope > moveData.maxSlope)
		return 0.0f;
	//Slope-mod
	return 1 / (1 + std::max(0.0f,slope*moveSlope) * moveData.slopeMod);
}

/*
Gives a position slightly over ground and water level.
*/
float CHoverMoveMath::yLevel(int xSquare, int zSquare) const
{
	return ground->GetHeight(xSquare*SQUARE_SIZE, zSquare*SQUARE_SIZE) + 10;
}


float CHoverMoveMath::yLevel(const float3& pos) const
{
	return ground->GetHeight(pos.x, pos.z) + 10;
}

