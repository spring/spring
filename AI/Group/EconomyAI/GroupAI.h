// GroupAI.h: interface for the CGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_

#include "ExternalAI/IGroupAI.h"
#include <set>
#include <map>
#include <vector>
#include <deque>
#include "float3.h"
#include "Helper.h"
#include "BoHandler.h"

struct UnitDef;
class IGroupAICallback;
class IAICallback;

const char AI_NAME[]="Economy AI";

using namespace std;

class CGroupAI : public IGroupAI
{
public:
	CGroupAI();
	virtual ~CGroupAI();

	virtual void InitAi(IGroupAICallback* callback);

	virtual bool AddUnit(int unit);
	virtual void RemoveUnit(int unit);

	virtual void GiveCommand(Command* c);
	virtual const vector<CommandDescription>& GetPossibleCommands();
	virtual int GetDefaultCmd(int unitid);
	virtual void CommandFinished(int unitid,int type);

	virtual void Update();
	virtual void DrawCommands();

	virtual void FindNewBuildTask();
	virtual void CalculateCurrentME();
	virtual void CalculateIdealME();
	virtual void SetUnitGuarding(int unit);
	virtual bool ValidCurrentBuilder();

	vector<CommandDescription> commands;
	deque<Command> commandQue;

	IGroupAICallback* callback;
	IAICallback* aicb;

	float idealME;				// ideal Metal / Energy ratio
	float currentME;			// current effective M / E ratio

	map<int,float> myUnits;		// map of all the units in the AI and their buildspeeds
	int currentBuilder;			// the id of the current main builder
	float totalBuildSpeed;		// sum of all the buildspeeds

	CHelper* helper;
	CBoHandler* boHandler;

	bool unitRemoved;

	bool newBuildTaskNeeded;
	int newBuildTaskFrame;

	bool initialized;

	float maxResourcePercentage;
	float totalMMenergyUpkeep;
};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
