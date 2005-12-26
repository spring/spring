#include "StdAfx.h"
#include "Mobility.h"

CR_BIND(CMobility);

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
