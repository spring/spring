#ifndef ATTACKHANDLER_H
#define ATTACKHANDLER_H
/*pragma once removed*/
#include "GlobalAI.h"

class CAttackGroup;

class CAttackHandler
{
	public:
	CAttackHandler(AIClasses* ai);

	virtual ~CAttackHandler();

	void AddUnit(int unitID);
	void Update();
	void UnitDestroyed(int unitID);
	

	//the kmeans is placed here for now =)
	vector<float3> KMeansIteration(vector<float3> means, vector<float3> unitPositions, int newK);
	float DistanceToBase(float3 pos);
	float3 GetClosestBaseSpot(float3 pos);
	bool PlaceIdleUnit(int unit);

	//for use for builders
	bool IsSafeBuildSpot(float3 mypos);
	bool IsVerySafeBuildSpot(float3 mypos);

//	float3 FindSafeArea();
//	float3 FindVerySafeArea();
//	float3 FindSafeSpot();
//	float3 CAttackHandler::FindVerySafeSpot();

	float3 FindSafeSpot(float3 myPos, float minSafety, float maxSafety);
	float3 FindSafeArea(float3 pos);
	float3 FindVerySafeArea(float3 pos);
	float3 FindUnsafeArea(float3 pos);



	void UpdateKMeans();

	void UpdateAir();

	void AssignTargets();
	void AssignTarget(CAttackGroup* group);

	//bool CAttackHandler::IsReadyToAttack(int unit);

	bool UnitGroundAttackFilter(int unit);
	//bool UnitBuildingFilter(int unit);
	bool UnitBuildingFilter(const UnitDef *ud);
	bool UnitReadyFilter(int unit);

	void CombineGroups();

private:
	AIClasses *ai;	

	list<int> units;
	list<pair<int,float3> > stuckUnits;

	list<int> airUnits;
	bool airIsAttacking;
	bool airPatrolOrdersGiven;
	int airTarget;

	//float3 mapMiddleHack;

//	int rushAtStartCounter;
	int newGroupID;

	list<CAttackGroup> attackGroups;
//	list<CAttackGroup> defenseGroups;

	int unitArray[MAXUNITS];

	vector<float3> kMeansBase;
	int kMeansK;
	vector<float3> kMeansEnemyBase;
	int kMeansEnemyK;
};

#endif /* ATTACKHANDLER_H */
