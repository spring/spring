#ifndef ATTACKHANDLER_H
#define ATTACKHANDLER_H


#include "GlobalAI.h"

using std::pair;
class CAttackGroup;

class CAttackHandler {
	public:
		CR_DECLARE(CAttackHandler);

		CAttackHandler(AIClasses* ai);
		~CAttackHandler(void);

		void AddUnit(int unitID);
		void Update(int);
		void UnitDestroyed(int unitID);

		// K-means functions are placed here for now
		vector<float3> KMeansIteration(vector<float3> means, vector<float3> unitPositions, int newK);
		float DistanceToBase(float3 pos);
		float3 GetClosestBaseSpot(float3 pos);
		bool PlaceIdleUnit(int unit);

		// for use for builders
		bool IsSafeBuildSpot(float3 mypos);

		float3 FindSafeSpot(float3 myPos, float minSafety, float maxSafety);
		float3 FindSafeArea(float3 pos);
		float3 FindVerySafeArea(float3 pos);
		float3 FindUnsafeArea(float3 pos);

		void AirAttack(int);
		void AirPatrol(int);

		void UpdateKMeans(void);
		void UpdateAir(int);
		void UpdateSea(int);

		// nuke-related functions
		void UpdateNukeSilos(int);
		int PickNukeSiloTarget(std::vector<std::pair<int, float> >&);
		void GetNukeSiloTargets(std::vector<std::pair<int, float> >&);

		void AssignTargets(int);
		void AssignTarget(CAttackGroup* group);

		bool UnitGroundAttackFilter(int unit);
		bool UnitBuildingFilter(const UnitDef* ud);
		bool UnitReadyFilter(int unit);

		void CombineGroups(void);

	private:
		AIClasses* ai;

		list<int> units;
		list< pair<int, float3> > stuckUnits;
		// TODO: should be sets
		list<int> unarmedAirUnits;
		list<int> armedAirUnits;

		bool airIsAttacking;
		bool airPatrolOrdersGiven;
		int airTarget;

		int newGroupID;

		list<CAttackGroup> attackGroups;
	//	list<CAttackGroup> defenseGroups;

		int unitArray[MAX_UNITS];

		vector<float3> kMeansBase;
		int kMeansK;
		vector<float3> kMeansEnemyBase;
		int kMeansEnemyK;
};


#endif
