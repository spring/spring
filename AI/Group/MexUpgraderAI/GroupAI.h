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

	void AutoFindMex(int unitid);			// finds the next upgradable mex for unit "unitid"
	void ManualFindMex();					// finds the next upgradable mex for the moho builder
	void Reset();							// clears all orders and stops all units
	void GuardMohoBuilder(int unitid);		// orders unitid to guard the mohobuilder
	void ReclaimMex(int unitid, int mex);	// orders unitid to reclaim mex
	int FindNearestMex(int unitid, int* possibleMexes, int size); // finds the nearest upgradable mex for unit "unitid" from possibleMexes with size "size"

	vector<CommandDescription> commands;	// the commands that can be given to this groupAI ("stop","area upgrade","mode")
	deque<Command> commandQue;				// queued-up area-upgrade commands

	IGroupAICallback* callback;
	IAICallback* aicb;

	typedef enum ModeType				// this group AI has two modes:
	{
		automatic,						// * automically find and replace every mex
		manual							// * select areas in which to find and replace mexes
	};

	ModeType mode;
	typedef enum StatusType				// keeps track of the status of units in the groupAI:
	{
			idle,						// does nothing
			reclaiming,					// is reclaiming a mex
			building,					// is building a moho
			guarding					// is guarding the mohoBuilder (see below)
	};

	struct UnitInfo{
		float maxExtractsMetal;			// the maximum amount of metal a unit buildable by othis unit can produce
		int wantedMohoId;				// build command ID of the building above
		string wantedMohoName;			// name of the building above

		int nearestMex;					// ID of nearest upgradable mex
		float3 wantedBuildSite;			// position of the upgradable mex, used to determine the moho position

		StatusType status;
	};

	map<int,UnitInfo*> myUnits;			// keeps track of the units in the group AI
	set<int> lockedMexxes;				// id's of mexes that are being reclaimed

	float maxMetal;						// the maximum amount of metal a unit buildable by one of the groupAI's units can produce
	int mohoBuilderId;					// the id of the unit that can build the unit that has maxMetal (above)
	bool unitsChanged;					// set this to true if a unit is added to or removed from the group

	int myTeam;
	int* friendlyUnits;
};

#endif // !defined(AFX_GroupAI_H__10718E36_5CDF_4CD4_8D90_F41311DD2694__INCLUDED_)
