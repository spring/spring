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

	float CAttackHandler::DistanceToBase(float3 pos);
	float3 CAttackHandler::GetClosestBaseSpot(float3 pos);

	float3 CAttackHandler::FindSafeSpot(float3 myPos, float minSafety, float maxSafety);
	float3 CAttackHandler::FindSafeArea(float3 pos);
	float3 CAttackHandler::FindVerySafeArea(float3 pos);
	float3 CAttackHandler::FindUnsafeArea(float3 pos);

	vector<float3>* CAttackHandler::GetKMeansBase();
	vector<float3>* CAttackHandler::GetKMeansEnemyBase();

	bool CAttackHandler::CanTravelToBase(float3 pos);
	bool CAttackHandler::CanTravelToEnemyBase(float3 pos);

	//for new and dead unit spring calls:
	bool CAttackHandler::CanHandleThisUnit(int unit);

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
	void CAttackHandler::UpdateKMeans();
	void CAttackHandler::UpdateAir();
	void CAttackHandler::AssignTargets();
	void CAttackHandler::AssignTarget(CAttackGroup* group);
	bool CAttackHandler::UnitGroundAttackFilter(int unit);
	bool CAttackHandler::UnitBuildingFilter(const UnitDef *ud);
	bool CAttackHandler::UnitReadyFilter(int unit);
	void CAttackHandler::CombineGroups();
	bool CAttackHandler::PlaceIdleUnit(int unit);

	vector<float3> CAttackHandler::KMeansIteration(vector<float3> means, vector<float3> unitPositions, int newK);

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