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

	virtual void AddBuildOptions(const UnitDef* unitDef);
	virtual void CalculateCurrentME();
	virtual void CalculateIdealME();
	virtual void FindNewBuildTask();
	virtual void SetUnitGuarding(int unit);

	vector<CommandDescription> commands;
	deque<Command> commandQue;

	IGroupAICallback* callback;
	IAICallback* aicb;

	map<string,BOInfo*> allBO;	// all build options
	vector<BOInfo*> bestMetal;	// ordered buildoptions for best metal production
	vector<BOInfo*> bestEnergy;	// ordered buildoptions for best energy production

	float idealME;				// ideal Metal / Energy ratio
	float currentME;			// current effective M / E ratio

	map<int,float> myUnits;		// map of all the units in the AI and their buildspeeds
	int currentBuilder;			// the id of the current main builder
	float totalBuildSpeed;		// sum of all the buildspeeds

	float tidalStrength;
	float avgWind;
	float avgMetal;

	CHelper* helper;

	bool BOchanged;
	bool unitRemoved;

	bool newBuildTaskNeeded;
	int newBuildTaskFrame;
};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
