#ifndef KAIK_ATTACKGROUP_HDR
#define KAIK_ATTACKGROUP_HDR

#include <list>
#include <vector>

#include "Defines.h"

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
		std::vector<float3> pathToTarget;
		void FindDefenseTarget(float3 groupPosition, int);

		int GetWorstMoveType();

		// for combining
		std::vector<int>* GetAllUnits();
		std::list<int> GetAssignedEnemies();
		void ClearTarget();

		bool NeedsNewTarget();
		int SelectEnemy(int, const float3&);
		void AttackEnemy(int, int, float, int);
		void AssignTarget(std::vector<float3> path, float3 target, float radius);
		void MoveAlongPath(float3& groupPosition, int numUnits);

	private:
		AIClasses* ai;
		std::vector<int> units;
		int groupID;
		bool isMoving;

		int pathIterator;
		float lowestAttackRange;
		float highestAttackRange;
		bool isShooting;

		int movementCounterForStuckChecking;
};


#endif
