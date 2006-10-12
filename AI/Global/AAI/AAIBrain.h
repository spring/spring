//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------


#pragma once

#include "aidef.h"
#include "AAISector.h"
#include "AAIGroup.h"

class AAI;
class AAIBuildTable;
class AAIExecute;
class AIIMap;

class AAIBrain
{
public:
	AAIBrain(AAI *ai);
	~AAIBrain(void);
	
	// adds sector to the base
	void AddSector(AAISector *sector);

	// for internal use
	bool SectorInList(list<AAISector*> mylist, AAISector *sector);
	list<AAISector*> GetSectors();

	void RemoveProducer(list<ProductionRequest> builders, int builder_id);   

	// returns dest attack sector
	AAISector* GetAttackDest(bool land, bool water, AttackType type);

	// returns a sector to proceed with attack
	AAISector* GetNextAttackDest(AAISector *current_sector, bool land, bool water);

	// checks for new neighbours (and removes old ones if necessary)
	void UpdateNeighbouringSectors(); 

	// recalculates the center of the base
	void UpdateBaseCenter();

	// updates max units spotted
	void UpdateMaxCombatUnitsSpotted(float *units_spotted);

	void UpdateAttackedByValues();
	void AttackedBy(int combat_category_id);

	void UpdateDefenceCapabilities();

	// returns true if sufficient ressources to build unit are availbale
	bool RessourcesForConstr(int unit, int workertime = 175);

	// returns true if enough metal for constr.
	bool MetalForConstr(int unit, int workertime = 175);

	// returns true if enough energy for constr.
	bool EnergyForConstr(int unit, int wokertime = 175);

	// returns pos where scout schould be sent to
	AAISector* GetNewScoutDest(int scout);

	// adds new sectors to base
	bool ExpandBase(SectorType sectorType);

	// returns how much ressources can be spent for unit construction atm 
	float Affordable();

	void DefendBuilding(int building, const UnitDef *def, int attacker);

	void DefendCommander(int attacker);

	void BuildUnits();

	void BuildUnitOfCategory(UnitCategory category, float cost, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, bool urgent);

	//  0 = sectors the ai uses to build its base, 1 = direct neighbours etc.
	list<AAISector*> *sectors; 
	int max_distance;

	float3 base_center;

	// are there any free metal spots within the base
	bool freeBaseSpots;		
	bool expandable; 

	// holding max number of units of a category spotted at the same time
	float *max_units_spotted;
	float *attacked_by;
	float *defence_power_vs;

	// pos where com spawned
	float3 start_pos;

	float best_power_plant_output;
		
	AAIExecute *execute;

private:
	AAI *ai;
	AAIMap *map;
	IAICallback *cb;
	AAIBuildTable *bt;
};
