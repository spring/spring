#ifndef GROUP_H
#define GROUP_H
// Group.h: interface for the CGroup class.
//
//////////////////////////////////////////////////////////////////////

#include "System/Object.h"
#include <string>
#include <set>
#include "Game/command.h"
#include "System/Platform/SharedLib.h"
class IGroupAI;
class CUnit;
class CFeature;
class CGroupAICallback;
class CGroupHandler;

using namespace std;

class CGroup : public CObject  
{
public:
	void CommandFinished(int unit,int type);
	CGroup(string dllName,int id,CGroupHandler* grouphandler);
	virtual ~CGroup();

	void Update();
	void SetNewAI(string dllName);

	void RemoveUnit(CUnit* unit);	//call setgroup(0) instead of calling this directly
	bool AddUnit(CUnit* unit);		//dont call this directly call unit.SetGroup and let that call this
	const vector<CommandDescription>& GetPossibleCommands();
	int GetDefaultCmd(CUnit* unit,CFeature* feature);
	void GiveCommand(Command c);
	void ClearUnits(void);

	int id;

	set<CUnit*> units;

	vector<CommandDescription> myCommands;
	SharedLib *lib;
	typedef int (* GETGROUPAIVERSION)();
	typedef IGroupAI* (* GETNEWAI)();
	typedef void (* RELEASEAI)(IGroupAI* i);
	
	GETGROUPAIVERSION GetGroupAiVersion;
	GETNEWAI GetNewAI;
	RELEASEAI ReleaseAI;
	int lastCommandPage;
	int currentAiNum;

	IGroupAI* ai;
	CGroupAICallback* callback;

	CGroupHandler* handler;
};

#endif /* GROUP_H */
