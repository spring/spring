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
	inline bool IsBlockedStructureNoSpeedModCheck(const MoveData& moveData, int xSquare, int zSquare) const;
	inline bool IsBlockedStructureXmax(const MoveData& moveData, int xSquare, int zSquare) const;
	inline bool IsBlockedStructureZmax(const MoveData& moveData, int xSquare, int zSquare) const;
	
	// tells whether a given object is blocking the given movedata
	static bool CrushResistant(const MoveData& moveData, const CSolidObject* object);
	static bool IsNonBlocking(const MoveData& moveData, const CSolidObject* object);

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


/* Optimized function to check if a given square-position has a structure block. */
inline bool CMoveMath::IsBlockedStructureNoSpeedModCheck(const MoveData& moveData, int xSquare, int zSquare) const
{
	const int xSize = moveData.xsize;
	const int zSize = moveData.zsize;
	const bool xSizeOdd = (xSize & 1) != 0;
	const bool zSizeOdd = (zSize & 1) != 0;
	const int xstep = 2, zstep = 2;
	const int xh = (xSize - 1) >> 1;
	const int zh = (zSize - 1) >> 1;
	const int xmin = xSquare - xh, xmax = xSquare + xh;
	const int zmin = zSquare - zh, zmax = zSquare + zh;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int x = xmin; x <= xmax; x += xstep) {
		for (int z = zmin; z <= zmax; z += zstep) {
			if (SquareIsBlocked(moveData, x, z) & BLOCK_STRUCTURE)
				return true;
		}
	}
	if (xSizeOdd && zSizeOdd)
		return false;
	// Try extending the footprint towards each possible "even" side, and
	// only consider it blocked if all those sides have a matching BlockType.
	// This is an approximation, but better than truncating it to odd size.
	bool bxmin = false, bxmax = false;
	if (!xSizeOdd) {
		for (int z = zmin; z <= zmax; z += zstep) {
			if (SquareIsBlocked(moveData, xmin - 1, z) & BLOCK_STRUCTURE) {
				bxmin = true;
				break;
			}
		}
		for (int z = zmin; z <= zmax; z += zstep) {
			if (SquareIsBlocked(moveData, xmax + 1, z) & BLOCK_STRUCTURE) {
				bxmax = true;
				break;
			}
		}
		if (bxmin && bxmax)
			return true;
		if (zSizeOdd)
			return false;
	}
	bool bzmin = false, bzmax = false;
	if (!zSizeOdd) {
		for (int x = xmin; x <= xmax; x += xstep) {
			if (SquareIsBlocked(moveData, x, zmin - 1) & BLOCK_STRUCTURE) {
				bzmin = true;
				break;
			}
		}
		for (int x = xmin; x <= xmax; x += xstep) {
			if (SquareIsBlocked(moveData, x, zmax + 1) & BLOCK_STRUCTURE) {
				bzmax = true;
				break;
			}
		}
		if (bzmin && bzmax)
			return true;
		if (xSizeOdd)
			return false;
	}
	return 
		((bxmin || bzmin || (SquareIsBlocked(moveData, xmin - 1, zmin - 1) & BLOCK_STRUCTURE)) &&
		 (bxmin || bzmax || (SquareIsBlocked(moveData, xmin - 1, zmax + 1) & BLOCK_STRUCTURE)) &&
		 (bxmax || bzmin || (SquareIsBlocked(moveData, xmax + 1, zmin - 1) & BLOCK_STRUCTURE)) &&
		 (bxmax || bzmax || (SquareIsBlocked(moveData, xmax + 1, zmax + 1) & BLOCK_STRUCTURE)));
}


/* Optimized function to check if the square at the given position has a structure block, 
   provided that the square at (xSquare - 1, zSquare) did not have a structure block */
inline bool CMoveMath::IsBlockedStructureXmax(const MoveData& moveData, int xSquare, int zSquare) const
{
	const int xSize = moveData.xsize;
	const int zSize = moveData.zsize;
	const bool xSizeOdd = (xSize & 1) != 0;
	const bool zSizeOdd = (zSize & 1) != 0;
	const int xstep = 2, zstep = 2;
	const int xh = (xSize - 1) >> 1;
	const int zh = (zSize - 1) >> 1;
	const int xmin = xSquare - xh, xmax = xSquare + xh;
	const int zmin = zSquare - zh, zmax = zSquare + zh;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int z = zmin; z <= zmax; z += zstep) {
		if (SquareIsBlocked(moveData, xmax, z) & BLOCK_STRUCTURE)
			return true;
	}
	if (zSizeOdd)
		return false;
	bool bmin = (SquareIsBlocked(moveData, xmax, zmin - 1) & BLOCK_STRUCTURE) != 0;
	bool bmax = (SquareIsBlocked(moveData, xmax, zmax + 1) & BLOCK_STRUCTURE) != 0;
	if (!bmin && !bmax)
		return false;
	if (bmin && bmax)
		return true;
	if (bmax) {
		if (!xSizeOdd && (SquareIsBlocked(moveData, xmin - 1, zmin - 1) & BLOCK_STRUCTURE))
			return true;
		for (int x = xmin; x < xmax; x += xstep) {
			if (SquareIsBlocked(moveData, x, zmin - 1) & BLOCK_STRUCTURE)
				return true;
		}
	}
	else {
		if (!xSizeOdd && (SquareIsBlocked(moveData, xmin - 1, zmax + 1) & BLOCK_STRUCTURE))
			return true;
		for (int x = xmin; x < xmax; x += xstep) {
			if (SquareIsBlocked(moveData, x, zmax + 1) & BLOCK_STRUCTURE)
				return true;
		}
	}
	return false;
}


/* Optimized function to check if the square at the given position has a structure block, 
   provided that the square at (xSquare, zSquare - 1) did not have a structure block */
inline bool CMoveMath::IsBlockedStructureZmax(const MoveData& moveData, int xSquare, int zSquare) const
{
	const int xSize = moveData.xsize;
	const int zSize = moveData.zsize;
	const bool xSizeOdd = (xSize & 1) != 0;
	const bool zSizeOdd = (zSize & 1) != 0;
	const int xstep = 2, zstep = 2;
	const int xh = (xSize - 1) >> 1;
	const int zh = (zSize - 1) >> 1;
	const int xmin = xSquare - xh, xmax = xSquare + xh;
	const int zmin = zSquare - zh, zmax = zSquare + zh;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int x = xmin; x <= xmax; x += xstep) {
		if (SquareIsBlocked(moveData, x, zmax) & BLOCK_STRUCTURE)
			return true;
	}
	if (xSizeOdd)
		return false;
	bool bmin = (SquareIsBlocked(moveData, xmin - 1, zmax) & BLOCK_STRUCTURE) != 0;
	bool bmax = (SquareIsBlocked(moveData, xmax + 1, zmax) & BLOCK_STRUCTURE) != 0;
	if (!bmin && !bmax)
		return false;
	if (bmin && bmax)
		return true;
	if (bmax) {
		if (!zSizeOdd && (SquareIsBlocked(moveData, xmin - 1, zmin - 1) & BLOCK_STRUCTURE))
			return true;
		for (int z = zmin; z < zmax; z += zstep) {
			if (SquareIsBlocked(moveData, xmin - 1, z) & BLOCK_STRUCTURE)
				return true;
		}
	}
	else {
		if (!zSizeOdd && (SquareIsBlocked(moveData, xmax + 1, zmin - 1) & BLOCK_STRUCTURE))
			return true;
		for (int z = zmin; z < zmax; z += zstep) {
			if (SquareIsBlocked(moveData, xmax + 1, z) & BLOCK_STRUCTURE)
				return true;
		}
	}
	return false;
}

#endif
