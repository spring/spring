/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEMATH_H
#define MOVEMATH_H

#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "System/float3.h"
#include "System/Misc/BitwiseEnum.h"


class CMoveMath {
	CR_DECLARE(CMoveMath);
	
protected:
	virtual float SpeedMod(const MoveDef& moveDef, float height, float slope) const = 0;
	virtual float SpeedMod(const MoveDef& moveDef, float height, float slope, float moveSlope) const = 0;

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
	float GetPosSpeedMod(const MoveDef& moveDef, int xSquare, int zSquare) const;
	float GetPosSpeedMod(const MoveDef& moveDef, int xSquare, int zSquare, const float3& moveDir) const;
	float GetPosSpeedMod(const MoveDef& moveDef, const float3& pos) const
	{
		return GetPosSpeedMod(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE);
	}
	float GetPosSpeedMod(const MoveDef& moveDef, const float3& pos, const float3& moveDir) const
	{
		return GetPosSpeedMod(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE, moveDir);
	}

	// tells whether a position is blocked (inaccessable for a given object's MoveDef)
	inline BlockType IsBlocked(const MoveDef& moveDef, const float3& pos) const;
	inline BlockType IsBlocked(const MoveDef& moveDef, int xSquare, int zSquare) const;
	BlockType IsBlockedNoSpeedModCheck(const MoveDef& moveDef, int xSquare, int zSquare) const;
	bool IsBlockedStructure(const MoveDef& moveDef, int xSquare, int zSquare) const;
	bool IsBlockedStructureXmax(const MoveDef& moveDef, int xSquare, int zSquare) const;
	bool IsBlockedStructureZmax(const MoveDef& moveDef, int xSquare, int zSquare) const;
	
	// tells whether a given object is blocking the given MoveDef
	static bool CrushResistant(const MoveDef& colliderMD, const CSolidObject* collidee);
	static bool IsNonBlocking(const MoveDef& colliderMD, const CSolidObject* collidee);

	// returns the block-status of a single quare
	static BlockType SquareIsBlocked(const MoveDef& moveDef, int xSquare, int zSquare);

	virtual ~CMoveMath() {}
};

/* Check if a given square-position is accessable by the MoveDef footprint. */
inline CMoveMath::BlockType CMoveMath::IsBlocked(const MoveDef& moveDef, int xSquare, int zSquare) const
{
	if (GetPosSpeedMod(moveDef, xSquare, zSquare) == 0.0f)
		return BLOCK_IMPASSABLE;
	return IsBlockedNoSpeedModCheck(moveDef, xSquare, zSquare);
}

/* Converts a point-request into a square-positional request. */
inline CMoveMath::BlockType CMoveMath::IsBlocked(const MoveDef& moveDef, const float3& pos) const
{
	return IsBlocked(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE);
}

#endif
