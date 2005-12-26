#ifndef MOBILITY_H
#define MOBILITY_H

#include "MoveInfo.h"
#include "creg/ClassReg.h"

class CMobility {
public:
	CMobility();

	CR_DECLARE(CMobility);

	MoveData* moveData;

	float maxSpeed;
	short maxTurnRate;

	float maxAcceleration;
	float maxBreaking;

	bool canFly;
	bool subMarine;
};

#endif
