#ifndef ATTACKHANDLER_H
#define ATTACKHANDLER_H

#include "System/StdAfx.h"
#include "creg/creg.h"
#include "creg/STL_List.h"
#include "ExternalAI/aibase.h"					// DLL exports and definitions
#include "ExternalAI/IGlobalAI.h"				// Main AI file
#include "Definitions.h"

#include "AttackGroup.h"

using std::vector;
using std::list;

class CAttackGroup;
class AIClasses;

class CAttackHandler
{
	public:
	CR_DECLARE(CAttackHandler);
	CAttackHandler(AIClasses* ai);
	virtual ~CAttackHandler();
	void PostLoad();

	void AddUnit(int unitID);
	void Update();
	void UnitDestroyed(int unitID);

	float DistanceToBase(float3 pos);
	float3 GetClosestBaseSpot(float3 pos);

	float3 FindSafeSpot(float3 myPos, float minSafety, float maxSafety);
	float3 FindSafeArea(float3 pos);
	float3 FindVerySafeArea(float3 pos);
	float3 FindUnsafeArea(float3 pos);

	vector<float3>* GetKMeansBase();
	vector<float3>* GetKMeansEnemyBase();

	bool CanTravelToBase(float3 pos);
	bool CanTravelToEnemyBase(float3 pos);

	//for new and dead unit spring calls:
	bool CanHandleThisUnit(int unit);

	int ah_timer_totalTime;
	int ah_timer_totalTimeMinusPather;
	int ah_timer_MicroUpdate;
	int ah_timer_Defend;
	int ah_timer_Flee;
	int ah_timer_MoveOrderUpdate;
	int ah_timer_NeedsNewTarget;

	bool debug;
	bool debugDraw;

private:
	void UpdateKMeans();
	void UpdateAir();
	void AssignTargets();
	void AssignTarget(CAttackGroup* group);
	bool UnitGroundAttackFilter(int unit);
	bool UnitBuildingFilter(const UnitDef *ud);
	bool UnitReadyFilter(int unit);
	void CombineGroups();
	bool PlaceIdleUnit(int unit);

	vector<float3> KMeansIteration(vector<float3> means, vector<float3> unitPositions, int newK);

	AIClasses *ai;	
	list<int> units;
	list<pair<int,float3> > stuckUnits;
	list<int> airUnits;
	bool airIsAttacking;
	bool airPatrolOrdersGiven;
	int airTarget;
	int newGroupID;
	list<CAttackGroup> attackGroups;
	int unitArray[MAXUNITS];
	vector<float3> kMeansBase;
	int kMeansK;
	vector<float3> kMeansEnemyBase;
	int kMeansEnemyK;
};


#endif /* ATTACKHANDLER_H */
