/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEMATH_H
#define MOVEMATH_H

#include "Sim/MoveTypes/MoveInfo.h"
#include "System/float3.h"
#include "Sim/Objects/SolidObject.h"


class CMoveMath {
	CR_DECLARE(CMoveMath);
	
protected:
	virtual float SpeedMod(const MoveData& moveData, float height, float slope) const = 0;
	virtual float SpeedMod(const MoveData& moveData, float height, float slope, float moveSlope) const = 0;

public:
	// gives the y-coordinate the unit will "stand on"
	virtual float yLevel(const float3& pos) const;
	virtual float yLevel(int xSquare, int Square) const = 0;

public:
	const static int BLOCK_MOVING      = 1;
	const static int BLOCK_MOBILE      = 2;
	const static int BLOCK_MOBILE_BUSY = 4;
	const static int BLOCK_STRUCTURE   = 8;
	const static int BLOCK_IMPASSABLE  = 16 | BLOCK_STRUCTURE;

	// returns a speed-multiplier for given position or data
	float GetPosSpeedMod(const MoveData& moveData, int xSquare, int zSquare) const;
	float GetPosSpeedMod(const MoveData& moveData, int xSquare, int zSquare, const float3& moveDir) const;
	float GetPosSpeedMod(const MoveData& moveData, const float3& pos) const
	{
		return GetPosSpeedMod(moveData, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE);
	}
	float GetPosSpeedMod(const MoveData& moveData, const float3& pos, const float3& moveDir) const
	{
		return GetPosSpeedMod(moveData, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE, moveDir);
	}

	// tells whether a position is blocked (inaccessable for a given object's movedata)
	int IsBlocked(const MoveData& moveData, const float3& pos) const;
	int IsBlocked(const MoveData& moveData, int xSquare, int zSquare) const;
	
	// tells whether a given object is blocking the given movedata
	static bool CrushResistant(const MoveData& moveData, const CSolidObject* object);
	static bool IsNonBlocking(const MoveData& moveData, const CSolidObject* object);

	// returns the block-status of a single quare
	static int SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare);

	virtual ~CMoveMath() {}
};

#endif
