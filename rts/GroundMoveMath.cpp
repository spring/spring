#include "StdAfx.h"
#include "GroundMoveMath.h"
#include "ReadMap.h"
#include "SolidObject.h"
#include "Feature.h"


/*
Calculate speed-multiplier for given height and slope data.
*/
float CGroundMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) {
	//Too slope?
	if(slope > moveData.maxSlope)
		return 0.0f;
	//Too depth?
	if(-height > moveData.depth)
		return 0.0f;
	//Slope-mod
	float mod = 1 / (1 + slope * moveData.slopeMod);
	//Under-water-mod
	if(height < 0) {
		mod /= 1 - max(-1.0f, (height * moveData.depthMod));
	}
	return mod;
}

float CGroundMoveMath::SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) {
	//Too slope?
	if(slope*moveSlope > moveData.maxSlope)
		return 0.0f;
	//Too depth?
	if(-height > moveData.depth && moveSlope<0)
		return 0.0f;
	//Slope-mod
	float mod = 1 / (1 + max(0.0f,slope*moveSlope) * moveData.slopeMod);
	//Under-water-mod
	if(height < 0) {
		mod /= 1 - max(-0.9f, height * moveData.depthMod);
	}
	return mod;
}

/*
Check if a given square-position is accessable by the footprint.
*/
bool CGroundMoveMath::IsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare) {
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
bool CGroundMoveMath::SquareIsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare) {
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
Checks if the given movedata could run thru or over the given object.
*/
bool CGroundMoveMath::IsBlocking(const MoveData& moveData, const CSolidObject* object) {
	return object->blocking && (!dynamic_cast<const CFeature*>(object) || object->mass > moveData.crushStrength);
}


/*
Gives the ground-level of given square.
*/
float CGroundMoveMath::yLevel(int xSquare, int zSquare) {
	return readmap->centerheightmap[xSquare + zSquare * gs->mapx];
}

