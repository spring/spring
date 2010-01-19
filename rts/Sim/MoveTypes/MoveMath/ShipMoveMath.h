/*
Used by mobile floating units.
Ie. ships.
*/

#ifndef SHIPMOVEMATH_H
#define SHIPMOVEMATH_H

#include "MoveMath.h"

class CShipMoveMath : public CMoveMath {
	CR_DECLARE(CShipMoveMath);
public:
	//SpeedMod returns a speed-multiplier for given position or data.
	float SpeedMod(const MoveData& moveData, float height, float slope);
	float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope);

	//Gives the y-coordinate the unit will "stand on".
	float yLevel(int xSquare, int zSquare);
	float yLevel(const float3& pos);
};

#endif
