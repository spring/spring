#ifndef __FACTORY_AI_H__
#define __FACTORY_AI_H__

#include "CommandAI.h"
#include <string>
#include <map>

class CFactoryCAI :
	public CCommandAI
{
public:
	CR_DECLARE(CFactoryCAI);
	CR_DECLARE_SUB(BuildOption);
	struct BuildOption {
		CR_DECLARE_STRUCT(BuildOption)
		std::string name;
		std::string fullName;
		int numQued;
	};

	CFactoryCAI(CUnit* owner);
	CFactoryCAI();
	~CFactoryCAI(void);
	void PostLoad();

	int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	void SlowUpdate();

	void GiveCommandReal(const Command& c);

	void InsertBuildCommand(CCommandQueue::iterator& it, const Command& c);
	void RemoveBuildCommand(CCommandQueue::iterator& it);

	void ExecuteStop(Command &c);

	void DrawCommands(void);
	void UpdateIconName(int id,BuildOption& bo);

	CCommandQueue newUnitCommands;

	std::map<int, BuildOption> buildOptions;

	bool building;

	int lastRestrictedWarning;

private:

	void CancelRestrictedUnit(const Command& c, BuildOption& buildOption);
};

#endif // __FACTORY_AI_H__
