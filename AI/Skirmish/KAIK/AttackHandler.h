#ifndef KAIK_ATTACKHANDLER_HDR
#define KAIK_ATTACKHANDLER_HDR

#include <list>
#include <utility>
#include <vector>

class CAttackGroup;
struct AIClasses;

class CAttackHandler {
public:
	CR_DECLARE(CAttackHandler);

	CAttackHandler(AIClasses* ai);
	~CAttackHandler(void);

	void AddUnit(int unitID);
	void Update(int);
	void UnitDestroyed(int unitID);

	// K-means functions are placed here for now
	std::vector<float3> KMeansIteration(std::vector<float3> means, std::vector<float3> unitPositions, int newK);
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

	std::list<int> attackUnits;
	std::list< std::pair<int, float3> > stuckUnits;
	// TODO: should be sets
	std::list<int> unarmedAirUnits;
	std::list<int> armedAirUnits;

	bool airIsAttacking;
	bool airPatrolOrdersGiven;
	int airTarget;

	int newGroupID;

	std::list<CAttackGroup> attackGroups;
//	std::list<CAttackGroup> defenseGroups;

	std::vector<float3> kMeansBase;
	std::vector<float3> kMeansEnemyBase;
	int kMeansK;
	int kMeansEnemyK;
};

#endif
