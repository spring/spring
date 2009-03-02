#ifndef ATTACKGROUP_H
#define ATTACKGROUP_H

#include "System/StdAfx.h"
#include "creg/creg.h"
#include "creg/STL_List.h"
#include "Defines.h"

#include "ExternalAI/aibase.h"					// DLL exports and definitions
#include "Sim/Misc/GlobalConstants.h"

using std::vector;
using std::list;

class AIClasses;

class CAttackGroup {
	public:
		CR_DECLARE(CAttackGroup);

		CAttackGroup();
		CAttackGroup(AIClasses* ai, int groupID_in);
		~CAttackGroup();

		void AddUnit(int unitID);
		void Update(int);
		void MoveTo(float3 newPosition);
		int Size();
		int GetGroupID();
		float3 GetGroupPos();
		bool RemoveUnit(int unitID);
		int PopStuckUnit();

		float Power();
		void Log();

		// hack to fix them suiciding on mexes in EE
		bool CloakedFix(int enemy);

		bool defending;
		float3 attackPosition;
		float attackRadius;
		vector<float3> pathToTarget;
		void FindDefenseTarget(float3 groupPosition, int);

		int GetWorstMoveType();

		// for combining
		vector<int>* GetAllUnits();

		list<int> GetAssignedEnemies();
		void ClearTarget();

		bool NeedsNewTarget();
		int SelectEnemy(int, const float3&);
		void AttackEnemy(int, int, float, int);
		void AssignTarget(vector<float3> path, float3 target, float radius);
		void MoveAlongPath(float3& groupPosition, int numUnits);


	private:
		AIClasses *ai;
		vector<int> units;
		int groupID;
		bool isMoving;

		int pathIterator;
		float lowestAttackRange;
		float highestAttackRange;
		bool isShooting;
		int unitArray[MAX_UNITS];

		int movementCounterForStuckChecking;
};


#endif
