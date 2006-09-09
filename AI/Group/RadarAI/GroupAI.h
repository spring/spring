// GroupAI.h: interface for the CGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
#define AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_

#include "ExternalAI/IGroupAI.h"
#include <set>
#include <map>
#include "float3.h"

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

	bool alertText;		// alert the player via a console message when enemy enters LOS / radar
	bool alertMinimap;	// alert the player via a minimap alert when enemy enters LOS / radar
	bool showGhosts;	// show the ghosts of enemy units that have been in LOS before

	IGroupAICallback* callback;
	IAICallback* aicb;

	virtual void InsertNewEnemy(int unitid);

	struct UnitInfo
	{
		bool		prevLos;
		bool		inRadar;
		bool		inLos;
		bool		isBuilding;
		const char* name;
		int			team;
		float3		pos;
		int			firstEnterFrame;
	};
	map<int,UnitInfo*> enemyUnits; //keeps track of enemy units

	int lastAlertFrame; // the last frame number in which an alert has been given to the user
	int lastEnterFrame; // the last frame when just 1 enemy unit entered LOS / radar
	int currentFrame;

	// used for calls to aicb->GetEnemyUnits ...
	int numEnemies;
	int numEnemiesLos;
	int* enemyIds;
	int* enemyIdsLos;

	set<int> newEnemyIds; //keeps track of new enemy units in radar / los
	set<int> oldEnemyIds; //keeps track of current units in radar / los
	set<int> oldEnemyIdsBackup;

};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
