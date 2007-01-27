#include "StdAfx.h"
#include "Mobility.h"

CR_BIND(CMobility, );

CR_REG_METADATA(CMobility, (
	CR_MEMBER(moveData),

	CR_MEMBER(maxSpeed),
	CR_MEMBER(maxTurnRate),

	CR_MEMBER(maxAcceleration),
	CR_MEMBER(maxBreaking),

	CR_MEMBER(canFly),
	CR_MEMBER(subMarine)));


/*
Constructor
*/
CMobility::CMobility() :
moveData(0),
maxSpeed(0),
maxTurnRate(0),
maxAcceleration(0),
maxBreaking(0),
canFly(false),
subMarine(false) {
}

CMobility::~CMobility() {
}
