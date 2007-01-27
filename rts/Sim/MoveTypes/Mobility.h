#ifndef MOBILITY_H
#define MOBILITY_H

#include "MoveInfo.h"
#include "creg/creg.h"

class CMobility {
	CR_DECLARE(CMobility);
public:
	CMobility();
	virtual ~CMobility();

	MoveData* moveData;

	float maxSpeed;
	short maxTurnRate;

	float maxAcceleration;
	float maxBreaking;

	bool canFly;
	bool subMarine;
};

#endif
