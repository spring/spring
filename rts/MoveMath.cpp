#include "stdafx.h"
#include "MoveMath.h"
#include "ReadMap.h"
//#include "mmgr.h"


/*
Converts a point-request into a square-positional request.
*/
float CMoveMath::SpeedMod(const MoveData& moveData, float3 pos) {
	int x = pos.x / SQUARE_SIZE;
	int z = pos.z / SQUARE_SIZE;
	return SpeedMod(moveData, x, z);
}


/*
Extract height and slope data from given square and make
a sub-functional call for calculations upon whose data.
*/
float CMoveMath::SpeedMod(const MoveData& moveData, int xSquare, int zSquare) {
	//Error-check
	if(xSquare < 0 || zSquare < 0
	|| xSquare >= gs->mapx || zSquare >= gs->mapy)
		return 0.0f;

	//Extract data.
	int square = xSquare + zSquare * gs->mapx;
	float height = readmap->centerheightmap[xSquare + zSquare * gs->mapx];
	float slope = 1 - readmap->facenormals[square*2].y;

	//Call calculations.
	return SpeedMod(moveData, height, slope);
}

float CMoveMath::SpeedMod(const MoveData& moveData, float3 pos,const float3& moveDir) {
	int x = pos.x / SQUARE_SIZE;
	int z = pos.z / SQUARE_SIZE;
	return SpeedMod(moveData, x, z,moveDir);
}


/*
Extract height and slope data from given square and make
a sub-functional call for calculations upon whose data.
*/
float CMoveMath::SpeedMod(const MoveData& moveData, int xSquare, int zSquare,const float3& moveDir) {
	//Error-check
	if(xSquare < 0 || zSquare < 0
	|| xSquare >= gs->mapx || zSquare >= gs->mapy)
		return 0.0f;

	//Extract data.
	int square = xSquare + zSquare * gs->mapx;
	float height = readmap->centerheightmap[xSquare + zSquare * gs->mapx];
	float slope = 1 - readmap->facenormals[square*2].y;
	float3 flatNorm=readmap->facenormals[square*2];
	flatNorm.y=0;
	flatNorm.Normalize();
	float moveSlope=-moveDir.dot(flatNorm);

	//Call calculations.
	return SpeedMod(moveData, height, slope,moveSlope);
}

/*
Converts a point-request into a square-positional request.
*/
bool CMoveMath::IsBlocked(const MoveData& moveData, int blockOpt, float3 pos) {
	int x = pos.x / SQUARE_SIZE;
	int z = pos.z / SQUARE_SIZE;
	return IsBlocked(moveData, blockOpt, x, z);
}


/*
Simply check if the object is blocking or not.
*/
bool CMoveMath::IsBlocking(const MoveData& moveData, const CSolidObject* object) {
	return object->blocking;
}


/*
Converts a point-request into a square-positional request.
*/
float CMoveMath::yLevel(const float3 pos) {
	int x = pos.x / SQUARE_SIZE;
	int z = pos.z / SQUARE_SIZE;
	return yLevel(x, z);
}