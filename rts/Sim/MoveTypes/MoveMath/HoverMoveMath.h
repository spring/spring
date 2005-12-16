/*
Used by hovering units.
Ie. hover-tanks.
*/

#ifndef HOVERMOVEMATH_H
#define HOVERMOVEMATH_H

#include "MoveMath.h"

class CHoverMoveMath : public CMoveMath {
public:
	//SpeedMod returns a speed-multiplier for given position or data.
	float SpeedMod(const MoveData& moveData, float height, float slope);
	float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope);

	//Gives the y-coordinate the unit will "stand on".
    float yLevel(int xSquare, int zSquare);

	static bool noWaterMove;
};

#endif
