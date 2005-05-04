/*

Used by mobile floating units.

Ie. ships.

*/



#ifndef SHIPMOVEMATH_H

#define SHIPMOVEMATH_H



#include "MoveMath.h"



class CShipMoveMath : public CMoveMath {

public:

	//SpeedMod returns a speed-multiplier for given position or data.

	float SpeedMod(const MoveData& moveData, float height, float slope);

	float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope);



	//IsBlocked tells whenever a position is blocked(=none-accessable) or not.

	bool IsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare);



	//Gives the y-coordinate the unit will "stand on".

    float yLevel(int xSquare, int zSquare);



private:

	//Investigate the block-status of a single quare.

	bool SquareIsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare);

};



#endif
