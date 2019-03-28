/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKIRMISH_AI_DATA_H
#define SKIRMISH_AI_DATA_H

#include <string>
#include <vector>

#include "ExternalAI/SkirmishAIBase.h"

#include "System/creg/creg_cond.h"
#include "System/UnorderedMap.hpp"


/**
 * Contains everything needed to initialize a Skirmish AI instance.
 * @see Game/GameSetup
 */
class SkirmishAIData : public SkirmishAIBase {
	CR_DECLARE_DERIVED(SkirmishAIData)

public:
	SkirmishAIData() = default;
	SkirmishAIData(const SkirmishAIData& d) = default;
	SkirmishAIData(SkirmishAIData&& d) { *this = std::move(d); }

	SkirmishAIData& operator = (const SkirmishAIData& d) = default;
	SkirmishAIData& operator = (SkirmishAIData&& d) {
		team = d.team;
		hostPlayer = d.hostPlayer;
		status = d.status;

		name = std::move(d.name);
		shortName = std::move(d.shortName);
		version = std::move(d.version);
		optionKeys = std::move(d.optionKeys);
		options = std::move(d.options);

		isLuaAI = d.isLuaAI;

		currentStats = d.currentStats;
		return *this;
	}

public:
	std::string shortName;
	std::string version;
	std::vector<std::string> optionKeys;
	spring::unordered_map<std::string, std::string> options;

	bool isLuaAI = false;

	SkirmishAIStatistics currentStats;
};

#endif // SKIRMISH_AI_DATA_H
