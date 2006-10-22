#ifndef GROUP_H
#define GROUP_H
// Group.h: interface for the CGroup class.
//
//////////////////////////////////////////////////////////////////////

#include "Object.h"
#include <string>
#include <set>
#include "Game/command.h"
#include "Platform/SharedLib.h"
#include "Sim/Units/UnitDef.h"
#include "ExternalAI/aikey.h"
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
	CGroup(AIKey aiKey,int id,CGroupHandler* grouphandler);
	virtual ~CGroup();

	void Update();
	void DrawCommands();
	void SetNewAI(AIKey aiKey);

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
	typedef IGroupAI* (* GETNEWAI)(unsigned aiNumber);
	typedef void (* RELEASEAI)(unsigned aiNumber,IGroupAI* i);
	typedef bool (* ISUNITSUITED)(unsigned aiNumber,const UnitDef* unitDef);
	
	GETGROUPAIVERSION GetGroupAiVersion;
	GETNEWAI GetNewAI;
	RELEASEAI ReleaseAI;
	ISUNITSUITED IsUnitSuited;
	int lastCommandPage;
	int currentAiNum;
	AIKey currentAiKey;

	IGroupAI* ai;
	CGroupAICallback* callback;

	CGroupHandler* handler;
};

#endif /* GROUP_H */
