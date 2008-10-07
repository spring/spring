// GroupAI.h: interface for the CGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IGroupAI.h"
#include <map>
#include <set>

const char AI_NAME[]="Metal maker AI";

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

	virtual void CommandFinished(int squad,int type){};

	virtual void Update();

	struct UnitInfo{
		float energyUse;
		bool turnedOn;
	};

	map<int,UnitInfo*> myUnits;

	int lastUpdate;
	float lastEnergy;

	vector<CommandDescription> commands;
	IGroupAiCallback* callback;

	set<int> currentCommands;
};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
