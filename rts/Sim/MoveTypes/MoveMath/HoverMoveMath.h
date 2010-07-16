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
	//SpeedMod returns a speed-multiplier for given position or data.
	float SpeedMod(const MoveData& moveData, float height, float slope);
	float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope);

	//Gives the y-coordinate the unit will "stand on".
	float yLevel(int xSquare, int zSquare);
	float yLevel(const float3& pos);

	static bool noWaterMove;
};

#endif
