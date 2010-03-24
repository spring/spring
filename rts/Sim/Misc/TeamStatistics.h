/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAMSTATISTICS_H
#define TEAMSTATISTICS_H

#include "Platform/byteorder.h"

#pragma pack(push, 1)

struct TeamStatistics
{
	float metalUsed,     energyUsed;
	float metalProduced, energyProduced;
	float metalExcess,   energyExcess;
	float metalReceived, energyReceived; ///< received from allies
	float metalSent,     energySent;     ///< sent to allies

	float damageDealt,   damageReceived; ///< Damage taken and dealt to enemy units

	int unitsProduced;
	int unitsDied;
	int unitsReceived;
	int unitsSent;
	/// units captured from enemy by us
	int unitsCaptured;
	/// units captured from us by enemy
	int unitsOutCaptured;
	/// how many enemy units have been killed by this teams units
	int unitsKilled;

	/// Change structure from host endian to little endian or vice versa.
	void swab();
};

#pragma pack(pop)

#endif
