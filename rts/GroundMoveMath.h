/*

Used by ground moving units.

Ie. tanks and k-bots.

*/



#ifndef GROUNDMOVEMATH_H

#define GROUNDMOVEMATH_H



#include "MoveMath.h"



class CGroundMoveMath :	public CMoveMath {

public:

	//SpeedMod returns a speed-multiplier for given position or data.

	float SpeedMod(const MoveData& moveData, float height, float slope);

	float SpeedMod(const MoveData& moveData, float height, float slope,float moveSlope);



	//IsBlocked tells whenever a position is blocked(=none-accessable) or not.

	bool IsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare);



	//IsBlocking tells whenever a given object are blocking the given movedata or not.

	bool IsBlocking(const MoveData& moveData, const CSolidObject* object);



	//Gives the y-coordinate the unit will "stand on".

    float yLevel(int xSquare, int zSquare);



private:

	//Investigate the block-status of a single quare.

	bool SquareIsBlocked(const MoveData& moveData, int blockOpt, int xSquare, int zSquare);

};



#endif
