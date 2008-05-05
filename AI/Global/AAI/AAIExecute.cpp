// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIExecute.h"
#include "AAIBuildTable.h"
#include "System/FastMath.h"

// all the static vars
float AAIExecute::current = 0.5;
float AAIExecute::learned = 2.5;


AAIExecute::AAIExecute(AAI *ai, AAIBrain *brain)
{
	issued_orders = 0;

	this->ai = ai;
	this->cb = ai->cb;
	this->bt = ai->bt;
	this->brain = brain;
	this->map = ai->map;
	this->ut = ai->ut;

	brain->execute = this;

//	buildques = 0;
//	factory_table = 0;
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

	srand( time(NULL) );
}

AAIExecute::~AAIExecute(void)
{
//	if(buildques)
//	{
//		for(int i = 0; i < numOfFactories; ++i)
//			buildques[i].clear();

//		delete [] buildques;
//	}

//	if(factory_table)
//		delete [] factory_table;
}

void AAIExecute::moveUnitTo(int unit, float3 *position)
{
	Command c;

	c.id = CMD_MOVE;

	c.params.resize(3);
	c.params[0] = position->x;
	c.params[1] = position->y;
	c.params[2] = position->z;

	//cb->GiveOrder(unit, &c);
	GiveOrder(&c, unit, "moveUnitTo");
	ut->SetUnitStatus(unit, MOVING);
}

void AAIExecute::stopUnit(int unit)
{
	Command c;
	c.id = CMD_STOP;

	//cb->GiveOrder(unit, &c);
	GiveOrder(&c, unit, "StopUnit");
	ut->SetUnitStatus(unit, UNIT_IDLE);
}

// returns true if unit is busy
bool AAIExecute::IsBusy(int unit)
{
	const CCommandQueue* commands = cb->GetCurrentUnitCommands(unit);

	if(commands->empty())
		return false;
	return true;
}

// adds a unit to the group of attackers
void AAIExecute::AddUnitToGroup(int unit_id, int def_id, UnitCategory category)
{
	UnitType unit_type = bt->GetUnitType(def_id);

	for(list<AAIGroup*>::iterator group = ai->group_list[category].begin(); group != ai->group_list[category].end(); ++group)
	{
		if((*group)->AddUnit(unit_id, def_id, unit_type))
		{
			ai->ut->units[unit_id].group = *group;
			return;
		}
	}

	// end of grouplist has been reached and unit has not been assigned to any group
	// -> create newone
	AAIGroup *new_group;

	try
	{
		new_group = new AAIGroup(cb, ai, bt->unitList[def_id-1], unit_type);
	}
	catch(...) //catches everything
	{
		fprintf(ai->file, "Exception thrown when allocating memory for new group");
		return;
	}

	ai->group_list[category].push_back(new_group);
	new_group->AddUnit(unit_id, def_id, unit_type);
	ai->ut->units[unit_id].group = new_group;
}

void AAIExecute::UpdateRecon()
{
	float3 pos;

	// update units in los
	ai->map->UpdateRecon();

	// explore map -> send scouts to different sectors, build new scouts if needed etc.
	// check number of scouts and order new ones if necessary
	if(ai->activeScouts + ai->futureScouts < cfg->MAX_SCOUTS)
	{
		int scout = 0;

		float cost; 
		float los;

		int period = brain->GetGamePeriod();

		if(period == 0)
		{
			cost = 2;
			los = 0.5;
		}
		else if(period == 1)
		{
			cost = 1;
			los = 2;
		}
		else 
		{
			cost = 0.5;
			los = 4.0;
		}

		// determine movement type of scout based on map
		// always: MOVE_TYPE_AIR, MOVE_TYPE_HOVER, MOVE_TYPE_AMPHIB
		unsigned int allowed_movement_types = 22;
		
		if(map->mapType == LAND_MAP)
			allowed_movement_types |= MOVE_TYPE_GROUND;
		else if(map->mapType == LAND_WATER_MAP)
		{
			allowed_movement_types |= MOVE_TYPE_GROUND;
			allowed_movement_types |= MOVE_TYPE_SEA;
		}
		else if(map->mapType == WATER_MAP)
			allowed_movement_types |= MOVE_TYPE_SEA;
		

		// request cloakable scouts from time to time
		if(rand()%5 == 1)
			scout = bt->GetScout(ai->side, los, cost, allowed_movement_types, 10, true, true);
		else
			scout = bt->GetScout(ai->side, los, cost, allowed_movement_types, 10, false, true);

		if(scout)
		{
			if(AddUnitToBuildque(scout, 1, false))
			{
				++ai->futureScouts;
				++bt->units_dynamic[scout].requested;
			}
		}
	}

	// get idle scouts and let them explore the map
	for(list<int>::iterator scout = ai->scouts.begin(); scout != ai->scouts.end(); ++scout)
	{
		if(!IsBusy(*scout))
		{
			pos = ZeroVector;
			
			// get scout dest
			brain->GetNewScoutDest(&pos, *scout);

			if(pos.x > 0)
				moveUnitTo(*scout, &pos);
		}
	}
}

float3 AAIExecute::GetBuildsite(int builder, int building, UnitCategory category)
{
	float3 pos;
	float3 builder_pos;
	const UnitDef *def = bt->unitList[building-1];

	// check the sector of the builder
	builder_pos = cb->GetUnitPos(builder);
	// look in the builders sector first
	int x = builder_pos.x/ai->map->xSectorSize;
	int y = builder_pos.z/ai->map->ySectorSize;

	if(ai->map->sector[x][y].distance_to_base == 0)
	{
		pos = ai->map->sector[x][y].GetBuildsite(building);

		// if suitable location found, return pos...
		if(pos.x)	
			return pos;
	}

	// look in any of the base sectors
	for(list<AAISector*>::iterator s = brain->sectors[0].begin(); s != brain->sectors[0].end(); ++s)
	{	
		pos = (*s)->GetBuildsite(building);

		// if suitable location found, return pos...
		if(pos.x)	
			return pos;
	}

	pos.x = pos.y = pos.z = 0;
	return pos;
}

float3 AAIExecute::GetUnitBuildsite(int builder, int unit)
{
	float3 builder_pos = cb->GetUnitPos(builder);
	float3 pos = ZeroVector, best_pos = ZeroVector;
	float min_dist = 1000000, dist;

	for(list<AAISector*>::iterator s = brain->sectors[1].begin(); s != brain->sectors[1].end(); ++s)
	{	
		bool water = false;

		if(bt->IsSea(unit))
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

AAIMetalSpot* AAIExecute::FindMetalSpot(bool land, bool water)
{
	// prevent crashes on smaller maps 
	int max_distance = min(cfg->MAX_MEX_DISTANCE, brain->max_distance);

	for(int sector_dist = 0; sector_dist < max_distance; ++sector_dist)
	{
		for(list<AAISector*>::iterator sector = brain->sectors[sector_dist].begin();sector != brain->sectors[sector_dist].end(); sector++)
		{
			if(sector_dist == 0)
			{
				if((*sector)->freeMetalSpots)
				{
					for(list<AAIMetalSpot*>::iterator spot = (*sector)->metalSpots.begin(); spot != (*sector)->metalSpots.end(); ++spot)
					{
						if(!(*spot)->occupied)	
						{
							if( (land && (*spot)->pos.y >= 0) ||(water && (*spot)->pos.y < 0) )
								return *spot;
						}
					}
				}
			}
			else
			{
				if((*sector)->freeMetalSpots && brain->IsSafeSector(*sector) && map->team_sector_map[(*sector)->x][(*sector)->y] == -1 )
				{
					for(list<AAIMetalSpot*>::iterator spot = (*sector)->metalSpots.begin(); spot != (*sector)->metalSpots.end(); ++spot)
					{
						if(!(*spot)->occupied)	
						{
							if( (land && (*spot)->pos.y >= 0) ||(water && (*spot)->pos.y < 0) )
								return *spot;
						}
					}
				}
			}
		}
	}

	return 0;
}

AAIMetalSpot* AAIExecute::FindMetalSpotClosestToBuilder(int land_mex, int water_mex)
{
	AAIMetalSpot *best_spot = 0;
	float shortest_dist = 10000.0f, dist;
	float3 builder_pos;
	AAIConstructor *builder = 0;

	// prevent crashes on smaller maps 
	int max_distance = min(cfg->MAX_MEX_DISTANCE, brain->max_distance);

	// dont search for too long if possible spot already found
	int min_spot_dist = -1;

	// look for free spots in all sectors within max range
	for(int sector_dist = 0; sector_dist < max_distance; ++sector_dist)
	{
		// skip search if spot already found 
		if(min_spot_dist >= 0 && sector_dist - min_spot_dist > 2)
			return best_spot;

		for(list<AAISector*>::iterator sector = brain->sectors[sector_dist].begin(); sector != brain->sectors[sector_dist].end(); sector++)
		{
			if((*sector)->freeMetalSpots && (*sector)->enemy_structures <= 0  && (*sector)->lost_units[MOBILE_CONSTRUCTOR-COMMANDER]  < 0.5 && (*sector)->threat <= 0)
			{
				for(list<AAIMetalSpot*>::iterator spot = (*sector)->metalSpots.begin(); spot != (*sector)->metalSpots.end(); ++spot)
				{
					if(!(*spot)->occupied)	
					{
						if((*spot)->pos.y > 0)
							builder = ut->FindClosestBuilder(land_mex, (*spot)->pos, true, 10);
						else
							builder = ut->FindClosestBuilder(water_mex, (*spot)->pos, true, 10);

						if(builder)
						{
							// get distance to pos
							builder_pos = cb->GetUnitPos(builder->unit_id);

							dist = sqrt( pow(((*spot)->pos.x - builder_pos.x), 2) + pow(((*spot)->pos.z - builder_pos.z),2) ) / bt->unitList[builder->def_id-1]->speed;

							if(dist < shortest_dist)
							{
								best_spot = *spot;
								shortest_dist = dist;

								min_spot_dist = sector_dist;
							}	
						}
					}	
				}
			}
		}
	}

	return best_spot;
}


float AAIExecute::GetTotalGroundPower()
{
	float power = 0;

	// get ground power of all ground assault units
	for(list<AAIGroup*>::iterator group = ai->group_list[GROUND_ASSAULT].begin(); group != ai->group_list[GROUND_ASSAULT].end(); group++)
		power += (*group)->GetPowerVS(0);
	
	return power;
}

float AAIExecute::GetTotalAirPower()
{
	float power = 0;

	for(list<AAIGroup*>::iterator group = ai->group_list[GROUND_ASSAULT].begin(); group != ai->group_list[GROUND_ASSAULT].end(); group++)
	{
		power += (*group)->GetPowerVS(1);
	}

	return power;
}

list<int>* AAIExecute::GetBuildqueOfFactory(int def_id)
{
	for(int i = 0; i < numOfFactories; ++i)
	{
		if(factory_table[i] == def_id)
			return &buildques[i];
	}

	return 0;
}

bool AAIExecute::AddUnitToBuildque(int def_id, int number, bool urgent)
{
	urgent = false;

	UnitCategory category = bt->units_static[def_id].category;

	if(category == UNKNOWN)
		return false;

	list<int> *buildque = 0, *temp = 0;
	
	float my_rating, best_rating = 0;

	for(list<int>::iterator fac = bt->units_static[def_id].builtByList.begin(); fac != bt->units_static[def_id].builtByList.end(); ++fac)
	{
		if(bt->units_dynamic[*fac].active > 0)
		{
			temp = GetBuildqueOfFactory(*fac);

			if(temp)
			{
				my_rating = (1 + 2 * (float) bt->units_dynamic[*fac].active) / (temp->size() + 3);

				if(map->mapType == WATER_MAP && !bt->CanPlacedWater(*fac))
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
			buildque = temp;
		}
	}

	

	// determine position
	if(buildque)
	{
		//fprintf(ai->file, "Found builque for %s\n", bt->unitList[def_id-1]->humanName.c_str());

		if(bt->IsBuilder(def_id))
		{	
			buildque->insert(buildque->begin(), number, def_id);
			return true;	
		}
		else if(category == SCOUT)
		{
			if(ai->activeScouts < 2)
			{
				buildque->insert(buildque->begin(), number, def_id);
				return true;
			}
			else
			{
				/*// insert after the last builder
				for(list<int>::iterator unit = buildque->begin(); unit != buildque->end();  ++unit)
				{
					if(!bt->IsBuilder(*unit))
					{
						buildque->insert(unit, number, def_id);
						return true;
					}
				}*/
				buildque->insert(buildque->begin(), number, def_id);
				return true;
			}
		}
		else if(buildque->size() < cfg->MAX_BUILDQUE_SIZE)
		{
			if(urgent)
				buildque->insert(buildque->begin(), number, def_id);
			else
				buildque->insert(buildque->end(), number, def_id);

			//fprintf(ai->file, "Added %s to buildque with length %i\n", bt->unitList[def_id-1]->humanName.c_str(), buildque->size());

			return true;
		}
	}
	//else
	//	fprintf(ai->file, "Could not find builque for %s\n", bt->unitList[def_id-1]->humanName.c_str());

	return false;
}

void AAIExecute::InitBuildques()
{
	// determine number of factories first
	numOfFactories = 0;
	
	// stationary factories
	for(list<int>::iterator cons = bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].begin(); cons != bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].end(); ++cons)
	{
		if(bt->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
			++numOfFactories;
	}
	// and look for all mobile factories
	for(list<int>::iterator cons = bt->units_of_category[MOBILE_CONSTRUCTOR][ai->side-1].begin(); cons != bt->units_of_category[MOBILE_CONSTRUCTOR][ai->side-1].end(); ++cons)
	{
		if(bt->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
			++numOfFactories;
	}
	// and add com
	for(list<int>::iterator cons = bt->units_of_category[COMMANDER][ai->side-1].begin(); cons != bt->units_of_category[COMMANDER][ai->side-1].end(); ++cons)
	{
		if(bt->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
			++numOfFactories;
	}

//	buildques = new list<int>[numOfFactories];
	buildques.resize(numOfFactories);

	// set up factory buildque identification
//	factory_table = new int[numOfFactories];
	factory_table.resize(numOfFactories);
	
	int i = 0;

	for(list<int>::iterator cons = bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].begin(); cons != bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].end(); ++cons)
	{
		if(bt->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
		{
			factory_table[i] = *cons;
			++i;
		}
	}

	for(list<int>::iterator cons = bt->units_of_category[MOBILE_CONSTRUCTOR][ai->side-1].begin(); cons != bt->units_of_category[MOBILE_CONSTRUCTOR][ai->side-1].end(); ++cons)
	{
		if(bt->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
		{
			factory_table[i] = *cons;
			++i;
		}
	}

	for(list<int>::iterator cons = bt->units_of_category[COMMANDER][ai->side-1].begin(); cons != bt->units_of_category[COMMANDER][ai->side-1].end(); ++cons)
	{
		if(bt->units_static[*cons].unit_type & UNIT_TYPE_FACTORY)
		{
			factory_table[i] = *cons;
			++i;
		}
	}
}

float3 AAIExecute::GetRallyPoint(unsigned int unit_movement_type, int min_dist, int max_dist)
{
	float3 pos;

	// check neighbouring sectors
	for(int i = min_dist; i <= max_dist; ++i)
	{
		if(unit_movement_type & MOVE_TYPE_GROUND)
			brain->sectors[i].sort(suitable_for_ground_rallypoint);
		else if(unit_movement_type & MOVE_TYPE_SEA)
			brain->sectors[i].sort(suitable_for_sea_rallypoint);
		else 
			brain->sectors[i].sort(suitable_for_all_rallypoint);

		for(list<AAISector*>::iterator sector = brain->sectors[i].begin(); sector != brain->sectors[i].end(); sector++)
		{
			pos = (*sector)->GetMovePos(unit_movement_type);

			if(pos.x > 0)
				return pos;
		}
	}

	return ZeroVector;	
}

float3 AAIExecute::GetRallyPointCloseTo(UnitCategory category, unsigned int unit_movement_type, float3 pos, int min_dist, int max_dist)
{
	//float3 pos;

	// check neighbouring sectors
	for(int i = min_dist; i <= max_dist; ++i)
	{
		if(unit_movement_type & MOVE_TYPE_GROUND)
			brain->sectors[i].sort(suitable_for_ground_rallypoint);
		else if(unit_movement_type & MOVE_TYPE_SEA)
			brain->sectors[i].sort(suitable_for_sea_rallypoint);
		else 
			brain->sectors[i].sort(suitable_for_all_rallypoint);

		for(list<AAISector*>::iterator sector = brain->sectors[i].begin(); sector != brain->sectors[i].end(); sector++)
		{
			pos = (*sector)->GetMovePos(unit_movement_type);

			if(pos.x > 0)
				return pos;
		}
	}

	return ZeroVector;	

	/*AAISector* best_sector = 0;
	float best_rating = 0, my_rating;
	float3 center;

	int combat_cat = bt->GetIDOfAssaultCategory(category);

	bool land = true; 
	bool water = true;

	if(!cfg->AIR_ONLY_MOD)
	{
		if(category == GROUND_ASSAULT)
			water = false;
		else if(category == SEA_ASSAULT || category == SUBMARINE_ASSAULT)
			land = false;
	}	

	// check neighbouring sectors
	for(int i = min_dist; i <= max_dist; i++)
	{
		for(list<AAISector*>::iterator sector = brain->sectors[i].begin(); sector != brain->sectors[i].end(); sector++)
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
	if(ai->activeFactories < 1 && ai->activeUnits[EXTRACTOR] >= 2)
		return true;

	AAIConstructor *builder; 
	AAIMetalSpot *spot = 0;
	float3 pos;
	int land_mex;
	int water_mex;
	bool water;

	float cost = 0.25f + brain->Affordable() / 6.0f;
	float efficiency = 6.0 / (cost + 0.75f);

	// check if metal map
	if(ai->map->metalMap)
	{
		// get id of an extractor and look for suitable builder
		land_mex = bt->GetMex(ai->side, cost, efficiency, false, false, false);

		if(land_mex && bt->units_dynamic[land_mex].buildersAvailable <= 0)
		{
			bt->BuildBuilderFor(land_mex);
			land_mex = bt->GetMex(ai->side, cost, efficiency, false, false, true);
		}

		builder = ut->FindBuilder(land_mex, true, 10);

		if(builder)
		{
			pos = GetBuildsite(builder->unit_id, land_mex, EXTRACTOR);

			if(pos.x != 0)
			{
				builder->GiveConstructionOrder(land_mex, pos, false);
			}
			return true;
		}
		else
		{
			bt->AddBuilder(land_mex);
			return false;
		}
	}
	else // normal map
	{
		// different behaviour dependent on number of builders (to save time in late game)
		if(ut->constructors.size() < 5)
		{
			// get id of an extractor and look for suitable builder
			land_mex = bt->GetMex(ai->side, cost, efficiency, false, false, false);

			if(land_mex && bt->units_dynamic[land_mex].buildersAvailable <= 0)
			{
				bt->BuildBuilderFor(land_mex);
				land_mex = bt->GetMex(ai->side, cost, efficiency, false, false, true);
			}

			water_mex = bt->GetMex(ai->side, cost, efficiency, false, true, false);

			if(water_mex && bt->units_dynamic[water_mex].buildersAvailable <= 0)
			{
				bt->BuildBuilderFor(water_mex);
				water_mex = bt->GetMex(ai->side, cost, efficiency, false, true, true);
			}

			// find metal spot with lowest distance to available builders
			spot = FindMetalSpotClosestToBuilder(land_mex, water_mex);
	
			if(spot)
			{	
				// check if land or sea spot
				if(cb->GetElevation(spot->pos.x, spot->pos.z) < 0)
				{
					water = true;

					// build the water mex instead of land mex
					land_mex = water_mex;	
				}
				else
					water = false;
		
				// look for suitable builder
				int x = spot->pos.x/map->xSectorSize;
				int y = spot->pos.z/map->ySectorSize;

				// only allow commander if spot is close to own base
				if(map->sector[x][y].distance_to_base <= 0 || (brain->sectors[0].size() < 3 && map->sector[x][y].distance_to_base <= 1) )
					builder = ut->FindClosestBuilder(land_mex, spot->pos, true, 10);
				else
					builder = ut->FindClosestBuilder(land_mex, spot->pos, false, 10);
		
				if(builder)
				{
					builder->GiveConstructionOrder(land_mex, spot->pos, water);
					spot->occupied = true;
				}
				else
				{
					bt->AddBuilder(land_mex);
					return false;
				}
			}
			else
			{	

				// request metal makers if no spot found (only if spot was not found due to no buidlers available)
				builder = ut->FindClosestBuilder(land_mex, brain->base_center, true, 10);

				if(!builder)	
					builder = ut->FindClosestBuilder(water_mex, brain->base_center, true, 10);
		
				if(!builder)
				{
					if(map->mapType == WATER_MAP && !cfg->AIR_ONLY_MOD)
						bt->AddBuilder(water_mex);
					else
						bt->AddBuilder(land_mex);
				
					return false;
				}
				else
				{
					if(ai->activeUnits[METAL_MAKER] < cfg->MAX_METAL_MAKERS && urgency[METAL_MAKER] <= GetMetalUrgency() / 2.0f)
						urgency[METAL_MAKER] = GetMetalUrgency() / 2.0f;
				}
			}
		}
		else
		{
			bool land = true, water = true;

			// get next free metal spot
			AAIMetalSpot *spot = FindMetalSpot(land, water);

			if(spot)
			{
				int x = spot->pos.x/map->xSectorSize;
				int y = spot->pos.z/map->ySectorSize;

				if(cb->GetElevation(spot->pos.x, spot->pos.z) > 0) 
					water = false;	
				else
					water = true;
				
				// choose mex dependend on safety
				bool armed = false;

				//if(map->sector[x][y].lost_units[EXTRACTOR-COMMANDER] > 0.01f || map->sector[x][y].distance_to_base > 2)
				if(map->sector[x][y].distance_to_base > 2)
				{
					cost = 6.0f;
					armed = true;
					efficiency = 1.0f;

					// check mex upgrade
					if(ai->futureUnits[EXTRACTOR] < 2)
						CheckMexUpgrade();
				}

				int mex = bt->GetMex(ai->side, cost, efficiency, armed, water, false);

				if(mex && bt->units_dynamic[mex].buildersAvailable <= 0)
				{
					bt->BuildBuilderFor(mex);

					mex = bt->GetMex(ai->side, cost, efficiency, armed, water, true);
				}

				if(mex)
				{
					if(map->sector[x][y].distance_to_base <= 0 || (brain->sectors[0].size() < 3 && map->sector[x][y].distance_to_base <= 1) )
						builder = ut->FindClosestBuilder(mex, brain->base_center, true, 10);
					else
						builder = ut->FindClosestBuilder(mex, brain->base_center, false, 10);

					if(builder)
					{
						builder->GiveConstructionOrder(mex, spot->pos, water);
						spot->occupied = true;
					}
				}
			}
			else
			{	
				// check mex upgrade
				if(ai->futureUnits[EXTRACTOR] < 2)
					CheckMexUpgrade();

				// request metal makers if no spot found
				if(ai->activeUnits[METAL_MAKER] < cfg->MAX_METAL_MAKERS && urgency[METAL_MAKER] <= GetMetalUrgency() / 2.0f)
					urgency[METAL_MAKER] = GetMetalUrgency() / 2.0f;
			}
		}
	}

	return true;
}

bool AAIExecute::BuildPowerPlant()
{	
	if(ai->futureUnits[POWER_PLANT] > 1)
		return true;
	else if(ai->futureUnits[POWER_PLANT] > 0)
	{
		// try to assist construction of other power plants first
		AAIConstructor *builder;

		for(list<AAIBuildTask*>::iterator task = ai->build_tasks.begin(); task != ai->build_tasks.end(); ++task)
		{
			if((*task)->builder_id >= 0)
				builder = ut->units[(*task)->builder_id].cons;
			else
				builder = 0;
			
			// find the power plant that is already under construction
			if(builder && builder->construction_category == POWER_PLANT)
			{
				// dont build further power plants if already buidling an expensive plant 
				if(bt->units_static[builder->construction_def_id].cost > bt->avg_cost[POWER_PLANT][ai->side-1])
					return true;
				
				// try to assist
				if(builder->assistants.size() < cfg->MAX_ASSISTANTS)
				{
					AAIConstructor *assistant = ut->FindClosestAssister(builder->build_pos, 5, true, bt->GetAllowedMovementTypesForAssister(builder->construction_def_id));

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

		// power plant construction has not started -> builder is still on its way to constrcution site, wait until starting a new power plant
		return false;
	}
	else if(ai->activeFactories < 1 && ai->activeUnits[POWER_PLANT] >= 2)
		return true;

	// stop building power plants if already to much available energy
	if(cb->GetEnergyIncome() > 1.5f * cb->GetEnergyUsage() + 200.0)
		return true;

	int ground_plant = 0;
	int water_plant = 0;
	
	AAIConstructor *builder;
	float3 pos;

	float current_energy = cb->GetEnergyIncome();
	bool checkWater, checkGround;
	float urgency;
	float max_power;
	float eff;
	float energy = cb->GetEnergyIncome()+1;

	// check if already one power_plant under construction and energy short 
	if(ai->futureUnits[POWER_PLANT] > 0 && ai->activeUnits[POWER_PLANT] > 9 && averageEnergySurplus < 100)
	{
		urgency = 0.4f + GetEnergyUrgency();
		max_power = 0.5f;
		eff = 2.2f - brain->Affordable() / 4.0f;
	}
	else
	{
		max_power = 0.5f + pow((float) ai->activeUnits[POWER_PLANT], 0.8f);
		eff = 0.5 + 1.0f / (brain->Affordable() + 0.5f);
		urgency = 0.5f + GetEnergyUrgency();
	}

	// sort sectors according to threat level
	learned = 70000.0 / (cb->GetCurrentFrame() + 35000) + 1;
	current = 2.5 - learned;

	if(ai->activeUnits[POWER_PLANT] >= 2)
		brain->sectors[0].sort(suitable_for_power_plant);

	// get water and ground plant
	ground_plant = bt->GetPowerPlant(ai->side, eff, urgency, max_power, energy, false, false, false);
	// currently aai cannot build this building
	if(ground_plant && bt->units_dynamic[ground_plant].buildersAvailable <= 0)
	{
		if( bt->units_dynamic[water_plant].buildersRequested <= 0)
			bt->AddBuilder(ground_plant);

		ground_plant = bt->GetPowerPlant(ai->side, eff, urgency, max_power, energy, false, false, true);
	}

	water_plant = bt->GetPowerPlant(ai->side, eff, urgency, max_power, energy, true, false, false);
	// currently aai cannot build this building
	if(water_plant && bt->units_dynamic[water_plant].buildersAvailable <= 0)
	{
		if( bt->units_dynamic[water_plant].buildersRequested <= 0)
			bt->AddBuilder(water_plant);

		water_plant = bt->GetPowerPlant(ai->side, eff, urgency, max_power, energy, true, false, true);
	}

	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
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

		if(checkWater == true)
		{
			fprintf(ai->file, "Water ratio in sector %i, %i: %f\n", (*sector)->x, (*sector)->y, (*sector)->water_ratio);
		}

		if(checkGround && ground_plant)
		{
			pos = (*sector)->GetBuildsite(ground_plant, false);

			if(pos.x > 0)
			{
				builder = ut->FindClosestBuilder(ground_plant, pos, true, 10);

				if(builder)
				{
					futureAvailableEnergy += bt->units_static[ground_plant].efficiency[0];
					builder->GiveConstructionOrder(ground_plant, pos, false);
					return true;
				}
				else
				{
					bt->AddBuilder(ground_plant);
					return false;
				}
			}
			else
			{
				brain->ExpandBase(LAND_SECTOR);
				fprintf(ai->file, "Base expanded by BuildPowerPlant()\n");
			}
		}

		if(checkWater && water_plant)
		{
			if(ut->constructors.size() > 1 || ai->activeUnits[POWER_PLANT] >= 2)
				pos = (*sector)->GetBuildsite(water_plant, true);
			else 
			{
				builder = ut->FindBuilder(water_plant, true, 10);
				
				if(builder)
				{
					pos = map->GetClosestBuildsite(bt->unitList[water_plant-1], cb->GetUnitPos(builder->unit_id), 40, true);

					if(pos.x <= 0)
						pos = (*sector)->GetBuildsite(water_plant, true);
				}
				else
					pos = (*sector)->GetBuildsite(water_plant, true);
			}

			if(pos.x > 0)
			{
				builder = ut->FindClosestBuilder(water_plant, pos, true, 10);

				if(builder)
				{
					futureAvailableEnergy += bt->units_static[water_plant].efficiency[0];
					builder->GiveConstructionOrder(water_plant, pos, true);
					return true;
				}
				else
				{
					bt->AddBuilder(water_plant);
					return false;
				}
			}
			else
			{
				brain->ExpandBase(WATER_SECTOR);
				fprintf(ai->file, "Base expanded by BuildPowerPlant() (water sector)\n");
			}
		}
	}

	return true;
}

bool AAIExecute::BuildMetalMaker()
{
	if(ai->activeFactories < 1 && ai->activeUnits[EXTRACTOR] >= 2)
		return true;

	if(ai->futureUnits[METAL_MAKER] > 0 || disabledMMakers >= 1)
		return true;

	bool checkWater, checkGround;
	int maker;
	AAIConstructor *builder;
	float3 pos;
	// urgency < 4
	
	float urgency = GetMetalUrgency() / 2.0f;
	
	float cost = 0.25f + brain->Affordable() / 2.0f;

	float efficiency = 0.25f + ai->activeUnits[METAL_MAKER] / 4.0f ;
	float metal = efficiency;


	// sort sectors according to threat level
	learned = 70000.0 / (cb->GetCurrentFrame() + 35000) + 1;
	current = 2.5 - learned;

	brain->sectors[0].sort(least_dangerous);
	
	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); sector++)
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
			maker = bt->GetMetalMaker(ai->side, cost,  efficiency, metal, urgency, false, false); 
	
			// currently aai cannot build this building
			if(maker && bt->units_dynamic[maker].buildersAvailable <= 0)
			{
				if(bt->units_dynamic[maker].buildersRequested <= 0)
					bt->BuildBuilderFor(maker);
				
				maker = bt->GetMetalMaker(ai->side, cost, efficiency, metal, urgency, false, true);
			}

			if(maker)
			{
				pos = (*sector)->GetBuildsite(maker, false);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(maker, pos, true, 10);

					if(builder)
					{
						futureRequestedEnergy += bt->unitList[maker-1]->energyUpkeep;
						builder->GiveConstructionOrder(maker, pos, false);
						return true;
					}
					else
					{
						bt->AddBuilder(maker);
						return false;
					}
				}
				else
				{
					brain->ExpandBase(LAND_SECTOR);	
					fprintf(ai->file, "Base expanded by BuildMetalMaker()\n");
				}
			}
		}

		if(checkWater)
		{
			maker = bt->GetMetalMaker(ai->side, brain->Affordable(),  8.0/(urgency+2.0), 64.0/(16*urgency+2.0), urgency, true, false); 
	
			// currently aai cannot build this building
			if(maker && bt->units_dynamic[maker].buildersAvailable <= 0)
			{
				if(bt->units_dynamic[maker].buildersRequested <= 0)
					bt->BuildBuilderFor(maker);
				
				maker = bt->GetMetalMaker(ai->side, brain->Affordable(),  8.0/(urgency+2.0), 64.0/(16*urgency+2.0), urgency, true, true);
			}

			if(maker)
			{
				pos = (*sector)->GetBuildsite(maker, true);
	
				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(maker, pos, true, 10);

					if(builder)
					{
						futureRequestedEnergy += bt->unitList[maker-1]->energyUpkeep;
						builder->GiveConstructionOrder(maker, pos, true);
						return true;
					}
					else
					{
						bt->AddBuilder(maker);
						return false;
					}
				}
				else
				{
					brain->ExpandBase(WATER_SECTOR);
					fprintf(ai->file, "Base expanded by BuildMetalMaker() (water sector)\n");
				}
			}
		}
	}
		
	return true;
}

bool AAIExecute::BuildStorage()
{
	if(ai->futureUnits[STORAGE] > 0 || ai->activeUnits[STORAGE] >= cfg->MAX_STORAGE)
		return true;

	if(ai->activeFactories < 2)
		return true;

	int storage = 0;
	bool checkWater, checkGround;
	AAIConstructor *builder;
	float3 pos;

	float metal = 4 / (cb->GetMetalStorage() + futureStoredMetal - cb->GetMetal() + 1);
	float energy = 2 / (cb->GetEnergyStorage() + futureStoredMetal - cb->GetEnergy() + 1);

	// urgency < 4
	float urgency = 16.0 / (ai->activeUnits[METAL_MAKER]+ai->futureUnits[METAL_MAKER]+4);

	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); sector++)
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
			storage = bt->GetStorage(ai->side, brain->Affordable(),  metal, energy, 1, false, false); 
	
			if(storage && bt->units_dynamic[storage].buildersAvailable <= 0 && bt->units_dynamic[storage].buildersRequested <= 0)
			{
				bt->BuildBuilderFor(storage);
				storage = bt->GetStorage(ai->side, brain->Affordable(),  metal, energy, 1, false, true); 
			}

			if(storage)
			{
				pos = (*sector)->GetBuildsite(storage, false);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(storage, pos, true, 10);
	
					if(builder)
					{
						builder->GiveConstructionOrder(storage, pos, false);
						return true;
					}
					else
					{
						bt->AddBuilder(storage);
						return false;
					}
				}
				else
				{
					brain->ExpandBase(LAND_SECTOR);
					fprintf(ai->file, "Base expanded by BuildStorage()\n");
				}
			}
		}

		if(checkWater)
		{
			storage = bt->GetStorage(ai->side, brain->Affordable(),  metal, energy, 1, true, false); 
	
			if(storage && bt->units_dynamic[storage].buildersAvailable <= 0 && bt->units_dynamic[storage].buildersRequested <= 0)
			{
				bt->BuildBuilderFor(storage);
				storage = bt->GetStorage(ai->side, brain->Affordable(),  metal, energy, 1, true, true); 
			}

			if(storage)
			{
				pos = (*sector)->GetBuildsite(storage, true);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(storage, pos, true, 10);

					if(builder)
					{
						builder->GiveConstructionOrder(storage, pos, true);
						return true;
					}
					else
					{
						bt->AddBuilder(storage);
						return false;
					}
				}
				else
				{
					brain->ExpandBase(WATER_SECTOR);
					fprintf(ai->file, "Base expanded by BuildStorage()\n");
				}
			}
		}
	}
		
	return true;
}

bool AAIExecute::BuildAirBase()
{
	if(ai->futureUnits[AIR_BASE] > 0 || ai->activeUnits[AIR_BASE] >= cfg->MAX_AIR_BASE)
		return true;

	int airbase = 0;
	bool checkWater, checkGround;
	AAIConstructor *builder;
	float3 pos;

	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); sector++)
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

			airbase = bt->GetAirBase(ai->side, brain->Affordable(), false, false); 
	
			if(airbase && bt->units_dynamic[airbase].buildersAvailable <= 0)
			{
				if(bt->units_dynamic[airbase].buildersRequested <= 0)
					bt->BuildBuilderFor(airbase);

				airbase = bt->GetAirBase(ai->side, brain->Affordable(), false, true);
			}

			if(airbase)
			{
				pos = (*sector)->GetBuildsite(airbase, false);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(airbase, pos, true, 10);
	
					if(builder)
					{
						builder->GiveConstructionOrder(airbase, pos, false);
						return true;
					}
					else
					{
						bt->AddBuilder(airbase);
						return false;
					}
				}
				else
				{
					brain->ExpandBase(LAND_SECTOR);
					fprintf(ai->file, "Base expanded by BuildAirBase()\n");
				}
			}
		}

		if(checkWater)
		{
			airbase = bt->GetAirBase(ai->side, brain->Affordable(), true, false); 
	
			if(airbase && bt->units_dynamic[airbase].buildersAvailable <= 0 )
			{
				if(bt->units_dynamic[airbase].buildersRequested <= 0)
					bt->BuildBuilderFor(airbase);

				airbase = bt->GetAirBase(ai->side, brain->Affordable(), true, true);  
			}

			if(airbase)
			{
				pos = (*sector)->GetBuildsite(airbase, true);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(airbase, pos, true, 10);

					if(builder)
					{
						builder->GiveConstructionOrder(airbase, pos, true);
						return true;
					}
					else
					{
						bt->AddBuilder(airbase);
						return false;
					}
				}
				else
				{
					brain->ExpandBase(WATER_SECTOR);
					fprintf(ai->file, "Base expanded by BuildAirBase() (water sector)\n");
				}
			}
		}
	}
		
	return true;
}

bool AAIExecute::BuildDefences()
{
	if(ai->futureUnits[STATIONARY_DEF] > 2 || next_defence <= 0)
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
	if(dest->allied_structures > 50)
		return BUILDORDER_SUCCESFUL;

	// dont start construction of further defences if expensive defences are already under construction in this sector
	for(list<AAIBuildTask*>::iterator task = ai->build_tasks.begin(); task != ai->build_tasks.end(); ++task)
	{
		if(bt->units_static[(*task)->def_id].category == STATIONARY_DEF)
		{
			if(dest->PosInSector(&(*task)->build_pos))
			{
				if(bt->units_static[(*task)->def_id].cost > 0.7 * bt->avg_cost[STATIONARY_DEF][ai->side-1])
					return BUILDORDER_SUCCESFUL;
			}
		}
	}

	
	double gr_eff = 0, air_eff = 0, hover_eff = 0, sea_eff = 0, submarine_eff = 0;
	
	bool checkWater, checkGround;
	int building;
	float3 pos;
	AAIConstructor *builder;

	float terrain = 3.0f;

	if(dest->distance_to_base > 0)
		terrain = 15.0f;

	if(dest->water_ratio < 0.15)
	{
		checkWater = false;
		checkGround = true;	
	}
	else if(dest->water_ratio < 0.85)
	{
		checkWater = true;
		checkGround = true;
	}
	else
	{
		checkWater = true;
		checkGround = false;
	}

	double urgency = 0.25 + 10.0 / (dest->defences.size() + dest->GetDefencePowerVs(category) + 1.0);
	double power = 0.5 + 2.0 / urgency;
	double eff = 0.5 + brain->Affordable()/6.0;
	double range = 0.5;
	double cost = 0.5 + brain->Affordable()/6.0;

	if(dest->defences.size() > 3)
	{
		int t = rand()%500;

		if(t < 70)
		{
			range = 2.5;
			terrain = 30.0f; 
		}
		else if(t < 200)
		{
			range = 1;
			terrain = 25.0f; 
		}

		t = rand()%500;

		if(t < 200)
			power += 1.5;
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
		if(rand()%cfg->LEARN_RATE == 1 && dest->defences.size() > 3)
			building = bt->GetRandomDefence(ai->side, category);
		else
			building = bt->GetDefenceBuilding(ai->side, eff, power, cost, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, range, 8, false, false);

		if(building && bt->units_dynamic[building].buildersAvailable <= 0 && bt->units_dynamic[building].buildersRequested <= 0)
		{
			bt->BuildBuilderFor(building);
			building = bt->GetDefenceBuilding(ai->side, eff, power, cost, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, range, 8, false, true);
		}

		// stop building of weak defences if urgency is too low (wait for better defences)
		if(urgency < 2.5)
		{
			int id = bt->GetIDOfAssaultCategory(category);
			
			if(bt->units_static[building].efficiency[id]  < bt->avg_eff[ai->side-1][5][id] / 2.0f)
				building = 0;
		}
		else if(urgency < 0.75)
		{
			int id = bt->GetIDOfAssaultCategory(category);
			
			if(bt->units_static[building].efficiency[id]  < bt->avg_eff[ai->side-1][5][id])
				building = 0;
		}
		
		if(building)
		{
			pos = dest->GetDefenceBuildsite(building, category, terrain, false);
				
			if(pos.x > 0)
			{
				builder = ut->FindClosestBuilder(building, pos, true, 10);	

				if(builder)
				{
					builder->GiveConstructionOrder(building, pos, false);

					// add defence to map
					map->AddDefence(&pos, building); 

					return BUILDORDER_SUCCESFUL;
				}
				else
				{
					bt->AddBuilder(building);	
					return BUILDORDER_NOBUILDER;
				}
			}
			else
				return BUILDORDER_NOBUILDPOS;
		}	
	}

	if(checkWater)
	{
		if(rand()%cfg->LEARN_RATE == 1 && dest->defences.size() > 3)
			building = bt->GetRandomDefence(ai->side, category);
		else
			building = bt->GetDefenceBuilding(ai->side, eff, power, cost, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, 1, 8, true, false);

		if(building && bt->units_dynamic[building].buildersAvailable <= 0 && bt->units_dynamic[building].buildersRequested <= 0)
		{
			bt->BuildBuilderFor(building);
			building = bt->GetDefenceBuilding(ai->side, eff, power, cost, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, 1,  8, true, true);
		}

		// stop building of weak defences if urgency is too low (wait for better defences)
		if(urgency < 2.5)
		{
			int id = bt->GetIDOfAssaultCategory(category);
			
			if(bt->units_static[building].efficiency[id]  < bt->avg_eff[ai->side-1][5][id] / 2.0f)
				building = 0;
		}
		else if(urgency < 0.75)
		{
			int id = bt->GetIDOfAssaultCategory(category);
			
			if(bt->units_static[building].efficiency[id]  < bt->avg_eff[ai->side-1][5][id])
				building = 0;
		}

		if(building)
		{
			pos = dest->GetDefenceBuildsite(building, category, terrain, true);
				
			if(pos.x > 0)
			{
				builder = ut->FindClosestBuilder(building, pos, true, 10);

				if(builder)
				{
					builder->GiveConstructionOrder(building, pos, true);

					// add defence to map
					map->AddDefence(&pos, building); 

					return BUILDORDER_SUCCESFUL;
				}
				else
				{
					bt->AddBuilder(building);
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
	if(ai->futureUnits[STATIONARY_ARTY])
		return true;

	AAIConstructor *builder;
	float3 pos;
	int arty = 0;
	bool checkWater, checkGround;

	brain->sectors[0].sort(suitable_for_arty);

	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
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
			arty = bt->GetStationaryArty(ai->side, 1, 2, 2, false, false); 
		
			if(arty)
			{
				if(bt->units_dynamic[arty].buildersAvailable <= 0)
				{
					if(bt->units_dynamic[arty].buildersRequested <= 0)
						bt->BuildBuilderFor(arty);
					
					return true;
				}

				pos = (*sector)->GetHighestBuildsite(arty);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(arty, pos, true, 10);

					if(builder)
					{
						builder->GiveConstructionOrder(arty, pos, false);
						return true;
					}
					else 
					{
						bt->AddBuilder(arty);
						return false;
					}
				}
			}
		}

		if(checkWater)
		{
			arty = bt->GetStationaryArty(ai->side, 1, 2, 2, true, false); 
		
			if(arty)
			{
				if(bt->units_dynamic[arty].buildersAvailable <= 0)
				{
					if(bt->units_dynamic[arty].buildersRequested <= 0)
						bt->BuildBuilderFor(arty);

					return true;
				}

				pos = (*sector)->GetBuildsite(arty, true);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(arty, pos, true, 10);

					if(builder)
					{
						builder->GiveConstructionOrder(arty, pos, true);
						return true;
					}
					else
					{
						bt->AddBuilder(arty);
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool AAIExecute::BuildFactory()
{
	if(ai->futureUnits[STATIONARY_CONSTRUCTOR] > 0)
		return true;

	AAIConstructor *builder = 0, *temp_builder;
	float3 pos = ZeroVector;
	float best_rating = 0, my_rating;
	int building = 0;
	
	// go through list of factories, build cheapest requested factory first
	for(list<int>::iterator fac = bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].begin(); fac != bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].end(); ++fac)
	{
		if(bt->units_dynamic[*fac].requested > 0)	
		{
			my_rating = bt->GetFactoryRating(*fac) / pow( (float) (1 + bt->units_dynamic[*fac].active), 2.0f);
			my_rating *= (1 + sqrt(2.0 + (float) GetBuildqueOfFactory(*fac)->size())); 

			if(ai->activeFactories < 1)
				my_rating /= bt->units_static[*fac].cost;

			// skip factories that could not be built 
			if(bt->units_static[*fac].efficiency[4] > 2)
			{
				my_rating = 0;
				bt->units_static[*fac].efficiency[4] = 0;
			}

			if(my_rating > best_rating)
			{
				// only check building if a suitable builder is available
				temp_builder = ut->FindBuilder(*fac, true, 10);
					
				if(temp_builder)
				{
					best_rating = my_rating;
					builder = temp_builder;
					building = *fac;
				}	
			}
		}
	}

	if(building)
	{
		bool water;

		// land  
		if(bt->CanPlacedLand(building))
		{
			water = false;

			brain->sectors[0].sort(suitable_for_ground_factory);

			// find buildpos
			for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
			{
				pos = (*sector)->GetRandomBuildsite(building, 20, false);

				if(pos.x > 0)
					break;
			}

			if(pos.x == 0)
			{
				for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
				{
					pos = (*sector)->GetBuildsite(building, false);
	
					if(pos.x > 0)
						break;
				}
			}
		}
		
		// try to build on water if land has not been possible
		if(pos.x == 0 && bt->CanPlacedWater(building))
		{
			water = true;

			brain->sectors[0].sort(suitable_for_sea_factory);

			// find buildpos
			for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
			{
				if((*sector)->ConnectedToOcean())
				{
					pos = (*sector)->GetRandomBuildsite(building, 20, true);

					if(pos.x > 0)
						break;
				}
			}

			if(pos.x == 0)
			{
				for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
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
			builder = ut->FindClosestBuilder(building, pos, true, 10);
			
			if(builder)
			{
				// give build order
				builder->GiveConstructionOrder(building, pos, water);
		
				// add average ressource usage
				futureRequestedMetal += bt->units_static[building].efficiency[0];
				futureRequestedEnergy += bt->units_static[building].efficiency[1];
			}
			else
			{
				if(bt->units_dynamic[building].buildersRequested <= 0 && bt->units_dynamic[building].buildersAvailable <= 0)
					bt->BuildBuilderFor(building);
					
				return false;
			}
		}
		else
		{
			bool expanded = false;

			// no suitable buildsite found
			if(bt->CanPlacedLand(building))
			{
				expanded = brain->ExpandBase(LAND_SECTOR);
				fprintf(ai->file, "Base expanded by BuildFactory()\n");
			}
			
			if(!expanded && bt->CanPlacedWater(building))
			{
				brain->ExpandBase(WATER_SECTOR);
				fprintf(ai->file, "Base expanded by BuildFactory() (water sector)\n");
			}

			// could not build due to lack of suitable buildpos
			++bt->units_static[building].efficiency[4];

			return true;
		}
	}

	return true;
}

void AAIExecute::BuildUnit(UnitCategory category, float speed, float cost, float range, float power, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, float eff, bool urgent)
{ 
	int unit = 0;

	if(category == GROUND_ASSAULT)
	{
		// choose random unit (to learn more)
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[GROUND_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetGroundAssault(ai->side, power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 15, false);

			if(unit && bt->units_dynamic[unit].buildersAvailable <= 0)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetGroundAssault(ai->side, power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 15, true);
			}
		}
	}
	else if(category == AIR_ASSAULT)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[AIR_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetAirAssault(ai->side, power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 9, false);

			if(unit && bt->units_dynamic[unit].buildersAvailable <= 0)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetAirAssault(ai->side, power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 9, true);
			}
		}
	}
	else if(category == HOVER_ASSAULT)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[HOVER_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetHoverAssault(ai->side, power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 9, false);

			if(unit && bt->units_dynamic[unit].buildersAvailable <= 0)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetHoverAssault(ai->side, power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 9, true);
			}
		}
	}
	else if(category == SEA_ASSAULT)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[SEA_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetSeaAssault(ai->side, power, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, eff, speed, range, cost, 9, false);

			if(unit && bt->units_dynamic[unit].buildersAvailable <= 0)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetSeaAssault(ai->side, power, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, eff, speed, range, cost, 9, false);
			}
		}
	}
	else if(category == SUBMARINE_ASSAULT)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[SUBMARINE_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetSubmarineAssault(ai->side, power, sea_eff, submarine_eff, stat_eff, eff, speed, range, cost, 9, false);

			if(unit && bt->units_dynamic[unit].buildersAvailable <= 0)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetSubmarineAssault(ai->side, power, sea_eff, submarine_eff, stat_eff, eff, speed, range, cost, 9, false);
			}
		}
	}


	if(unit)
	{
		if(bt->units_dynamic[unit].buildersAvailable > 0)
		{	
			if(bt->units_static[unit].cost < cfg->MAX_COST_LIGHT_ASSAULT * bt->max_cost[category][ai->side-1])
			{
				if(AddUnitToBuildque(unit, 3, urgent))
					bt->units_dynamic[unit].requested += 3;
			}
			else if(bt->units_static[unit].cost < cfg->MAX_COST_MEDIUM_ASSAULT * bt->max_cost[category][ai->side-1])
			{
				if(AddUnitToBuildque(unit, 2, urgent))
					bt->units_dynamic[unit].requested += 2;
			}
			else
			{
				if(AddUnitToBuildque(unit, 1, urgent))
					bt->units_dynamic[unit].requested += 1;
			}
		}
		else
			bt->BuildFactoryFor(unit);
	}
}

bool AAIExecute::BuildRecon()
{
	if(ai->futureUnits[STATIONARY_RECON])
		return true;

	int radar = 0;
	AAIConstructor *builder;
	float3 pos;
	bool checkWater, checkGround;

	float cost = brain->Affordable();
	float range = 10.0 / (cost + 1);
	
	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
	{
		if((*sector)->unitsOfType[STATIONARY_RECON] > 0)
		{
			checkWater = false;
			checkGround = false;
		}
		else if((*sector)->water_ratio < 0.15)
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
			// find radar
			radar = bt->GetRadar(ai->side, cost, range, false, false);

			if(radar && bt->units_dynamic[radar].buildersAvailable <= 0 && bt->units_dynamic[radar].buildersRequested <= 0)
			{
				bt->BuildBuilderFor(radar);
				radar = bt->GetRadar(ai->side, cost, range, false, true);
			}
		
			if(radar)
			{
				pos = (*sector)->GetHighestBuildsite(radar);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(radar, pos, true, 10);

					if(builder)
					{
						builder->GiveConstructionOrder(radar, pos, false);
						return true;
					}
					else
					{
						bt->AddBuilder(radar);
						return false;
					}
				}
			}
		}

		if(checkWater)
		{
			// find radar
			radar = bt->GetRadar(ai->side, cost, range, true, false);

			if(radar && bt->units_dynamic[radar].buildersAvailable <= 0 && bt->units_dynamic[radar].buildersRequested <= 0)
			{
				bt->BuildBuilderFor(radar);
				radar = bt->GetRadar(ai->side, cost, range, true, true);
			}
		
			if(radar)
			{
				pos = (*sector)->GetBuildsite(radar, true);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(radar, pos, true, 10);

					if(builder)
					{
						builder->GiveConstructionOrder(radar, pos, true);
						return true;
					}
					else
					{
						bt->AddBuilder(radar);
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool AAIExecute::BuildJammer()
{
	int jammer = 0;

	float cost = brain->Affordable();
	float range = 10.0 / (cost + 1);

	jammer = bt->GetJammer(ai->side, cost, range, false, false);

	if(jammer && bt->units_dynamic[jammer].buildersAvailable <= 0)
	{
		if(bt->units_dynamic[jammer].buildersRequested <= 0)
			bt->BuildBuilderFor(jammer);

		jammer = bt->GetJammer(ai->side, cost, range, false, true);
	}
	
	if(jammer)
	{
		AAIConstructor *builder = ut->FindBuilder(jammer, true, 10);

		if(builder)
		{
			AAISector *best_sector = 0;
			float best_rating = 0, my_rating;
			
			for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); sector++)
			{
				if((*sector)->unitsOfType[STATIONARY_JAMMER] == 0 && ((*sector)->unitsOfType[STATIONARY_CONSTRUCTOR] > 0 || (*sector)->unitsOfType[POWER_PLANT] > 0) )
					my_rating = (*sector)->GetOverallThreat(1, 1);
				else 
					my_rating = 0;

				if(my_rating > best_rating)
				{
					best_rating = my_rating;
					best_sector = *sector;
				}
			}

			// find centered spot in that sector
			if(best_sector)
			{
				float3 pos = best_sector->GetCenterBuildsite(jammer, false);

				if(pos.x > 0)
				{
					builder->GiveConstructionOrder(jammer, pos, false);
					futureRequestedEnergy += (bt->unitList[jammer-1]->energyUpkeep - bt->unitList[jammer-1]->energyMake);
				}
			}
		}
		else
			return false;
	}

	return true;
}

void AAIExecute::DefendMex(int mex, int def_id)
{
	if(ai->activeFactories < cfg->MIN_FACTORIES_FOR_DEFENCES)
		return;

	float3 pos = cb->GetUnitPos(mex);
	float3 base_pos = brain->base_center;

	int x = pos.x/map->xSectorSize;
	int y = pos.z/map->ySectorSize;

	if(x >= 0 && y >= 0 && x < map->xSectors && y < map->ySectors)
	{
		AAISector *sector = &map->sector[x][y];

		if(sector->distance_to_base > 0 && sector->distance_to_base <= cfg->MAX_MEX_DEFENCE_DISTANCE && sector->allied_structures < 500)
		{
			int defence = 0;
			bool water;

			// get defence building dependend on water or land mex
			if(bt->unitList[def_id-1]->minWaterDepth > 0)
			{
				water = true;

				if(cfg->AIR_ONLY_MOD)
					defence = bt->GetDefenceBuilding(ai->side, 2, 1, 1, 1, 0.5, 0, 0, 0, 4, 0.1, 1, true, true);
				else
					defence = bt->GetDefenceBuilding(ai->side, 2, 1, 1, 0, 0, 0, 1.5, 0.5, 4, 0.1, 1, true, true); 
			}
			else 
			{
				if(cfg->AIR_ONLY_MOD)
					defence = bt->GetDefenceBuilding(ai->side, 2, 1, 1, 1, 0.5, 0, 0, 0, 4, 0.1, 1, false, true);
				else
				{
					if(map->mapType == AIR_MAP)
						defence = bt->GetDefenceBuilding(ai->side, 2, 1, 1, 0, 2, 0, 0, 0, 4, 0.1, 1, false, true); 
					else
						defence = bt->GetDefenceBuilding(ai->side, 2, 1, 1, 1.5, 0.5, 0, 0, 0, 4, 0.1, 3, false, true); 
				}
				
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
				pos = cb->ClosestBuildSite(bt->unitList[defence-1], pos, 1400.0, 2);

				if(pos.x > 0)
				{
					AAIConstructor *builder;

					if(brain->sectors[0].size() > 2)
						builder = ut->FindClosestBuilder(defence, pos, false, 10);
					else
						builder = ut->FindClosestBuilder(defence, pos, true, 10);

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
	metalSurplus[counter] = cb->GetMetalIncome() - cb->GetMetalUsage();
	if(metalSurplus[counter] < 0) metalSurplus[counter] = 0;

	energySurplus[counter] = cb->GetEnergyIncome() - cb->GetEnergyUsage();
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
	counter = (++counter)%8;
}

void AAIExecute::CheckStationaryArty()
{
	if(cfg->MAX_STAT_ARTY == 0)
		return;

	if(ai->futureUnits[STATIONARY_ARTY] > 0)
		return;

	if(ai->activeUnits[STATIONARY_ARTY] >= cfg->MAX_STAT_ARTY)
		return;

	float temp = 0.05f;

	if(temp > urgency[STATIONARY_ARTY])
		urgency[STATIONARY_ARTY] = temp;
}

void AAIExecute::CheckBuildques()
{
	int req_units = 0;
	int active_factory_types = 0;
	
	for(int i = 0; i < numOfFactories; ++i)
	{
		// sum up builque lengths of active factory types
		if(bt->units_dynamic[factory_table[i]].active > 0)
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

			fprintf(ai->file, "Increasing unit production rate to %i\n", unitProductionRate);
		}
		else if( (float)req_units / (float)active_factory_types > (float)cfg->MAX_BUILDQUE_SIZE / 1.5f )
		{
			if(unitProductionRate > 1)
			{
				--unitProductionRate;
				fprintf(ai->file, "Decreasing unit production rate to %i\n", unitProductionRate);
			}
		}
	}
}

void AAIExecute::CheckDefences()
{
	if(ai->activeFactories < cfg->MIN_FACTORIES_FOR_DEFENCES || ai->futureUnits[STATIONARY_DEF] > 2)
		return;

	int t = brain->GetGamePeriod();

	int max_dist = 1;

	double rating, highest_rating = 0;

	AAISector *first = 0, *second = 0;
	UnitCategory cat1 = UNKNOWN, cat2 = UNKNOWN;

	for(int dist = 0; dist <= max_dist; ++dist)
	{
		for(list<AAISector*>::iterator sector = brain->sectors[dist].begin(); sector != brain->sectors[dist].end(); ++sector)
		{
			// stop building further defences if maximum has been reached / sector contains allied buildings / is occupied by another aai instance
			if((*sector)->defences.size() < cfg->MAX_DEFENCES && (*sector)->allied_structures < 100 && map->team_sector_map[(*sector)->x][(*sector)->y] != cb->GetMyAllyTeam())
			{
				if((*sector)->failed_defences > 1)
				{
					(*sector)->failed_defences = 0;
				}
				else
				{
					for(list<int>::iterator cat = map->map_categories_id.begin(); cat!= map->map_categories_id.end(); ++cat)
					{
						// anti air defences may be built anywhere
						if(cfg->AIR_ONLY_MOD || *cat == AIR_ASSAULT)
							rating = (1.0f + sqrt((*sector)->own_structures)) * (1.0f + bt->attacked_by_category[1][*cat][t]) * (*sector)->GetThreatByID(*cat, learned, current) / ( (*sector)->GetDefencePowerVsID(*cat) );
						// dont build anti ground/hover/sea defences in interior sectors
						else if(!(*sector)->interior)
							rating = (1.0f + sqrt((*sector)->own_structures)) * (1.0f + bt->attacked_by_category[1][*cat][t]) * (*sector)->GetThreatByID(*cat, learned, current) / ( (*sector)->GetDefencePowerVsID(*cat));		
						else
							rating = 0;
								
						if(rating > highest_rating)
						{
							// dont block empty sectors with too much aa
							if(bt->GetAssaultCategoryOfID(*cat) != AIR_ASSAULT || ((*sector)->unitsOfType[POWER_PLANT] > 0 || (*sector)->unitsOfType[STATIONARY_CONSTRUCTOR] > 0 ) )
							{
								second = first;
								cat2 = cat1;

								first = *sector;
								cat1 = bt->GetAssaultCategoryOfID(*cat);

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
			float temp = 0.3 + 6.0 / ( (float) first->defences.size() + 1.0f); 

			if(urgency[STATIONARY_DEF] < temp)
				urgency[STATIONARY_DEF] = temp;

			next_defence = first;
			def_category = cat1;
		}
		else if(status == BUILDORDER_NOBUILDPOS)
		{
			++first->failed_defences;
		}
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
	if(ai->activeUnits[STORAGE] + ai->futureUnits[STORAGE] < cfg->MAX_STORAGE 
		&& ai->activeFactories >= cfg->MIN_FACTORIES_FOR_STORAGE)
	{
		float temp = max(GetMetalStorageUrgency(), GetEnergyStorageUrgency());
		
		if(temp > urgency[STORAGE])
			urgency[STORAGE] = temp;
	}

	// energy low
	if(averageEnergySurplus < 1.5 * cfg->METAL_ENERGY_RATIO)
	{
		// try to accelerate power plant construction
		if(ai->futureUnits[POWER_PLANT] > 0)
			AssistConstructionOfCategory(POWER_PLANT, 10);

		// try to disbale some metal makers
		if((ai->activeUnits[METAL_MAKER] - disabledMMakers) > 0)
		{
			for(set<int>::iterator maker = ut->metal_makers.begin(); maker != ut->metal_makers.end(); maker++)
			{
				if(cb->IsUnitActivated(*maker))
				{
					Command c;
					c.id = CMD_ONOFF;
					c.params.push_back(0);
					//cb->GiveOrder(*maker, &c);
					GiveOrder(&c, *maker, "ToggleMMaker");

					futureRequestedEnergy += cb->GetUnitDef(*maker)->energyUpkeep;
					++disabledMMakers;
					break;
				}
			}
		}
	}
	// try to enable some metal makers
	else if(averageEnergySurplus > cfg->MIN_METAL_MAKER_ENERGY && disabledMMakers > 0)
	{
		for(set<int>::iterator maker = ut->metal_makers.begin(); maker != ut->metal_makers.end(); maker++)
		{
			if(!cb->IsUnitActivated(*maker))
			{
				float usage = cb->GetUnitDef(*maker)->energyUpkeep;

				if(averageEnergySurplus > usage * 0.7f)
				{
					Command c;
					c.id = CMD_ONOFF;
					c.params.push_back(1);
					//cb->GiveOrder(*maker, &c);
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
		if(ai->futureUnits[EXTRACTOR] > 0)
			AssistConstructionOfCategory(EXTRACTOR, 10);

		// try to accelerate mex construction
		if(ai->futureUnits[METAL_MAKER] > 0 && averageEnergySurplus > cfg->MIN_METAL_MAKER_ENERGY)
			AssistConstructionOfCategory(METAL_MAKER, 10);
	}
}

void AAIExecute::CheckMexUpgrade()
{
	float cost = 0.25f + brain->Affordable() / 8.0f;
	float eff = 6.0f / (cost + 0.75f);

	const UnitDef *my_def; 
	const UnitDef *land_def = 0;
	const UnitDef *water_def = 0; 

	float gain, highest_gain = 0;
	AAIMetalSpot *best_spot = 0;

	int my_team = cb->GetMyTeam();

	int land_mex = bt->GetMex(ai->side, cost, eff, false, false, false);

	if(land_mex && bt->units_dynamic[land_mex].buildersAvailable <= 0)
	{
		bt->BuildBuilderFor(land_mex);

		land_mex = bt->GetMex(ai->side, cost, eff, false, false, true);
	}

	int water_mex = bt->GetMex(ai->side, cost, eff, false, true, false);

	if(water_mex && bt->units_dynamic[water_mex].buildersAvailable <= 0)
	{
		bt->BuildBuilderFor(water_mex);

		water_mex = bt->GetMex(ai->side, cost, eff, false, true, true);
	}

	if(land_mex)
		land_def = bt->unitList[land_mex-1];

	if(water_mex)
		water_def = bt->unitList[water_mex-1];	

	// check extractor upgrades
	for(int dist = 0; dist < 2; ++dist)
	{
		for(list<AAISector*>::iterator sector = brain->sectors[dist].begin(); sector != brain->sectors[dist].end(); ++sector)
		{
			for(list<AAIMetalSpot*>::iterator spot = (*sector)->metalSpots.begin(); spot != (*sector)->metalSpots.end(); ++spot)
			{
				// quit when finding empty spots
				if(!(*spot)->occupied && (*sector)->enemy_structures <= 0  && (*sector)->lost_units[MOBILE_CONSTRUCTOR-COMMANDER] < 0.2)
					return;
				
				if((*spot)->extractor_def > 0 && (*spot)->extractor > -1 && (*spot)->extractor < cfg->MAX_UNITS 
					&& cb->GetUnitTeam((*spot)->extractor) == my_team)	// only upgrade own extractors
				{
					my_def = bt->unitList[(*spot)->extractor_def-1];

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
		AAIConstructor *builder = ut->FindClosestAssister(best_spot->pos, 10, true, bt->GetAllowedMovementTypesForAssister(best_spot->extractor_def) ); 

		if(builder)
			builder->GiveReclaimOrder(best_spot->extractor);
	}
}


void AAIExecute::CheckRadarUpgrade()
{
	if(ai->futureUnits[STATIONARY_RECON] > 0)
		return;

	float cost = brain->Affordable();
	float range = 10.0f / (cost + 1.0f);

	const UnitDef *my_def; 
	const UnitDef *land_def = 0;
	const UnitDef *water_def = 0; 

	int land_recon = bt->GetRadar(ai->side, cost, range, false, true);
	int water_recon = bt->GetRadar(ai->side, cost, range, true, true);

	if(land_recon)
		land_def = bt->unitList[land_recon-1];

	if(water_recon)
		water_def = bt->unitList[water_recon-1];	

	// check radar upgrades
	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
	{
		for(set<int>::iterator recon = ut->recon.begin(); recon != ut->recon.end(); ++recon)
		{
			my_def = cb->GetUnitDef(*recon);

			if(my_def)
			{
				if(my_def->minWaterDepth <= 0)	// land recon
				{
					if(land_def && my_def->radarRadius < land_def->radarRadius)
					{
						// better radar found, clear buildpos
						AAIConstructor *builder = ut->FindClosestAssister(cb->GetUnitPos(*recon), 10, true, bt->GetAllowedMovementTypesForAssister(my_def->id) );

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
						AAIConstructor *builder = ut->FindClosestAssister(cb->GetUnitPos(*recon), 10, true, bt->GetAllowedMovementTypesForAssister(my_def->id) );

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
}

void AAIExecute::CheckJammerUpgrade()
{
	if(ai->futureUnits[STATIONARY_JAMMER] > 0)
		return;

	float cost = brain->Affordable();
	float range = 10.0 / (cost + 1);

	const UnitDef *my_def; 
	const UnitDef *land_def = 0;
	const UnitDef *water_def = 0; 

	int land_jammer = bt->GetJammer(ai->side, cost, range, false, true);
	int water_jammer = bt->GetJammer(ai->side, cost, range, true, true);

	if(land_jammer)
		land_def = bt->unitList[land_jammer-1];

	if(water_jammer)
		water_def = bt->unitList[water_jammer-1];	

	// check radar upgrades
	for(set<int>::iterator jammer = ut->jammers.begin(); jammer != ut->jammers.end(); ++jammer)
	{
		my_def = cb->GetUnitDef(*jammer);

		if(my_def)
		{
			if(my_def->minWaterDepth <= 0)	// land jammer
			{
				if(land_def && my_def->jammerRadius < land_def->jammerRadius)
				{
					// better jammer found, clear buildpos
					AAIConstructor *builder = ut->FindClosestAssister(cb->GetUnitPos(*jammer), 10, true, bt->GetAllowedMovementTypesForAssister(my_def->id) );

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
					AAIConstructor *builder = ut->FindClosestAssister(cb->GetUnitPos(*jammer), 10, true, bt->GetAllowedMovementTypesForAssister(my_def->id) );
				
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
	float surplus = averageEnergySurplus + futureAvailableEnergy * 0.5;
	
	if(surplus < 0)
		surplus = 0;

	if(ai->activeUnits[POWER_PLANT] > 8)
	{
		if(averageEnergySurplus > 1000)
			return 0;
		else
			return 8.0 / pow( surplus / cfg->METAL_ENERGY_RATIO + 2.0f, 2.0f);
	}
	else if(ai->activeUnits[POWER_PLANT] > 0)
		return 15.0 / pow( surplus / cfg->METAL_ENERGY_RATIO + 2.0f, 2.0f);
	else
		return 6.0;
}

float AAIExecute::GetMetalUrgency()
{
	if(ai->activeUnits[EXTRACTOR] > 0)
		return 20.0 / pow(averageMetalSurplus * cfg->METAL_ENERGY_RATIO + 2.0f, 2.0f);
	else
		return 6.5;
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
	if(averageMetalSurplus > 2.0f && (cb->GetMetalStorage() + futureStoredMetal - cb->GetMetal()) < 100.0f)
		return 0.3f;
	else
		return 0;
}

void AAIExecute::CheckFactories()
{
	if(ai->futureUnits[STATIONARY_CONSTRUCTOR] > 0)
		return;

	for(list<int>::iterator fac = bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].begin(); fac != bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].end(); ++fac)
	{
		if(bt->units_dynamic[*fac].requested > 0)
		{
			// at least one requested factory has not been built yet
			float urgency;

			if(ai->activeFactories > 0)
				urgency = 0.3f;
			else
				urgency = 3.0f;

			if(this->urgency[STATIONARY_CONSTRUCTOR] < urgency)
				this->urgency[STATIONARY_CONSTRUCTOR] = urgency;

			return;
		}
	}
}

void AAIExecute::CheckRecon()
{
	if(ai->activeFactories < cfg->MIN_FACTORIES_FOR_RADAR_JAMMER
		|| ai->activeUnits[STATIONARY_RECON] >= brain->sectors[0].size())
		return;

	float urgency = 1.0f / (ai->activeUnits[STATIONARY_RECON]+1);

	if(this->urgency[STATIONARY_RECON] < urgency)
		this->urgency[STATIONARY_RECON] = urgency;
}

void AAIExecute::CheckAirBase()
{
	if(cfg->MAX_AIR_BASE > 0)
	{
		// only build repair pad if any air units have been built yet
		if(ai->activeUnits[AIR_BASE] + ai->futureUnits[AIR_BASE] < cfg->MAX_AIR_BASE && ai->group_list[AIR_ASSAULT].size() > 0)
			urgency[AIR_BASE] = 0.5f;	
	}
}

void AAIExecute::CheckJammer()
{
	if(ai->activeFactories < 2 || ai->activeUnits[STATIONARY_JAMMER] > brain->sectors[0].size())
	{
		this->urgency[STATIONARY_JAMMER] = 0;
	}
	else
	{
		float temp = 0.2f / ((float) (ai->activeUnits[STATIONARY_JAMMER]+1)) + 0.05f;

		if(urgency[STATIONARY_JAMMER] < temp)
			urgency[STATIONARY_JAMMER] = temp;
	}
}

void AAIExecute::CheckConstruction()
{	
	UnitCategory category = UNKNOWN; 
	float highest_urgency = 0.3f;		// min urgency (prevents aai from building things it doesnt really need that much)
	bool construction_started = false;

	//fprintf(ai->file, "\n");
	// get category with highest urgency  
	for(int i = 1; i <= METAL_MAKER; ++i)
	{
		//fprintf(ai->file, "%-20s: %f\n", bt->GetCategoryString2((UnitCategory)i), urgency[i]);
		if(urgency[i] > highest_urgency)
		{
			highest_urgency = urgency[i];
			category = (UnitCategory)i;
		}
	}
	
	//cb->SendTextMsg( bt->GetCategoryString2(category), 0);

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
		if(BuildRecon())
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
	else 
	{
		construction_started = true;
	}

	if(construction_started)
		urgency[category] = 0;

	for(int i = 1; i <= METAL_MAKER; ++i)
	{
		urgency[i] *= 1.035f;
		
		if(urgency[i] > 18.0f)
			urgency[i] -= 2.0f;
	}
}

bool AAIExecute::AssistConstructionOfCategory(UnitCategory category, int importance)
{
	AAIConstructor *builder, *assistant;

	for(list<AAIBuildTask*>::iterator task = ai->build_tasks.begin(); task != ai->build_tasks.end(); ++task)
	{
		if((*task)->builder_id >= 0)
			builder = ut->units[(*task)->builder_id].cons;
		else
			builder = NULL;

		if(builder && builder->construction_category == category && builder->assistants.size() < cfg->MAX_ASSISTANTS)
		{
			assistant = ut->FindClosestAssister(builder->build_pos, 5, true, bt->GetAllowedMovementTypesForAssister(builder->construction_def_id));

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

bool AAIExecute::least_dangerous(AAISector *left, AAISector *right)
{
	if(cfg->AIR_ONLY_MOD || left->map->mapType == AIR_MAP)
		return (left->GetThreatBy(AIR_ASSAULT, learned, current) < right->GetThreatBy(AIR_ASSAULT, learned, current));  
	else
	{
		if(left->map->mapType == LAND_MAP)
			return ((left->GetThreatBy(GROUND_ASSAULT, learned, current) + left->GetThreatBy(AIR_ASSAULT, learned, current) + left->GetThreatBy(HOVER_ASSAULT, learned, current)) 
					< (right->GetThreatBy(GROUND_ASSAULT, learned, current) + right->GetThreatBy(AIR_ASSAULT, learned, current) + right->GetThreatBy(HOVER_ASSAULT, learned, current)));  
	
		else if(left->map->mapType == LAND_WATER_MAP)
		{
			return ((left->GetThreatBy(GROUND_ASSAULT, learned, current) + left->GetThreatBy(SEA_ASSAULT, learned, current) + left->GetThreatBy(AIR_ASSAULT, learned, current) + left->GetThreatBy(HOVER_ASSAULT, learned, current)) 
					< (right->GetThreatBy(GROUND_ASSAULT, learned, current) + right->GetThreatBy(SEA_ASSAULT, learned, current) + right->GetThreatBy(AIR_ASSAULT, learned, current) + right->GetThreatBy(HOVER_ASSAULT, learned, current)));  
		}
		else if(left->map->mapType == WATER_MAP)
		{
			return ((left->GetThreatBy(SEA_ASSAULT, learned, current) + left->GetThreatBy(AIR_ASSAULT, learned, current) + left->GetThreatBy(HOVER_ASSAULT, learned, current)) 
					< (right->GetThreatBy(SEA_ASSAULT, learned, current) + right->GetThreatBy(AIR_ASSAULT, learned, current) + right->GetThreatBy(HOVER_ASSAULT, learned, current)));  
		} else throw "AAIExecute::least_dangerous: invalid mapType";
	}
}

bool AAIExecute::suitable_for_power_plant(AAISector *left, AAISector *right)
{
	if(cfg->AIR_ONLY_MOD || left->map->mapType == AIR_MAP)
		return (left->GetThreatBy(AIR_ASSAULT, learned, current) * left->GetMapBorderDist()  < right->GetThreatBy(AIR_ASSAULT, learned, current) * right->GetMapBorderDist());  
	else
	{
		if(left->map->mapType == LAND_MAP)
			return ((left->GetThreatBy(GROUND_ASSAULT, learned, current) + left->GetThreatBy(AIR_ASSAULT, learned, current) + left->GetThreatBy(HOVER_ASSAULT, learned, current) * left->GetMapBorderDist() ) 
					< (right->GetThreatBy(GROUND_ASSAULT, learned, current) + right->GetThreatBy(AIR_ASSAULT, learned, current) + right->GetThreatBy(HOVER_ASSAULT, learned, current) * right->GetMapBorderDist()));  
	
		else if(left->map->mapType == LAND_WATER_MAP)
		{
			return ((left->GetThreatBy(GROUND_ASSAULT, learned, current) + left->GetThreatBy(SEA_ASSAULT, learned, current) + left->GetThreatBy(AIR_ASSAULT, learned, current) + left->GetThreatBy(HOVER_ASSAULT, learned, current) * left->GetMapBorderDist() ) 
					< (right->GetThreatBy(GROUND_ASSAULT, learned, current) + right->GetThreatBy(SEA_ASSAULT, learned, current) + right->GetThreatBy(AIR_ASSAULT, learned, current) + right->GetThreatBy(HOVER_ASSAULT, learned, current) * right->GetMapBorderDist()));  
		}
		else if(left->map->mapType == WATER_MAP)
		{
			return ((left->GetThreatBy(SEA_ASSAULT, learned, current) + left->GetThreatBy(AIR_ASSAULT, learned, current) + left->GetThreatBy(HOVER_ASSAULT, learned, current) * left->GetMapBorderDist()) 
					< (right->GetThreatBy(SEA_ASSAULT, learned, current) + right->GetThreatBy(AIR_ASSAULT, learned, current) + right->GetThreatBy(HOVER_ASSAULT, learned, current) * right->GetMapBorderDist()));  
		} else throw "AAIExecute::suitable_for_power_plant: invalid mapType";
	}
}

bool AAIExecute::suitable_for_ground_factory(AAISector *left, AAISector *right)
{
	return ( (4 * (left->flat_ratio - left->water_ratio) + 2 * left->GetMapBorderDist())   
			> (4 * (right->flat_ratio - right->water_ratio) + 2 * right->GetMapBorderDist()) );
}

bool AAIExecute::suitable_for_sea_factory(AAISector *left, AAISector *right)
{
	return ( (4 * left->water_ratio + 2 * left->GetMapBorderDist())   
			> (4 * right->water_ratio + 2 * right->GetMapBorderDist()) );
}

bool AAIExecute::suitable_for_arty(AAISector *left, AAISector *right)
{
	float left_rating = 3.0f * left->GetMapBorderDist();
	float right_rating = 3.0f * right->GetMapBorderDist();

	if(!left->interior)
		left_rating += 3.0f;

	if(!right->interior)
		right_rating += 3.0f;

	return ( left_rating / sqrt(left->own_structures+ 1.0f) >  right_rating / sqrt(right->own_structures+ 1.0f) );
}

bool AAIExecute::suitable_for_ground_rallypoint(AAISector *left, AAISector *right)
{
	return ( (left->flat_ratio  + 0.5 * left->GetMapBorderDist())/ ((float) (left->rally_points + 1) )
		>  (right->flat_ratio  + 0.5 * right->GetMapBorderDist())/ ((float) (right->rally_points + 1) ) );
}

bool AAIExecute::suitable_for_sea_rallypoint(AAISector *left, AAISector *right)
{
	return ( (left->water_ratio  + 0.5 * left->GetMapBorderDist())/ ((float) (left->rally_points + 1) )
		>  (right->water_ratio  + 0.5 * right->GetMapBorderDist())/ ((float) (right->rally_points + 1) ) );
}

bool AAIExecute::suitable_for_all_rallypoint(AAISector *left, AAISector *right)
{
	return ( (left->flat_ratio + left->water_ratio + 0.5 * left->GetMapBorderDist())/ ((float) (left->rally_points + 1) )
		>  (right->flat_ratio + right->water_ratio + 0.5 * right->GetMapBorderDist())/ ((float) (right->rally_points + 1) ) );
}

bool AAIExecute::defend_vs_ground(AAISector *left, AAISector *right)
{
	return ((2 + 2 * left->GetThreatBy(GROUND_ASSAULT, learned, current)) / (left->GetDefencePowerVs(GROUND_ASSAULT)+ 1))
		>  ((2 + 2 * right->GetThreatBy(GROUND_ASSAULT, learned, current)) / (right->GetDefencePowerVs(GROUND_ASSAULT)+ 1));
}

bool AAIExecute::defend_vs_air(AAISector *left, AAISector *right)
{
	return ((2 + 2 * left->GetThreatBy(AIR_ASSAULT, learned, current)) / (left->GetDefencePowerVs(AIR_ASSAULT)+ 1))
		>  ((2 + 2 * right->GetThreatBy(AIR_ASSAULT, learned, current)) / (right->GetDefencePowerVs(AIR_ASSAULT)+ 1));
}

bool AAIExecute::defend_vs_hover(AAISector *left, AAISector *right)
{
	return ((2 + 2 * left->GetThreatBy(HOVER_ASSAULT, learned, current)) / (left->GetDefencePowerVs(HOVER_ASSAULT)+ 1))
		>  ((2 + 2 * right->GetThreatBy(HOVER_ASSAULT, learned, current)) / (right->GetDefencePowerVs(HOVER_ASSAULT)+ 1));

}

bool AAIExecute::defend_vs_sea(AAISector *left, AAISector *right)
{
	return ((2 + 2 * left->GetThreatBy(SEA_ASSAULT, learned, current)) / (left->GetDefencePowerVs(SEA_ASSAULT)+ 1))
		>  ((2 + 2 * right->GetThreatBy(SEA_ASSAULT, learned, current)) / (right->GetDefencePowerVs(SEA_ASSAULT)+ 1));

}

void AAIExecute::ConstructionFinished()
{
}

void AAIExecute::ConstructionFailed(float3 build_pos, int def_id)
{
	const UnitDef *def = bt->unitList[def_id-1];
	UnitCategory category = bt->units_static[def_id].category;

	int x = build_pos.x/map->xSectorSize;
	int y = build_pos.z/map->ySectorSize;

	bool validSector = false;

	if(x >= 0 && y >= 0 && x < map->xSectors && y < map->ySectors)
		validSector = true;

	// decrease number of units of that category in the target sector 
	if(validSector)
	{
		--map->sector[x][y].unitsOfType[category];
		map->sector[x][y].own_structures -= bt->units_static[def->id].cost;

		if(map->sector[x][y].own_structures < 0)
			map->sector[x][y].own_structures = 0;
	}
	
	// free metalspot if mex was odered to be built
	if(category == EXTRACTOR && build_pos.x > 0)
	{
		map->sector[x][y].FreeMetalSpot(build_pos, def);
	}
	else if(category == POWER_PLANT)
	{
		futureAvailableEnergy -= bt->units_static[def_id].efficiency[0];

		if(futureAvailableEnergy < 0)
			futureAvailableEnergy = 0;
	}
	else if(category == STORAGE)
	{
		futureStoredEnergy -= bt->unitList[def->id-1]->energyStorage;
		futureStoredMetal -= bt->unitList[def->id-1]->metalStorage;
	}
	else if(category == METAL_MAKER)
	{
		futureRequestedEnergy -= bt->unitList[def->id-1]->energyUpkeep;

		if(futureRequestedEnergy < 0)
			futureRequestedEnergy = 0;
	}
	else if(category == STATIONARY_JAMMER)
	{
		futureRequestedEnergy -= bt->units_static[def->id].efficiency[0];

		if(futureRequestedEnergy < 0)
			futureRequestedEnergy = 0;
	}
	else if(category == STATIONARY_RECON)
	{
		futureRequestedEnergy -= bt->units_static[def->id].efficiency[0];

		if(futureRequestedEnergy < 0)
			futureRequestedEnergy = 0;
	}
	else if(category == STATIONARY_DEF)
	{
		// remove defence from map				
		map->RemoveDefence(&build_pos, def->id); 
	}
	
	// clear buildmap
	if(category == STATIONARY_CONSTRUCTOR)
	{
		bool water = false;
		int value = 0;

		if(!bt->CanPlacedLand(def_id))
		{
			water = true;
			value = 4;
		}

		--ai->futureFactories;

		for(list<int>::iterator unit = bt->units_static[def->id].canBuildList.begin();  unit != bt->units_static[def->id].canBuildList.end(); ++unit)		
			bt->units_dynamic[*unit].buildersRequested -= 1;

		// remove future ressource demand now factory has been finished
		futureRequestedMetal -= bt->units_static[def->id].efficiency[0];
		futureRequestedEnergy -= bt->units_static[def->id].efficiency[1];

		// update buildmap of sector
		map->Pos2BuildMapPos(&build_pos, def);
		map->CheckRows(build_pos.x, build_pos.z, def->xsize, def->ysize, false, water);
		
		map->SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->ysize, value);
		map->BlockCells(build_pos.x, build_pos.z - 8, def->xsize, 8, false, water);
		map->BlockCells(build_pos.x + def->xsize, build_pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, false, water);
		map->BlockCells(build_pos.x, build_pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, false, water);
	}
	else // normal building
	{
		map->Pos2BuildMapPos(&build_pos, def);
			
		if(def->minWaterDepth <= 0)
		{
			map->SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->ysize, 0);
			map->CheckRows(build_pos.x, build_pos.z, def->xsize, def->ysize, false, false);
		}
		else
		{
				map->SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->ysize, 4);
				map->CheckRows(build_pos.x, build_pos.z, def->xsize, def->ysize, false, true);
		}
	}
}

void AAIExecute::AddStartFactory()
{
	float best_rating = 0, my_rating;
	int best_factory = 0;

	for(list<int>::iterator fac = bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].begin(); fac != bt->units_of_category[STATIONARY_CONSTRUCTOR][ai->side-1].end(); ++fac)
	{
		if(bt->units_dynamic[*fac].buildersAvailable > 0)
		{
			my_rating = bt->GetFactoryRating(*fac);
			my_rating *= (1 - (bt->units_static[*fac].cost / bt->max_cost[STATIONARY_CONSTRUCTOR][ai->side-1]));

			if(my_rating > best_rating)
			{
				best_rating = my_rating;
				best_factory = *fac;
			}
		}
	}

	if(best_factory)
	{
		bt->units_dynamic[best_factory].requested += 1;

		fprintf(ai->file, "%s requested\n", bt->unitList[best_factory-1]->humanName.c_str());

		for(list<int>::iterator j = bt->units_static[best_factory].canBuildList.begin(); j != bt->units_static[best_factory].canBuildList.end(); ++j)
			bt->units_dynamic[*j].buildersRequested += 1;
	}
}


AAIGroup* AAIExecute::GetClosestGroupOfCategory(UnitCategory category, UnitType type, float3 pos, int importance)
{
	AAIGroup *best_group = 0;
	float best_rating = 0, my_rating;
	float3 group_pos; 

	for(list<AAIGroup*>::iterator group = ai->group_list[category].begin(); group != ai->group_list[category].end(); ++group)
	{
		if((*group)->group_unit_type == type && !(*group)->attack)
		{
			if((*group)->task == GROUP_IDLE || (*group)->task_importance < importance)
			{
				group_pos = (*group)->GetGroupPos();

				my_rating = (*group)->avg_speed / sqrt(1 +  pow(pos.x - group_pos.x, 2.0f) + pow(pos.z - group_pos.z, 2.0f) );

				if(my_rating > best_rating)
				{
					best_group = *group;
					best_rating = my_rating;
				}
			}

		}
	}

	return best_group;
}

void AAIExecute::DefendUnitVS(int unit, const UnitDef *def, UnitCategory category, float3 *enemy_pos, int importance)
{
	if(unit < 0)
		return;

	float3 pos = cb->GetUnitPos(unit);

	AAISector *sector = map->GetSectorOfPos(&pos);

	// unit/building located in invalid sector
	if(!sector)
		return;

	AAIGroup *support = 0;

	// anti air mods have special behaviour
	if(cfg->AIR_ONLY_MOD)
	{
		support = GetClosestGroupOfCategory(AIR_ASSAULT, ASSAULT_UNIT, pos, 100);

		if(!support)
			support = GetClosestGroupOfCategory(HOVER_ASSAULT, ASSAULT_UNIT, pos, 100);

		if(!support)
			support = GetClosestGroupOfCategory(GROUND_ASSAULT, ASSAULT_UNIT, pos, 100);

		if(support)
			support->Defend(unit, enemy_pos, importance);
	}
	// normal mods
	else  
	{
		bool land, water;

		if(sector->water_ratio > 0.6)
		{
			land = false;
			water = true;
		}
		else
		{
			land = true;
			water = false;
		}

		// find possible defenders depending on category of attacker

		// anti air 
		if(category == AIR_ASSAULT)
		{
			// try to call fighters first
			support = GetClosestGroupOfCategory(AIR_ASSAULT, ANTI_AIR_UNIT, pos, 100);
				
			// no fighters available
			if(!support)
			{	
				// try to get ground or hover aa support
				if(land)
				{
					support = GetClosestGroupOfCategory(GROUND_ASSAULT, ANTI_AIR_UNIT, pos, 100);

					if(!support)
						support = GetClosestGroupOfCategory(HOVER_ASSAULT, ANTI_AIR_UNIT, pos, 100);
				}

				// try to get sea or hover aa support
				if(water)
				{
					support = GetClosestGroupOfCategory(SEA_ASSAULT, ANTI_AIR_UNIT, pos, 100);

					if(!support)
						support = GetClosestGroupOfCategory(HOVER_ASSAULT, ANTI_AIR_UNIT, pos, 100);
				}
			}

			if(support)
			{
				// dont defend units too far away from own base
				if(sector->distance_to_base > 3 && support->category != AIR_ASSAULT)
					return;
	
				support->Defend(unit, NULL, importance);
			}
		}
		// ground unit attacked
		else if(land)
		{
			if(category == GROUND_ASSAULT || category == HOVER_ASSAULT || category == GROUND_ARTY || category == HOVER_ARTY)
			{
				support = GetClosestGroupOfCategory(AIR_ASSAULT, ASSAULT_UNIT, pos, 100);

				if(!support)
					support = GetClosestGroupOfCategory(GROUND_ASSAULT, ASSAULT_UNIT, pos, 100);

				if(!support)
					support = GetClosestGroupOfCategory(HOVER_ASSAULT, ASSAULT_UNIT, pos, 100);
			}
			else if(category == SEA_ASSAULT || category == SEA_ARTY)
			{
				support = GetClosestGroupOfCategory(AIR_ASSAULT, ASSAULT_UNIT, pos, 100);

				if(!support)
					support = GetClosestGroupOfCategory(SEA_ASSAULT, ASSAULT_UNIT, pos, 100);

				if(!support)
					support = GetClosestGroupOfCategory(HOVER_ASSAULT, ASSAULT_UNIT, pos, 100);
			}

			if(support)
			{
				// dont defend units too far away from own base
				if(sector->distance_to_base > 3 && support->category != AIR_ASSAULT)
					return;

				support->Defend(unit, enemy_pos, importance);
			}
		}
		// water unit attacked
		else if(water)
		{
			if(category == GROUND_ASSAULT || category == GROUND_ARTY)
			{
				support = GetClosestGroupOfCategory(AIR_ASSAULT, ASSAULT_UNIT, pos, 100);

				if(!support)
					support = GetClosestGroupOfCategory(GROUND_ASSAULT, ASSAULT_UNIT, pos, 100);

				if(!support)
					support = GetClosestGroupOfCategory(HOVER_ASSAULT, ASSAULT_UNIT, pos, 100);
			}
			else if(category == SEA_ASSAULT || category == HOVER_ASSAULT || category == SEA_ARTY || category == HOVER_ARTY)
			{
				support = GetClosestGroupOfCategory(AIR_ASSAULT, ASSAULT_UNIT, pos, 100);

				if(!support)
					support = GetClosestGroupOfCategory(SEA_ASSAULT, ASSAULT_UNIT, pos, 100);

				if(!support)
					support = GetClosestGroupOfCategory(HOVER_ASSAULT, ASSAULT_UNIT, pos, 100);
			}

			if(support)
			{
				// dont defend units too far away from own base
				if(sector->distance_to_base > 3 && support->category != AIR_ASSAULT)
					return;

				support->Defend(unit, enemy_pos, importance);
			}
		}	
	}
}

float3 AAIExecute::GetSafePos(int def_id)
{
	// determine if land or water pos needed
	bool land = false;
	bool water = false;

	if(cfg->AIR_ONLY_MOD)
	{
		land = true; 
		water = true;
	}
	else
	{
		if(bt->unitList[def_id-1]->movedata)
		{
			if(bt->unitList[def_id-1]->movedata->moveType == MoveData::Ground_Move)
				land = true;
			else if(bt->unitList[def_id-1]->movedata->moveType == MoveData::Ship_Move)
				water = true;
			else // hover 
			{
				land = true; 
				water = true;
			}
		}
		else // air
		{
			land = true; 
			water = true;
		}
	}

	return GetSafePos(land, water);
}

float3 AAIExecute::GetSafePos(bool land, bool water)
{
	// check all base sectors
	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
	{
		if((*sector)->threat < 1)
		{
			if( (land && (*sector)->water_ratio < 0.7) || (water && (*sector)->water_ratio > 0.35)) 
				return (*sector)->GetCenter();
		}
	}

	return ZeroVector;
}

void AAIExecute::ChooseDifferentStartingSector(int x, int y)
{
	// get possible start sectors
	list<AAISector*> sectors;

	if(x >= 1)
	{
		sectors.push_back( &map->sector[x-1][y] );

		if(y >= 1)
			sectors.push_back( &map->sector[x-1][y-1] );

		if(y < map->ySectors-1)
			sectors.push_back( &map->sector[x-1][y+1] );
	}

	if(x < map->xSectors-1)
	{
		sectors.push_back( &map->sector[x+1][y] );

		if(y >= 1)
			sectors.push_back( &map->sector[x+1][y-1] );

		if(y < map->ySectors-1)
			sectors.push_back( &map->sector[x+1][y+1] );
	}

	if(y >= 1)
		sectors.push_back( &map->sector[x][y-1] );

	if(y < map->ySectors-1)
		sectors.push_back( &map->sector[x][y+1] );

	// choose best
	AAISector *best_sector = 0;
	float my_rating, best_rating = 0;

	for(list<AAISector*>::iterator sector = sectors.begin(); sector != sectors.end(); ++sector)
	{
		if(map->team_sector_map[(*sector)->x][(*sector)->y] != -1)
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
		brain->AddSector(best_sector);
		brain->start_pos = best_sector->GetCenter();

		brain->UpdateNeighbouringSectors();
		brain->UpdateBaseCenter();
	}
}

void AAIExecute::CheckFallBack(int unit_id, int def_id)
{
	float range = bt->units_static[def_id].range;

	if(range > cfg->MIN_FALLBACK_RANGE)
	{
		float3 pos;

		GetFallBackPos(&pos, unit_id, range);

		if(pos.x > 0)
		{	
			Command c;
			c.id = CMD_MOVE;
			c.params.resize(3);

			c.params[0] = pos.x;
			c.params[1] = cb->GetElevation(pos.x, pos.z);
			c.params[2] = pos.z;

			//cb->GiveOrder(unit_id, &c);
			GiveOrder(&c, unit_id, "Fallback");
		}
	}
}


void AAIExecute::GetFallBackPos(float3 *pos, int unit_id, float range)
{
	float3 unit_pos = cb->GetUnitPos(unit_id), temp_pos;

	*pos = ZeroVector; 

	// get list of enemies within weapons range
	int number_of_enemies = cb->GetEnemyUnits(&(map->unitsInLos.front()), unit_pos, range * cfg->FALLBACK_DIST_RATIO);

	if(number_of_enemies > 0)
	{
		float dist;

		for(int k = 0; k < number_of_enemies; ++k)
		{
			// get distance to enemy 
			temp_pos = cb->GetUnitPos(map->unitsInLos[k]);

			dist = fastmath::sqrt( (temp_pos.x - unit_pos.x) * (temp_pos.x - unit_pos.x) - (temp_pos.z - unit_pos.z) * (temp_pos.z - unit_pos.z) );

			// get dir from unit to enemy
			temp_pos.x -= unit_pos.x;
			temp_pos.z -= unit_pos.z;

			// 
			pos->x += (dist / range - 1) * temp_pos.x;
			pos->z += (dist / range - 1) * temp_pos.z;
		}

		pos->x /= (float)number_of_enemies;
		pos->z /= (float)number_of_enemies;

		pos->x += unit_pos.x;
		pos->z += unit_pos.z;
	}	
}

void AAIExecute::GiveOrder(Command *c, int unit, const char *owner)
{
	//++issued_orders;

	//if(issued_orders%50 == 0)
	//	fprintf(ai->file, "%i th order has been given by %s in frame %i\n", issued_orders, owner,  cb->GetCurrentFrame());

	cb->GiveOrder(unit, c);
}