/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FACTORY_AI_H_
#define _FACTORY_AI_H_

#include "CommandAI.h"
#include "CommandQueue.h"

#include <string>
#include "System/UnorderedMap.hpp"

class CUnit;
class CFeature;
struct Command;

class CFactoryCAI : public CCommandAI
{
public:
	CR_DECLARE(CFactoryCAI)

	CFactoryCAI(CUnit* owner);
	CFactoryCAI();

	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	void SlowUpdate();

	void GiveCommandReal(const Command& c, bool fromSynced = true);

	void InsertBuildCommand(CCommandQueue::iterator& it, const Command& c);
	bool RemoveBuildCommand(CCommandQueue::iterator& it);

	void DecreaseQueueCount(const Command& c, int& buildOption);
	void FactoryFinishBuild(const Command& command);
	void ExecuteStop(Command& c);

	CCommandQueue newUnitCommands;

	spring::unordered_map<int, int> buildOptions;

private:
	void UpdateIconName(int id, const int& numQueued);
};

#endif // _FACTORY_AI_H_
