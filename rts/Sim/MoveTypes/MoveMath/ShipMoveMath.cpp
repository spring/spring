#include "StdAfx.h"
#include "ShipMoveMath.h"
#include "Sim/Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
//#include "mmgr.h"


/*
Calculate speed-multiplier for given height and slope data.
*/
float CShipMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) {
	//Too ground?
	if(-height < moveData.depth)
		return 0.0f;
	return 1.0f;
}

float CShipMoveMath::SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) {
	if(-height < moveData.depth && moveSlope>0)
		return 0.0f;
	return 1.0f;
}


/*
Ships are always in water level.
*/
float CShipMoveMath::yLevel(int xSquare, int zSquare) {
	return 0.0f;
}

