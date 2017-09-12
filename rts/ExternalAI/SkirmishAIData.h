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
	CR_DECLARE(SkirmishAIData)

public:
	virtual ~SkirmishAIData() {}
	std::string shortName;
	std::string version;
	std::vector<std::string> optionKeys;
	spring::unordered_map<std::string, std::string> options;

	bool isLuaAI;

	SkirmishAIStatistics currentStats;
};

#endif // SKIRMISH_AI_DATA_H
