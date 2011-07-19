/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamStatistics.h"

#include "System/Platform/byteorder.h"

void TeamStatistics::swab()
{
	swabFloatInPlace(metalUsed);
	swabFloatInPlace(energyUsed);
	swabFloatInPlace(metalProduced);
	swabFloatInPlace(energyProduced);
	swabFloatInPlace(metalExcess);
	swabFloatInPlace(energyExcess);
	swabFloatInPlace(metalReceived);
	swabFloatInPlace(energyReceived);
	swabFloatInPlace(metalSent);
	swabFloatInPlace(energySent);
	swabFloatInPlace(damageDealt);
	swabFloatInPlace(damageReceived);
	swabDWordInPlace(unitsProduced);
	swabDWordInPlace(unitsDied);
	swabDWordInPlace(unitsReceived);
	swabDWordInPlace(unitsSent);
	swabDWordInPlace(unitsCaptured);
	swabDWordInPlace(unitsOutCaptured);
	swabDWordInPlace(unitsKilled);
}
