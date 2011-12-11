/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEMATH_H
#define MOVEMATH_H

#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Objects/SolidObject.h"
#include "System/float3.h"
#include "System/Misc/BitwiseEnum.h"


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
	enum BlockTypes {
		BLOCK_NONE        = 0,
		BLOCK_MOVING      = 1,
		BLOCK_MOBILE      = 2,
		BLOCK_MOBILE_BUSY = 4,
		BLOCK_STRUCTURE   = 8,
		BLOCK_IMPASSABLE  = 24 // := 16 | BLOCK_STRUCTURE;
	};
	typedef BitwiseEnum<BlockTypes> BlockType;
	

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
	inline BlockType IsBlocked(const MoveData& moveData, const float3& pos) const;
	inline BlockType IsBlocked(const MoveData& moveData, int xSquare, int zSquare) const;
	BlockType IsBlockedNoSpeedModCheck(const MoveData& moveData, int xSquare, int zSquare) const;
	bool IsBlockedStructure(const MoveData& moveData, int xSquare, int zSquare) const;
	bool IsBlockedStructureXmax(const MoveData& moveData, int xSquare, int zSquare) const;
	bool IsBlockedStructureZmax(const MoveData& moveData, int xSquare, int zSquare) const;
	
	// tells whether a given object is blocking the given movedata
	static bool CrushResistant(const MoveData& colliderMD, const CSolidObject* collidee);
	static bool IsNonBlocking(const MoveData& colliderMD, const CSolidObject* collidee);

	// returns the block-status of a single quare
	static BlockType SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare);

	virtual ~CMoveMath() {}
};

/* Check if a given square-position is accessable by the movedata footprint. */
inline CMoveMath::BlockType CMoveMath::IsBlocked(const MoveData& moveData, int xSquare, int zSquare) const
{
	if (GetPosSpeedMod(moveData, xSquare, zSquare) == 0.0f)
		return BLOCK_IMPASSABLE;
	return IsBlockedNoSpeedModCheck(moveData, xSquare, zSquare);
}

/* Converts a point-request into a square-positional request. */
inline CMoveMath::BlockType CMoveMath::IsBlocked(const MoveData& moveData, const float3& pos) const
{
	return IsBlocked(moveData, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE);
}

#endif
