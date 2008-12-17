#ifndef GROUP_H
#define GROUP_H
// Group.h: interface for the CGroup class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <set>
#include "Sim/Units/CommandAI/Command.h"
#include "Platform/SharedLib.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitSet.h"
#include "aikey.h"
class IGroupAI;
class CUnit;
class CFeature;
class CGroupAICallback;
class CGroupHandler;

class CGroup
{
public:
	CR_DECLARE(CGroup);
	CGroup(AIKey aiKey, int id, CGroupHandler* groupHandler);
	virtual ~CGroup();
	void Serialize(creg::ISerializer *s);
	void PostLoad();

	void Update();
	void DrawCommands();
	void SetNewAI(AIKey aiKey);

	void CommandFinished(int unitId, int commandTopicId);
	void RemoveUnit(CUnit* unit);	//call setgroup(0) instead of calling this directly
	bool AddUnit(CUnit* unit);		//dont call this directly call unit.SetGroup and let that call this
	const vector<CommandDescription>& GetPossibleCommands();
	int GetDefaultCmd(CUnit* unit,CFeature* feature);
	void GiveCommand(Command c);
	void ClearUnits(void);

	int id;

	CUnitSet units;

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

#endif // GROUP_H
