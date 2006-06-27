#ifndef MOBILITY_H
#define MOBILITY_H

#include "MoveInfo.h"
#include "creg/creg.h"

class CMobility {
public:
	CMobility();
	virtual ~CMobility();

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
