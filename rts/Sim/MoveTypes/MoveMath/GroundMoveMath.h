/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDMOVEMATH_H
#define GROUNDMOVEMATH_H

#include "MoveMath.h"

/**
 * Used by ground moving units, ie. tanks and k-bots.
 */
class CGroundMoveMath :	public CMoveMath {
	CR_DECLARE(CGroundMoveMath);
public:
	float SpeedMod(const MoveDef& moveDef, float height, float slope) const;
	float SpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeScale) const;

	float yLevel(int xSquare, int zSquare) const;
	float yLevel(const float3& pos) const;

	static float waterDamageCost;
};

#endif
