#ifndef __FACTORY_AI_H__
#define __FACTORY_AI_H__

#include "CommandAI.h"
#include <string>
#include <map>

using namespace std;

class CFactoryCAI :
	public CCommandAI
{
public:
	struct BuildOption {
		string name;
		string fullName;
		int numQued;
	};

	CFactoryCAI(CUnit* owner);
	~CFactoryCAI(void);

	int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	void SlowUpdate();

	void GiveCommandReal(const Command& c);

	void InsertBuildCommand(CCommandQueue::iterator& it, const Command& c);
	void RemoveBuildCommand(CCommandQueue::iterator& it);

	void ExecuteStop(Command &c);

	void DrawCommands(void);
	void UpdateIconName(int id,BuildOption& bo);

	CCommandQueue newUnitCommands;

	map<int,BuildOption> buildOptions;

	bool building;

	int lastRestrictedWarning;
};

#endif // __FACTORY_AI_H__
