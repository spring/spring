#ifndef MOVEMATH_H
#define MOVEMATH_H

#include "Sim/MoveTypes/MoveInfo.h"
#include "System/float3.h"
#include "Sim/Objects/SolidObject.h"

class CMoveMath {
public:
	//Block-check-options
	//Note: Whose options are hierarchial, with CHECK_STRUCTURE as the first check.
	const static int BLOCK_MOVING = 1;
	const static int BLOCK_MOBILE = 2;
	const static int BLOCK_MOBILE_BUSY = 4;
	const static int BLOCK_STRUCTURE = 8;
	const static int BLOCK_TERRAIN = 16;

	//SpeedMod returns a speed-multiplier for given position or data.
	float SpeedMod(const MoveData& moveData, float3 pos);
	float SpeedMod(const MoveData& moveData, int xSquare, int zSquare);
	virtual float SpeedMod(const MoveData& moveData, float height, float slope) = 0;
	float SpeedMod(const MoveData& moveData, float3 pos,const float3& moveDir);
	float SpeedMod(const MoveData& moveData, int xSquare, int zSquare,const float3& moveDir);
	virtual float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) = 0;

	//IsBlocked tells whenever a position is blocked(=none-accessable) or not.
	int IsBlocked(const MoveData& moveData, float3 pos);
	int IsBlocked(const MoveData& moveData, int xSquare, int zSquare);

	int IsBlocked2(const MoveData& moveData, int xSquare, int zSquare);

	//IsBlocking tells whenever a given object are blocking the given movedata or not.
	bool IsBlocking(const MoveData& moveData, const CSolidObject* object);

	//Gives the y-coordinate the unit will "stand on".
	float yLevel(float3 pos);
	virtual float yLevel(int xSquare, int Square) = 0;

	//Investigate the block-status of a single quare.
	int SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare);
};

#endif
