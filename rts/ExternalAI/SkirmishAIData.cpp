/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExternalAI/SkirmishAIData.h"

#include "System/creg/STL_Map.h"
#include "System/Platform/byteorder.h" // for swabDWord


CR_BIND_DERIVED(SkirmishAIStatistics, TeamControllerStatistics, );
CR_REG_METADATA(SkirmishAIStatistics, (
	CR_MEMBER(cpuTime)
));


CR_BIND(SkirmishAIData,);
CR_REG_METADATA(SkirmishAIData, (
	// from TeamController
	CR_MEMBER(name),
	CR_MEMBER(team),
	// from SkirmishAIBase
	CR_MEMBER(hostPlayer),
	CR_ENUM_MEMBER(status),
	// from SkirmishAIData
	CR_MEMBER(shortName),
	CR_MEMBER(version),
	CR_MEMBER(optionKeys),
	CR_MEMBER(options),
	CR_MEMBER(isLuaAI),
	CR_MEMBER(currentStats)
));


void SkirmishAIStatistics::swab() {
	swabDWordInPlace(cpuTime);
	TeamControllerStatistics::swab();
}
