#include "StdAfx.h"
#include "ShipMoveMath.h"
#include "ReadMap.h"
#include "SolidObject.h"
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
Check if a given square-position is accessable by the footprint.
*/
bool CShipMoveMath::IsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare) {
	if(CMoveMath::SpeedMod(moveData, xSquare, zSquare) == 0.0f)
		return true;

	if(blockOpt & CHECK_STRUCTURES) {
		//Check the center and four corners of the footprint.
		if(SquareIsBlocked(moveData, blockOpt, xSquare, zSquare))
			return true;
		if(SquareIsBlocked(moveData, blockOpt, xSquare - moveData.size/2, zSquare - moveData.size/2))
			return true;
		if(SquareIsBlocked(moveData, blockOpt, xSquare + moveData.size/2 - 1, zSquare - moveData.size/2))
			return true;
		if(SquareIsBlocked(moveData, blockOpt, xSquare - moveData.size/2, zSquare + moveData.size/2 - 1))
			return true;
		if(SquareIsBlocked(moveData, blockOpt, xSquare + moveData.size/2 - 1, zSquare + moveData.size/2 - 1))
			return true;
	}

	//Nothing found.
	return false;
}


/*
Check if a single square is accessable.
*/
bool CShipMoveMath::SquareIsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare) {
	//Error-check
	if(xSquare < 0 || zSquare < 0
	|| xSquare >= gs->mapx || zSquare >= gs->mapy)
		return true;

	CSolidObject* block = readmap->GroundBlocked(xSquare + zSquare * gs->mapx);
	if(block) {
		if(block->mobility) {
			if(!(blockOpt & CHECK_MOBILE) || block->isMoving) {
				return false;
			} else {
				if(block->mass <= moveData.crushStrength)
					return false;
				else
					return true;
			}
		} else {
			if(block->mass <= moveData.crushStrength)
				return false;
			else
				return true;
		}
	} else
		return false;
}


/*
Ships are always in water level.
*/
float CShipMoveMath::yLevel(int xSquare, int zSquare) {
	return 0.0f;
}

