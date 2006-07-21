#pragma once

#include "aidef.h"

class AAI;
class AAIBuildTable;

class AAIUnitTable
{
public:
	AAIUnitTable(AAI *ai, AAIBuildTable *bt);
	~AAIUnitTable(void);

	bool AddUnit(int unit_id, int def_id, AAIGroup *group = 0, AAIBuilder *builder = 0, AAIFactory *factory = 0);
	void RemoveUnit(int unit_id);

	void AddBuilder(int unit_id, int def_id);
	void RemoveBuilder(int unit_id, int def_id);

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

	void AddFactory(int unit_id, int def_id);
	void RemoveFactory(int unit_id);

	void AddCommander(int unit_id, const UnitDef *def);
	void RemoveCommander(int unit_id);

	AAIBuilder* FindBuilder(int building, bool commander, int importance);
	AAIBuilder* FindClosestBuilder(int building, float3 pos, bool commander, int importance);
	AAIBuilder* FindAssistBuilder(float3 pos, int importance, bool water = false, bool floater = false);

	bool IsUnitCommander(int unit_id);
	bool IsDefCommander(int def_id);

	AAI *ai;
	AAIBuildTable *bt;
	IAICallback* cb;

	AAIUnit *units;

	// commanders id
	int cmdr;

	set<int> builders;
	set<int> factories;
	set<int> metal_makers;
	set<int> jammers; 
	set<int> recon;
	set<int> extractors;
	set<int> power_plants;
	set<int> stationary_arty;

};
