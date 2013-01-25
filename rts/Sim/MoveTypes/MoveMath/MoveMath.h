/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVEMATH_H
#define MOVEMATH_H

#include "Map/ReadMap.h"
#include "System/float3.h"
#include "System/Misc/BitwiseEnum.h"

struct MoveDef;
class CSolidObject;
class CMoveMath {
	CR_DECLARE(CMoveMath);

protected:
	static float GroundSpeedMod(const MoveDef& moveDef, float height, float slope);
	static float GroundSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod);
	static float HoverSpeedMod(const MoveDef& moveDef, float height, float slope);
	static float HoverSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod);
	static float ShipSpeedMod(const MoveDef& moveDef, float height, float slope);
	static float ShipSpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeMod);

public:
	// gives the y-coordinate the unit will "stand on"
	static float yLevel(const MoveDef& moveDef, const float3& pos);
	static float yLevel(const MoveDef& moveDef, int xSquare, int zSquare);

public:
	enum BlockTypes {
		BLOCK_NONE        = 0,
		BLOCK_MOVING      = 1,
		BLOCK_MOBILE      = 2,
		BLOCK_MOBILE_BUSY = 4,
		BLOCK_STRUCTURE   = 8,
		BLOCK_IMPASSABLE  = 24 // := 16 | BLOCK_STRUCTURE;
	};
	typedef Bitwise::BitwiseEnum<BlockTypes> BlockType;
	

	// returns a speed-multiplier for given position or data
	static float GetPosSpeedMod(const MoveDef& moveDef, int xSquare, int zSquare);
	static float GetPosSpeedMod(const MoveDef& moveDef, int xSquare, int zSquare, const float3& moveDir);
	static float GetPosSpeedMod(const MoveDef& moveDef, const float3& pos)
	{
		return GetPosSpeedMod(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE);
	}
	static float GetPosSpeedMod(const MoveDef& moveDef, const float3& pos, const float3& moveDir)
	{
		return GetPosSpeedMod(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE, moveDir);
	}

	// tells whether a position is blocked (inaccessable for a given object's MoveDef)
	static inline BlockType IsBlocked(const MoveDef& moveDef, const float3& pos, const CSolidObject* collider);
	static inline BlockType IsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static BlockType IsBlockedNoSpeedModCheck(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static bool IsBlockedStructure(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static bool IsBlockedStructureXmax(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static bool IsBlockedStructureZmax(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	
	// tells whether a given object is blocking the given MoveDef
	static bool CrushResistant(const MoveDef& colliderMD, const CSolidObject* collidee);
	static bool IsNonBlocking(const MoveDef& colliderMD, const CSolidObject* collidee, const CSolidObject* collider);
	static bool IsNonBlocking(const CSolidObject* collidee, const MoveDef* colliderMD, const float3 colliderPos, const float colliderHeight);

	// returns the block-status of a single quare
	static BlockType SquareIsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider);
	static BlockType SquareIsBlocked(const MoveDef& moveDef, const float3& pos, const CSolidObject* collider) {
		return SquareIsBlocked(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE, collider);
	}

public:
	static bool noHoverWaterMove;
	static float waterDamageCost;
};



/* Check if a given square-position is accessable by the MoveDef footprint. */
inline CMoveMath::BlockType CMoveMath::IsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	if (GetPosSpeedMod(moveDef, xSquare, zSquare) == 0.0f)
		return BLOCK_IMPASSABLE;

	return IsBlockedNoSpeedModCheck(moveDef, xSquare, zSquare, collider);
}

/* Converts a point-request into a square-positional request. */
inline CMoveMath::BlockType CMoveMath::IsBlocked(const MoveDef& moveDef, const float3& pos, const CSolidObject* collider)
{
	return IsBlocked(moveDef, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE, collider);
}

#endif

