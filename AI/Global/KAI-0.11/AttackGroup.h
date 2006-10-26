#ifndef ATTACKGROUP_H
#define ATTACKGROUP_H
/*pragma once removed*/
#include "GlobalAI.h"


class CAttackGroup
{
	public:
		CAttackGroup(AIClasses* ai, int groupID_in);

	virtual ~CAttackGroup();

	void AddUnit(int unitID);
	void Update();
	void MoveTo(float3 newPosition);
	int Size();
	int GetGroupID();
	float3 GetGroupPos();
	bool RemoveUnit(int unitID);
	int PopStuckUnit();

	float Power();
	void Log();

	//hack to fix them suiciding on mexes in EE
	bool CloakedFix(int enemy);

	bool defending;
	float3 attackPosition;
	float attackRadius;
	vector<float3> pathToTarget;

	void FindDefenseTarget(float3 groupPosition);

	int GetWorstMoveType();

	vector<int>* GetAllUnits(); // for combining

	list<int> GetAssignedEnemies();
	void ClearTarget();

	bool NeedsNewTarget();
	void AssignTarget(vector<float3> path, float3 target, float radius);


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
