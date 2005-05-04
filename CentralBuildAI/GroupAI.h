// GroupAI.h: interface for the CGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GROUPAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GROUPAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IGroupAI.h"
#include <set>
#include <map>
#include "float3.h"

const char AI_NAME[]="Central build AI";

using namespace std;

class CGroupAI : public IGroupAI  
{
public:
	CGroupAI();
	virtual ~CGroupAI();

	virtual void InitAi(IGroupAiCallback* callback);

	virtual bool AddUnit(int unit);
	virtual void RemoveUnit(int unit);

	virtual void GiveCommand(Command* c);
	virtual const vector<CommandDescription>& GetPossibleCommands(){return commands;};
	virtual int GetDefaultCmd(int unitid);
	virtual void CommandFinished(int squad,int type);
	virtual void Update();

	void UpdateAvailableCommands(void);

	int frameNum;

	struct UnitInfo{
		int lastGivenOrder;							//<0 -> set to guard this unit >0 -> set to build this project
		set<int> possibleBuildOrders;
		vector<int> orderedBuildOrders;			//the build orders in the same order as in the normal gui
		set<int> unitsGuardingMe;
		float buildSpeed;
		float moveSpeed;
		float totalGuardSpeed;					//total build speed of all units guarding me
	};

	map<int,UnitInfo*> myUnits;
	map<int,UnitInfo*>::iterator updateUnit;
	bool unitsChanged;

	struct QuedBuilding{
		int type;
		float3 pos;
		std::set<int> unitsOnThis;
		float totalBuildSpeed;
		float buildTimeLeft;
		int failedTries;
		int startFrame;
	};
	map<int,QuedBuilding*> quedBuildngs;
	int nextBuildingId;

	struct BuildOption{
		int type;					//depends on if the unit is build in a factory or should be placed on the map
		string name;
		int numQued;			//how many of these we have qued if its factory built
		float buildTime;
	};
	map<int,BuildOption*> buildOptions;

	vector<CommandDescription> commands;
	IGroupAiCallback* callback;
	void UpdateFactoryIcon(CommandDescription* cd, int numQued);
	void FindNewJob(int unit);
	void SendTxt(const char *fmt, ...);
	int FindCloseQuedBuilding(float3 pos, float radius);
	void FinishBuilderTask(int unit,bool failure);
};

#endif // !defined(AFX_GROUPAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
