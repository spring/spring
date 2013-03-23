/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamStatistics.h"

#include "System/Platform/byteorder.h"


CR_BIND(TeamStatistics, );
CR_REG_METADATA(TeamStatistics, (
	CR_MEMBER(frame),
	CR_MEMBER(metalUsed),
	CR_MEMBER(energyUsed),
	CR_MEMBER(metalProduced),
	CR_MEMBER(energyProduced),
	CR_MEMBER(metalExcess),
	CR_MEMBER(energyExcess),
	CR_MEMBER(metalReceived),
	CR_MEMBER(energyReceived),
	CR_MEMBER(metalSent),
	CR_MEMBER(energySent),
	CR_MEMBER(damageDealt),
	CR_MEMBER(damageReceived),
	CR_MEMBER(unitsProduced),
	CR_MEMBER(unitsDied),
	CR_MEMBER(unitsReceived),
	CR_MEMBER(unitsSent),
	CR_MEMBER(unitsCaptured),
	CR_MEMBER(unitsOutCaptured),
	CR_MEMBER(unitsKilled)
));


void TeamStatistics::swab()
{
	swabDWordInPlace(frame);
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
