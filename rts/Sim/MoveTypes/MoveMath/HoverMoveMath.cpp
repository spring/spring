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
float CHoverMoveMath::SpeedMod(const MoveData& moveData, float height, float slope) {
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

float CHoverMoveMath::SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) {
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
float CHoverMoveMath::yLevel(int xSquare, int zSquare) {
	return ground->GetHeight(xSquare*SQUARE_SIZE, zSquare*SQUARE_SIZE) + 10;
}

