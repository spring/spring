/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdlib>

#include "PlayerBase.h"
#include "System/StringHash.h"
#include "System/creg/STL_Map.h"


CR_BIND_DERIVED(PlayerBase,TeamController, )
CR_REG_METADATA(PlayerBase, (
	CR_MEMBER(rank),
	CR_MEMBER(countryCode),
	CR_MEMBER(spectator),
	CR_MEMBER(isFromDemo),
	CR_MEMBER(readyToStart),
	CR_MEMBER(desynced),
	CR_MEMBER(cpuUsage),
	CR_MEMBER(customValues)
))



PlayerBase::PlayerBase() {
	// NB: sync-safe so long as PlayerHandler destroys all players on reload
	customValues.reserve(8);
}

void PlayerBase::SetValue(const std::string& key, const std::string& value)
{
	switch (hashString(key.c_str())) {
		case hashString("team"): {
			team = std::atoi(value.c_str());
		} break;
		case hashString("name"): {
			name = value;
		} break;
		case hashString("rank"): {
			rank = std::atoi(value.c_str());
		} break;
		case hashString("countrycode"): {
			countryCode = value;
		} break;
		case hashString("spectator"): {
			spectator = static_cast<bool>(std::atoi(value.c_str()));
		} break;
		case hashString("isfromdemo"): {
			isFromDemo = static_cast<bool>(std::atoi(value.c_str()));
		} break;
		default: {
			customValues[key] = value;
		} break;
	}
}
