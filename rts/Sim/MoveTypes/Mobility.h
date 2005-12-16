#ifndef MOBILITY_H
#define MOBILITY_H

#include "MoveInfo.h"

class CMobility {
public:
	CMobility();

	MoveData* moveData;

	float maxSpeed;
	short maxTurnRate;

	float maxAcceleration;
	float maxBreaking;

	bool canFly;
	bool subMarine;
};

#endif
