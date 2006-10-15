#ifndef ATTACKGROUP_H
#define ATTACKGROUP_H
/*pragma once removed*/
#include "GlobalAI.h"


class CAttackGroup
{
	public:
		CAttackGroup(AIClasses* ai, int groupID_in);

	virtual ~CAttackGroup();

	void CAttackGroup::AddUnit(int unitID);
	void CAttackGroup::Update();
	void CAttackGroup::MoveTo(float3 newPosition);
	int CAttackGroup::Size();
	int CAttackGroup::GetGroupID();
	float3 CAttackGroup::GetGroupPos();
	bool CAttackGroup::RemoveUnit(int unitID);
	int CAttackGroup::PopStuckUnit();

	float CAttackGroup::Power();
	void CAttackGroup::Log();

	//hack to fix them suiciding on mexes in EE
	bool CAttackGroup::CloakedFix(int enemy);

	bool defending;
	float3 attackPosition;
	float attackRadius;
	vector<float3> pathToTarget;

	void CAttackGroup::FindDefenseTarget(float3 groupPosition);

	int CAttackGroup::GetWorstMoveType();

	vector<int>* CAttackGroup::GetAllUnits(); // for combining

	list<int> CAttackGroup::GetAssignedEnemies();
	void CAttackGroup::ClearTarget();

	bool CAttackGroup::NeedsNewTarget();
	void CAttackGroup::AssignTarget(vector<float3> path, float3 target, float radius);


	private:
	AIClasses *ai;	
	vector<int> units;
	int groupID;
//	int myEnemy;
	bool isMoving;

	int pathIterator;
	float lowestAttackRange;
	float highestAttackRange;
	bool isShooting;
	int unitArray[MAXUNITS];

	int movementCounterForStuckChecking;

};

#endif /* ATTACKGROUP_H */
