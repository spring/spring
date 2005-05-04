#include "stdafx.h"
#include "HoverMoveMath.h"
#include "ReadMap.h"
#include "SolidObject.h"
#include "Ground.h"

const float HOVERING_HEIGHT = 5;


/*
Calculate speed-multiplier for given height and slope data.
*/
float CHoverMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) {
	//On water?
	if(height < 0)
		return 1.0f;
	//Too slope?
	if(slope > moveData.maxSlope)
		return 0.0f;
	//Slope-mod
	return 1 / (1 + slope * moveData.slopeMod);
}

float CHoverMoveMath::SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) {
	//On water?
	if(height < 0)
		return 1.0f;
	//Too slope?
	if(slope*moveSlope > moveData.maxSlope)
		return 0.0f;
	//Slope-mod
	return 1 / (1 + max(0.0f,slope*moveSlope) * moveData.slopeMod);
}

/*
Check if a given square-position is accessable by the footprint.
*/
bool CHoverMoveMath::IsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare) {
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
bool CHoverMoveMath::SquareIsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare) {
	//Error-check
	if(xSquare < 0 || zSquare < 0
	|| xSquare >= gs->mapx || zSquare >= gs->mapy)
		return true;

	CSolidObject* obstacle = readmap->GroundBlocked(xSquare + zSquare * gs->mapx);
	if(obstacle && IsBlocking(moveData, obstacle)) {
		if(obstacle->mobility) {
			if(!(blockOpt & CHECK_MOBILE) || obstacle->isMoving) {
				return false;
			} else {
				return true;
			}
		} else {
			return true;
		}
	} else
		return false;
}


/*
Tells whenever a hovercraft could pass over an object.
*/
bool CHoverMoveMath::IsBlocking(const MoveData& moveData, const CSolidObject* object) {
	return (object->blocking && object->height > HOVERING_HEIGHT);
}


/*
Gives a position slightly over ground and water level.
*/
float CHoverMoveMath::yLevel(int xSquare, int zSquare) {
	return ground->GetHeight(xSquare*SQUARE_SIZE, zSquare*SQUARE_SIZE) + 10;
}