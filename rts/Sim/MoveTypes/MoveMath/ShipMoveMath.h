/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHIPMOVEMATH_H
#define SHIPMOVEMATH_H

#include "MoveMath.h"

/**
 * Used by mobile floating units, ie. ships.
 */
class CShipMoveMath : public CMoveMath {
	CR_DECLARE(CShipMoveMath);
public:
	float SpeedMod(const MoveData& moveData, float height, float slope) const;
	float SpeedMod(const MoveData& moveData, float height, float slope, float moveSlope) const;

	// ships are always at water-level
	float yLevel(int xSquare, int zSquare) const { return 0.0f; }
	float yLevel(const float3& pos) const { return 0.0f; }
};

#endif
