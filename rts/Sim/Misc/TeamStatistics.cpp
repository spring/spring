/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamStatistics.h"

#include "Platform/byteorder.h"

void TeamStatistics::swab()
{
	metalUsed        = swabfloat(metalUsed);
	energyUsed       = swabfloat(energyUsed);
	metalProduced    = swabfloat(metalProduced);
	energyProduced   = swabfloat(energyProduced);
	metalExcess      = swabfloat(metalExcess);
	energyExcess     = swabfloat(energyExcess);
	metalReceived    = swabfloat(metalReceived);
	energyReceived   = swabfloat(energyReceived);
	metalSent        = swabfloat(metalSent);
	energySent       = swabfloat(energySent);
	damageDealt      = swabfloat(damageDealt);
	damageReceived   = swabfloat(damageReceived);
	unitsProduced    = swabdword(unitsProduced);
	unitsDied        = swabdword(unitsDied);
	unitsReceived    = swabdword(unitsReceived);
	unitsSent        = swabdword(unitsSent);
	unitsCaptured    = swabdword(unitsCaptured);
	unitsOutCaptured = swabdword(unitsOutCaptured);
	unitsKilled      = swabdword(unitsKilled);
}
