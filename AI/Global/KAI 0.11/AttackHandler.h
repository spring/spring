#pragma once
#include "GlobalAI.h"

class CAttackGroup;

class CAttackHandler
{
	public:
	CAttackHandler(AIClasses* ai);

	virtual ~CAttackHandler();

	void CAttackHandler::AddUnit(int unitID);
	void CAttackHandler::Update();
	void CAttackHandler::UnitDestroyed(int unitID);
	

	//the kmeans is placed here for now =)
	vector<float3> CAttackHandler::KMeansIteration(vector<float3> means, vector<float3> unitPositions, int newK);
	float CAttackHandler::DistanceToBase(float3 pos);
	float3 CAttackHandler::GetClosestBaseSpot(float3 pos);
	bool CAttackHandler::PlaceIdleUnit(int unit);

	//for use for builders
	bool CAttackHandler::IsSafeBuildSpot(float3 mypos);
	bool CAttackHandler::IsVerySafeBuildSpot(float3 mypos);

//	float3 CAttackHandler::FindSafeArea();
//	float3 CAttackHandler::FindVerySafeArea();
//	float3 CAttackHandler::FindSafeSpot();
//	float3 CAttackHandler::FindVerySafeSpot();

	float3 CAttackHandler::FindSafeSpot(float3 myPos, float minSafety, float maxSafety);
	float3 CAttackHandler::FindSafeArea(float3 pos);
	float3 CAttackHandler::FindVerySafeArea(float3 pos);
	float3 CAttackHandler::FindUnsafeArea(float3 pos);



	void CAttackHandler::UpdateKMeans();

	void CAttackHandler::UpdateAir();

	void CAttackHandler::AssignTargets();
	void CAttackHandler::AssignTarget(CAttackGroup* group);

	//bool CAttackHandler::IsReadyToAttack(int unit);

	bool CAttackHandler::UnitGroundAttackFilter(int unit);
	//bool CAttackHandler::UnitBuildingFilter(int unit);
	bool CAttackHandler::UnitBuildingFilter(const UnitDef *ud);
	bool CAttackHandler::UnitReadyFilter(int unit);

	void CAttackHandler::CombineGroups();

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
