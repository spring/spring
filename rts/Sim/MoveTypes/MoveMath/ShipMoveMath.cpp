/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "ShipMoveMath.h"
#include "Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
#include "mmgr.h"

CR_BIND_DERIVED(CShipMoveMath, CMoveMath, );

/*
Calculate speed-multiplier for given height and slope data.
*/
float CShipMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) const
{
	//Too ground?
	if(-height < moveData.depth)
		return 0.0f;
	return 1.0f;
}

float CShipMoveMath::SpeedMod(const MoveData& moveData, float height, float slope, float moveSlope) const
{
	if(-height < moveData.depth && moveSlope>0)
		return 0.0f;
	return 1.0f;
}


/*
Ships are always in water level.
*/
float CShipMoveMath::yLevel(int xSquare, int zSquare) const
{
	return 0.0f;
}


float CShipMoveMath::yLevel(const float3& pos) const
{
	return 0.0f;
}

