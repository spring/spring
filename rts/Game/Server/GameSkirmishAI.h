/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_SKIRMISH_AI_H
#define _GAME_SKIRMISH_AI_H

#include "ExternalAI/SkirmishAIBase.h"

class GameSkirmishAI : public SkirmishAIBase
{
public:
	GameSkirmishAI() :
		SkirmishAIBase()
		//, cpuUsage (0.0f)
		{}

	GameSkirmishAI& operator=(const SkirmishAIBase& base) { SkirmishAIBase::operator=(base); return *this; };

	//State myState;

	//float cpuUsage;

	//bool isLocal;
	SkirmishAIStatistics lastStats;
};

#endif // _GAME_SKIRMISH_AI_H
