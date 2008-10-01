#pragma once

#include <set>
#include "aidef.h"

using std::set;

class AAI;
class AAIBuildTable;
class AAIExecute;

class AAIUnitTable
{
public:
	AAIUnitTable(AAI *ai, AAIBuildTable *bt);
	~AAIUnitTable(void);

	bool AddUnit(int unit_id, int def_id, AAIGroup *group = 0, AAIConstructor *cons = 0);
	void RemoveUnit(int unit_id);

	void AddConstructor(int unit_id, int def_id);
	void RemoveConstructor(int unit_id, int def_id);

	void AddCommander(int unit_id, int def_id);
	void RemoveCommander(int unit_id, int def_id);

	void AddExtractor(int unit_id);
	void RemoveExtractor(int unit_id);

	void AddPowerPlant(int unit_id, int def_id);
	void RemovePowerPlant(int unit_id);

	void AddMetalMaker(int unit_id, int def_id);
	void RemoveMetalMaker(int unit_id);

	void AddJammer(int unit_id, int def_id);
	void RemoveJammer(int unit_id);

	void AddRecon(int unit_id, int def_id);
	void RemoveRecon(int unit_id);

	void AddStationaryArty(int unit_id, int def_id);
	void RemoveStationaryArty(int unit_id);

	AAIConstructor* FindBuilder(int building, bool commander);
	AAIConstructor* FindClosestBuilder(int building, float3 pos, bool commander);
	AAIConstructor* FindClosestAssistant(float3 pos, int importance, bool commander);

	void EnemyKilled(int unit);

	void SetUnitStatus(int unit, UnitTask status);

	void AssignGroupToEnemy(int unit, AAIGroup *group);

	bool IsUnitCommander(int unit_id);
	bool IsDefCommander(int def_id);

	bool IsBuilder(int unit_id);

	AAI *ai;
	AAIExecute *execute;
	AAIBuildTable *bt;
	IAICallback* cb;

	vector<AAIUnit> units;
	// units[i].unitId = -1 -> not used , -2 -> enemy unit

	// commanders id
	int cmdr;

	set<int> constructors;
	set<int> metal_makers;
	set<int> jammers; 
	set<int> recon;
	set<int> extractors;
	set<int> power_plants;
	set<int> stationary_arty;

};
