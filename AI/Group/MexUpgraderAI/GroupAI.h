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

const char AI_NAME[]="MexUpgrader AI";

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

	vector<CommandDescription> commands;


	IGroupAICallback* callback;
	IAICallback* aicb;

	struct UnitInfo{
		float maxExtractsMetal;
		int wantedMohoId;
		string wantedMohoName;

		int nearestMex;
		float3 wantedBuildSite;

		bool reclaiming;
		bool building;
		bool hasReported;
	};

	map<int,UnitInfo*> myUnits;
	set<int> lockedMexxes;

	int lastUpdate;
	int myTeam;
	int* friendlyUnits;
	int numFriendlyUnits;
	int numUpgradableMexxes;
	float minSqDist;


};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
