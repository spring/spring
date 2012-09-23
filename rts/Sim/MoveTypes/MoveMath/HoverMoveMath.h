/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifndef HOVERMOVEMATH_H
#define HOVERMOVEMATH_H

#include "MoveMath.h"

/**
 * Used by hovering units, ie. hover-tanks.
 */
class CHoverMoveMath : public CMoveMath {
	CR_DECLARE(CHoverMoveMath);
public:
	float SpeedMod(const MoveDef& moveDef, float height, float slope) const;
	float SpeedMod(const MoveDef& moveDef, float height, float slope, float dirSlopeScale) const;

	float yLevel(int xSquare, int zSquare) const;
	float yLevel(const float3& pos) const;

	static bool noWaterMove;
};

#endif
