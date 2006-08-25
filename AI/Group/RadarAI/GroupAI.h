// GroupAI.h: interface for the CGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_

#include "ExternalAI/IGroupAI.h"
#include <set>

class IGroupAICallback;
class IAICallback;

const char AI_NAME[]="Radar AI";

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
	virtual void CommandFinished(int squad,int type){};

	virtual void Update();

	vector<CommandDescription> commands;

	bool alertText;
	bool alertMinimap;

	IGroupAICallback* callback;
	IAICallback* aicb;

	int lastUpdate;
	int lastEnterTime;
	int numEnemies;
	int numPrevEnemies;
	int* enemyIds;
	set<int> prevEnemyIds;

};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
