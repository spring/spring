/*
Used by ground moving units.
Ie. tanks and k-bots.
*/

#ifndef GROUNDMOVEMATH_H
#define GROUNDMOVEMATH_H

#include "MoveMath.h"

class CGroundMoveMath :	public CMoveMath {
	CR_DECLARE(CGroundMoveMath);
public:
	//SpeedMod returns a speed-multiplier for given position or data.
	float SpeedMod(const MoveData& moveData, float height, float slope);
	float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope);

	//Gives the y-coordinate the unit will "stand on".
	float yLevel(int xSquare, int zSquare);
	float yLevel(const float3& pos);

	static float waterCost;
};

#endif
