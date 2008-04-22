#include "StdAfx.h"
#include "MoveMath.h"
#include "Map/ReadMap.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "LogOutput.h"
#include "mmgr.h"

CR_BIND_INTERFACE(CMoveMath);

/*
Converts a point-request into a square-positional request.
*/
float CMoveMath::SpeedMod(const MoveData& moveData, float3 pos) {
	int x = ((int)pos.x) / SQUARE_SIZE;
	int z = ((int)pos.z) / SQUARE_SIZE;
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
	int square = xSquare/2 + zSquare/2 * gs->hmapx;
	float height = readmap->mipHeightmap[1][square];
	float slope = readmap->slopemap[square];
	float typemod=moveinfo->terrainType2MoveFamilySpeed[readmap->typemap[square]][moveData.moveFamily];

	//Call calculations.
	return SpeedMod(moveData, height, slope)*typemod;
}

float CMoveMath::SpeedMod(const MoveData& moveData, float3 pos,const float3& moveDir) {
	int x = (int)pos.x / SQUARE_SIZE;
	int z = (int)pos.z / SQUARE_SIZE;
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
	int square = xSquare/2 + zSquare/2 * gs->hmapx;
	float height = readmap->mipHeightmap[1][square];
	float slope = readmap->slopemap[square];

	float3 flatNorm=readmap->facenormals[(xSquare + zSquare * gs->mapx)*2];
	flatNorm.y=0;
	flatNorm.Normalize();
	float moveSlope=-moveDir.dot(flatNorm);
	float typemod=moveinfo->terrainType2MoveFamilySpeed[readmap->typemap[square]][moveData.moveFamily];

	//Call calculations.
	return SpeedMod(moveData, height, slope,moveSlope)*typemod;
}

/*
Converts a point-request into a square-positional request.
*/
int CMoveMath::IsBlocked(const MoveData& moveData, float3 pos) {
	int x = (int)(pos.x / SQUARE_SIZE);
	int z = (int)(pos.z / SQUARE_SIZE);
	return IsBlocked(moveData, x, z);
}

/*
Check if a given square-position is accessable by the footprint.
this one check terrain
*/
int CMoveMath::IsBlocked(const MoveData& moveData, int xSquare, int zSquare) {
	if(CMoveMath::SpeedMod(moveData, xSquare, zSquare) == 0.0f)
		return true;

	int ret=0;

	ret|=SquareIsBlocked(moveData, xSquare, zSquare);
	ret|=SquareIsBlocked(moveData, xSquare - moveData.size/2, zSquare - moveData.size/2);
	ret|=SquareIsBlocked(moveData, xSquare + moveData.size/2 - 1, zSquare - moveData.size/2);
	ret|=SquareIsBlocked(moveData, xSquare - moveData.size/2, zSquare + moveData.size/2 - 1);
	ret|=SquareIsBlocked(moveData, xSquare + moveData.size/2 - 1, zSquare + moveData.size/2 - 1);

	return ret;
}

/*
Check if a given square-position is accessable by the footprint.
this one doesnt check terrain
but do check so it doesnt run over something small if its big
*/
int CMoveMath::IsBlocked2(const MoveData& moveData, int xSquare, int zSquare) 
{
	int ret=0;

	switch(moveData.size){
	case 12:
	case 11:
		ret|=SquareIsBlocked(moveData, xSquare+4, zSquare+4);
		ret|=SquareIsBlocked(moveData, xSquare-6, zSquare+4);
		ret|=SquareIsBlocked(moveData, xSquare-6, zSquare-6);
		ret|=SquareIsBlocked(moveData, xSquare+4, zSquare-6);
	case 8:
	case 7:
		ret|=SquareIsBlocked(moveData, xSquare+2, zSquare+2);
		ret|=SquareIsBlocked(moveData, xSquare-4, zSquare+2);
		ret|=SquareIsBlocked(moveData, xSquare-4, zSquare-4);
		ret|=SquareIsBlocked(moveData, xSquare+2, zSquare-4);
	case 4:
	case 3:
		ret|=SquareIsBlocked(moveData, xSquare, zSquare);
		ret|=SquareIsBlocked(moveData, xSquare-2, zSquare);
		ret|=SquareIsBlocked(moveData, xSquare, zSquare-2);
		ret|=SquareIsBlocked(moveData, xSquare-2, zSquare-2);
		break;

	case 14:
	case 13:
		ret|=SquareIsBlocked(moveData, xSquare+6, zSquare+6);
		ret|=SquareIsBlocked(moveData, xSquare-6, zSquare+6);
		ret|=SquareIsBlocked(moveData, xSquare-6, zSquare-6);
		ret|=SquareIsBlocked(moveData, xSquare+6, zSquare-6);
	case 10:
	case 9:
		ret|=SquareIsBlocked(moveData, xSquare+4, zSquare+4);
		ret|=SquareIsBlocked(moveData, xSquare-4, zSquare+4);
		ret|=SquareIsBlocked(moveData, xSquare-4, zSquare-4);
		ret|=SquareIsBlocked(moveData, xSquare+4, zSquare-4);
	case 6:
	case 5:
		ret|=SquareIsBlocked(moveData, xSquare+2, zSquare+2);
		ret|=SquareIsBlocked(moveData, xSquare-2, zSquare+2);
		ret|=SquareIsBlocked(moveData, xSquare-2, zSquare-2);
		ret|=SquareIsBlocked(moveData, xSquare+2, zSquare-2);
	case 2:
	case 1:
		ret|=SquareIsBlocked(moveData, xSquare, zSquare);
		break;

	default:
		logOutput.Print("Unknown size in IsBlocked2 %i",moveData.size);
		break;
	};
	return ret;
}

/*
Simply check if the object is blocking or not.
*/
bool CMoveMath::IsBlocking(const MoveData& moveData, const CSolidObject* object) {
	return object->blocking && (!dynamic_cast<const CFeature*>(object) || object->mass > moveData.crushStrength);
}


/*
Converts a point-request into a square-positional request.
*/
float CMoveMath::yLevel(const float3 pos) {
	int x = (int)(pos.x / SQUARE_SIZE);
	int z = (int)(pos.z / SQUARE_SIZE);
	return yLevel(x, z);
}

/*
Check if a single square is accessable.
*/
int CMoveMath::SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare) {
	//Error-check
	if(xSquare < 0 || zSquare < 0
	|| xSquare >= gs->mapx || zSquare >= gs->mapy)
		return true;

	CSolidObject* obstacle = groundBlockingObjectMap->GroundBlockedUnsafe(xSquare + zSquare * gs->mapx);
	if(obstacle) {
		if(obstacle->mobility) {
			if(obstacle->isMoving){
				return BLOCK_MOVING;
			} else {
				if(!((CUnit*)obstacle)->beingBuilt && ((CUnit*)obstacle)->commandAI->commandQue.empty()){
					return BLOCK_MOBILE;
				} else {
					return BLOCK_MOBILE_BUSY;
				}
			}
		} else {
			if(IsBlocking(moveData, obstacle))
				return BLOCK_STRUCTURE;
		}
	}
  return 0;
}

CMoveMath::~CMoveMath() {
}
