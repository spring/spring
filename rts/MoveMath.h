#ifndef MOVEMATH_H
#define MOVEMATH_H

#include "MoveInfo.h"
#include "float3.h"
#include "SolidObject.h"

class CMoveMath {
public:
	//Block-check-options
	//Note: Whose options are hierarchial, with CHECK_STRUCTURE as the first check.
	const static int CHECK_STRUCTURES = 1;
	const static int CHECK_MOBILE = 2;

	//SpeedMod returns a speed-multiplier for given position or data.
	float SpeedMod(const MoveData& moveData, float3 pos);
	float SpeedMod(const MoveData& moveData, int xSquare, int zSquare);
	virtual float SpeedMod(const MoveData& moveData, float height, float slope) = 0;
	float SpeedMod(const MoveData& moveData, float3 pos,const float3& moveDir);
	float SpeedMod(const MoveData& moveData, int xSquare, int zSquare,const float3& moveDir);
	virtual float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) = 0;

	//IsBlocked tells whenever a position is blocked(=none-accessable) or not.
	bool IsBlocked(const MoveData& moveData, int blockOpt, float3 pos);
	virtual bool IsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare) = 0;

	//IsBlocking tells whenever a given object are blocking the given movedata or not.
	virtual bool IsBlocking(const MoveData& moveData, const CSolidObject* object);

	//Gives the y-coordinate the unit will "stand on".
	float yLevel(float3 pos);
	virtual float yLevel(int xSquare, int Square) = 0;
};

#endif