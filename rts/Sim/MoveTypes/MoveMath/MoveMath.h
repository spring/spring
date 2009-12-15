#ifndef MOVEMATH_H
#define MOVEMATH_H

#include "Sim/MoveTypes/MoveInfo.h"
#include "float3.h"
#include "Sim/Objects/SolidObject.h"

class CMoveMath {
	CR_DECLARE(CMoveMath);
public:
	// Block-check-options
	// Note: options are hierarchical, with CHECK_STRUCTURE as the first check.
	const static int BLOCK_MOVING = 1;
	const static int BLOCK_MOBILE = 2;
	const static int BLOCK_MOBILE_BUSY = 4;
	const static int BLOCK_STRUCTURE = 8;
	const static int BLOCK_TERRAIN = 16;

	// returns a speed-multiplier for given position or data
	float SpeedMod(const MoveData& moveData, float3 pos);
	float SpeedMod(const MoveData& moveData, int xSquare, int zSquare);
	virtual float SpeedMod(const MoveData& moveData, float height, float slope) = 0;
	float SpeedMod(const MoveData& moveData, float3 pos,const float3& moveDir);
	float SpeedMod(const MoveData& moveData, int xSquare, int zSquare,const float3& moveDir);
	virtual float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope) = 0;

	// tells whether a position is blocked (inaccessable for a given object's movedata)
	int IsBlocked(const MoveData& moveData, float3 pos, bool fromEst = false);
	int IsBlocked(const MoveData& moveData, int xSquare, int zSquare, bool fromEst = false);
	int IsBlocked2(const MoveData& moveData, int xSquare, int zSquare, bool fromEst = false);

	// tells whether a given object is blocking the given movedata
	bool CrushResistant(const MoveData& moveData, const CSolidObject* object);
	bool IsNonBlocking(const MoveData& moveData, const CSolidObject* object);

	// gives the y-coordinate the unit will "stand on"
	virtual float yLevel(const float3& pos);
	virtual float yLevel(int xSquare, int Square) = 0;

	// returns the block-status of a single quare
	int SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare, bool fromEst = false);

	virtual ~CMoveMath();
};

#endif
