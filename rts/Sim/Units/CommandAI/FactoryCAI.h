/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FACTORY_AI_H_
#define _FACTORY_AI_H_

#include "CommandAI.h"
#include "CommandQueue.h"

#include <string>
#include <map>

class CUnit;
class CFeature;
struct Command;

class CFactoryCAI : public CCommandAI
{
public:
	CR_DECLARE(CFactoryCAI)
	CR_DECLARE_SUB(BuildOption)

	struct BuildOption {
		CR_DECLARE_STRUCT(BuildOption)
		std::string name;
		std::string fullName;
		int numQued;
	};

	CFactoryCAI(CUnit* owner);
	CFactoryCAI();

	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	void SlowUpdate();

	void GiveCommandReal(const Command& c, bool fromSynced = true);

	void InsertBuildCommand(CCommandQueue::iterator& it, const Command& c);
	bool RemoveBuildCommand(CCommandQueue::iterator& it);

	void DecreaseQueueCount(const Command& c, BuildOption& buildOption);
	void FactoryFinishBuild(const Command& command);
	void ExecuteStop(Command& c);

	CCommandQueue newUnitCommands;

	std::map<int, BuildOption> buildOptions;

private:
	void UpdateIconName(int id, const BuildOption& bo);
};

#endif // _FACTORY_AI_H_
