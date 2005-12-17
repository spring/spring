#include "System/StdAfx.h"
#include "GroundMoveMath.h"
#include "Sim/Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Misc/Feature.h"

float CGroundMoveMath::waterCost=0;

/*
Calculate speed-multiplier for given height and slope data.
*/
float CGroundMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) 
{
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
		mod*=waterCost;
	}
	return mod;
}

float CGroundMoveMath::SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) 
{
	//Too slope?
/*	if(slope*moveSlope > moveData.maxSlope)
		return 0.0f;
	//Too depth?
	if(-height > moveData.depth && moveSlope<0)
		return 0.0f;
*/	//Slope-mod
	float mod = 1 / (1 + max(0.0f,slope*moveSlope) * moveData.slopeMod);
	//Under-water-mod
	if(height < 0) {
		mod /= 1 - max(-0.9f, height * moveData.depthMod);
	}
	return mod;
}

/*
Gives the ground-level of given square.
*/
float CGroundMoveMath::yLevel(int xSquare, int zSquare) {
	return readmap->centerheightmap[xSquare + zSquare * gs->mapx];
}

