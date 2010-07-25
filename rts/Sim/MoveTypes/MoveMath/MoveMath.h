/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
	float SpeedMod(const MoveData& moveData, const float3& pos) const;
	float SpeedMod(const MoveData& moveData, int xSquare, int zSquare) const;
	virtual float SpeedMod(const MoveData& moveData, float height, float slope) const = 0;
	float SpeedMod(const MoveData& moveData, const float3& pos, const float3& moveDir) const;
	float SpeedMod(const MoveData& moveData, int xSquare, int zSquare,const float3& moveDir) const;
	virtual float SpeedMod(const MoveData& moveData, float height, float slope, float moveSlope) const = 0;

	// tells whether a position is blocked (inaccessable for a given object's movedata)
	int IsBlocked(const MoveData& moveData, const float3& pos) const;
	int IsBlocked(const MoveData& moveData, int xSquare, int zSquare) const;
	int IsBlocked2(const MoveData& moveData, int xSquare, int zSquare) const;

	// tells whether a given object is blocking the given movedata
	bool CrushResistant(const MoveData& moveData, const CSolidObject* object) const;
	bool IsNonBlocking(const MoveData& moveData, const CSolidObject* object) const;

	// gives the y-coordinate the unit will "stand on"
	virtual float yLevel(const float3& pos) const;
	virtual float yLevel(int xSquare, int Square) const = 0;

	// returns the block-status of a single quare
	int SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare) const;

	virtual ~CMoveMath();
};

#endif
