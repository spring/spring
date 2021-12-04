// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------


#include "AAI.h"
#include "AAIExecute.h"
#include "AAIBuildTable.h"
#include "AAIBrain.h"
#include "AAIUnitTable.h"
#include "AAIConstructor.h"
#include "AAIBuildTask.h"
#include "AAIConfig.h"
#include "AAIMap.h"
#include "AAIGroup.h"
#include "AAISector.h"

#include "LegacyCpp/UnitDef.h"
#include "LegacyCpp/CommandQueue.h"
using namespace springLegacyAI;


// all the static vars
float AAIExecute::current = 0.5;
float AAIExecute::learned = 2.5;


AAIExecute::AAIExecute(AAI *ai)
{
	issued_orders = 0;

	this->ai = ai;

	unitProductionRate = 1;

	futureRequestedMetal = 0;
	futureRequestedEnergy = 0;
	futureAvailableMetal = 0;
	futureAvailableEnergy = 0;
	futureStoredMetal = 0;
	futureStoredEnergy = 0;
	averageMetalUsage = 0;
	averageEnergyUsage = 0;
	averageMetalSurplus = 0;
	averageEnergySurplus = 0;
	disabledMMakers = 0;

	next_defence = 0;
	def_category = UNKNOWN;

	for(int i = 0; i <= METAL_MAKER; ++i)
		urgency[i] = 0;

	for(int i = 0; i < 8; i++)
	{
		metalSurplus[i] = 0;
		energySurplus[i] = 0;
	}

	counter = 0;
}

AAIExecute::~AAIExecute(void)
{
//	if(buildques)
//	{
//		for(int i = 0; i < numOfFactories; ++i)
//			buildques[i].clear();

//		spring::SafeDeleteArray(buildques);
//	}

//	if(factory_table)
//		spring::SafeDeleteArray(factory_table);
}


void AAIExecute::InitAI(int commander_unit_id, const UnitDef* commander_def)
{
	//debug
	ai->Log("Playing as %s\n", ai->Getbt()->sideNames[ai->Getside()].c_str());

	if(ai->Getside() < 1 || ai->Getside() > ai->Getbt()->numOfSides)
	{
		ai->LogConsole("ERROR: invalid side id %i\n", ai->Getside());
		return;
	}

	// tell the brain about the starting sector
	float3 pos = ai->Getcb()->GetUnitPos(commander_unit_id);
	int x = pos.x/ai->Getmap()->xSectorSize;
	int y = pos.z/ai->Getmap()->ySectorSize;

	if(x < 0)
		x = 0;
	if(y < 0 )
		y = 0;
	if(x >= ai->Getmap()->xSectors)
		x = ai->Getmap()->xSectors-1;
	if(y >= ai->Getmap()->ySectors)
		y = ai->Getmap()->ySectors-1;

	// set sector as part of the base
	if(ai->Getmap()->team_sector_map[x][y] < 0)
	{
		ai->Getbrain()->AddSector(&ai->Getmap()->sector[x][y]);
		ai->Getbrain()->start_pos = pos;
		ai->Getbrain()->UpdateNeighbouringSectors();
		ai->Getbrain()->UpdateBaseCenter();
	}
	else
	{
		// sector already occupied by another aai team (coms starting too close to each other)
		// choose next free sector
		ChooseDifferentStartingSector(x, y);
	}

	if(ai->Getmap()->map_type == WATER_MAP)
		ai->Getbrain()->ExpandBase(WATER_SECTOR);
	else if(ai->Getmap()->map_type == LAND_MAP)
		ai->Getbrain()->ExpandBase(LAND_SECTOR);
	else
		ai->Getbrain()->ExpandBase(LAND_WATER_SECTOR);

	// now that we know the side, init buildques
	InitBuildques();

	ai->Getbt()->InitCombatEffCache(ai->Getside());

	ai->Getut()->AddCommander(commander_unit_id, commander_def->id);

	// add the highest rated, buildable factory
	AddStartFactory();

	// get economy working
	CheckRessources();
}

void AAIExecute::CreateBuildTask(int unit, const UnitDef *def, float3 *pos)
{
	AAIBuildTask *task = new AAIBuildTask(ai, unit, def->id, pos, ai->Getcb()->GetCurrentFrame());
	ai->Getbuild_tasks().push_back(task);

	// find builder and associate building with that builder
	task->builder_id = -1;

	for(set<int>::iterator i = ai->Getut()->constructors.begin(); i != ai->Getut()->constructors.end(); ++i)
	{
		if(ai->Getut()->units[*i].cons->build_pos.x == pos->x && ai->Getut()->units[*i].cons->build_pos.z == pos->z)
		{
			ai->Getut()->units[*i].cons->construction_unit_id = unit;
			task->builder_id = ai->Getut()->units[*i].cons->unit_id;
			ai->Getut()->units[*i].cons->build_task = task;
			ai->Getut()->units[*i].cons->CheckAssistance();
			break;
		}
	}
}

bool AAIExecute::InitBuildingAt(const UnitDef *def, float3 *pos, bool water)
{
	// determine target sector
	int x = pos->x / ai->Getmap()->xSectorSize;
	int y = pos->z / ai->Getmap()->ySectorSize;

	// update buildmap
	ai->Getmap()->UpdateBuildMap(*pos, def, true, water, ai->Getbt()->IsFactory(def->id));

	// update defence map (if necessary)
	if(ai->Getbt()->units_static[def->id].category == STATIONARY_DEF)
		ai->Getmap()->AddDefence(pos, def->id);

	// drop bad sectors (should only happen when defending mexes at the edge of the map)
	if(x >= 0 && y >= 0 && x < ai->Getmap()->xSectors && y < ai->Getmap()->ySectors)
	{
		// increase number of units of that category in the target sector
		ai->Getmap()->sector[x][y].my_buildings[ai->Getbt()->units_static[def->id].category] += 1;

		return true;
	}
	else
		return false;
}

void AAIExecute::MoveUnitTo(int unit, float3 *position)
{
	Command c;

	c.id = CMD_MOVE;

	c.params.resize(3);
	c.params[0] = position->x;
	c.params[1] = position->y;
	c.params[2] = position->z;

	//ai->Getcb()->GiveOrder(unit, &c);
	GiveOrder(&c, unit, "MoveUnitTo");
	ai->Getut()->SetUnitStatus(unit, MOVING);
}

void AAIExecute::stopUnit(int unit)
{
	Command c;
	c.id = CMD_STOP;

	//ai->Getcb()->GiveOrder(unit, &c);
	GiveOrder(&c, unit, "StopUnit");
	ai->Getut()->SetUnitStatus(unit, UNIT_IDLE);
}

// returns true if unit is busy
bool AAIExecute::IsBusy(int unit)
{
	const CCommandQueue* commands = ai->Getcb()->GetCurrentUnitCommands(unit);

	if(commands->empty())
		return false;
	return true;
}

// adds a unit to the group of attackers
void AAIExecute::AddUnitToGroup(int unit_id, int def_id, UnitCategory category)
{
	UnitType unit_type = ai->Getbt()->GetUnitType(def_id);

	// determine continent if necessary
	int continent_id = -1;

	if(ai->Getbt()->units_static[def_id].movement_type & MOVE_TYPE_CONTINENT_BOUND)
	{
		float3 unitPos = ai->Getcb()->GetUnitPos(unit_id);
		continent_id = ai->Getmap()->GetContinentID(&unitPos);
	}

	// try to add unit to an existing group
	for(list<AAIGroup*>::iterator group = ai->Getgroup_list()[category].begin(); group != ai->Getgroup_list()[category].end(); ++group)
	{
		if((*group)->AddUnit(unit_id, def_id, unit_type, continent_id))
		{
			ai->Getut()->units[unit_id].group = *group;
			return;
		}
	}

	// end of grouplist has been reached and unit has not been assigned to any group
	// -> create new one

	// get continent for ground assault units, even if they are amphibious (otherwise non amphib ground units will be added no matter which continent they are on)
	if(category == GROUND_ASSAULT  && continent_id == -1) {
		float3 pos = ai->Getcb()->GetUnitPos(unit_id);
		continent_id = ai->Getmap()->GetContinentID(&pos);
	}
	AAIGroup *new_group = new AAIGroup(ai, &ai->Getbt()->GetUnitDef(def_id), unit_type, continent_id);

	ai->Getgroup_list()[category].push_back(new_group);
	new_group->AddUnit(unit_id, def_id, unit_type, continent_id);
	ai->Getut()->units[unit_id].group = new_group;
}

void AAIExecute::BuildScouts()
{
	// check number of scouts and order new ones if necessary
	if(ai->Getut()->activeUnits[SCOUT] + ai->Getut()->futureUnits[SCOUT] + ai->Getut()->requestedUnits[SCOUT] < cfg->MAX_SCOUTS)
	{
		int scout = 0;

		float cost;
		float los;

		int period = ai->Getbrain()->GetGamePeriod();

		if(period == 0)
		{
			cost = 2.0f;
			los = 0.5f;
		}
		else if(period == 1)
		{
			cost = 1.0f;
			los = 2.0f;
		}
		else
		{
			cost = 0.5f;
			los = 4.0f;
		}

		// determine movement type of scout based on map
		// always: MOVE_TYPE_AIR, MOVE_TYPE_HOVER, MOVE_TYPE_AMPHIB
		unsigned int allowed_movement_types = 22;

		if(ai->Getmap()->map_type == LAND_MAP)
			allowed_movement_types |= MOVE_TYPE_GROUND;
		else if(ai->Getmap()->map_type == LAND_WATER_MAP)
		{
			allowed_movement_types |= MOVE_TYPE_GROUND;
			allowed_movement_types |= MOVE_TYPE_SEA;
		}
		else if(ai->Getmap()->map_type == WATER_MAP)
			allowed_movement_types |= MOVE_TYPE_SEA;


		// request cloakable scouts from time to time
		if(rand()%5 == 1)
			scout = ai->Getbt()->GetScout(ai->Getside(), los, cost, allowed_movement_types, 10, true, true);
		else
			scout = ai->Getbt()->GetScout(ai->Getside(), los, cost, allowed_movement_types, 10, false, true);

		if(scout)
		{
			bool urgent = true;

			if(ai->Getut()->activeUnits[SCOUT] >= 2)
				urgent = false;

			if(AddUnitToBuildqueue(scout, 1, urgent))
			{
				ai->Getut()->UnitRequested(SCOUT);
				++ai->Getbt()->units_dynamic[scout].requested;
			}
		}
	}
}

void AAIExecute::SendScoutToNewDest(int scout)
{
	float3 pos = ZeroVector;

	// get scout dest
	ai->Getbrain()->GetNewScoutDest(&pos, scout);

	if(pos.x > 0)
		MoveUnitTo(scout, &pos);
}

float3 AAIExecute::GetBuildsite(int builder, int building, UnitCategory /*category*/)
{
	float3 pos;
	float3 builder_pos;
	//const UnitDef *def = ai->Getbt()->GetUnitDef(building);

	// check the sector of the builder
	builder_pos = ai->Getcb()->GetUnitPos(builder);
	// look in the builders sector first
	int x = builder_pos.x/ai->Getmap()->xSectorSize;
	int y = builder_pos.z/ai->Getmap()->ySectorSize;

	if(ai->Getmap()->sector[x][y].distance_to_base == 0)
	{
		pos = ai->Getmap()->sector[x][y].GetBuildsite(building);

		// if suitable location found, return pos...
		if(pos.x)
			return pos;
	}

	// look in any of the base sectors
	for(list<AAISector*>::iterator s = ai->Getbrain()->sectors[0].begin(); s != ai->Getbrain()->sectors[0].end(); ++s)
	{
		pos = (*s)->GetBuildsite(building);

		// if suitable location found, return pos...
		if(pos.x)
			return pos;
	}

	pos.x = pos.y = pos.z = 0;
	return pos;
}

float3 AAIExecute::GetUnitBuildsite(int /*builder*/, int unit)
{
//	float3 builder_pos = ai->Getcb()->GetUnitPos(builder);
	float3 pos = ZeroVector, best_pos = ZeroVector;
	float min_dist = 1000000, dist;

	for(list<AAISector*>::iterator s = ai->Getbrain()->sectors[1].begin(); s != ai->Getbrain()->sectors[1].end(); ++s)
	{
		bool water = false;

		if(ai->Getbt()->IsSea(unit))
			water = true;

		pos = (*s)->GetBuildsite(unit, water);

		if(pos.x)
		{
			dist = sqrt( pow(pos.x - best_pos.x ,2.0f) + pow(pos.z - best_pos.z, 2.0f) );

			if(dist < min_dist)
			{
				min_dist = dist;
				best_pos = pos;
			}
		}
	}

	return best_pos;
}

float AAIExecute::GetTotalGroundPower()
{
	float power = 0;

	// get ground power of all ground assault units
	for(list<AAIGroup*>::iterator group = ai->Getgroup_list()[GROUND_ASSAULT].begin(); group != ai->Getgroup_list()[GROUND_ASSAULT].end(); ++group)
		power += (*group)->GetCombatPowerVsCategory(0);

	return power;
}

float AAIExecute::GetTotalAirPower()
{
	float power = 0;

	for(list<AAIGroup*>::iterator group = ai->Getgroup_list()[GROUND_ASSAULT].begin(); group != ai->Getgroup_list()[GROUND_ASSAULT].end(); ++group)
	{
		power += (*group)->GetCombatPowerVsCategory(1);
	}

	return power;
}

list<int>* AAIExecute::GetBuildqueueOfFactory(int def_id)
{
	for(int i = 0; i < numOfFactories; ++i)
	{
		if(factory_table[i] == def_id)
			return &buildques[i];
	}

	return 0;
}

bool AAIExecute::AddUnitToBuildqueue(int def_id, int number, bool urgent)
{
	urgent = false;

	//UnitCategory category = ai->Getbt()->units_static[def_id].category;

	list<int> *buildqueue = 0, *temp_buildqueue = 0;

	float my_rating, best_rating = 0;

	for(list<int>::iterator fac = ai->Getbt()->units_static[def_id].builtByList.begin(); fac != ai->Getbt()->units_static[def_id].builtByList.end(); ++fac)
	{
		if(ai->Getbt()->units_dynamic[*fac].active > 0)
		{
			temp_buildqueue = GetBuildqueueOfFactory(*fac);

			if(temp_buildqueue)
			{
				my_rating = (1 + 2 * (float) ai->Getbt()->units_dynamic[*fac].active) / (temp_buildqueue->size() + 3);

				if(ai->Getmap()->map_type == WATER_MAP && !ai->Getbt()->CanPlacedWater(*fac))
					my_rating /= 10.0;
			}
			else
				my_rating = 0;
		}
		else
			my_rating = 0;

		if(my_rating > best_rating)
		{
			best_rating = my_rating;
			buildqueue = temp_buildqueue;
		}
	}

	// determine position
	if(buildqueue)
	{
		if(urgent)
		{
				buildqueue->insert(buildqueue->begin(), number, def_id);
				return true;
		}
		else if(buildqueue->size() < cfg->MAX_BUILDQUE_SIZE)
		{
			buildqueue->insert(buildqueue->end(), number, def_id);
			return true;
		}
	}

	return false;
}

void AAIExecute::InitBuildques()
{
	// determine number of factories first
	numOfFactories = 0;

	// stationary factories
	for(list<int>::iterator cons = ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].begin(); cons != ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].end(); ++cons)
	{
		if(ai->Getbt()->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
			++numOfFactories;
	}
	// and look for all mobile factories
	for(list<int>::iterator cons = ai->Getbt()->units_of_category[MOBILE_CONSTRUCTOR][ai->Getside()-1].begin(); cons != ai->Getbt()->units_of_category[MOBILE_CONSTRUCTOR][ai->Getside()-1].end(); ++cons)
	{
		if(ai->Getbt()->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
			++numOfFactories;
	}
	// and add com
	for(list<int>::iterator cons = ai->Getbt()->units_of_category[COMMANDER][ai->Getside()-1].begin(); cons != ai->Getbt()->units_of_category[COMMANDER][ai->Getside()-1].end(); ++cons)
	{
		if(ai->Getbt()->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
			++numOfFactories;
	}

//	buildques = new list<int>[numOfFactories];
	buildques.resize(numOfFactories);

	// set up factory buildque identification
//	factory_table = new int[numOfFactories];
	factory_table.resize(numOfFactories);

	int i = 0;

	for(list<int>::iterator cons = ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].begin(); cons != ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].end(); ++cons)
	{
		if(ai->Getbt()->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
		{
			factory_table[i] = *cons;
			++i;
		}
	}

	for(list<int>::iterator cons = ai->Getbt()->units_of_category[MOBILE_CONSTRUCTOR][ai->Getside()-1].begin(); cons != ai->Getbt()->units_of_category[MOBILE_CONSTRUCTOR][ai->Getside()-1].end(); ++cons)
	{
		if(ai->Getbt()->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
		{
			factory_table[i] = *cons;
			++i;
		}
	}

	for(list<int>::iterator cons = ai->Getbt()->units_of_category[COMMANDER][ai->Getside()-1].begin(); cons != ai->Getbt()->units_of_category[COMMANDER][ai->Getside()-1].end(); ++cons)
	{
		if(ai->Getbt()->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
		{
			factory_table[i] = *cons;
			++i;
		}
	}
}

float3 AAIExecute::GetRallyPoint(unsigned int unit_movement_type, int continent_id, int min_dist, int max_dist)
{
	float3 pos;

	// continent bound units must get a rally point on their current continent
	if(unit_movement_type & MOVE_TYPE_CONTINENT_BOUND)
	{
		// check neighbouring sectors
		for(int i = min_dist; i <= max_dist; ++i)
		{
			if(unit_movement_type & MOVE_TYPE_GROUND)
				ai->Getbrain()->sectors[i].sort(suitable_for_ground_rallypoint);
			else if(unit_movement_type & MOVE_TYPE_SEA)
				ai->Getbrain()->sectors[i].sort(suitable_for_sea_rallypoint);

			for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[i].begin(); sector != ai->Getbrain()->sectors[i].end(); ++sector)
			{
				(*sector)->GetMovePosOnContinent(&pos, unit_movement_type, continent_id);

				if(pos.x > 0)
					return pos;
			}
		}
	}
	else // non continent bound units may get rally points at any pos (sea or ground)
	{
		// check neighbouring sectors
		for(int i = min_dist; i <= max_dist; ++i)
		{
			ai->Getbrain()->sectors[i].sort(suitable_for_all_rallypoint);

			for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[i].begin(); sector != ai->Getbrain()->sectors[i].end(); ++sector)
			{
				(*sector)->GetMovePos(&pos);

				if(pos.x > 0)
					return pos;
			}
		}
	}

	return ZeroVector;
}

float3 AAIExecute::GetRallyPointCloseTo(UnitCategory /*category*/, unsigned int unit_movement_type, int continent_id, float3 /*pos*/, int min_dist, int max_dist)
{
	float3 move_pos;


	if(unit_movement_type & MOVE_TYPE_CONTINENT_BOUND)
	{
		for(int i = min_dist; i <= max_dist; ++i)
		{
			if(unit_movement_type & MOVE_TYPE_GROUND)
				ai->Getbrain()->sectors[i].sort(suitable_for_ground_rallypoint);
			else if(unit_movement_type & MOVE_TYPE_SEA)
				ai->Getbrain()->sectors[i].sort(suitable_for_sea_rallypoint);

			for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[i].begin(); sector != ai->Getbrain()->sectors[i].end(); ++sector)
			{
				(*sector)->GetMovePosOnContinent(&move_pos, unit_movement_type, continent_id);

				if(move_pos.x > 0)
					return move_pos;
			}
		}
	}
	else
	{
		for(int i = min_dist; i <= max_dist; ++i)
		{
			ai->Getbrain()->sectors[i].sort(suitable_for_all_rallypoint);

			for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[i].begin(); sector != ai->Getbrain()->sectors[i].end(); ++sector)
			{
				(*sector)->GetMovePos(&move_pos);

				if(move_pos.x > 0)
					return move_pos;
			}
		}
	}


	return ZeroVector;

	/*AAISector* best_sector = 0;
	float best_rating = 0, my_rating;
	float3 center;

	// check neighbouring sectors
	for(int i = min_dist; i <= max_dist; i++)
	{
		for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[i].begin(); sector != ai->Getbrain()->sectors[i].end(); sector++)
		{
			my_rating = 24.0f / ((*sector)->mobile_combat_power[combat_cat] + 4.0f);

			if(land)
				my_rating += 8 * pow((*sector)->flat_ratio, 2.0f);

			if(water)
				my_rating += 8 * pow((*sector)->water_ratio, 2.0f);

			center = (*sector)->GetCenter();

			my_rating /= sqrt( sqrt( pow( pos.x - center.x , 2.0f) + pow( pos.z - center.z , 2.0f) ) );

			if(my_rating > best_rating)
			{
				best_rating = my_rating;
				best_sector = *sector;
			}
		}
	}

	if(best_sector)
		return best_sector->GetMovePos(unit_movement_type);
	else
		return ZeroVector;	*/
}


// ****************************************************************************************************
// all building functions
// ****************************************************************************************************

bool AAIExecute::BuildExtractor()
{
	AAIConstructor *builder, *land_builder = 0, *water_builder = 0;
	float3 pos;
	int land_mex = 0, water_mex = 0;
	float min_dist;

	float cost = 0.25f + ai->Getbrain()->Affordable() / 6.0f;
	float efficiency = 6.0 / (cost + 0.75f);

	// check if metal map
	if(ai->Getmap()->metalMap)
	{
		// get id of an extractor and look for suitable builder
		land_mex = ai->Getbt()->GetMex(ai->Getside(), cost, efficiency, false, false, false);

		if(land_mex && ai->Getbt()->units_dynamic[land_mex].constructorsAvailable <= 0 && ai->Getbt()->units_dynamic[land_mex].constructorsRequested <= 0)
		{
			ai->Getbt()->BuildBuilderFor(land_mex);
			land_mex = ai->Getbt()->GetMex(ai->Getside(), cost, efficiency, false, false, true);
		}

		land_builder = ai->Getut()->FindBuilder(land_mex, true);

		if(land_builder)
		{
			pos = GetBuildsite(land_builder->unit_id, land_mex, EXTRACTOR);

			if(pos.x != 0)
				land_builder->GiveConstructionOrder(land_mex, pos, false);

			return true;
		}
		else
		{
			ai->Getbt()->BuildBuilderFor(land_mex);
			return false;
		}
	}

	// normal map

	// select a land/water mex
	if(ai->Getmap()->land_metal_spots > 0)
	{
		land_mex = ai->Getbt()->GetMex(ai->Getside(), cost, efficiency, false, false, false);

		if(land_mex && ai->Getbt()->units_dynamic[land_mex].constructorsAvailable + ai->Getbt()->units_dynamic[land_mex].constructorsRequested <= 0)
		{
			ai->Getbt()->BuildBuilderFor(land_mex);
			land_mex = ai->Getbt()->GetMex(ai->Getside(), cost, efficiency, false, false, true);
		}

		land_builder = ai->Getut()->FindBuilder(land_mex, true);
	}

	if(ai->Getmap()->water_metal_spots > 0)
	{
		water_mex = ai->Getbt()->GetMex(ai->Getside(), cost, efficiency, false, true, false);

		if(water_mex && ai->Getbt()->units_dynamic[water_mex].constructorsAvailable + ai->Getbt()->units_dynamic[water_mex].constructorsRequested <= 0)
		{
			ai->Getbt()->BuildBuilderFor(water_mex);
			water_mex = ai->Getbt()->GetMex(ai->Getside(), cost, efficiency, false, true, true);
		}

		water_builder = ai->Getut()->FindBuilder(water_mex, true);
	}

	// check if there is any builder for at least one of the selected extractors available
	if(!land_builder && !water_builder)
		return false;

	// check the first 10 free spots for the one with least distance to available builder
	int max_spots = 10;
	int current_spot = 0;
	bool free_spot_found = false;

	vector<AAIMetalSpot*> spots;
	spots.resize(max_spots);
	vector<AAIConstructor*> builders;
	builders.resize(max_spots, 0);

	vector<float> dist_to_builder;

	// determine max search dist - prevent crashes on smaller maps
	int max_search_dist = min(cfg->MAX_MEX_DISTANCE, ai->Getbrain()->max_distance);

	for(int sector_dist = 0; sector_dist < max_search_dist; ++sector_dist)
	{
		if(sector_dist == 1)
			ai->Getbrain()->freeBaseSpots = false;

		for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[sector_dist].begin(); sector != ai->Getbrain()->sectors[sector_dist].end(); ++sector)
		{
			if(ai->Getbrain()->MexConstructionAllowedInSector(*sector))
			{
				for(list<AAIMetalSpot*>::iterator spot = (*sector)->metalSpots.begin(); spot != (*sector)->metalSpots.end(); ++spot)
				{
					if(!(*spot)->occupied)
					{
						//
						if((*spot)->pos.y >= 0 && land_builder)
						{
							free_spot_found = true;

							builder = ai->Getut()->FindClosestBuilder(land_mex, &(*spot)->pos, ai->Getbrain()->CommanderAllowedForConstructionAt(*sector, &(*spot)->pos), &min_dist);

							if(builder)
							{
								dist_to_builder.push_back(min_dist);
								spots[current_spot] = *spot;
								builders[current_spot] = builder;

								++current_spot;
							}
						}
						else if((*spot)->pos.y < 0 && water_builder)
						{
							free_spot_found = true;

							builder = ai->Getut()->FindClosestBuilder(water_mex, &(*spot)->pos, ai->Getbrain()->CommanderAllowedForConstructionAt(*sector, &(*spot)->pos), &min_dist);

							if(builder)
							{
								dist_to_builder.push_back(min_dist);
								spots[current_spot] = *spot;
								builders[current_spot] = builder;

								++current_spot;
							}
						}

						if(current_spot >= max_spots)
							break;
					}
				}
			}

			if(current_spot >= max_spots)
				break;
		}

		if(current_spot >= max_spots)
			break;
	}

	// look for spot with minimum dist to available builder
	int best = -1;
	min_dist = 1000000.0f;

	for(size_t i = 0; i < dist_to_builder.size(); ++i)
	{
		if(dist_to_builder[i] < min_dist)
		{
			best = i;
			min_dist = dist_to_builder[i];
		}
	}

	// order mex construction for best spot
	if(best >= 0)
	{
		if(spots[best]->pos.y < 0)
			builders[best]->GiveConstructionOrder(water_mex, spots[best]->pos, true);
		else
			builders[best]->GiveConstructionOrder(land_mex, spots[best]->pos, false);

		spots[best]->occupied = true;

		return true;
	}

	// dont build other things if construction could not be started due to unavailable builders
	if(free_spot_found)
		return false;
	else
		return true;
}

bool AAIExecute::BuildPowerPlant()
{
	if(ai->Getut()->futureUnits[POWER_PLANT] + ai->Getut()->requestedUnits[POWER_PLANT] > 1)
		return true;
	else if(ai->Getut()->futureUnits[POWER_PLANT] <= 0 && ai->Getut()->requestedUnits[POWER_PLANT] > 0)
		return true;
	else if(ai->Getut()->futureUnits[POWER_PLANT] > 0)
	{
		// try to assist construction of other power plants first
		AAIConstructor *builder;

		for(list<AAIBuildTask*>::iterator task = ai->Getbuild_tasks().begin(); task != ai->Getbuild_tasks().end(); ++task)
		{
			if((*task)->builder_id >= 0)
				builder = ai->Getut()->units[(*task)->builder_id].cons;
			else
				builder = 0;

			// find the power plant that is already under construction
			if(builder && builder->construction_category == POWER_PLANT)
			{
				// dont build further power plants if already building an expensive plant
				if(ai->Getbt()->units_static[builder->construction_def_id].cost > ai->Getbt()->avg_cost[POWER_PLANT][ai->Getside()-1])
					return true;

				// try to assist
				if(builder->assistants.size() < cfg->MAX_ASSISTANTS)
				{
					AAIConstructor *assistant = ai->Getut()->FindClosestAssistant(builder->build_pos, 5, true);

					if(assistant)
					{
						builder->assistants.insert(assistant->unit_id);
						assistant->AssistConstruction(builder->unit_id, (*task)->unit_id);
						return true;
					}
					else
						return false;
				}
			}
		}

		// power plant construction has not started -> builder is still on its way to construction site, wait until starting a new power plant
		return false;
	}
	else if(ai->Getut()->activeFactories < 1 && ai->Getut()->activeUnits[POWER_PLANT] >= 2)
		return true;

	const float current_energy = ai->Getcb()->GetEnergyIncome();

	// stop building power plants if already to much available energy
	if(current_energy > 1.5f * ai->Getcb()->GetEnergyUsage() + 200.0f)
		return true;

	int ground_plant = 0;
	int water_plant = 0;

	AAIConstructor *builder;
	float3 pos;

	bool checkWater, checkGround;
	float urgency;
	float max_power;
	float eff;
	float energy = ai->Getcb()->GetEnergyIncome()+1.0f;

	// check if already one power_plant under construction and energy short
	if(ai->Getut()->futureUnits[POWER_PLANT] + ai->Getut()->requestedUnits[POWER_PLANT]> 0 && ai->Getut()->activeUnits[POWER_PLANT] > 9 && averageEnergySurplus < 100)
	{
		urgency = 0.4f + GetEnergyUrgency();
		max_power = 0.5f;
		eff = 2.2f - ai->Getbrain()->Affordable() / 4.0f;
	}
	else
	{
		max_power = 0.5f + pow((float) ai->Getut()->activeUnits[POWER_PLANT], 0.8f);
		eff = 0.5 + 1.0f / (ai->Getbrain()->Affordable() + 0.5f);
		urgency = 0.5f + GetEnergyUrgency();
	}

	// sort sectors according to threat level
	learned = 70000.0f / (float)(ai->Getcb()->GetCurrentFrame() + 35000) + 1.0f;
	current = 2.5f - learned;

	if(ai->Getut()->activeUnits[POWER_PLANT] >= 2)
		ai->Getbrain()->sectors[0].sort(suitable_for_power_plant);

	// get water and ground plant
	ground_plant = ai->Getbt()->GetPowerPlant(ai->Getside(), eff, urgency, max_power, energy, false, false, false);
	// currently aai cannot build this building
	if(ground_plant && ai->Getbt()->units_dynamic[ground_plant].constructorsAvailable <= 0)
	{
		if( ai->Getbt()->units_dynamic[water_plant].constructorsRequested <= 0)
			ai->Getbt()->BuildBuilderFor(ground_plant);

		ground_plant = ai->Getbt()->GetPowerPlant(ai->Getside(), eff, urgency, max_power, energy, false, false, true);
	}

	water_plant = ai->Getbt()->GetPowerPlant(ai->Getside(), eff, urgency, max_power, energy, true, false, false);
	// currently aai cannot build this building
	if(water_plant && ai->Getbt()->units_dynamic[water_plant].constructorsAvailable <= 0)
	{
		if( ai->Getbt()->units_dynamic[water_plant].constructorsRequested <= 0)
			ai->Getbt()->BuildBuilderFor(water_plant);

		water_plant = ai->Getbt()->GetPowerPlant(ai->Getside(), eff, urgency, max_power, energy, true, false, true);
	}

	for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
	{
		if((*sector)->water_ratio < 0.15)
		{
			checkWater = false;
			checkGround = true;
		}
		else if((*sector)->water_ratio < 0.85)
		{
			checkWater = true;
			checkGround = true;
		}
		else
		{
			checkWater = true;
			checkGround = false;
		}

		if(checkGround && ground_plant)
		{
			pos = (*sector)->GetBuildsite(ground_plant, false);

			if(pos.x > 0)
			{
				float min_dist;
				builder = ai->Getut()->FindClosestBuilder(ground_plant, &pos, true, &min_dist);

				if(builder)
				{
					futureAvailableEnergy += ai->Getbt()->units_static[ground_plant].efficiency[0];
					builder->GiveConstructionOrder(ground_plant, pos, false);
					return true;
				}
				else
				{
					ai->Getbt()->BuildBuilderFor(ground_plant);
					return false;
				}
			}
			else
			{
				ai->Getbrain()->ExpandBase(LAND_SECTOR);
				ai->Log("Base expanded by BuildPowerPlant()\n");
			}
		}

		if(checkWater && water_plant)
		{
			if(ai->Getut()->constructors.size() > 1 || ai->Getut()->activeUnits[POWER_PLANT] >= 2)
				pos = (*sector)->GetBuildsite(water_plant, true);
			else
			{
				builder = ai->Getut()->FindBuilder(water_plant, true);

				if(builder)
				{
					pos = ai->Getmap()->GetClosestBuildsite(&ai->Getbt()->GetUnitDef(water_plant), ai->Getcb()->GetUnitPos(builder->unit_id), 40, true);

					if(pos.x <= 0)
						pos = (*sector)->GetBuildsite(water_plant, true);
				}
				else
					pos = (*sector)->GetBuildsite(water_plant, true);
			}

			if(pos.x > 0)
			{
				float min_dist;
				builder = ai->Getut()->FindClosestBuilder(water_plant, &pos, true, &min_dist);

				if(builder)
				{
					futureAvailableEnergy += ai->Getbt()->units_static[water_plant].efficiency[0];
					builder->GiveConstructionOrder(water_plant, pos, true);
					return true;
				}
				else
				{
					ai->Getbt()->BuildBuilderFor(water_plant);
					return false;
				}
			}
			else
			{
				ai->Getbrain()->ExpandBase(WATER_SECTOR);
				ai->Log("Base expanded by BuildPowerPlant() (water sector)\n");
			}
		}
	}

	return true;
}

bool AAIExecute::BuildMetalMaker()
{
	if(ai->Getut()->activeFactories < 1 && ai->Getut()->activeUnits[EXTRACTOR] >= 2)
		return true;

	if(ai->Getut()->futureUnits[METAL_MAKER] + ai->Getut()->requestedUnits[METAL_MAKER] > 0 || disabledMMakers >= 1)
		return true;

	bool checkWater, checkGround;
	int maker;
	AAIConstructor *builder;
	float3 pos;
	// urgency < 4

	float urgency = GetMetalUrgency() / 2.0f;

	float cost = 0.25f + ai->Getbrain()->Affordable() / 2.0f;

	float efficiency = 0.25f + ai->Getut()->activeUnits[METAL_MAKER] / 4.0f ;
	float metal = efficiency;


	// sort sectors according to threat level
	learned = 70000.0 / (ai->Getcb()->GetCurrentFrame() + 35000) + 1;
	current = 2.5 - learned;

	ai->Getbrain()->sectors[0].sort(least_dangerous);

	for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
	{
		if((*sector)->water_ratio < 0.15)
		{
			checkWater = false;
			checkGround = true;
		}
		else if((*sector)->water_ratio < 0.85)
		{
			checkWater = true;
			checkGround = true;
		}
		else
		{
			checkWater = true;
			checkGround = false;
		}

		if(checkGround)
		{
			maker = ai->Getbt()->GetMetalMaker(ai->Getside(), cost,  efficiency, metal, urgency, false, false);

			// currently aai cannot build this building
			if(maker && ai->Getbt()->units_dynamic[maker].constructorsAvailable <= 0)
			{
				if(ai->Getbt()->units_dynamic[maker].constructorsRequested <= 0)
					ai->Getbt()->BuildBuilderFor(maker);

				maker = ai->Getbt()->GetMetalMaker(ai->Getside(), cost, efficiency, metal, urgency, false, true);
			}

			if(maker)
			{
				pos = (*sector)->GetBuildsite(maker, false);

				if(pos.x > 0)
				{
					float min_dist;
					builder = ai->Getut()->FindClosestBuilder(maker, &pos, true, &min_dist);

					if(builder)
					{
						futureRequestedEnergy += ai->Getbt()->GetUnitDef(maker).energyUpkeep;
						builder->GiveConstructionOrder(maker, pos, false);
						return true;
					}
					else
					{
						ai->Getbt()->BuildBuilderFor(maker);
						return false;
					}
				}
				else
				{
					ai->Getbrain()->ExpandBase(LAND_SECTOR);
					ai->Log("Base expanded by BuildMetalMaker()\n");
				}
			}
		}

		if(checkWater)
		{
			maker = ai->Getbt()->GetMetalMaker(ai->Getside(), ai->Getbrain()->Affordable(),  8.0/(urgency+2.0), 64.0/(16*urgency+2.0), urgency, true, false);

			// currently aai cannot build this building
			if(maker && ai->Getbt()->units_dynamic[maker].constructorsAvailable <= 0)
			{
				if(ai->Getbt()->units_dynamic[maker].constructorsRequested <= 0)
					ai->Getbt()->BuildBuilderFor(maker);

				maker = ai->Getbt()->GetMetalMaker(ai->Getside(), ai->Getbrain()->Affordable(),  8.0/(urgency+2.0), 64.0/(16*urgency+2.0), urgency, true, true);
			}

			if(maker)
			{
				pos = (*sector)->GetBuildsite(maker, true);

				if(pos.x > 0)
				{
					float min_dist;
					builder = ai->Getut()->FindClosestBuilder(maker, &pos, true, &min_dist);

					if(builder)
					{
						futureRequestedEnergy += ai->Getbt()->GetUnitDef(maker).energyUpkeep;
						builder->GiveConstructionOrder(maker, pos, true);
						return true;
					}
					else
					{
						ai->Getbt()->BuildBuilderFor(maker);
						return false;
					}
				}
				else
				{
					ai->Getbrain()->ExpandBase(WATER_SECTOR);
					ai->Log("Base expanded by BuildMetalMaker() (water sector)\n");
				}
			}
		}
	}

	return true;
}

bool AAIExecute::BuildStorage()
{
	if(ai->Getut()->futureUnits[STORAGE] + ai->Getut()->requestedUnits[STORAGE]> 0 || ai->Getut()->activeUnits[STORAGE] >= cfg->MAX_STORAGE)
		return true;

	if(ai->Getut()->activeFactories < 2)
		return true;

	int storage = 0;
	bool checkWater, checkGround;
	AAIConstructor *builder;
	float3 pos;

	float metal = 4 / (ai->Getcb()->GetMetalStorage() + futureStoredMetal - ai->Getcb()->GetMetal() + 1);
	float energy = 2 / (ai->Getcb()->GetEnergyStorage() + futureStoredMetal - ai->Getcb()->GetEnergy() + 1);

	// urgency < 4
//	float urgency = 16.0 / (ai->Getut()->activeUnits[METAL_MAKER] + ai->Getut()->futureUnits[METAL_MAKER] + 4);

	for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
	{
		if((*sector)->water_ratio < 0.15)
		{
			checkWater = false;
			checkGround = true;
		}
		else if((*sector)->water_ratio < 0.85)
		{
			checkWater = true;
			checkGround = true;
		}
		else
		{
			checkWater = true;
			checkGround = false;
		}

		if(checkGround)
		{
			storage = ai->Getbt()->GetStorage(ai->Getside(), ai->Getbrain()->Affordable(),  metal, energy, 1, false, false);

			if(storage && ai->Getbt()->units_dynamic[storage].constructorsAvailable <= 0)
			{
				if(ai->Getbt()->units_dynamic[storage].constructorsRequested <= 0)
					ai->Getbt()->BuildBuilderFor(storage);

				storage = ai->Getbt()->GetStorage(ai->Getside(), ai->Getbrain()->Affordable(),  metal, energy, 1, false, true);
			}

			if(storage)
			{
				pos = (*sector)->GetBuildsite(storage, false);

				if(pos.x > 0)
				{
					float min_dist;
					builder = ai->Getut()->FindClosestBuilder(storage, &pos, true, &min_dist);

					if(builder)
					{
						builder->GiveConstructionOrder(storage, pos, false);
						return true;
					}
					else
					{
						ai->Getbt()->BuildBuilderFor(storage);
						return false;
					}
				}
				else
				{
					ai->Getbrain()->ExpandBase(LAND_SECTOR);
					ai->Log("Base expanded by BuildStorage()\n");
				}
			}
		}

		if(checkWater)
		{
			storage = ai->Getbt()->GetStorage(ai->Getside(), ai->Getbrain()->Affordable(),  metal, energy, 1, true, false);

			if(storage && ai->Getbt()->units_dynamic[storage].constructorsAvailable <= 0)
			{
				if( ai->Getbt()->units_dynamic[storage].constructorsRequested <= 0)
					ai->Getbt()->BuildBuilderFor(storage);

				storage = ai->Getbt()->GetStorage(ai->Getside(), ai->Getbrain()->Affordable(),  metal, energy, 1, true, true);
			}

			if(storage)
			{
				pos = (*sector)->GetBuildsite(storage, true);

				if(pos.x > 0)
				{
					float min_dist;
					builder = ai->Getut()->FindClosestBuilder(storage, &pos, true, &min_dist);

					if(builder)
					{
						builder->GiveConstructionOrder(storage, pos, true);
						return true;
					}
					else
					{
						ai->Getbt()->BuildBuilderFor(storage);
						return false;
					}
				}
				else
				{
					ai->Getbrain()->ExpandBase(WATER_SECTOR);
					ai->Log("Base expanded by BuildStorage()\n");
				}
			}
		}
	}

	return true;
}

bool AAIExecute::BuildAirBase()
{
	if(ai->Getut()->futureUnits[AIR_BASE] + ai->Getut()->requestedUnits[AIR_BASE] > 0 || ai->Getut()->activeUnits[AIR_BASE] >= cfg->MAX_AIR_BASE)
		return true;

	int airbase = 0;
	bool checkWater, checkGround;
	AAIConstructor *builder;
	float3 pos;

	for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
	{
		if((*sector)->water_ratio < 0.15)
		{
			checkWater = false;
			checkGround = true;
		}
		else if((*sector)->water_ratio < 0.85)
		{
			checkWater = true;
			checkGround = true;
		}
		else
		{
			checkWater = true;
			checkGround = false;
		}

		if(checkGround)
		{

			airbase = ai->Getbt()->GetAirBase(ai->Getside(), ai->Getbrain()->Affordable(), false, false);

			if(airbase && ai->Getbt()->units_dynamic[airbase].constructorsAvailable <= 0)
			{
				if(ai->Getbt()->units_dynamic[airbase].constructorsRequested <= 0)
					ai->Getbt()->BuildBuilderFor(airbase);

				airbase = ai->Getbt()->GetAirBase(ai->Getside(), ai->Getbrain()->Affordable(), false, true);
			}

			if(airbase)
			{
				pos = (*sector)->GetBuildsite(airbase, false);

				if(pos.x > 0)
				{
					float min_dist;
					builder = ai->Getut()->FindClosestBuilder(airbase, &pos, true, &min_dist);

					if(builder)
					{
						builder->GiveConstructionOrder(airbase, pos, false);
						return true;
					}
					else
					{
						ai->Getbt()->BuildBuilderFor(airbase);
						return false;
					}
				}
				else
				{
					ai->Getbrain()->ExpandBase(LAND_SECTOR);
					ai->Log("Base expanded by BuildAirBase()\n");
				}
			}
		}

		if(checkWater)
		{
			airbase = ai->Getbt()->GetAirBase(ai->Getside(), ai->Getbrain()->Affordable(), true, false);

			if(airbase && ai->Getbt()->units_dynamic[airbase].constructorsAvailable <= 0 )
			{
				if(ai->Getbt()->units_dynamic[airbase].constructorsRequested <= 0)
					ai->Getbt()->BuildBuilderFor(airbase);

				airbase = ai->Getbt()->GetAirBase(ai->Getside(), ai->Getbrain()->Affordable(), true, true);
			}

			if(airbase)
			{
				pos = (*sector)->GetBuildsite(airbase, true);

				if(pos.x > 0)
				{
					float min_dist;
					builder = ai->Getut()->FindClosestBuilder(airbase, &pos, true, &min_dist);

					if(builder)
					{
						builder->GiveConstructionOrder(airbase, pos, true);
						return true;
					}
					else
					{
						ai->Getbt()->BuildBuilderFor(airbase);
						return false;
					}
				}
				else
				{
					ai->Getbrain()->ExpandBase(WATER_SECTOR);
					ai->Log("Base expanded by BuildAirBase() (water sector)\n");
				}
			}
		}
	}

	return true;
}

bool AAIExecute::BuildDefences()
{
	if(ai->Getut()->futureUnits[STATIONARY_DEF] + ai->Getut()->requestedUnits[STATIONARY_DEF] > 2 || next_defence == 0)
		return true;

	BuildOrderStatus status = BuildStationaryDefenceVS(def_category, next_defence);

	if(status == BUILDORDER_NOBUILDER)
		return false;
	else if(status == BUILDORDER_NOBUILDPOS)
		++next_defence->failed_defences;


	next_defence = 0;

	return true;
}

BuildOrderStatus AAIExecute::BuildStationaryDefenceVS(UnitCategory category, AAISector *dest)
{
	// dont build in sectors already occupied by allies
	if(dest->allied_structures > 5)
		return BUILDORDER_SUCCESFUL;

	// dont start construction of further defences if expensive defences are already under construction in this sector
	for(list<AAIBuildTask*>::iterator task = ai->Getbuild_tasks().begin(); task != ai->Getbuild_tasks().end(); ++task)
	{
		if(ai->Getbt()->units_static[(*task)->def_id].category == STATIONARY_DEF)
		{
			if(dest->PosInSector(&(*task)->build_pos))
			{
				if(ai->Getbt()->units_static[(*task)->def_id].cost > 0.7f * ai->Getbt()->avg_cost[STATIONARY_DEF][ai->Getside()-1])
					return BUILDORDER_SUCCESFUL;
			}
		}
	}

	double gr_eff = 0, air_eff = 0, hover_eff = 0, sea_eff = 0, submarine_eff = 0;

	bool checkWater, checkGround;
	int building;
	float3 pos;
	AAIConstructor *builder;

	float terrain = 2.0f;

	if(dest->distance_to_base > 0)
		terrain = 5.0f;

	if(dest->water_ratio < 0.15f)
	{
		checkWater = false;
		checkGround = true;
	}
	else if(dest->water_ratio < 0.85f)
	{
		checkWater = true;
		checkGround = true;
	}
	else
	{
		checkWater = true;
		checkGround = false;
	}

	float dynDef  = dest->GetMyDefencePowerAgainstAssaultCategory(ai->Getbt()->GetIDOfAssaultCategory(category));
	float baseDef = dest->my_buildings[STATIONARY_DEF];
	float urgency = 0.25f + 10.0f / (std::max(0.0f, dynDef + baseDef) + 1.0);
	float power = 0.5 + baseDef;
	float eff = 0.2 + 2.5 / (urgency + 1.0);
	float range = 0.2 + 0.6 / (urgency + 1.0);
	float cost = 0.5 + ai->Getbrain()->Affordable()/5.0;

	if(dest->my_buildings[STATIONARY_DEF] > 3)
	{
		int t = rand()%500;

		if(t < 70)
		{
			range = 2.5;
			terrain = 10.0f;
		}
		else if(t < 200)
		{
			range = 1;
			terrain = 5.0f;
		}
	}

	if(category == GROUND_ASSAULT)
		gr_eff = 2;
	else if(category == AIR_ASSAULT)
		air_eff = 2;
	else if(category == HOVER_ASSAULT)
	{
		gr_eff = 1;
		sea_eff = 1;
		hover_eff = 4;
	}
	else if(category == SEA_ASSAULT)
	{
		sea_eff = 4;
		submarine_eff = 1;
	}
	else if(category == SUBMARINE_ASSAULT)
	{
		sea_eff = 1;
		submarine_eff = 4;
	}

	if(checkGround)
	{
		if(dest->my_buildings[STATIONARY_DEF] > 2 && rand()%cfg->LEARN_RATE == 1)
			building = ai->Getbt()->GetRandomDefence(ai->Getside(), category);
		else
			building = ai->Getbt()->GetDefenceBuilding(ai->Getside(), eff, power, cost, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, range, 8, false, false);

		if(building && ai->Getbt()->units_dynamic[building].constructorsAvailable <= 0)
		{
			if(ai->Getbt()->units_dynamic[building].constructorsRequested <= 0)
				ai->Getbt()->BuildBuilderFor(building);

			building = ai->Getbt()->GetDefenceBuilding(ai->Getside(), eff, power, cost, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, range, 8, false, true);
		}

		// stop building weak defences if urgency is too low (wait for better defences)
		if(dest->my_buildings[STATIONARY_DEF] > 3)
		{
			if(ai->Getbt()->units_static[building].efficiency[ ai->Getbt()->GetIDOfAssaultCategory(category) ]  < 0.75f * ai->Getbt()->avg_eff[ai->Getside()-1][5][  ai->Getbt()->GetIDOfAssaultCategory(category) ] )
				building = 0;
		}
		else if(dest->my_buildings[STATIONARY_DEF] > 6)
		{
			if(ai->Getbt()->units_static[building].efficiency[ ai->Getbt()->GetIDOfAssaultCategory(category) ] < ai->Getbt()->avg_eff[ai->Getside()-1][5][ ai->Getbt()->GetIDOfAssaultCategory(category) ] )
				building = 0;
		}

		if(building)
		{
			pos = dest->GetDefenceBuildsite(building, category, terrain, false);

			if(pos.x > 0)
			{
				float min_dist;
				builder = ai->Getut()->FindClosestBuilder(building, &pos, true, &min_dist);

				if(builder)
				{
					builder->GiveConstructionOrder(building, pos, false);
					return BUILDORDER_SUCCESFUL;
				}
				else
				{
					ai->Getbt()->BuildBuilderFor(building);
					return BUILDORDER_NOBUILDER;
				}
			}
			else
				return BUILDORDER_NOBUILDPOS;
		}
	}

	if(checkWater)
	{
		if(rand()%cfg->LEARN_RATE == 1 && dest->my_buildings[STATIONARY_DEF] > 3)
			building = ai->Getbt()->GetRandomDefence(ai->Getside(), category);
		else
			building = ai->Getbt()->GetDefenceBuilding(ai->Getside(), eff, power, cost, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, 1, 8, true, false);

		if(building && ai->Getbt()->units_dynamic[building].constructorsAvailable <= 0)
		{
			if(ai->Getbt()->units_dynamic[building].constructorsRequested <= 0)
				ai->Getbt()->BuildBuilderFor(building);

			building = ai->Getbt()->GetDefenceBuilding(ai->Getside(), eff, power, cost, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, 1,  8, true, true);
		}

		// stop building of weak defences if urgency is too low (wait for better defences)
		if(dest->my_buildings[STATIONARY_DEF] > 3)
		{
			if(ai->Getbt()->units_static[building].efficiency[ ai->Getbt()->GetIDOfAssaultCategory(category) ]  < 0.75f * ai->Getbt()->avg_eff[ai->Getside()-1][5][  ai->Getbt()->GetIDOfAssaultCategory(category) ] )
				building = 0;
		}
		else if(dest->my_buildings[STATIONARY_DEF] > 6)
		{
			if(ai->Getbt()->units_static[building].efficiency[ ai->Getbt()->GetIDOfAssaultCategory(category) ]  < ai->Getbt()->avg_eff[ai->Getside()-1][5][  ai->Getbt()->GetIDOfAssaultCategory(category) ] )
				building = 0;
		}

		if(building)
		{
			pos = dest->GetDefenceBuildsite(building, category, terrain, true);

			if(pos.x > 0)
			{
				float min_dist;
				builder = ai->Getut()->FindClosestBuilder(building, &pos, true, &min_dist);

				if(builder)
				{
					builder->GiveConstructionOrder(building, pos, true);

					// add defence to map
					ai->Getmap()->AddDefence(&pos, building);

					return BUILDORDER_SUCCESFUL;
				}
				else
				{
					ai->Getbt()->BuildBuilderFor(building);
					return BUILDORDER_NOBUILDER;
				}
			}
			else
				return BUILDORDER_NOBUILDPOS;
		}
	}

	return BUILDORDER_FAILED;
}

bool AAIExecute::BuildArty()
{
	if(ai->Getut()->futureUnits[STATIONARY_ARTY] + ai->Getut()->requestedUnits[STATIONARY_ARTY] > 0)
		return true;

	int ground_arty = 0;
	int sea_arty = 0;

	float3 my_pos, best_pos;
	float my_rating, best_rating = -100000.0f;
	int arty = 0;

	// get ground radar
	if(ai->Getmap()->land_ratio > 0.02f)
	{
		ground_arty = ai->Getbt()->GetStationaryArty(ai->Getside(), 1, 2, 2, false, false);

		if(ground_arty && ai->Getbt()->units_dynamic[ground_arty].constructorsAvailable <= 0)
		{
			if(ai->Getbt()->units_dynamic[ground_arty].constructorsRequested <= 0)
				ai->Getbt()->BuildBuilderFor(ground_arty);

			ground_arty =ai->Getbt()->GetStationaryArty(ai->Getside(), 1, 2, 2, false, true);
		}
	}

	// get sea radar
	if(ai->Getmap()->water_ratio > 0.02f)
	{
		sea_arty = ai->Getbt()->GetStationaryArty(ai->Getside(), 1, 2, 2, true, false);

		if(sea_arty && ai->Getbt()->units_dynamic[sea_arty].constructorsAvailable <= 0)
		{
			if(ai->Getbt()->units_dynamic[sea_arty].constructorsRequested <= 0)
				ai->Getbt()->BuildBuilderFor(sea_arty);

			sea_arty = ai->Getbt()->GetStationaryArty(ai->Getside(), 1, 2, 2, true, true);
		}
	}

	for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
	{
		if((*sector)->my_buildings[STATIONARY_ARTY] < 2)
		{
			my_pos = ZeroVector;

			if(ground_arty && (*sector)->water_ratio < 0.9f)
				my_pos = (*sector)->GetRadarArtyBuildsite(ground_arty, ai->Getbt()->units_static[ground_arty].range/4.0f, false);

			if(my_pos.x <= 0 && sea_arty && (*sector)->water_ratio > 0.1f)
			{
				my_pos = (*sector)->GetRadarArtyBuildsite(sea_arty, ai->Getbt()->units_static[sea_arty].range/4.0f, true);

				if(my_pos.x > 0)
					ground_arty = sea_arty;
			}

			if(my_pos.x > 0)
			{
				my_rating = - ai->Getmap()->GetEdgeDistance(&my_pos);

				if(my_rating > best_rating)
				{
					best_rating = my_rating;
					best_pos = my_pos;
					arty = ground_arty;
				}
			}
		}
	}

	if(arty)
	{
		float min_dist;
		AAIConstructor *builder = ai->Getut()->FindClosestBuilder(arty, &best_pos, true, &min_dist);

		if(builder)
		{
			builder->GiveConstructionOrder(arty, best_pos, false);
			return true;
		}
		else
		{
			ai->Getbt()->BuildBuilderFor(ground_arty);
			return false;
		}
	}

	return true;
}

bool AAIExecute::BuildFactory()
{
	if(ai->Getut()->futureUnits[STATIONARY_CONSTRUCTOR] + ai->Getut()->requestedUnits[STATIONARY_CONSTRUCTOR] > 0)
		return true;

	AAIConstructor *builder = 0, *temp_builder;
	float3 pos = ZeroVector;
	float best_rating = 0, my_rating;
	int building = 0, factory_types_requested = 0;

	// go through list of factories, build cheapest requested factory first
	for(list<int>::iterator fac = ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].begin(); fac != ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].end(); ++fac)
	{
		if(ai->Getbt()->units_dynamic[*fac].requested > 0)
		{
			++factory_types_requested;

			const float activeFacsOfType = ai->Getbt()->units_dynamic[*fac].active;

			my_rating = ai->Getbt()->GetFactoryRating(*fac) / pow(activeFacsOfType + 1.0f, 2.0f);
			my_rating *= (1 + sqrt(2.0 + (float) GetBuildqueueOfFactory(*fac)->size()));

			if(ai->Getut()->activeFactories < 1)
				my_rating /= ai->Getbt()->units_static[*fac].cost;

			// skip factories that could not be built
			if(ai->Getbt()->units_static[*fac].efficiency[4] > 1)
			{
				my_rating = 0;
				ai->Getbt()->units_static[*fac].efficiency[4] = 0;
			}

			// only check building if a suitable builder is available
			temp_builder = ai->Getut()->FindBuilder(*fac, true);

			if(temp_builder && my_rating > best_rating)
			{
				best_rating = my_rating;
				building = *fac;
			}
		}
	}

	if(building)
	{
		bool water = false;

		// land
		if(ai->Getbt()->CanPlacedLand(building))
		{
			water = false;

			ai->Getbrain()->sectors[0].sort(suitable_for_ground_factory);

			// find buildpos
			for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
			{
				pos = (*sector)->GetRandomBuildsite(building, 20, false);

				if(pos.x > 0)
					break;
			}

			if(pos.x <= 0)
			{
				for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
				{
					pos = (*sector)->GetBuildsite(building, false);

					if(pos.x > 0)
						break;
				}
			}
		}

		// try to build on water if land has not been possible
		if(pos.x <= 0 && ai->Getbt()->CanPlacedWater(building))
		{
			water = true;

			ai->Getbrain()->sectors[0].sort(suitable_for_sea_factory);

			// find buildpos
			for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
			{
				if((*sector)->ConnectedToOcean())
				{
					pos = (*sector)->GetRandomBuildsite(building, 20, true);

					if(pos.x > 0)
						break;
				}
			}

			if(pos.x <= 0)
			{
				for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
				{
					if((*sector)->ConnectedToOcean())
					{
						pos = (*sector)->GetBuildsite(building, true);

						if(pos.x > 0)
							break;
					}
				}
			}
		}

		if(pos.x > 0)
		{
			float min_dist;

			builder = ai->Getut()->FindClosestBuilder(building, &pos, true, &min_dist);

			if(builder)
			{
				ai->Getbt()->units_dynamic[building].requested -= 1;

				// give build order
				builder->GiveConstructionOrder(building, pos, water);

				// add average ressource usage
				futureRequestedMetal += ai->Getbt()->units_static[building].efficiency[0];
				futureRequestedEnergy += ai->Getbt()->units_static[building].efficiency[1];

				return true;
			}
			else
			{
				if(ai->Getbt()->units_dynamic[building].constructorsRequested + ai->Getbt()->units_dynamic[building].constructorsAvailable <= 0)
					ai->Getbt()->BuildBuilderFor(building);

				return false;
			}
		}
		else
		{
			bool expanded = false;

			// no suitable buildsite found
			if(ai->Getbt()->CanPlacedLand(building))
			{
				expanded = ai->Getbrain()->ExpandBase(LAND_SECTOR);
				ai->Log("Base expanded by BuildFactory()\n");
			}

			if(!expanded && ai->Getbt()->CanPlacedWater(building))
			{
				ai->Getbrain()->ExpandBase(WATER_SECTOR);
				ai->Log("Base expanded by BuildFactory() (water sector)\n");
			}

			// could not build due to lack of suitable buildpos
			++ai->Getbt()->units_static[building].efficiency[4];

			if(ai->Getbt()->units_static[building].efficiency[4] > 1)
				return true;
			else
				return false;
		}
	}
	else
	{
		// keep factory at highest urgency if the construction failed due to (temporarily) unavailable builder
		if(factory_types_requested > 0)
			return false;
	}

	return true;
}

/*
void AAIExecute::BuildUnit(UnitCategory category, float speed, float cost, float range, float power, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, float eff, bool urgent)
{

}
*/

bool AAIExecute::BuildRadar()
{
	if(ai->Getut()->activeUnits[STATIONARY_RECON] + ai->Getut()->futureUnits[STATIONARY_RECON] + ai->Getut()->requestedUnits[STATIONARY_RECON] > ai->Getbrain()->sectors[0].size())
		return true;

	int ground_radar = 0;
	int sea_radar = 0;
	float3 my_pos, best_pos;
	float my_rating, best_rating = -1000000;
	int radar = 0;

	float cost = ai->Getbrain()->Affordable();
	float range = 10.0 / (cost + 1);

	// get ground radar
	if(ai->Getmap()->land_ratio > 0.02f)
	{
		ground_radar = ai->Getbt()->GetRadar(ai->Getside(), cost, range, false, false);

		if(ground_radar && ai->Getbt()->units_dynamic[ground_radar].constructorsAvailable <= 0)
		{
			if(ai->Getbt()->units_dynamic[ground_radar].constructorsRequested <= 0)
				ai->Getbt()->BuildBuilderFor(ground_radar);

			ground_radar = ai->Getbt()->GetRadar(ai->Getside(), cost, range, false, true);
		}
	}

	// get sea radar
	if(ai->Getmap()->water_ratio > 0.02f)
	{
		sea_radar = ai->Getbt()->GetRadar(ai->Getside(), cost, range, false, false);

		if(sea_radar && ai->Getbt()->units_dynamic[sea_radar].constructorsAvailable <= 0)
		{
			if(ai->Getbt()->units_dynamic[sea_radar].constructorsRequested <= 0)
				ai->Getbt()->BuildBuilderFor(sea_radar);

			sea_radar = ai->Getbt()->GetRadar(ai->Getside(), cost, range, false, true);
		}
	}

	for(int dist = 0; dist < 2; ++dist)
	{
		for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[dist].begin(); sector != ai->Getbrain()->sectors[dist].end(); ++sector)
		{
			if((*sector)->my_buildings[STATIONARY_RECON] <= 0)
			{
				my_pos = ZeroVector;

				if(ground_radar && (*sector)->water_ratio < 0.9f)
					my_pos = (*sector)->GetRadarArtyBuildsite(ground_radar, ai->Getbt()->units_static[ground_radar].range, false);

				if(my_pos.x <= 0 && sea_radar && (*sector)->water_ratio > 0.1f)
				{
					my_pos = (*sector)->GetRadarArtyBuildsite(sea_radar, ai->Getbt()->units_static[sea_radar].range, true);

					if(my_pos.x > 0)
						ground_radar = sea_radar;
				}

				if(my_pos.x > 0)
				{
					my_rating = - ai->Getmap()->GetEdgeDistance(&my_pos);

					if(my_rating > best_rating)
					{
						radar = ground_radar;
						best_pos = my_pos;
						best_rating = my_rating;
					}
				}
			}
		}
	}

	if(radar)
	{
		float min_dist;
		AAIConstructor *builder = ai->Getut()->FindClosestBuilder(radar, &best_pos, true, &min_dist);

		if(builder)
		{
			builder->GiveConstructionOrder(radar, best_pos, false);
			return true;
		}
		else
		{
			ai->Getbt()->BuildBuilderFor(radar);
			return false;
		}
	}

	return true;
}

bool AAIExecute::BuildJammer()
{
	if(ai->Getut()->futureUnits[STATIONARY_JAMMER] + ai->Getut()->requestedUnits[STATIONARY_JAMMER] > 0)
		return true;

	float3 pos = ZeroVector;

	float cost = ai->Getbrain()->Affordable();
	float range = 10.0 / (cost + 1);

	int ground_jammer = 0;
	int sea_jammer = 0;

	// get ground jammer
	if(ai->Getmap()->land_ratio > 0.02f)
	{
		ground_jammer = ai->Getbt()->GetJammer(ai->Getside(), cost, range, false, false);

		if(ground_jammer && ai->Getbt()->units_dynamic[ground_jammer].constructorsAvailable <= 0)
		{
			if(ai->Getbt()->units_dynamic[ground_jammer].constructorsRequested <= 0)
				ai->Getbt()->BuildBuilderFor(ground_jammer);

			ground_jammer = ai->Getbt()->GetJammer(ai->Getside(), cost, range, false, true);
		}
	}

	// get sea jammer
	if(ai->Getmap()->water_ratio > 0.02f)
	{
		sea_jammer = ai->Getbt()->GetJammer(ai->Getside(), cost, range, false, false);

		if(sea_jammer && ai->Getbt()->units_dynamic[sea_jammer].constructorsAvailable <= 0)
		{
			if(ai->Getbt()->units_dynamic[sea_jammer].constructorsRequested <= 0)
				ai->Getbt()->BuildBuilderFor(sea_jammer);

			sea_jammer = ai->Getbt()->GetJammer(ai->Getside(), cost, range, false, true);
		}
	}

	for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
	{
		if((*sector)->my_buildings[STATIONARY_JAMMER] <= 0)
		{
			if(ground_jammer && (*sector)->water_ratio < 0.9f)
				pos = (*sector)->GetCenterBuildsite(ground_jammer, false);

			if(pos.x == 0 && sea_jammer && (*sector)->water_ratio > 0.1f)
			{
				pos = (*sector)->GetCenterBuildsite(sea_jammer, true);

				if(pos.x > 0)
					ground_jammer = sea_jammer;
			}

			if(pos.x > 0)
			{
				float min_dist;
				AAIConstructor *builder = ai->Getut()->FindClosestBuilder(ground_jammer, &pos, true, &min_dist);

				if(builder)
				{
					builder->GiveConstructionOrder(ground_jammer, pos, false);
					return true;
				}
				else
				{
					ai->Getbt()->BuildBuilderFor(ground_jammer);
					return false;
				}
			}
		}
	}

	return true;
}

void AAIExecute::DefendMex(int mex, int def_id)
{
	if(ai->Getut()->activeFactories < cfg->MIN_FACTORIES_FOR_DEFENCES)
		return;

	float3 pos = ai->Getcb()->GetUnitPos(mex);
	float3 base_pos = ai->Getbrain()->base_center;

	// check if mex is located in a small pond/on a little island
	if(ai->Getmap()->LocatedOnSmallContinent(&pos))
		return;

	int x = pos.x/ai->Getmap()->xSectorSize;
	int y = pos.z/ai->Getmap()->ySectorSize;

	if(x >= 0 && y >= 0 && x < ai->Getmap()->xSectors && y < ai->Getmap()->ySectors)
	{
		AAISector *sector = &ai->Getmap()->sector[x][y];

		if(sector->distance_to_base > 0 && sector->distance_to_base <= cfg->MAX_MEX_DEFENCE_DISTANCE && sector->my_buildings[STATIONARY_DEF] < 1)
		{
			int defence = 0;
			bool water;

			// get defence building dependend on water or land mex
			if(ai->Getbt()->GetUnitDef(def_id).minWaterDepth > 0)
			{
				water = true;

				if(cfg->AIR_ONLY_MOD)
					defence = ai->Getbt()->GetCheapDefenceBuilding(ai->Getside(), 1, 2, 1, 1, 1, 0.5, 0, 0, 0, true);
				else
					defence = ai->Getbt()->GetCheapDefenceBuilding(ai->Getside(), 1, 2, 1, 1, 0, 0, 0.5, 1.5, 0.5, true);
			}
			else
			{
				if(cfg->AIR_ONLY_MOD)
					defence = ai->Getbt()->GetCheapDefenceBuilding(ai->Getside(), 1, 2, 1, 1, 1, 0.5, 0, 0, 0, false);
				else
					defence = ai->Getbt()->GetCheapDefenceBuilding(ai->Getside(), 1, 2, 1, 1, 1.5, 0, 0.5, 0, 0, false);

				water = false;
			}

			// find closest builder
			if(defence)
			{
				// place defences according to the direction of the main base
				if(pos.x > base_pos.x + 500)
					pos.x += 120;
				else if(pos.x > base_pos.x + 300)
					pos.x += 70;
				else if(pos.x < base_pos.x - 500)
					pos.x -= 120;
				else if(pos.x < base_pos.x - 300)
					pos.x -= 70;

				if(pos.z > base_pos.z + 500)
					pos.z += 70;
				else if(pos.z > base_pos.z + 300)
					pos.z += 120;
				else if(pos.z < base_pos.z - 500)
					pos.z -= 120;
				else if(pos.z < base_pos.z - 300)
					pos.z -= 70;

				// get suitable pos
				pos = ai->Getcb()->ClosestBuildSite(&ai->Getbt()->GetUnitDef(defence), pos, 1400.0, 2);

				if(pos.x > 0)
				{
					AAIConstructor *builder;
					float min_dist;

					if(ai->Getbrain()->sectors[0].size() > 2)
						builder = ai->Getut()->FindClosestBuilder(defence, &pos, false, &min_dist);
					else
						builder = ai->Getut()->FindClosestBuilder(defence, &pos, true, &min_dist);

					if(builder)
						builder->GiveConstructionOrder(defence, pos, water);
				}
			}
		}
	}
}

void AAIExecute::UpdateRessources()
{
	// get current metal/energy surplus
	metalSurplus[counter] = ai->Getcb()->GetMetalIncome() - ai->Getcb()->GetMetalUsage();
	if(metalSurplus[counter] < 0) metalSurplus[counter] = 0;

	energySurplus[counter] = ai->Getcb()->GetEnergyIncome() - ai->Getcb()->GetEnergyUsage();
	if(energySurplus[counter] < 0) energySurplus[counter] = 0;

	// calculate average value
	averageMetalSurplus = 0;
	averageEnergySurplus = 0;

	for(int i = 0; i < 8; i++)
	{
		averageMetalSurplus += metalSurplus[i];
		averageEnergySurplus += energySurplus[i];
	}

	averageEnergySurplus /= 8.0f;
	averageMetalSurplus /= 8.0f;

	// increase counter
	counter = (counter + 1) % 8;
}

void AAIExecute::CheckStationaryArty()
{
	if(cfg->MAX_STAT_ARTY == 0)
		return;

	if(ai->Getut()->futureUnits[STATIONARY_ARTY] +  ai->Getut()->requestedUnits[STATIONARY_ARTY]> 0)
		return;

	if(ai->Getut()->activeUnits[STATIONARY_ARTY] >= cfg->MAX_STAT_ARTY)
		return;

	float temp = 0.05f;

	if(temp > urgency[STATIONARY_ARTY])
		urgency[STATIONARY_ARTY] = temp;
}

void AAIExecute::CheckBuildqueues()
{
	int req_units = 0;
	int active_factory_types = 0;

	for(int i = 0; i < numOfFactories; ++i)
	{
		// sum up builque lengths of active factory types
		if(ai->Getbt()->units_dynamic[factory_table[i]].active > 0)
		{
			req_units += (int) buildques[i].size();
			++active_factory_types;
		}
	}

	if(active_factory_types > 0)
	{
		if( (float)req_units / (float)active_factory_types < (float)cfg->MAX_BUILDQUE_SIZE / 2.5f )
		{
			if(unitProductionRate < 70)
				++unitProductionRate;

			//ai->Log("Increasing unit production rate to %i\n", unitProductionRate);
		}
		else if( (float)req_units / (float)active_factory_types > (float)cfg->MAX_BUILDQUE_SIZE / 1.5f )
		{
			if(unitProductionRate > 1)
			{
				--unitProductionRate;
				//ai->Log("Decreasing unit production rate to %i\n", unitProductionRate);
			}
		}
	}
}

void AAIExecute::CheckDefences()
{
	if(ai->Getut()->activeFactories < cfg->MIN_FACTORIES_FOR_DEFENCES || ai->Getut()->futureUnits[STATIONARY_DEF] +  ai->Getut()->requestedUnits[STATIONARY_DEF] > 2)
		return;

	int game_period = ai->Getbrain()->GetGamePeriod();

	int max_dist = 2;

	float rating, highest_rating = 0;

	AAISector *first = 0, *second = 0;
	UnitCategory cat1 = UNKNOWN, cat2 = UNKNOWN;

	for(int dist = 0; dist <= max_dist; ++dist)
	{
		for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[dist].begin(); sector != ai->Getbrain()->sectors[dist].end(); ++sector)
		{
			// stop building further defences if maximum has been reached / sector contains allied buildings / is occupied by another aai instance
			if((*sector)->my_buildings[STATIONARY_DEF] < cfg->MAX_DEFENCES && (*sector)->allied_structures < 4 && ai->Getmap()->team_sector_map[(*sector)->x][(*sector)->y] != ai->Getcb()->GetMyAllyTeam())
			{
				if((*sector)->failed_defences > 1)
					(*sector)->failed_defences = 0;
				else
				{
					for(list<int>::iterator cat = ai->Getmap()->map_categories_id.begin(); cat!= ai->Getmap()->map_categories_id.end(); ++cat)
					{
						// anti air defences may be built anywhere
						if(cfg->AIR_ONLY_MOD || *cat == AIR_ASSAULT)
						{
							//rating = (*sector)->own_structures * (0.25 + ai->Getbrain()->GetAttacksBy(*cat, game_period)) * (0.25 + (*sector)->GetThreatByID(*cat, learned, current)) / ( 0.1 + (*sector)->GetMyDefencePowerAgainstAssaultCategory(*cat));
							// how often did units of category attack that sector compared to current def power
							rating = (1.0f + (*sector)->GetThreatByID(*cat, learned, current)) / ( 1.0f + (*sector)->GetMyDefencePowerAgainstAssaultCategory(*cat));

							// how often did unist of that category attack anywere in the current period of the game
							rating *= (0.1f + ai->Getbrain()->GetAttacksBy(*cat, game_period));
						}
						//else if(!(*sector)->interior)
						else if((*sector)->distance_to_base > 0) // dont build anti ground/hover/sea defences in interior sectors
						{
							// how often did units of category attack that sector compared to current def power
							rating = (1.0f + (*sector)->GetThreatByID(*cat, learned, current)) / ( 1.0f + (*sector)->GetMyDefencePowerAgainstAssaultCategory(*cat));

							// how often did units of that category attack anywere in the current period of the game
							rating *= (0.1f + ai->Getbrain()->GetAttacksBy(*cat, game_period));
						}
						else
							rating = 0;

						if(rating > highest_rating)
						{
							// dont block empty sectors with too much aa
							if(ai->Getbt()->GetAssaultCategoryOfID(*cat) != AIR_ASSAULT || ((*sector)->my_buildings[POWER_PLANT] > 0 || (*sector)->my_buildings[STATIONARY_CONSTRUCTOR] > 0 ) )
							{
								second = first;
								cat2 = cat1;

								first = *sector;
								cat1 = ai->Getbt()->GetAssaultCategoryOfID(*cat);

								highest_rating = rating;
							}
						}
					}
				}
			}
		}
	}

	if(first)
	{
		// if no builder available retry later
		BuildOrderStatus status = BuildStationaryDefenceVS(cat1, first);

		if(status == BUILDORDER_NOBUILDER)
		{
			float temp = 0.03f + 1.0f / ( (float) first->my_buildings[STATIONARY_DEF] + 0.5f);

			if(urgency[STATIONARY_DEF] < temp)
				urgency[STATIONARY_DEF] = temp;

			next_defence = first;
			def_category = cat1;
		}
		else if(status == BUILDORDER_NOBUILDPOS)
			++first->failed_defences;
	}

	if(second)
		BuildStationaryDefenceVS(cat2, second);
}

void AAIExecute::CheckRessources()
{
	// prevent float rounding errors
	if(futureAvailableEnergy < 0)
		futureAvailableEnergy = 0;

	// determine how much metal/energy is needed based on net surplus
	float temp = GetMetalUrgency();

	if(urgency[EXTRACTOR] < temp) // && urgency[EXTRACTOR] > 0.05)
		urgency[EXTRACTOR] = temp;

	temp = GetEnergyUrgency();
	if(urgency[POWER_PLANT] < temp) // && urgency[POWER_PLANT] > 0.05)
		urgency[POWER_PLANT] = temp;

	// build storages if needed
	if(ai->Getut()->activeUnits[STORAGE] + ai->Getut()->requestedUnits[STORAGE] + ai->Getut()->futureUnits[STORAGE] < cfg->MAX_STORAGE
		&& ai->Getut()->activeFactories >= cfg->MIN_FACTORIES_FOR_STORAGE)
	{
		float temp = max(GetMetalStorageUrgency(), GetEnergyStorageUrgency());

		if(temp > urgency[STORAGE])
			urgency[STORAGE] = temp;
	}

	// energy low
	if(averageEnergySurplus < 1.5 * cfg->METAL_ENERGY_RATIO)
	{
		// try to accelerate power plant construction
		if(ai->Getut()->futureUnits[POWER_PLANT] +  ai->Getut()->requestedUnits[POWER_PLANT]> 0)
			AssistConstructionOfCategory(POWER_PLANT, 10);

		// try to disbale some metal makers
		if((ai->Getut()->activeUnits[METAL_MAKER] - disabledMMakers) > 0)
		{
			for(set<int>::iterator maker = ai->Getut()->metal_makers.begin(); maker != ai->Getut()->metal_makers.end(); ++maker)
			{
				if(ai->Getcb()->IsUnitActivated(*maker))
				{
					Command c;
					c.id = CMD_ONOFF;
					c.params.push_back(0);
					//ai->Getcb()->GiveOrder(*maker, &c);
					GiveOrder(&c, *maker, "ToggleMMaker");

					futureRequestedEnergy += ai->Getcb()->GetUnitDef(*maker)->energyUpkeep;
					++disabledMMakers;
					break;
				}
			}
		}
	}
	// try to enable some metal makers
	else if(averageEnergySurplus > cfg->MIN_METAL_MAKER_ENERGY && disabledMMakers > 0)
	{
		for(set<int>::iterator maker = ai->Getut()->metal_makers.begin(); maker != ai->Getut()->metal_makers.end(); ++maker)
		{
			if(!ai->Getcb()->IsUnitActivated(*maker))
			{
				float usage = ai->Getcb()->GetUnitDef(*maker)->energyUpkeep;

				if(averageEnergySurplus > usage * 0.7f)
				{
					Command c;
					c.id = CMD_ONOFF;
					c.params.push_back(1);
					//ai->Getcb()->GiveOrder(*maker, &c);
					GiveOrder(&c, *maker, "ToggleMMaker");

					futureRequestedEnergy -= usage;
					--disabledMMakers;
					break;
				}
			}
		}
	}

	// metal low
	if(averageMetalSurplus < 15.0/cfg->METAL_ENERGY_RATIO)
	{
		// try to accelerate mex construction
		if(ai->Getut()->futureUnits[EXTRACTOR] > 0)
			AssistConstructionOfCategory(EXTRACTOR, 10);

		// try to accelerate mex construction
		if(ai->Getut()->futureUnits[METAL_MAKER] > 0 && averageEnergySurplus > cfg->MIN_METAL_MAKER_ENERGY)
			AssistConstructionOfCategory(METAL_MAKER, 10);
	}
}

void AAIExecute::CheckMexUpgrade()
{
	if(ai->Getbrain()->freeBaseSpots)
		return;

	float cost = 0.25f + ai->Getbrain()->Affordable() / 8.0f;
	float eff = 6.0f / (cost + 0.75f);

	const UnitDef *my_def;
	const UnitDef *land_def = 0;
	const UnitDef *water_def = 0;

	float gain, highest_gain = 0;
	AAIMetalSpot *best_spot = 0;

	int my_team = ai->Getcb()->GetMyTeam();

	int land_mex = ai->Getbt()->GetMex(ai->Getside(), cost, eff, false, false, false);

	if(land_mex && ai->Getbt()->units_dynamic[land_mex].constructorsAvailable + ai->Getbt()->units_dynamic[land_mex].constructorsRequested <= 0)
	{
		ai->Getbt()->BuildBuilderFor(land_mex);

		land_mex = ai->Getbt()->GetMex(ai->Getside(), cost, eff, false, false, true);
	}

	int water_mex = ai->Getbt()->GetMex(ai->Getside(), cost, eff, false, true, false);

	if(water_mex && ai->Getbt()->units_dynamic[water_mex].constructorsAvailable + ai->Getbt()->units_dynamic[water_mex].constructorsRequested  <= 0)
	{
		ai->Getbt()->BuildBuilderFor(water_mex);

		water_mex = ai->Getbt()->GetMex(ai->Getside(), cost, eff, false, true, true);
	}

	if(land_mex)
		land_def = &ai->Getbt()->GetUnitDef(land_mex);

	if(water_mex)
		water_def = &ai->Getbt()->GetUnitDef(water_mex);

	// check extractor upgrades
	for(int dist = 0; dist < 2; ++dist)
	{
		for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[dist].begin(); sector != ai->Getbrain()->sectors[dist].end(); ++sector)
		{
			for(list<AAIMetalSpot*>::iterator spot = (*sector)->metalSpots.begin(); spot != (*sector)->metalSpots.end(); ++spot)
			{
				// quit when finding empty spots
				if(!(*spot)->occupied && (*sector)->enemy_structures <= 0  && (*sector)->lost_units[MOBILE_CONSTRUCTOR-COMMANDER] < 0.2)
					return;

				if((*spot)->extractor_def > 0 && (*spot)->extractor > -1 && (*spot)->extractor < cfg->MAX_UNITS
					&& ai->Getcb()->GetUnitTeam((*spot)->extractor) == my_team)	// only upgrade own extractors
				{
					my_def = &ai->Getbt()->GetUnitDef((*spot)->extractor_def);

					if(my_def->minWaterDepth <= 0 && land_def)	// land mex
					{
						gain = land_def->extractsMetal - my_def->extractsMetal;

						if(gain > 0.0001f && gain > highest_gain)
						{
							highest_gain = gain;
							best_spot = *spot;
						}
					}
					else	// water mex
					{
						gain = water_def->extractsMetal - my_def->extractsMetal;

						if(gain > 0.0001f && gain > highest_gain)
						{
							highest_gain = gain;
							best_spot = *spot;
						}
					}
				}
			}
		}
	}

	if(best_spot)
	{
		AAIConstructor *builder = ai->Getut()->FindClosestAssistant(best_spot->pos, 10, true);

		if(builder)
			builder->GiveReclaimOrder(best_spot->extractor);
	}
}


void AAIExecute::CheckRadarUpgrade()
{
	if(ai->Getut()->futureUnits[STATIONARY_RECON] + ai->Getut()->requestedUnits[STATIONARY_RECON]  > 0)
		return;

	float cost = ai->Getbrain()->Affordable();
	float range = 10.0f / (cost + 1.0f);

	const UnitDef *my_def;
	const UnitDef *land_def = 0;
	const UnitDef *water_def = 0;

	int land_radar = ai->Getbt()->GetRadar(ai->Getside(), cost, range, false, true);
	int water_radar = ai->Getbt()->GetRadar(ai->Getside(), cost, range, true, true);

	if(land_radar)
		land_def = &ai->Getbt()->GetUnitDef(land_radar);

	if(water_radar)
		water_def = &ai->Getbt()->GetUnitDef(water_radar);

	// check radar upgrades
	for(set<int>::iterator recon = ai->Getut()->recon.begin(); recon != ai->Getut()->recon.end(); ++recon)
	{
		my_def = ai->Getcb()->GetUnitDef(*recon);

		if(my_def)
		{
			if(my_def->minWaterDepth <= 0)	// land recon
			{
				if(land_def && my_def->radarRadius < land_def->radarRadius)
				{
					// better radar found, clear buildpos
					AAIConstructor *builder = ai->Getut()->FindClosestAssistant(ai->Getcb()->GetUnitPos(*recon), 10, true);

					if(builder)
					{
						builder->GiveReclaimOrder(*recon);
						return;
					}
				}
			}
			else	// water radar
			{
				if(water_def && my_def->radarRadius < water_def->radarRadius)
				{
					// better radar found, clear buildpos
					AAIConstructor *builder = ai->Getut()->FindClosestAssistant(ai->Getcb()->GetUnitPos(*recon), 10, true );

					if(builder)
					{
						builder->GiveReclaimOrder(*recon);
						return;
					}
				}
			}
		}
	}
}

void AAIExecute::CheckJammerUpgrade()
{
	if(ai->Getut()->futureUnits[STATIONARY_JAMMER] + ai->Getut()->requestedUnits[STATIONARY_JAMMER]  > 0)
		return;

	float cost = ai->Getbrain()->Affordable();
	float range = 10.0 / (cost + 1);

	const UnitDef *my_def;
	const UnitDef *land_def = 0;
	const UnitDef *water_def = 0;

	int land_jammer = ai->Getbt()->GetJammer(ai->Getside(), cost, range, false, true);
	int water_jammer = ai->Getbt()->GetJammer(ai->Getside(), cost, range, true, true);

	if(land_jammer)
		land_def = &ai->Getbt()->GetUnitDef(land_jammer);

	if(water_jammer)
		water_def = &ai->Getbt()->GetUnitDef(water_jammer);

	// check radar upgrades
	for(set<int>::iterator jammer = ai->Getut()->jammers.begin(); jammer != ai->Getut()->jammers.end(); ++jammer)
	{
		my_def = ai->Getcb()->GetUnitDef(*jammer);

		if(my_def)
		{
			if(my_def->minWaterDepth <= 0)	// land jammer
			{
				if(land_def && my_def->jammerRadius < land_def->jammerRadius)
				{
					// better jammer found, clear buildpos
					AAIConstructor *builder = ai->Getut()->FindClosestAssistant(ai->Getcb()->GetUnitPos(*jammer), 10, true);

					if(builder)
					{
						builder->GiveReclaimOrder(*jammer);
						return;
					}
				}
			}
			else	// water jammer
			{
				if(water_def && my_def->jammerRadius < water_def->jammerRadius)
				{
					// better radar found, clear buildpos
					AAIConstructor *builder = ai->Getut()->FindClosestAssistant(ai->Getcb()->GetUnitPos(*jammer), 10, true);

					if(builder)
					{
						builder->GiveReclaimOrder(*jammer);
						return;
					}
				}
			}
		}
	}
}

float AAIExecute::GetEnergyUrgency()
{
	float surplus = averageEnergySurplus + futureAvailableEnergy * 0.5f;

	if(surplus < 0)
		surplus = 0;

	if(ai->Getut()->activeUnits[POWER_PLANT] > 8)
	{
		if(averageEnergySurplus > 1000)
			return 0;
		else
			return 8.0f / pow( surplus / cfg->METAL_ENERGY_RATIO + 2.0f, 2.0f);
	}
	else if(ai->Getut()->activeUnits[POWER_PLANT] > 0)
		return 15.0f / pow( surplus / cfg->METAL_ENERGY_RATIO + 2.0f, 2.0f);
	else
		return 6.0f;
}

float AAIExecute::GetMetalUrgency()
{
	if(ai->Getut()->activeUnits[EXTRACTOR] > 0)
		return 20.0f / pow(averageMetalSurplus * cfg->METAL_ENERGY_RATIO + 2.0f, 2.0f);
	else
		return 7.0f;
}

float AAIExecute::GetEnergyStorageUrgency()
{
	if(averageEnergySurplus / cfg->METAL_ENERGY_RATIO > 4.0f)
		return 0.2f;
	else
		return 0;
}

float AAIExecute::GetMetalStorageUrgency()
{
	if(averageMetalSurplus > 2.0f && (ai->Getcb()->GetMetalStorage() + futureStoredMetal - ai->Getcb()->GetMetal()) < 100.0f)
		return 0.3f;
	else
		return 0;
}

void AAIExecute::CheckFactories()
{
	if(ai->Getut()->futureUnits[STATIONARY_CONSTRUCTOR] + ai->Getut()->requestedUnits[STATIONARY_CONSTRUCTOR] > 0)
		return;

	for(list<int>::iterator fac = ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].begin(); fac != ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].end(); ++fac)
	{
		if(ai->Getbt()->units_dynamic[*fac].requested > 0)
		{
			// at least one requested factory has not been built yet
			float urgency;

			if(ai->Getut()->activeFactories > 0)
				urgency = 0.4f;
			else
				urgency = 3.5f;

			if(this->urgency[STATIONARY_CONSTRUCTOR] < urgency)
				this->urgency[STATIONARY_CONSTRUCTOR] = urgency;

			return;
		}
	}
}

void AAIExecute::CheckRecon()
{
	float urgency = 0.02f + 0.5f / ((float)(2 * ai->Getut()->activeUnits[STATIONARY_RECON] + 1));

	if(this->urgency[STATIONARY_RECON] < urgency)
		this->urgency[STATIONARY_RECON] = urgency;
}

void AAIExecute::CheckAirBase()
{
	// only build repair pad if any air units have been built yet
	if(ai->Getut()->activeUnits[AIR_BASE] +  ai->Getut()->requestedUnits[AIR_BASE] + ai->Getut()->futureUnits[AIR_BASE] < cfg->MAX_AIR_BASE && ai->Getgroup_list()[AIR_ASSAULT].size() > 0)
			urgency[AIR_BASE] = 0.5f;
}

void AAIExecute::CheckJammer()
{
	if(ai->Getut()->activeFactories < 2 || ai->Getut()->activeUnits[STATIONARY_JAMMER] > ai->Getbrain()->sectors[0].size())
	{
		this->urgency[STATIONARY_JAMMER] = 0;
	}
	else
	{
		float temp = 0.2f / ((float) (ai->Getut()->activeUnits[STATIONARY_JAMMER]+1)) + 0.05f;

		if(urgency[STATIONARY_JAMMER] < temp)
			urgency[STATIONARY_JAMMER] = temp;
	}
}

void AAIExecute::CheckConstruction()
{
	UnitCategory category = UNKNOWN;
	float highest_urgency = 0.5f;		// min urgency (prevents aai from building things it doesnt really need that much)
	bool construction_started = false;

	// get category with highest urgency
	if(ai->Getbrain()->enemy_pressure_estimation > 0.01f)
	{
//		double current_urgency;

		for(int i = 1; i <= METAL_MAKER; ++i)
		{
/*
			current_urgency = urgency[i];

			if(i != STATIONARY_DEF && i != POWER_PLANT && i != EXTRACTOR && i != STATIONARY_CONSTRUCTOR)
				current_urgency *= (1.1f - ai->Getbrain()->enemy_pressure_estimation);
*/
			if(urgency[i] > highest_urgency)
			{
				highest_urgency = urgency[i];
				category = (UnitCategory)i;
			}
		}
	}
	else
	{
		for(int i = 1; i <= METAL_MAKER; ++i)
		{
			if(urgency[i] > highest_urgency)
			{
				highest_urgency = urgency[i];
				category = (UnitCategory)i;
			}
		}
	}

	if(category == POWER_PLANT)
	{
		if(BuildPowerPlant())
			construction_started = true;
	}
	else if(category == EXTRACTOR)
	{
		if(BuildExtractor())
			construction_started = true;
	}
	else if(category == STATIONARY_CONSTRUCTOR)
	{
		if(BuildFactory())
			construction_started = true;
	}
	else if(category == STATIONARY_DEF)
	{
		if(BuildDefences())
			construction_started = true;
	}
	else if(category == STATIONARY_RECON)
	{
		if(BuildRadar())
			construction_started = true;
	}
	else if(category == STATIONARY_JAMMER)
	{
		if(BuildJammer())
			construction_started = true;
	}
	else if(category == STATIONARY_ARTY)
	{
		if(BuildArty())
			construction_started = true;
	}
	else if(category == STORAGE)
	{
		if(BuildStorage())
			construction_started = true;
	}
	else if(category == METAL_MAKER)
	{
		if(BuildMetalMaker())
			construction_started = true;
	}
	else if(category == AIR_BASE)
	{
		if(BuildAirBase())
			construction_started = true;
	}

	/*if(construction_started)
	{
		ai->Log("\n");

		for(int i = 1; i < METAL_MAKER; ++i)
			ai->Log("%s: %f\n", ai->Getbt()->GetCategoryString2((UnitCategory)i), urgency[i]);

		ai->Log("Selected category: %s\n", ai->Getbt()->GetCategoryString2(category));
	}*/

	if(construction_started)
		urgency[category] = 0;

	for(int i = 1; i <= METAL_MAKER; ++i)
	{
		urgency[i] *= 1.03f;

		if(urgency[i] > 20.0f)
			urgency[i] -= 1.0f;
	}
}

bool AAIExecute::AssistConstructionOfCategory(UnitCategory category, int /*importance*/)
{
	AAIConstructor *builder, *assistant;

	for(list<AAIBuildTask*>::iterator task = ai->Getbuild_tasks().begin(); task != ai->Getbuild_tasks().end(); ++task)
	{
		if((*task)->builder_id >= 0)
			builder = ai->Getut()->units[(*task)->builder_id].cons;
		else
			builder = NULL;

		if(builder && builder->construction_category == category && builder->assistants.size() < cfg->MAX_ASSISTANTS)
		{
			assistant = ai->Getut()->FindClosestAssistant(builder->build_pos, 5, true);

			if(assistant)
			{
				builder->assistants.insert(assistant->unit_id);
				assistant->AssistConstruction(builder->unit_id, (*task)->unit_id);
				return true;
			}
		}
	}

	return false;
}

float AAIExecute::sector_threat(AAISector *sec)
{
	float threat = sec->GetThreatBy(AIR_ASSAULT, learned, current);

	if(cfg->AIR_ONLY_MOD)
		return threat;

	threat += sec->GetThreatBy(HOVER_ASSAULT, learned, current);

	if(sec->Getai()->Getmap()->map_type == LAND_MAP || sec->Getai()->Getmap()->map_type == LAND_WATER_MAP)
		threat += sec->GetThreatBy(GROUND_ASSAULT, learned, current);
	if(sec->Getai()->Getmap()->map_type == WATER_MAP || sec->Getai()->Getmap()->map_type == LAND_WATER_MAP)
		threat += sec->GetThreatBy(SEA_ASSAULT, learned, current);
	return threat;
}

bool AAIExecute::least_dangerous(AAISector *left, AAISector *right)
{
	return sector_threat(left) < sector_threat(right);
}

bool AAIExecute::suitable_for_power_plant(AAISector *left, AAISector *right)
{
	return sector_threat(left) * (float)left->map_border_dist < sector_threat(right) * (float)right->map_border_dist;
}

bool AAIExecute::suitable_for_ground_factory(AAISector *left, AAISector *right)
{
	return ( (2.0f * left->flat_ratio + left->map_border_dist)
			> (2.0f * right->flat_ratio + right->map_border_dist) );
}

bool AAIExecute::suitable_for_sea_factory(AAISector *left, AAISector *right)
{
	return ( (2.0f * left->water_ratio + left->map_border_dist)
			> (2.0f * right->water_ratio + right->map_border_dist) );
}

bool AAIExecute::suitable_for_ground_rallypoint(AAISector *left, AAISector *right)
{
	return ( (left->flat_ratio  + 0.5f * left->map_border_dist)/ ((float) (left->rally_points + 1) )
		>  (right->flat_ratio  + 0.5f * right->map_border_dist)/ ((float) (right->rally_points + 1) ) );
}

bool AAIExecute::suitable_for_sea_rallypoint(AAISector *left, AAISector *right)
{
	return ( (left->water_ratio  + 0.5f * left->map_border_dist)/ ((float) (left->rally_points + 1) )
		>  (right->water_ratio  + 0.5f * right->map_border_dist)/ ((float) (right->rally_points + 1) ) );
}

bool AAIExecute::suitable_for_all_rallypoint(AAISector *left, AAISector *right)
{
	return ( (left->flat_ratio + left->water_ratio + 0.5f * left->map_border_dist)/ ((float) (left->rally_points + 1) )
		>  (right->flat_ratio + right->water_ratio + 0.5f * right->map_border_dist)/ ((float) (right->rally_points + 1) ) );
}

bool AAIExecute::defend_vs_ground(AAISector *left, AAISector *right)
{
	return ((2.0f + left->GetThreatBy(GROUND_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(0)+ 0.5f))
		>  ((2.0f + right->GetThreatBy(GROUND_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(0)+ 0.5f));
}

bool AAIExecute::defend_vs_air(AAISector *left, AAISector *right)
{
	return ((2.0f + left->GetThreatBy(AIR_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(1)+ 0.5f))
		>  ((2.0f + right->GetThreatBy(AIR_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(1)+ 0.5f));
}

bool AAIExecute::defend_vs_hover(AAISector *left, AAISector *right)
{
	return ((2.0f + left->GetThreatBy(HOVER_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(2)+ 0.5f))
		>  ((2.0f + right->GetThreatBy(HOVER_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(2)+ 0.5f));

}

bool AAIExecute::defend_vs_sea(AAISector *left, AAISector *right)
{
	return ((2.0f + left->GetThreatBy(SEA_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(3)+ 0.5f))
		>  ((2.0f + right->GetThreatBy(SEA_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(3)+ 0.5f));
}

bool AAIExecute::defend_vs_submarine(AAISector *left, AAISector *right)
{
	return ((2.0f + left->GetThreatBy(SUBMARINE_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(4)+ 0.5f))
		>  ((2.0f + right->GetThreatBy(SUBMARINE_ASSAULT, learned, current)) / (left->GetMyDefencePowerAgainstAssaultCategory(4)+ 0.5f));
}

void AAIExecute::ConstructionFinished()
{
}

void AAIExecute::ConstructionFailed(float3 build_pos, int def_id)
{
	const UnitDef *def = &ai->Getbt()->GetUnitDef(def_id);
	UnitCategory category = ai->Getbt()->units_static[def_id].category;

	int x = build_pos.x/ai->Getmap()->xSectorSize;
	int y = build_pos.z/ai->Getmap()->ySectorSize;

	bool validSector = false;

	if(x >= 0 && y >= 0 && x < ai->Getmap()->xSectors && y < ai->Getmap()->ySectors)
		validSector = true;

	// decrease number of units of that category in the target sector
	if(validSector)
		ai->Getmap()->sector[x][y].RemoveBuildingType(def_id);

	// free metalspot if mex was odered to be built
	if(category == EXTRACTOR && build_pos.x > 0)
	{
		ai->Getmap()->sector[x][y].FreeMetalSpot(build_pos, def);
	}
	else if(category == POWER_PLANT)
	{
		futureAvailableEnergy -= ai->Getbt()->units_static[def_id].efficiency[0];

		if(futureAvailableEnergy < 0)
			futureAvailableEnergy = 0;
	}
	else if(category == STORAGE)
	{
		futureStoredEnergy -= ai->Getbt()->GetUnitDef(def->id).energyStorage;
		futureStoredMetal -= ai->Getbt()->GetUnitDef(def->id).metalStorage;
	}
	else if(category == METAL_MAKER)
	{
		futureRequestedEnergy -= ai->Getbt()->GetUnitDef(def->id).energyUpkeep;

		if(futureRequestedEnergy < 0)
			futureRequestedEnergy = 0;
	}
	else if(category == STATIONARY_JAMMER)
	{
		futureRequestedEnergy -= ai->Getbt()->units_static[def->id].efficiency[0];

		if(futureRequestedEnergy < 0)
			futureRequestedEnergy = 0;
	}
	else if(category == STATIONARY_RECON)
	{
		futureRequestedEnergy -= ai->Getbt()->units_static[def->id].efficiency[0];

		if(futureRequestedEnergy < 0)
			futureRequestedEnergy = 0;
	}
	else if(category == STATIONARY_DEF)
	{
		ai->Getmap()->RemoveDefence(&build_pos, def_id);
	}

	// clear buildmap
	if(category == STATIONARY_CONSTRUCTOR)
	{
		ai->Getut()->futureFactories -= 1;

		for(list<int>::iterator unit = ai->Getbt()->units_static[def_id].canBuildList.begin();  unit != ai->Getbt()->units_static[def_id].canBuildList.end(); ++unit)
			ai->Getbt()->units_dynamic[*unit].constructorsRequested -= 1;

		// remove future ressource demand since factory is no longer being built
		futureRequestedMetal -= ai->Getbt()->units_static[def->id].efficiency[0];
		futureRequestedEnergy -= ai->Getbt()->units_static[def->id].efficiency[1];

		if(futureRequestedEnergy < 0)
			futureRequestedEnergy = 0;

		if(futureRequestedMetal < 0)
			futureRequestedMetal = 0;

		// update buildmap of sector
		ai->Getmap()->UpdateBuildMap(build_pos, def, false, ai->Getbt()->CanPlacedWater(def_id), true);
	}
	else // normal building
	{
		// update buildmap of sector
		ai->Getmap()->UpdateBuildMap(build_pos, def, false, ai->Getbt()->CanPlacedWater(def_id), false);
	}
}

void AAIExecute::AddStartFactory()
{
	float best_rating = 0, my_rating;
	int best_factory = 0;

	for(list<int>::iterator fac = ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].begin(); fac != ai->Getbt()->units_of_category[STATIONARY_CONSTRUCTOR][ai->Getside()-1].end(); ++fac)
	{
		if(ai->Getbt()->units_dynamic[*fac].constructorsAvailable > 0)
		{
			my_rating = ai->Getbt()->GetFactoryRating(*fac);
			my_rating *= (2.0 - (ai->Getbt()->units_static[*fac].cost / ai->Getbt()->max_cost[STATIONARY_CONSTRUCTOR][ai->Getside()-1]));

			if(my_rating > best_rating)
			{
				best_rating = my_rating;
				best_factory = *fac;
			}
		}
	}

	if(best_factory)
	{
		ai->Getbt()->units_dynamic[best_factory].requested += 1;
		urgency[STATIONARY_CONSTRUCTOR] = 3.0f;

		ai->Log("%s requested\n", ai->Getbt()->GetUnitDef(best_factory).humanName.c_str());

		for(list<int>::iterator j = ai->Getbt()->units_static[best_factory].canBuildList.begin(); j != ai->Getbt()->units_static[best_factory].canBuildList.end(); ++j)
			ai->Getbt()->units_dynamic[*j].constructorsRequested += 1;
	}
}

AAIGroup* AAIExecute::GetClosestGroupForDefence(UnitType group_type, float3 *pos, int continent, int /*importance*/)
{
	AAIGroup *best_group = 0;
	float best_rating = 0, my_rating;
	float3 group_pos;

	for(list<UnitCategory>::iterator category = ai->Getbt()->assault_categories.begin(); category != ai->Getbt()->assault_categories.end(); ++category)
	{
		for(list<AAIGroup*>::iterator group = ai->Getgroup_list()[*category].begin(); group != ai->Getgroup_list()[*category].end(); ++group)
		{
			if((*group)->group_unit_type == group_type && !(*group)->attack)
			{
				if((*group)->continent == -1 || (*group)->continent == continent)
				{
					if((*group)->task == GROUP_IDLE) // || (*group)->task_importance < importance)
					{
						group_pos = (*group)->GetGroupPos();

						my_rating = (*group)->avg_speed / ( 1.0 + fastmath::apxsqrt((pos->x - group_pos.x) * (pos->x - group_pos.x)  + (pos->z - group_pos.z) * (pos->z - group_pos.z) ));

						if(my_rating > best_rating)
						{
							best_group = *group;
							best_rating = my_rating;
						}
					}
				}
			}
		}
	}

	return best_group;
}

void AAIExecute::DefendUnitVS(int unit, unsigned int enemy_movement_type, float3 *enemy_pos, int importance)
{
	AAIGroup *support = 0;

	int continent = ai->Getmap()->GetContinentID(enemy_pos);

	UnitType group_type;

	// anti air needed
	if(enemy_movement_type & MOVE_TYPE_AIR && !cfg->AIR_ONLY_MOD)
		group_type = ANTI_AIR_UNIT;
	else
		group_type = ASSAULT_UNIT;

	support = GetClosestGroupForDefence(group_type, enemy_pos, continent, 100);

	if(support)
		support->Defend(unit, enemy_pos, importance);
}

float3 AAIExecute::GetSafePos(int def_id, float3 unit_pos)
{
	float3 best_pos = ZeroVector;
	float my_rating, best_rating = -10000.0f;

	if(ai->Getbt()->units_static[def_id].movement_type & MOVE_TYPE_CONTINENT_BOUND)
	{
		// get continent id of the unit pos
		float3 pos;
		int cont_id = ai->Getmap()->GetContinentID(&unit_pos);

		for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
		{
			pos = (*sector)->GetCenter();

			if(ai->Getmap()->GetContinentID(&pos) == cont_id)
			{
				my_rating = (*sector)->map_border_dist - (*sector)->GetEnemyThreatToMovementType(ai->Getbt()->units_static[def_id].movement_type);

				if(my_rating > best_rating)
				{
					best_rating = my_rating;
					best_pos = pos;
				}
			}
		}

	}
	else // non continent bound movement types (air, hover, amphibious)
	{
		for(list<AAISector*>::iterator sector = ai->Getbrain()->sectors[0].begin(); sector != ai->Getbrain()->sectors[0].end(); ++sector)
		{
			my_rating = (*sector)->map_border_dist - (*sector)->GetEnemyThreatToMovementType(ai->Getbt()->units_static[def_id].movement_type);

			if(my_rating > best_rating)
			{
				best_rating = my_rating;
				best_pos = (*sector)->GetCenter();
			}
		}
	}

	return best_pos;
}

void AAIExecute::ChooseDifferentStartingSector(int x, int y)
{
	// get possible start sectors
	list<AAISector*> sectors;

	if(x >= 1)
	{
		sectors.push_back( &ai->Getmap()->sector[x-1][y] );

		if(y >= 1)
			sectors.push_back( &ai->Getmap()->sector[x-1][y-1] );

		if(y < ai->Getmap()->ySectors-1)
			sectors.push_back( &ai->Getmap()->sector[x-1][y+1] );
	}

	if(x < ai->Getmap()->xSectors-1)
	{
		sectors.push_back( &ai->Getmap()->sector[x+1][y] );

		if(y >= 1)
			sectors.push_back( &ai->Getmap()->sector[x+1][y-1] );

		if(y < ai->Getmap()->ySectors-1)
			sectors.push_back( &ai->Getmap()->sector[x+1][y+1] );
	}

	if(y >= 1)
		sectors.push_back( &ai->Getmap()->sector[x][y-1] );

	if(y < ai->Getmap()->ySectors-1)
		sectors.push_back( &ai->Getmap()->sector[x][y+1] );

	// choose best
	AAISector *best_sector = 0;
	float my_rating, best_rating = 0;

	for(list<AAISector*>::iterator sector = sectors.begin(); sector != sectors.end(); ++sector)
	{
		if(ai->Getmap()->team_sector_map[(*sector)->x][(*sector)->y] != -1)
			my_rating = 0;
		else
			my_rating = ( (float)(2 * (*sector)->GetNumberOfMetalSpots() + 1) ) * (*sector)->flat_ratio * (*sector)->flat_ratio;

		if(my_rating > best_rating)
		{
			best_rating = my_rating;
			best_sector = *sector;
		}
	}

	// add best sector to base
	if(best_sector)
	{
		ai->Getbrain()->AddSector(best_sector);
		ai->Getbrain()->start_pos = best_sector->GetCenter();

		ai->Getbrain()->UpdateNeighbouringSectors();
		ai->Getbrain()->UpdateBaseCenter();
	}
}

void AAIExecute::CheckFallBack(int unit_id, int def_id)
{
	float max_weapon_range = ai->Getbt()->units_static[def_id].range;

	if(max_weapon_range > cfg->MIN_FALLBACK_RANGE && ai->Getbt()->GetUnitDef(def_id).turnRate >= cfg->MIN_FALLBACK_TURNRATE)
	{
		if(max_weapon_range > cfg->MAX_FALLBACK_RANGE)
			max_weapon_range = cfg->MAX_FALLBACK_RANGE;

		float3 pos;

		GetFallBackPos(&pos, unit_id, max_weapon_range);

		if(pos.x > 0)
		{
			Command c;
			c.id = CMD_MOVE;
			c.params.resize(3);

			c.params[0] = pos.x;
			c.params[1] = ai->Getcb()->GetElevation(pos.x, pos.z);
			c.params[2] = pos.z;

			//ai->Getcb()->GiveOrder(unit_id, &c);
			GiveOrder(&c, unit_id, "Fallback");
		}
	}
}


void AAIExecute::GetFallBackPos(float3 *pos, int unit_id, float max_weapon_range) const
{
	*pos = ZeroVector;

	const float3 unit_pos = ai->Getcb()->GetUnitPos(unit_id);

	// units without range should not end up here; this is for attacking units only
	// prevents a NaN
	assert(max_weapon_range != 0.0f);

	// get list of enemies within weapons range
	const int number_of_enemies = ai->Getcb()->GetEnemyUnits(&(ai->Getmap()->units_in_los.front()), unit_pos, max_weapon_range * cfg->FALLBACK_DIST_RATIO);

	if(number_of_enemies > 0)
	{
		float3 enemy_pos;

		for(int k = 0; k < number_of_enemies; ++k)
		{
			enemy_pos = ai->Getcb()->GetUnitPos(ai->Getmap()->units_in_los[k]);

			// get distance to enemy
			float dx   = enemy_pos.x - unit_pos.x;
			float dz   = enemy_pos.z - unit_pos.z;
			float dist = fastmath::apxsqrt(dx*dx + dz*dz);

			// get dir from unit to enemy
			enemy_pos.x -= unit_pos.x;
			enemy_pos.z -= unit_pos.z;

			// move closer to enemy if we are out of range,
			// and away if we are closer then our max range
			pos->x += ((dist / max_weapon_range) - 1) * enemy_pos.x;
			pos->z += ((dist / max_weapon_range) - 1) * enemy_pos.z;
		}

		// move less if lots of enemies are close
		pos->x /= (float)number_of_enemies;
		pos->z /= (float)number_of_enemies;

		// apply relative move distance to the current position
		// to get the target position
		pos->x += unit_pos.x;
		pos->z += unit_pos.z;
	}
}

void AAIExecute::GiveOrder(Command *c, int unit, const char *owner)
{
	++issued_orders;

	if(issued_orders%500 == 0)
		ai->Log("%i th order has been given by %s in frame %i\n", issued_orders, owner,  ai->Getcb()->GetCurrentFrame());

	ai->Getut()->units[unit].last_order = ai->Getcb()->GetCurrentFrame();

	ai->Getcb()->GiveOrder(unit, c);
}
