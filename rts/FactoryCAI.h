#pragma once
#include "commandai.h"
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
	void GiveCommand(Command& c);
	void DrawCommands(void);
	void UpdateIconName(int id,BuildOption& bo);

	std::deque<Command> newUnitCommands;

	map<int,BuildOption> buildOptions;

	bool building;
};
