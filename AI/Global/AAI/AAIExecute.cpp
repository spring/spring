//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------

#include "AAIExecute.h"

// all the static vars
float AAIExecute::current = 0.5;
float AAIExecute::learned = 2.5;


AAIExecute::AAIExecute(AAI *ai, AAIBrain *brain)
{
	this->ai = ai;
	this->cb = ai->cb;
	this->bt = ai->bt;
	this->brain = brain;
	this->map = ai->map;
	this->ut = ai->ut;

	brain->execute = this;

	buildques = 0;
	factory_table = 0;
	numOfFactories = 0;
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

	for(int i = 0; i <= METAL_MAKER+1; ++i)
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
	if(buildques)
	{
		for(int i = 0; i < numOfFactories; ++i)
			buildques[i].clear();

		delete [] buildques;
	}

	if(factory_table)
		delete [] factory_table;
}

void AAIExecute::moveUnitTo(int unit, float3 *position)
{
	Command c;

	c.id = CMD_MOVE;

	c.params.resize(3);
	c.params[0] = position->x;
	c.params[1] = position->y;
	c.params[2] = position->z;

	cb->GiveOrder(unit, &c);
	ut->SetUnitStatus(unit, MOVING);
}

void AAIExecute::stopUnit(int unit)
{
	Command c;
	c.id = CMD_STOP;

	cb->GiveOrder(unit, &c);
	ut->SetUnitStatus(unit, UNIT_IDLE);
}

// returns true if unit is busy
bool AAIExecute::IsBusy(int unit)
{
	const deque<Command> *commands = cb->GetCurrentUnitCommands(unit);

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
		int scout;

		float speed = 1; 
		float los = 0.1;

		// get new scout according to map type
		if(cfg->AIR_ONLY_MOD || map->mapType == AIR_MAP)
		{
			scout = bt->GetScout(ai->side, speed, los,  brain->Affordable(), AIR_SCOUT, 5, true);

			if(!scout)	
			{
				scout = bt->GetScout(ai->side, speed, los,  brain->Affordable(), AIR_SCOUT, 5, false);
				
				if(scout)
				{
					bt->BuildFactoryFor(scout);

					// wait until factory has been finished if one scout of that type has already been requested
					if(bt->units_dynamic[scout].requested > 0)
						scout = 0;
				}
			}
		}
		else
		{
			int random;

			if(map->mapType == LAND_MAP)
			{
				random = rand()%2;

				if(random)
				{
					scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), AIR_SCOUT, 5, true);

					if(!scout)	
					{
						scout = bt->GetScout(ai->side, speed, los,  brain->Affordable(), AIR_SCOUT, 5, false);
						
						if(scout)
						{
							bt->BuildFactoryFor(scout);

							// wait until factory has been finished if one scout of that type has already been requested
							if(bt->units_dynamic[scout].requested > 0)
								scout = 0;
						}
					}
				}
				else
				{
					scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), GROUND_SCOUT, HOVER_SCOUT, 5, true);
				
					if(!scout)	
					{
						scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), GROUND_SCOUT, HOVER_SCOUT, 5, false);
						
						if(scout)
						{
							bt->BuildFactoryFor(scout);

							// wait until factory has been finished if one scout of that type has already been requested
							if(bt->units_dynamic[scout].requested > 0)
								scout = 0;
						}
					}
				}
			}
			else if (map->mapType == LAND_WATER_MAP)
			{
				random = rand()%3;

				if(random == 0)
				{
					scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), AIR_SCOUT, 5, true);
				}
				else if(random == 1)
				{
					scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), SEA_SCOUT, HOVER_SCOUT, 5, true);

					if(!scout)	
					{
						scout = bt->GetScout(ai->side, speed, los, brain->Affordable(),  SEA_SCOUT, HOVER_SCOUT, 5, false);
					
						if(scout)
						{
							bt->BuildFactoryFor(scout);

							// wait until factory has been finished if one scout of that type has already been requested
							if(bt->units_dynamic[scout].requested > 0)
								scout = 0;
						}
					}
				}
				else
				{
					scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), GROUND_SCOUT, HOVER_SCOUT, 5, true);
				
					if(!scout)	
					{
						scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), GROUND_SCOUT, HOVER_SCOUT, 5, false);
						
						if(scout)
						{
							bt->BuildFactoryFor(scout);

							// wait until factory has been finished if one scout of that type has already been requested
							if(bt->units_dynamic[scout].requested > 0)
								scout = 0;
						}
					}
				}
			}
			else
			{
				random = rand()%2;

				if(random)
				{
					scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), AIR_SCOUT, 5, true);

					if(!scout)	
					{
						scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), AIR_SCOUT, 5, false);
						
						if(scout)
						{
							bt->BuildFactoryFor(scout);

							// wait until factory has been finished if one scout of that type has already been requested
							if(bt->units_dynamic[scout].requested > 0)
								scout = 0;
						}
					}
				}
				else
				{
					scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), SEA_SCOUT, HOVER_SCOUT, 5, true);

					if(!scout)	
					{
						scout = bt->GetScout(ai->side, speed, los, brain->Affordable(), SEA_SCOUT, HOVER_SCOUT, 5, false);
						
						if(scout)
						{
							bt->BuildFactoryFor(scout);

							// wait until factory has been finished if one scout of that type has already been requested
							if(bt->units_dynamic[scout].requested > 0)
								scout = 0;
						}
					}
				}
			}
		}

		if(scout)
		{
			if(AddUnitToBuildque(scout, 1, false))
			{
				++ai->futureScouts;
				++bt->units_dynamic[scout].requested;

				//char c[120];
				//sprintf(c, "requested scout: %i", ai->futureScouts); 
				//cb->SendTextMsg(c, 0);
			}
		}
		
	}

	AAISector *dest;
	int scout_x, scout_y;

	// get idle scouts and let them explore the map
	for(list<int>::iterator scout = ai->scouts.begin(); scout != ai->scouts.end(); ++scout)
	{
		if(!IsBusy(*scout))
		{
			// get scout dest
			dest = brain->GetNewScoutDest(*scout);

			// get sector of scout
			pos = cb->GetUnitPos(*scout);
			scout_x = pos.x/map->xSectorSize;
			scout_y = pos.z/map->xSectorSize;

			if(dest->x > scout_x)
				pos.x = (dest->left + 7 * dest->right)/8;
			else if(dest->x < scout_x)
				pos.x = (7 * dest->left + dest->right)/8;
			else
				pos.x = (dest->left + dest->right)/2;

			if(dest->y > scout_y)
				pos.z = (7 * dest->bottom + dest->top)/8;
			else if(dest->y < scout_y)
				pos.z = (dest->bottom + 7 * dest->top)/8;
			else
				pos.z = (dest->bottom + dest->top)/2;

			pos.y = cb->GetElevation(pos.x, pos.z);

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
	for(list<AAISector*>::iterator s = brain->sectors[0].begin(); s != brain->sectors[0].end(); s++)
	{	
		pos = (*s)->GetBuildsite(building);

		// if suitable location found, return pos...
		if(pos.x)	
			return pos;
	}

	// no construction site found 
	//if(!brain->ExpandBase(LAND_SECTOR))
	//	fprintf(ai->file, "PROBLEM: Couldnt find buildsite for %s", bt->unitList[building-1]->humanName.c_str());

	pos.x = pos.y = pos.z = 0;
	return pos;
}

AAIMetalSpot* AAIExecute::FindBaseMetalSpot(float3 builder_pos)
{
	AAIMetalSpot *spot = 0;

	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); sector++)
	{
		spot = (*sector)->GetFreeMetalSpot(builder_pos);

		if(spot)
		{
			//cb->SendTextMsg("Base spot found",0);
			break;
		}
	}

	return spot;
}

AAIMetalSpot* AAIExecute::FindExternalMetalSpot(int land_mex, int water_mex)
{
	AAIMetalSpot *spot, *best_spot = 0;
	float shortest_dist = 1000000.0f, dist;
	float3 builder_pos;
	AAIBuilder *builder;

	// prevent crashes on smaller maps 
	int max_distance = min(cfg->MAX_MEX_DISTANCE, brain->max_distance);

	// look for free spots in all sectors within max range
	for(int sector_dist = 1; sector_dist < max_distance; ++sector_dist)
	{
		for(list<AAISector*>::iterator sector = brain->sectors[sector_dist].begin();sector != brain->sectors[sector_dist].end(); sector++)
		{
			if((*sector)->freeMetalSpots && (*sector)->enemy_structures < 30 &&
				((*sector)->lost_units[GROUND_BUILDER-COMMANDER] + (*sector)->lost_units[AIR_BUILDER-COMMANDER]) < 2.5)
			{
				spot = (*sector)->GetFreeMetalSpot();
				
				if(spot)
				{	
					if(spot->pos.y > 0)
						builder = ut->FindClosestBuilder(land_mex, spot->pos, true, 10);
					else
						builder = ut->FindClosestBuilder(water_mex, spot->pos, true, 10);

					if(builder)
					{
						// get distance to pos
						builder_pos = cb->GetUnitPos(builder->unit_id);

						dist = sqrt( pow((spot->pos.x - builder_pos.x), 2) + pow((spot->pos.z - builder_pos.z),2) ) / bt->unitList[builder->def_id-1]->speed;

						if(dist < shortest_dist)
						{
							best_spot = spot;
							shortest_dist = dist;
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
	UnitCategory category = bt->units_static[def_id].category;

	if(category == UNKNOWN)
		return false;

	list<int> *buildque = 0, *temp = 0;
	
	float my_rating, best_rating = 0;

	for(list<int>::iterator fac = bt->units_static[def_id].builtByList.begin(); fac != bt->units_static[def_id].builtByList.end(); ++fac)
	{
		if(bt->units_dynamic[*fac].active + bt->units_dynamic[*fac].requested > 0)
		{
			temp = GetBuildqueOfFactory(*fac);

			if(temp)
			{
				my_rating = (1 + 2 * (float) bt->units_dynamic[*fac].active) / (temp->size() + 3);

				if(map->mapType == WATER_MAP && bt->units_static[*fac].category == GROUND_FACTORY)
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
		if(bt->IsBuilder(category))
		{	
			// 
			if(bt->units_dynamic[def_id].active + bt->units_dynamic[def_id].requested > 2)
			{
				buildque->insert(buildque->end(), number, def_id);
			}
			else
			{
				// insert at beginning
				if(buildque->empty())
				{
					buildque->insert(buildque->begin(), number, def_id);
				}

				// insert after the last builder
				for(list<int>::iterator unit = buildque->begin(); unit != buildque->end();  ++unit)
				{
					if(!bt->IsBuilder(bt->units_static[*unit].category))
					{
						buildque->insert(unit, number, def_id);
						return true;
					}
				}
			}

			return false;
		}
		else if(bt->IsScout(category))
		{
			if(ai->activeScouts < 2)
			{
				buildque->insert(buildque->begin(), number, def_id);
				return true;
			}
			else
			{
				// insert after the last builder
				for(list<int>::iterator unit = buildque->begin(); unit != buildque->end();  ++unit)
				{
					if(!bt->IsBuilder(bt->units_static[*unit].category))
					{
						buildque->insert(unit, number, def_id);
						return true;
					}
				}
			}
		}
		else if(buildque->size() < cfg->MAX_BUILDQUE_SIZE)
		{
			if(urgent)
			{
				buildque->insert(buildque->begin(), number, def_id);
			}
			else
				buildque->insert(buildque->end(), number, def_id);

			//fprintf(ai->file, "Added %s to buildque with length %i\n", bt->unitList[def_id-1]->humanName.c_str(), buildque->size());

			return true;
		}
	}
	else
	{
		if(bt->IsBuilder(category))
		{
			//cb->SendTextMsg("No buildque",0);
			return false;
		}

		fprintf(ai->file, "Could not find builque for %s\n", bt->unitList[def_id-1]->humanName.c_str());
	}

	return false;
}

void AAIExecute::InitBuildques()
{
	// set up buildques for different types of factories
	numOfFactories = bt->units_of_category[GROUND_FACTORY][ai->side-1].size() + bt->units_of_category[SEA_FACTORY][ai->side-1].size();
	buildques = new list<int>[numOfFactories];

	// set up factory buildque identification
	factory_table = new int[numOfFactories];
	
	int i = 0;
	for(list<int>::iterator factory = bt->units_of_category[GROUND_FACTORY][ai->side-1].begin(); factory != bt->units_of_category[GROUND_FACTORY][ai->side-1].end(); ++factory)
	{
		factory_table[i] = *factory;
		++i;
	}
	for(list<int>::iterator factory = bt->units_of_category[SEA_FACTORY][ai->side-1].begin(); factory != bt->units_of_category[SEA_FACTORY][ai->side-1].end(); ++factory)
	{
		factory_table[i] = *factory;
		++i;
	}
}

float3 AAIExecute::GetRallyPoint(UnitCategory category)
{
	AAISector* best_sector = 0;
	float best_rating = 0, my_rating;
	
	int start = 1;

	if(brain->sectors[0].size() <= 2)
		start = 3;
	else if(brain->sectors[0].size() <= 4)
		start = 2;

	float learned = 70000.0 / (cb->GetCurrentFrame() + 35000) + 1;
	float current = 2.5 - learned;

	// check neighbouring sectors
	//for(int i = 1; i < 2; i++)
	//{
		for(list<AAISector*>::iterator sector = brain->sectors[start].begin(); sector != brain->sectors[start].end(); sector++)
		{
			// get most dangerous sector with flat terrain
			if(category == GROUND_ASSAULT)		
			{
				my_rating = 1 + (current * (*sector)->combats[0][0] + learned * (*sector)->combats[1][0]); 
				my_rating *= ((*sector)->GetMapBorderDist()+0.5) * (*sector)->flat_ratio / (2.0*(*sector)->water_ratio+0.2);
			}
			// get safest sector
			else if(category == AIR_ASSAULT)
			{
				my_rating = (((*sector)->flat_ratio - (*sector)->water_ratio) * 4 + 0.2) / (1 + (*sector)->lost_units[AIR_ASSAULT-COMMANDER]);
			}
			else if(category == HOVER_ASSAULT)
			{
				my_rating = pow((*sector)->flat_ratio, 2) * (*sector)->GetMapBorderDist();

				if(map->mapType == LAND_MAP)
					my_rating *= (current * ((*sector)->combats[0][2] + (*sector)->combats[0][0]) + learned * ((*sector)->combats[1][2] + (*sector)->combats[1][0]));
				else if(map->mapType == LAND_WATER_MAP)
					my_rating *= (current * ((*sector)->combats[0][2] + (*sector)->combats[0][0] + (*sector)->combats[0][3]) + learned * ((*sector)->combats[1][2] + (*sector)->combats[1][0] + (*sector)->combats[1][3]));
				else
					my_rating *= (current * ((*sector)->combats[0][2] + (*sector)->combats[0][3]) + learned * ((*sector)->combats[1][2] + (*sector)->combats[1][3]));
			}
			// get sector with most water
			else if(category == SEA_ASSAULT)
			{
				my_rating = (current * (*sector)->combats[0][3] + learned * (*sector)->combats[1][3]); 
				my_rating *= (2 * (*sector)->GetMapBorderDist()+0.5) * (*sector)->water_ratio;
			}
			else 
				my_rating = 0;

			my_rating *= (2 + rand()%5);

			if(my_rating > best_rating)
			{
				best_rating = my_rating;
				best_sector = *sector;
			}
		}
	//}

	if(best_sector)
		return best_sector->GetCenter();
	else
		return ZeroVector;	
}

// ****************************************************************************************************
// all building functions
// ****************************************************************************************************

bool AAIExecute::BuildExtractor()
{
	if(ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] == 0 && ai->activeUnits[EXTRACTOR] >= 2)
		return true;

	AAIBuilder *builder; 
	AAIMetalSpot *spot;
	float3 pos;
	int mex;
	bool external = false;
	bool water;

	float cost = brain->Affordable();
	float efficiency = 10.0 / (cost + 1);

	// check if metal map
	if(ai->map->metalMap)
	{
		// get id of an extractor and look for suitable builder
		mex = bt->GetMex(ai->side, cost, efficiency, false, false, false);

		if(mex && !bt->units_dynamic[mex].builderAvailable)
		{
			bt->BuildBuilderFor(mex);
			mex = bt->GetMex(ai->side, cost, efficiency, false, false, true);
		}

		builder = ut->FindBuilder(mex, true, 10);

		if(builder)
		{
			pos = GetBuildsite(builder->unit_id, mex, EXTRACTOR);

			if(pos.x != 0)
			{
				builder->GiveBuildOrder(mex, pos, false);
			}
			return true;
		}
		else
		{
			bt->CheckAddBuilder(mex);
			return false;
		}
	}
	else // normal map
	{
		// try to build mex within base
		if(brain->freeBaseSpots)
		{
			spot = FindBaseMetalSpot(pos);

			if(spot)
			{
				int type = map->buildmap[(int) spot->pos.x/8 + (int)(spot->pos.z/8 )* map->xMapSize];
				// check if land or sea spot
				if(type == 5 || type == 4)
					water = true;
				else
					water = false;

				// get id of an extractor and look for suitable builder
				mex = bt->GetMex(ai->side, cost, efficiency, false, false, false);

				if(mex && !bt->units_dynamic[mex].builderAvailable)
				{
					bt->BuildBuilderFor(mex);
					mex = bt->GetMex(ai->side, cost, efficiency, false, false, true);
				}

				if(mex)
				{
					builder = ut->FindClosestBuilder(mex, spot->pos, true, 10);

					if(builder)
					{
						builder->GiveBuildOrder(mex, spot->pos, water);
						spot->occupied = true;
					}
					else
					{
						bt->CheckAddBuilder(mex);
						return false;
					}
				}
			}
			// no free spots in the base anymore
			else
			{
				brain->freeBaseSpots = false;
					
				if(brain->expandable && brain->sectors[0].size() <= 3)
				{
					brain->ExpandBase(LAND_SECTOR);
					//fprintf(ai->file, "Base expanded by BuildExtractor()\n");
				}

				external = false;
			}
		}
		else // try to build extern metalspot
			external = true;

		if(external)
		{
			// get id of an extractor and look for suitable builder
			mex = bt->GetMex(ai->side, 8, 1, true, false, false);

			if(mex && !bt->units_dynamic[mex].builderAvailable)
			{
				bt->BuildBuilderFor(mex);
				mex = bt->GetMex(ai->side, 8, 1, true, false, true);
			}

			int water_mex = bt->GetMex(ai->side, 8, 1, true, true, false);

			if(water_mex && !bt->units_dynamic[water_mex].builderAvailable)
			{
				bt->BuildBuilderFor(water_mex);
				water_mex = bt->GetMex(ai->side, 8, 1, true, true, true);
			}

			// look for empty metal spot
			spot = FindExternalMetalSpot(mex, water_mex);
			
			if(spot)
			{
				// check if land or sea spot
				int type = map->buildmap[(int) spot->pos.x/8 + (int)(spot->pos.z/8 )* map->xMapSize];
	
				if(type == 5 || type == 4)
				{
					water = true;
					mex = water_mex;
				}
				else
					water = false;

				// look for suitable builder
				if(brain->sectors[0].size() > 2)
				{
					int x = spot->pos.x/map->xSectorSize;
					int y = spot->pos.z/map->ySectorSize;

					// only allow commander if spot is close to own base
					if(map->sector[x][y].distance_to_base <= 1)
						builder = ut->FindClosestBuilder(mex, spot->pos, true, 10);
					else
						builder = ut->FindClosestBuilder(mex, spot->pos, false, 10);
				}
				else
					builder = ut->FindClosestBuilder(mex, spot->pos, true, 10);
				

				if(builder)
				{
					builder->GiveBuildOrder(mex, spot->pos, water);
					spot->occupied = true;
				}
				else
				{
					bt->CheckAddBuilder(mex);
					return false;
				}
			}
			else
			{
				if(ai->activeUnits[METAL_MAKER] < cfg->MAX_METAL_MAKERS && urgency[METAL_MAKER] <= 0.6)
					urgency[METAL_MAKER] = 0.4;
			}
		}
	}
	
	return true;
}

bool AAIExecute::BuildPowerPlant()
{	
	if(futureRequestedEnergy < 0)
		futureRequestedEnergy = 0;

	if(ai->futureUnits[POWER_PLANT] > 0)
	{
		// try to assist construction of other power plants first
		if(AssistConstructionOfCategory(POWER_PLANT, 10))
			return true;
	}

	if(ai->futureUnits[POWER_PLANT] >= 2)
		return true;

	if(ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] < 1 && ai->activeUnits[POWER_PLANT] >= 2)
		return true;

	int ground_plant = 0;
	int water_plant = 0;
	
	AAIBuilder *builder;
	float3 pos;

	float current_energy = cb->GetEnergyIncome();
	bool checkWater, checkGround;
	float urgency;
	float max_power;
	float eff;
	float energy = cb->GetEnergyIncome()+1;

	// check if already one power_plant under cosntruction and energy short 
	if(ai->futureUnits[POWER_PLANT] > 0 && ai->activeUnits[POWER_PLANT] > 8 && averageEnergySurplus < 200)
	{
		urgency = 0.5 + GetEnergyUrgency();
		max_power = 0.5;
		eff = 1 + brain->Affordable();
	}
	else
	{
		max_power = 0.5 + pow((float) ai->activeUnits[POWER_PLANT], 0.8f);
		eff = 0.5 + 1.5f / (brain->Affordable() + 0.5f);
		urgency = 0.5 + GetEnergyUrgency()/1.5f;
	}

	// sort sectors according to threat level
	learned = 70000.0 / (cb->GetCurrentFrame() + 35000) + 1;
	current = 2.5 - learned;

	if(ai->activeUnits[POWER_PLANT] >= 2)
		brain->sectors[0].sort(suitable_for_power_plant);

	// get water and ground plant
	ground_plant = bt->GetPowerPlant(ai->side, eff, urgency, max_power, energy, false, false, false);
	// currently aai cannot build this building
	if(ground_plant && !bt->units_dynamic[ground_plant].builderAvailable)
	{
		bt->CheckAddBuilder(ground_plant);
		ground_plant = bt->GetPowerPlant(ai->side, eff, urgency, max_power, energy, false, false, true);
	}

	water_plant = bt->GetPowerPlant(ai->side, eff, urgency, max_power, energy, true, false, false);
	// currently aai cannot build this building
	if(water_plant && !bt->units_dynamic[water_plant].builderAvailable)
	{
		bt->CheckAddBuilder(water_plant);
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

		if(checkGround && ground_plant)
		{
			if(ut->builders.size() > 1 || ai->activeUnits[POWER_PLANT] >= 2)
				pos = (*sector)->GetBuildsite(ground_plant, false);
			else 
			{
				builder = ut->FindBuilder(ground_plant, true, 10);

				if(builder)
				{
					pos = map->GetClosestBuildsite(bt->unitList[ground_plant-1], cb->GetUnitPos(builder->unit_id), 40, false);

					if(pos.x <= 0)
						pos = (*sector)->GetBuildsite(ground_plant, false);
				}
				else
					pos = (*sector)->GetBuildsite(ground_plant, false);
			}

			if(pos.x > 0)
			{
				builder = ut->FindClosestBuilder(ground_plant, pos, true, 10);

				if(builder)
				{
					futureAvailableEnergy += bt->units_static[ground_plant].efficiency[0];
					builder->GiveBuildOrder(ground_plant, pos, false);
					return true;
				}
				else
				{
					bt->CheckAddBuilder(ground_plant);
					return false;
				}
			}
			else
			{
				brain->ExpandBase(LAND_SECTOR);
				//fprintf(ai->file, "Base expanded by BuildPowerPlant()\n");
			}
		}

		if(checkWater && water_plant)
		{
			if(ut->builders.size() > 1 || ai->activeUnits[POWER_PLANT] >= 2)
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
					builder->GiveBuildOrder(water_plant, pos, true);
					return true;
				}
				else
				{
					bt->CheckAddBuilder(water_plant);
					return false;
				}
			}
			else
				brain->ExpandBase(WATER_SECTOR);
		}
	}

	return true;
}

bool AAIExecute::BuildMetalMaker()
{
	if(ai->futureUnits[METAL_MAKER] > 0 || disabledMMakers >= 1)
		return true;

	bool checkWater, checkGround;
	int maker;
	AAIBuilder *builder;
	float3 pos;
	// urgency < 4
	float urgency = 16.0 / (ai->activeUnits[METAL_MAKER]+ai->futureUnits[METAL_MAKER]+4);

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
			maker = bt->GetMetalMaker(ai->side, brain->Affordable(),  8.0/(urgency+2.0), 64.0/(16*urgency+2.0), urgency, false, false); 
	
			// currently aai cannot build this building
			if(maker && !bt->units_dynamic[maker].builderAvailable)
			{
				bt->CheckAddBuilder(maker);
				maker = bt->GetMetalMaker(ai->side, brain->Affordable(),  8.0/(urgency+2.0), 64.0/(16*urgency+2.0), urgency, false, true);
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
						builder->GiveBuildOrder(maker, pos, false);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(maker);
						return false;
					}
				}
			}
		}

		if(checkWater)
		{
			//cb->SendTextMsg("water maker",0);
			maker = bt->GetMetalMaker(ai->side, brain->Affordable(),  8.0/(urgency+2.0), 64.0/(16*urgency+2.0), urgency, true, false); 
	
			// currently aai cannot build this building
			if(maker && !bt->units_dynamic[maker].builderAvailable)
			{
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
						builder->GiveBuildOrder(maker, pos, true);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(maker);
						return false;
					}
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

	if(ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] < 2)
		return true;

	int storage = 0;
	bool checkWater, checkGround;
	AAIBuilder *builder;
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
	
			if(storage && !bt->units_dynamic[storage].builderAvailable)
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
						builder->GiveBuildOrder(storage, pos, false);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(storage);
						return false;
					}
				}
				else
				{
					brain->ExpandBase(LAND_SECTOR);
					//fprintf(ai->file, "Base expanded by BuildStorage()\n");
				}
			}
		}

		if(checkWater)
		{
			storage = bt->GetStorage(ai->side, brain->Affordable(),  metal, energy, 1, true, false); 
	
			if(storage && !bt->units_dynamic[storage].builderAvailable)
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
						builder->GiveBuildOrder(storage, pos, true);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(storage);
						return false;
					}
				}
				else
					brain->ExpandBase(WATER_SECTOR);
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
	AAIBuilder *builder;
	float3 pos;

	cb->SendTextMsg("building airbase", 0);

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
	
			if(airbase && !bt->units_dynamic[airbase].builderAvailable)
			{
				bt->BuildBuilderFor(airbase);
				airbase = bt->GetAirBase(ai->side, brain->Affordable(), false, true);
			}

			cb->SendTextMsg("ground airbase", 0);

			if(airbase)
			{
				cb->SendTextMsg(bt->unitList[airbase-1]->humanName.c_str(), 0);

				pos = (*sector)->GetBuildsite(airbase, false);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(airbase, pos, true, 10);
	
					if(builder)
					{
						builder->GiveBuildOrder(airbase, pos, false);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(airbase);
						return false;
					}
				}
				else
				{
					brain->ExpandBase(LAND_SECTOR);
					//fprintf(ai->file, "Base expanded by BuildAirBase()\n");
				}
			}
		}

		if(checkWater)
		{
			airbase = bt->GetAirBase(ai->side, brain->Affordable(), true, false); 
	
			if(airbase && !bt->units_dynamic[airbase].builderAvailable)
			{
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
						builder->GiveBuildOrder(airbase, pos, true);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(airbase);
						return false;
					}
				}
				else
					brain->ExpandBase(WATER_SECTOR);
			}
		}
	}
		
	return true;
}

bool AAIExecute::BuildDefences()
{
	if(ai->futureUnits[STATIONARY_DEF] > 3)
		return true;

	AAISector *dest = 0;
	BuildOrderStatus status;

	learned = 70000.0 / (cb->GetCurrentFrame() + 35000) + 1;
	current = 2.5 - learned;

	//
	if(cfg->AIR_ONLY_MOD)
	{
		brain->sectors[0].sort(defend_vs_ground);

		for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
		{
			status = BuildStationaryDefenceVS(GROUND_ASSAULT, *sector);

			if(!(status == BUILDORDER_NOBUILDPOS))
				break;
		}
	
		brain->sectors[0].sort(defend_vs_air);

		for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
		{
			status = BuildStationaryDefenceVS(AIR_ASSAULT, *sector);

			if(!(status == BUILDORDER_NOBUILDPOS))
				break;
		}	

	
		brain->sectors[0].sort(defend_vs_hover);

		for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
		{
			status = BuildStationaryDefenceVS(HOVER_ASSAULT, *sector);

			if(!(status == BUILDORDER_NOBUILDPOS))
				break;
		}	
	}
	else
	{
		// check for ground defence
		if(map->mapType == LAND_MAP || map->mapType == LAND_WATER_MAP)
		{
			brain->sectors[0].sort(defend_vs_ground);

			for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
			{
				status = BuildStationaryDefenceVS(GROUND_ASSAULT, *sector);

				if(!(status == BUILDORDER_NOBUILDPOS))
					break;
			}
		}

		brain->sectors[0].sort(defend_vs_air);

		for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
		{
			// prevent aai from blocking recently added sectors with  aa defences befor something else has been built
			if((*sector)->unitsOfType[GROUND_FACTORY] > 0 || (*sector)->unitsOfType[POWER_PLANT] > 0)
			{
				status = BuildStationaryDefenceVS(AIR_ASSAULT, *sector);

				if(!(status == BUILDORDER_NOBUILDPOS))
					break;
			}
		}	

		if(map->mapType == WATER_MAP || map->mapType == LAND_WATER_MAP)
		{
			brain->sectors[0].sort(defend_vs_sea);

			for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
			{
				status = BuildStationaryDefenceVS(SEA_ASSAULT, *sector);

				if(!(status == BUILDORDER_NOBUILDPOS))
					break;
			}	
		}
	}

	return true;
}

BuildOrderStatus AAIExecute::BuildStationaryDefenceVS(UnitCategory category, AAISector *dest)
{
	// dont build too many defences 
	if(dest->defences.size() > 7 && dest->GetThreatBy(category, learned, current) / (2 * dest->GetDefencePowerVs(category)+ 2) < cfg->MIN_SECTOR_THREAT)
		return BUILDORDER_NOBUILDPOS;

	float gr_eff = 0, air_eff = 0, hover_eff = 0, sea_eff = 0, submarine_eff = 0;

	bool checkWater, checkGround;
	int building;
	float3 pos;
	AAIBuilder *builder;

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

	float urgency = 25.0 / (2 * dest->defences.size() + dest->GetDefencePowerVs(category) + 3.0) + 0.3;
	float power = 0.5 + 1.0f / urgency;
	float eff = 0.5 + brain->Affordable()/2.0f;
	float range = 0.5;

	if(dest->defences.size() >= ((float)cfg->MAX_DEFENCES )/3.0f)
	{
		if(rand()%3 == 1)
			range = 3;
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
		if(rand()%cfg->LEARN_RATE == 1 && dest->defences.size() >= 3)
			building = bt->GetRandomDefence(ai->side, category);
		else
			building = bt->GetDefenceBuilding(ai->side, eff, power, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff,  urgency, range, 5, false, false);

		if(building && !bt->units_dynamic[building].builderAvailable)
		{
			bt->BuildBuilderFor(building);
			building = bt->GetDefenceBuilding(ai->side, eff, power, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, range, 5, false, true);
		}
				
		if(building)
		{
			pos = dest->GetDefenceBuildsite(building, category, false);
				
			if(pos.x > 0)
			{
				builder = ut->FindClosestBuilder(building, pos, true, 10);	

				if(builder)
				{
					builder->GiveBuildOrder(building, pos, false);
					return BUILDORDER_SUCCESFUL;
				}
				else
				{
					bt->CheckAddBuilder(building);	
					return BUILDORDER_NOBUILDER;
				}
			}
			else
				return BUILDORDER_NOBUILDPOS;
		}	
	}

	if(checkWater)
	{
		if(rand()%cfg->LEARN_RATE == 1 && dest->defences.size() >= 3)
			building = bt->GetRandomDefence(ai->side, category);
		else
			building = bt->GetDefenceBuilding(ai->side, eff, power, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, 1, 5, true, false);

		if(building && !bt->units_dynamic[building].builderAvailable)
		{
			bt->BuildBuilderFor(building);
			building = bt->GetDefenceBuilding(ai->side, eff, power, gr_eff, air_eff, hover_eff, sea_eff, submarine_eff, urgency, 1,  5, true, true);
		}

		if(building)
		{
			pos = dest->GetDefenceBuildsite(building, category, true);
				
			if(pos.x > 0)
			{
				builder = ut->FindClosestBuilder(building, pos, true, 10);

				if(builder)
				{
					builder->GiveBuildOrder(building, pos, true);
					return BUILDORDER_SUCCESFUL;
				}
				else
				{
					bt->CheckAddBuilder(building);
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

	AAIBuilder *builder;
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
				if(!bt->units_dynamic[arty].builderAvailable)
				{
					bt->BuildBuilderFor(arty);
					return true;
				}

				pos = (*sector)->GetHighestBuildsite(arty);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(arty, pos, true, 10);

					if(builder)
					{
						builder->GiveBuildOrder(arty, pos, false);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(arty);
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
				if(!bt->units_dynamic[arty].builderAvailable)
				{
					bt->BuildBuilderFor(arty);
					return true;
				}

				pos = (*sector)->GetBuildsite(arty, true);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(arty, pos, true, 10);

					if(builder)
					{
						builder->GiveBuildOrder(arty, pos, true);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(arty);
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
	if(ai->futureUnits[GROUND_FACTORY] + ai->futureUnits[SEA_FACTORY] > 0)
		return true;

	AAIBuilder *builder = 0, *temp_builder;
	float3 pos;
	float best_rating = 0, my_rating;
	int building = 0;
	

	if( !(map->mapType == 4 && ai->activeUnits[SEA_FACTORY] == 0 && !bt->units_of_category[SEA_FACTORY][ai->side-1].empty()) )
	{
		// go through list of factories, build cheapest requested factory first
		for(list<int>::iterator fac = bt->units_of_category[GROUND_FACTORY][ai->side-1].begin(); fac != bt->units_of_category[GROUND_FACTORY][ai->side-1].end(); ++fac)
		{
			if(bt->units_static[*fac].efficiency[4] >= 2)
			{	
				bt->units_static[*fac].efficiency[4] = 0;
			}
			else if(bt->units_dynamic[*fac].requested - bt->units_dynamic[*fac].active > 0)	
			{
				my_rating = bt->GetFactoryRating(*fac) / pow( (float) 1 + bt->units_dynamic[*fac].active, 2.0f);

				if(ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] < 1)
					my_rating /= bt->units_static[*fac].cost;

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
	}

	// check sea factories if water map
	if(map->mapType > 2)
	{
		for(list<int>::iterator fac = bt->units_of_category[SEA_FACTORY][ai->side-1].begin(); fac != bt->units_of_category[SEA_FACTORY][ai->side-1].end(); ++fac)
		{
			if(bt->units_static[*fac].efficiency[4] >= 2)
			{	
				bt->units_static[*fac].efficiency[4] = 0;
			}
			else if(bt->units_dynamic[*fac].requested - bt->units_dynamic[*fac].active > 0)	
			{
				my_rating = bt->GetFactoryRating(*fac);

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
	}


	if(building)
	{
		bool water;
		
		// sort sectors 
		
		if(bt->units_static[building].category == GROUND_FACTORY)
		{
			if(ai->activeUnits[GROUND_FACTORY] > 0)
				brain->sectors[0].sort(suitable_for_ground_factory);
			
			water = false;
		}
		else
		{
			if(ai->activeUnits[SEA_FACTORY] > 0)
				brain->sectors[0].sort(suitable_for_sea_factory);
				
			water = true;
		}
		
		// find buildpos
		for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
		{
			pos = (*sector)->GetRandomBuildsite(building, 20, water);

			if(pos.x > 0)
				break;
		}

		if(pos.x == 0)
		{
			for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
			{
				pos = (*sector)->GetBuildsite(building, water);

				if(pos.x > 0)
					break;
			}
		}

			
		if(pos.x > 0)
		{
			builder = ut->FindClosestBuilder(building, pos, true, 10);
			
			if(builder)
			{
				// give build order
				builder->GiveBuildOrder(building, pos, water);
					
				// add average ressoruce usage
				futureRequestedMetal += bt->units_static[building].efficiency[0];
				futureRequestedEnergy += bt->units_static[building].efficiency[1];
			}
			else
			{
				if(bt->units_dynamic[building].builderRequested == 0)
					bt->BuildBuilderFor(building);
					
				return false;
			}
		}
		else
		{
			// no suitable buildsite found
			if(bt->units_static[building].category == GROUND_FACTORY)
			{
				brain->ExpandBase(LAND_SECTOR);
				//fprintf(ai->file, "Base expanded by BuildFactory()\n");
			}
			else
			{
				brain->ExpandBase(WATER_SECTOR);
				//fprintf(ai->file, "Base expanded by BuildFactory()\n");
			}

			// could not build due to lack of suitable buildpos
			++bt->units_static[building].efficiency[4];

			return false;
		}
	}

	return true;
}

void AAIExecute::BuildUnit(UnitCategory category, float speed, float cost, float range, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, bool urgent)
{
	int unit = 0;

	if(category == GROUND_ASSAULT)
	{
		// choose random unit (to learn more)
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[GROUND_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetGroundAssault(ai->side, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, speed, range, cost, 15, false);

			if(unit && !bt->units_dynamic[unit].builderAvailable)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetGroundAssault(ai->side, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, speed, range, cost, 15, true);
			}
		}
	}
	else if(category == AIR_ASSAULT)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[AIR_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetAirAssault(ai->side, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, speed, range, cost, 9, false);

			if(unit && !bt->units_dynamic[unit].builderAvailable)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetAirAssault(ai->side, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, speed, range, cost, 9, true);
			}
		}
	}
	else if(category == HOVER_ASSAULT)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[HOVER_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetHoverAssault(ai->side, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, speed, range, cost, 9, false);

			if(unit && !bt->units_dynamic[unit].builderAvailable)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetHoverAssault(ai->side, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, speed, range, cost, 9, true);
			}
		}
	}
	else if(category == SEA_ASSAULT)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[SEA_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetSeaAssault(ai->side, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, speed, range, cost, 9, false);

			if(unit && !bt->units_dynamic[unit].builderAvailable)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetSeaAssault(ai->side, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, speed, range, cost, 9, false);
			}
		}
	}
	else if(category == SUBMARINE_ASSAULT)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = bt->GetRandomUnit(bt->units_of_category[SUBMARINE_ASSAULT][ai->side-1]);
		else
		{
			unit = bt->GetSubmarineAssault(ai->side, sea_eff, submarine_eff, stat_eff, speed, range, cost, 9, false);

			if(unit && !bt->units_dynamic[unit].builderAvailable)
			{
				bt->BuildFactoryFor(unit);
				unit = bt->GetSubmarineAssault(ai->side, sea_eff, submarine_eff, stat_eff, speed, range, cost, 9, false);
			}
		}
	}


	if(unit)
	{
		//cb->SendTextMsg(bt->unitList[unit-1]->humanName.c_str(), 0);

		if(bt->units_dynamic[unit].builderAvailable)
		{	
			if(bt->units_static[unit].cost < cfg->MAX_COST_LIGHT_ASSAULT * bt->max_cost[category][ai->side-1])
				AddUnitToBuildque(unit, 3, urgent);
			else if(bt->units_static[unit].cost < cfg->MAX_COST_MEDIUM_ASSAULT * bt->max_cost[category][ai->side-1])
				AddUnitToBuildque(unit, 2, urgent);
			else
				AddUnitToBuildque(unit, 1, urgent);	
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
	AAIBuilder *builder;
	float3 pos;
	bool checkWater, checkGround;

	
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
			radar = bt->GetRadar(ai->side, brain->Affordable(), 0.5, false, false);

			if(radar && !bt->units_dynamic[radar].builderAvailable)
			{
				bt->BuildBuilderFor(radar);
				radar = bt->GetRadar(ai->side, brain->Affordable(), 0.5, false, true);
			}
		
			if(radar)
			{
				pos = (*sector)->GetHighestBuildsite(radar);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(radar, pos, true, 10);

					if(builder)
					{
						builder->GiveBuildOrder(radar, pos, false);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(radar);
						return false;
					}
				}
			}
		}

		if(checkWater)
		{
			// find radar
			radar = bt->GetRadar(ai->side, brain->Affordable(), 0.5, true, false);

			if(radar && !bt->units_dynamic[radar].builderAvailable)
			{
				bt->BuildBuilderFor(radar);
				radar = bt->GetRadar(ai->side, brain->Affordable(), 0.5, true, true);
			}
		
			if(radar)
			{
				pos = (*sector)->GetBuildsite(radar, true);

				if(pos.x > 0)
				{
					builder = ut->FindClosestBuilder(radar, pos, true, 10);

					if(builder)
					{
						builder->GiveBuildOrder(radar, pos, true);
						return true;
					}
					else
					{
						bt->CheckAddBuilder(radar);
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

	// find radar
	jammer = bt->GetJammer(ai->side, brain->Affordable(), 0.5, false, false);

	if(jammer && !bt->units_dynamic[jammer].builderAvailable)
	{
		bt->BuildBuilderFor(jammer);
		jammer = bt->GetJammer(ai->side, brain->Affordable(), 0.5, false, true);
	}
	
	if(jammer)
	{
		AAIBuilder *builder = ut->FindBuilder(jammer, true, 10);

		if(builder)
		{
			AAISector *best_sector = 0;
			float best_rating = 0, my_rating;
			
			for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); sector++)
			{
				
				if(!(*sector)->unitsOfType[STATIONARY_JAMMER] && ((*sector)->unitsOfType[GROUND_FACTORY] > 0 || (*sector)->unitsOfType[POWER_PLANT] > 0) )
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
					builder->GiveBuildOrder(jammer, pos, false);
					futureRequestedEnergy += (bt->unitList[jammer-1]->energyUpkeep - bt->unitList[jammer-1]->energyMake);
					return true;
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
	float3 pos = cb->GetUnitPos(mex);
	float3 base_pos = brain->base_center;

	int x = pos.x/map->xSectorSize;
	int y = pos.z/map->ySectorSize;

	if(x >= 0 && y >= 0 && x < map->xSectors && y < map->ySectors)
	{
		AAISector *sector = &map->sector[x][y];

		if(sector->distance_to_base > 0 && sector->distance_to_base <= cfg->MAX_MEX_DEFENCE_DISTANCE && sector->allied_structures < 600)
		{
			int defence = 0;
			bool water;

			// get defence building dependend on water or land mex
			if(bt->unitList[def_id-1]->minWaterDepth > 0)
			{
				water = true;
				defence = bt->GetDefenceBuilding(ai->side, 3, 0.5, 0, 0, 0, 1.5, 0.5, 6, 0.1, 1, true, true); 
			}
			else 
			{
				if(map->mapType == AIR_MAP)
					defence = bt->GetDefenceBuilding(ai->side, 3, 0.5, 0, 2, 0, 0, 0, 6, 0.1, 1, false, true); 
				else
					defence = bt->GetDefenceBuilding(ai->side, 3, 0.5, 1.5, 0.5, 0, 0, 0, 6, 0.1, 3, false, true); 
				
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
					AAIBuilder *builder = ut->FindClosestBuilder(defence, pos, false, 10);

					if(builder)
						builder->GiveBuildOrder(defence, pos, water);	
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

	averageEnergySurplus /= 8;
	averageMetalSurplus /= 8;

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

	float temp = 0.05;

	if(temp > urgency[STATIONARY_ARTY])
		urgency[STATIONARY_ARTY] = temp;
}

void AAIExecute::CheckUnits()
{
	/*float temp;
	
	if(cfg->AIR_ONLY_MOD)
		temp = 48.0 / (ai->activeUnits[AIR_ASSAULT]+ai->futureUnits[AIR_ASSAULT] + 16);
	else
		temp = 48.0 / (ai->activeUnits[GROUND_ASSAULT]+ai->futureUnits[GROUND_ASSAULT] + 16);

	if(temp < urgency[METAL_MAKER+1])
		urgency[METAL_MAKER+1] = temp;*/
}

void AAIExecute::CheckBuildques()
{
	int req_units = 0;
	int active_factories = 0;

	for(int i = 0; i < numOfFactories; ++i)
	{
		// sum up builque lengths of active factory types
		if(bt->units_dynamic[factory_table[i]].active)
		{
			req_units += buildques[i].size();
			++active_factories;
		}
	}

	if(active_factories > 0)
	{
		if( (float)req_units / (float)active_factories < cfg->MAX_BUILDQUE_SIZE / 5.0 )
		{
			if(unitProductionRate < 70)
				++unitProductionRate;

			fprintf(ai->file, "Increasing unit production rate to %i\n", unitProductionRate);
		}
		else if( (float)req_units / (float)active_factories > cfg->MAX_BUILDQUE_SIZE / 2.0 )
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
	if(ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] < cfg->MIN_FACTORIES_FOR_DEFENCES)
		return;

	float defences = 1;

	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
		defences += (*sector)->defences.size();

	// stop building further defences if maximum has been reached
	if(defences < brain->sectors[0].size() * cfg->MAX_DEFENCES)
	{ 
		float urgency = 2.0 / (defences / brain->sectors[0].size()) + 0.1;
		
		if(urgency > this->urgency[STATIONARY_DEF])
			this->urgency[STATIONARY_DEF] = urgency;
	}
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
		&& ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] >= cfg->MIN_FACTORIES_FOR_STORAGE)
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
					cb->GiveOrder(*maker, &c);

					futureRequestedEnergy += cb->GetUnitDef(*maker)->energyUpkeep;
					++disabledMMakers;
					break;
				}
			}
		}
	}
	// try to enbale some metal makers
	else if(averageEnergySurplus > cfg->MIN_METAL_MAKER_ENERGY && disabledMMakers > 0)
	{
		for(set<int>::iterator maker = ut->metal_makers.begin(); maker != ut->metal_makers.end(); maker++)
		{
			if(!cb->IsUnitActivated(*maker))
			{
				float usage = cb->GetUnitDef(*maker)->energyUpkeep;

				if(averageEnergySurplus > usage * 0.7)
				{
					Command c;
					c.id = CMD_ONOFF;
					c.params.push_back(1);
					cb->GiveOrder(*maker, &c);

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
	if(brain->freeBaseSpots)
		return;

	//cb->SendTextMsg("upgrading mex",0);

	float cost = brain->Affordable();
	float eff = 10.0 / (cost + 1);

	const UnitDef *my_def; 
	const UnitDef *land_def = 0;
	const UnitDef *water_def = 0; 

	int land_mex = bt->GetMex(ai->side, cost, eff, false, false, true);
	int water_mex = bt->GetMex(ai->side, cost, eff, false, true, true);

	if(land_mex)
		land_def = bt->unitList[land_mex-1];

	if(water_mex)
		water_def = bt->unitList[water_mex-1];	

	// check extractor upgrades
	for(list<AAISector*>::iterator sector = brain->sectors[0].begin(); sector != brain->sectors[0].end(); ++sector)
	{
		for(list<AAIMetalSpot*>::iterator spot = (*sector)->metalSpots.begin(); spot != (*sector)->metalSpots.end(); ++spot)
		{
			if((*spot)->extractor_def > 0 && (*spot)->extractor > -1 && (*spot)->extractor < cfg->MAX_UNITS)
			{
				my_def = bt->unitList[(*spot)->extractor_def-1];

				if(my_def->minWaterDepth <= 0 && land_def)	// land mex
				{
					if(land_def->extractsMetal - my_def->extractsMetal > 0.0001)
					{
						// better mex found, clear buildpos
						//Command c;
						//c.id = CMD_SELFD;

						//cb->GiveOrder((*spot)->extractor, &c);
						//return;
						AAIBuilder *builder = ut->FindAssistBuilder((*spot)->pos, 10, false, false);

						if(builder)
						{
							builder->GiveReclaimOrder((*spot)->extractor);
							return;
						}
					}
				}
				else	// water mex
				{
					if(water_def && water_def->extractsMetal - my_def->extractsMetal > 0.0001)
					{
						// better mex found, clear buildpos
						/*Command c;
						c.id = CMD_SELFD;

						cb->GiveOrder((*spot)->extractor, &c);
						return;*/

						AAIBuilder *builder = ut->FindAssistBuilder((*spot)->pos, 10, true, bt->unitList[(*spot)->extractor_def-1]->floater);

						if(builder)
						{
							builder->GiveReclaimOrder((*spot)->extractor);
							return;
						}
					}
				}
			}
		}
	}
}


void AAIExecute::CheckRadarUpgrade()
{
	if(ai->futureUnits[STATIONARY_RECON] > 0)
		return;

	float cost = brain->Affordable();
	float range = 10.0 / (cost + 1);

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
						AAIBuilder *builder = ut->FindAssistBuilder(cb->GetUnitPos(*recon), 10, false, false);

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
						AAIBuilder *builder = ut->FindAssistBuilder(cb->GetUnitPos(*recon), 10, true, my_def->floater);

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
					AAIBuilder *builder = ut->FindAssistBuilder(cb->GetUnitPos(*jammer), 10, false, false);

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
					AAIBuilder *builder = ut->FindAssistBuilder(cb->GetUnitPos(*jammer), 10, true, my_def->floater);
				
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
	float surplus = averageEnergySurplus + futureAvailableEnergy * 0.35;
	if(surplus < 0)
		surplus = 0;

	if(ai->activeUnits[POWER_PLANT] > 0)
		return 18.0 / pow( surplus / cfg->METAL_ENERGY_RATIO + 2.0f, 2.0f);
	else
		return 6.0;
}

float AAIExecute::GetMetalUrgency()
{
	if(ai->activeUnits[EXTRACTOR] > 0)
		return 20.0 / pow(averageMetalSurplus + 2.0f, 2.0f);
	else
		return 5.5;
}

float AAIExecute::GetEnergyStorageUrgency()
{
	if(averageEnergySurplus / cfg->METAL_ENERGY_RATIO > 3)
		return 0.2;
	else
		return 0;
}

float AAIExecute::GetMetalStorageUrgency()
{
	if(averageMetalSurplus > 2 && (cb->GetMetalStorage() + futureStoredMetal - cb->GetMetal()) < 100)
		return 0.3;
	else
		return 0;
}

void AAIExecute::CheckFactories()
{
	if(ai->futureUnits[GROUND_FACTORY] + ai->futureUnits[SEA_FACTORY] > 0)
		return;

	int activeFactories = ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] + ai->futureUnits[GROUND_FACTORY] + ai->futureUnits[SEA_FACTORY];

	if(urgency[GROUND_FACTORY] == 0)
	{
		for(list<int>::iterator fac = bt->units_of_category[GROUND_FACTORY][ai->side-1].begin(); fac != bt->units_of_category[GROUND_FACTORY][ai->side-1].end(); ++fac)
		{
			if(bt->units_dynamic[*fac].requested - bt->units_dynamic[*fac].active > 0)
			{
				float urgency;

				if(activeFactories == 0)
					urgency = 3;
				if(activeFactories == 1)
					urgency = 0.8;
				else
					urgency = 0.35;

				if(this->urgency[GROUND_FACTORY] < urgency)
					this->urgency[GROUND_FACTORY] = urgency;
			}
		}
	}

	// add sea factories
	if(ai->map->mapType > 1 && urgency[SEA_FACTORY] == 0)
	{
		for(list<int>::iterator fac = bt->units_of_category[SEA_FACTORY][ai->side-1].begin(); fac != bt->units_of_category[SEA_FACTORY][ai->side-1].end(); ++fac)
		{
			if(bt->units_dynamic[*fac].requested - bt->units_dynamic[*fac].active > 0)
			{
				if(activeFactories > 1)
					urgency[SEA_FACTORY] = 0.5;
				else
					urgency[SEA_FACTORY] = 2.5;
			}
		}
	}
}

void AAIExecute::CheckRecon()
{
	if(ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] < cfg->MIN_FACTORIES_FOR_RADAR_JAMMER
		|| ai->activeUnits[STATIONARY_RECON] >= brain->sectors[0].size())
		return;

	float urgency = 1.0 / (ai->activeUnits[STATIONARY_RECON]+1);

	if(this->urgency[STATIONARY_RECON] < urgency)
		this->urgency[STATIONARY_RECON] = urgency;
}

void AAIExecute::CheckAirBase()
{
	if(cfg->MAX_AIR_BASE > 0)
	{
		// only build repair pad if any air units have been built yet
		if(ai->activeUnits[AIR_BASE] + ai->futureUnits[AIR_BASE] < cfg->MAX_AIR_BASE && ai->group_list[AIR_ASSAULT].size() > 0)
			urgency[AIR_BASE] = 0.5;	
	}
}

void AAIExecute::CheckJammer()
{
	if(ai->activeUnits[GROUND_FACTORY] + ai->activeUnits[SEA_FACTORY] < 2 || ai->activeUnits[STATIONARY_JAMMER] > brain->sectors[0].size())
	{
		this->urgency[STATIONARY_JAMMER] = 0;
	}
	else
	{
		float temp = 0.2 / (ai->activeUnits[STATIONARY_JAMMER]+1) + 0.05;

		if(urgency[STATIONARY_JAMMER] < temp)
			urgency[STATIONARY_JAMMER] = temp;
	}
}

void AAIExecute::CheckConstruction()
{	
	UnitCategory category = UNKNOWN; 
	float highest_urgency = 0.5;
	bool construction_started = false;

	//fprintf(ai->file, "\n");
	// get category with highest urgency
	for(int i = 1; i <= METAL_MAKER+1; i++)
	{
		//fprintf(ai->file, "%-20s: %f\n", bt->GetCategoryString2((UnitCategory)i), urgency[i]);
		if(urgency[i] > highest_urgency)
		{
			highest_urgency = urgency[i];
			category = (UnitCategory)i;
		}
	}
	//fprintf(ai->file, "Chosen: %s\n", bt->GetCategoryString2(category));

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
	else if(category == GROUND_FACTORY || category == SEA_FACTORY)
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
	else 
	{
		construction_started = true;
	}

	if(construction_started)
	{
		urgency[category] = 0;

		//cb->SendTextMsg(bt->GetCategoryString2(category), 0);

		for(int i = 1; i <= METAL_MAKER+1; ++i)
			urgency[i] *= 1.035;
	}
}

bool AAIExecute::AssistConstructionOfCategory(UnitCategory category, int importance)
{
	AAIBuilder *builder, *assistant;

	for(list<AAIBuildTask*>::iterator task = ai->build_tasks.begin(); task != ai->build_tasks.end(); ++task)
	{
		if((*task)->builder_id >= 0)
			builder = ut->units[(*task)->builder_id].builder;
		else
			builder = 0;

		if(builder && builder->building_category == category && builder->assistants.size() < cfg->MAX_ASSISTANTS)
		{
			if(bt->unitList[builder->building_def_id-1]->minWaterDepth > 0)
			{
				if(bt->unitList[builder->building_def_id-1]->floater)
					assistant = ut->FindAssistBuilder(builder->build_pos, 5, true, true);
				else
					assistant = ut->FindAssistBuilder(builder->build_pos, 5, true, false);
			}
			else
				assistant = ut->FindAssistBuilder(builder->build_pos, 5, false, false);

			if(assistant)
			{
				builder->assistants.insert(assistant->unit_id);
				assistant->AssistConstruction(builder->unit_id, (*task)->unit_id, importance);
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
		}
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
		}
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
	return (left->arty_efficiency[0] > right->arty_efficiency[0]);
}

bool AAIExecute::defend_vs_ground(AAISector *left, AAISector *right)
{
	return ((2 + 2 * left->GetThreatBy(GROUND_ASSAULT, learned, current)) / (left->GetDefencePowerVs(GROUND_ASSAULT)+ 1))
		>  ((2 + 2 * right->GetThreatBy(GROUND_ASSAULT, learned, current)) / (right->GetDefencePowerVs(GROUND_ASSAULT)+ 1));
}

bool AAIExecute::defend_vs_air(AAISector *left, AAISector *right)
{
	return ((2 + 2 * left->GetThreatBy(HOVER_ASSAULT, learned, current)) / (left->GetDefencePowerVs(HOVER_ASSAULT)+ 1))
		>  ((2 + 2 * right->GetThreatBy(HOVER_ASSAULT, learned, current)) / (right->GetDefencePowerVs(HOVER_ASSAULT)+ 1));
}

bool AAIExecute::defend_vs_hover(AAISector *left, AAISector *right)
{
	return ((2 + 2 * left->GetThreatBy(SEA_ASSAULT, learned, current)) / (left->GetDefencePowerVs(SEA_ASSAULT)+ 1))
		>  ((2 + 2 * right->GetThreatBy(SEA_ASSAULT, learned, current)) / (right->GetDefencePowerVs(SEA_ASSAULT)+ 1));

}

bool AAIExecute::defend_vs_sea(AAISector *left, AAISector *right)
{
	return ((2 + 2 * left->GetThreatBy(SEA_ASSAULT, learned, current)) / (left->GetDefencePowerVs(SEA_ASSAULT)+ 1))
		>  ((2 + 2 * right->GetThreatBy(SEA_ASSAULT, learned, current)) / (right->GetDefencePowerVs(SEA_ASSAULT)+ 1));

}

void AAIExecute::ConstructionFinished()
{
}

void AAIExecute::ConstructionFailed(int unit_id, float3 build_pos, int def_id)
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
	}
	
	// free metalspot if mex was odered to be built
	if(category == EXTRACTOR && build_pos.x > 0)
	{
		map->sector[x][y].FreeMetalSpot(build_pos, def);
	}
	else if(category == POWER_PLANT)
	{
		futureAvailableEnergy -= bt->units_static[def_id].efficiency[0];
	}
	else if(category == STORAGE)
	{
		futureStoredEnergy -= bt->unitList[def->id-1]->energyStorage;
		futureStoredMetal -= bt->unitList[def->id-1]->metalStorage;
	}
	else if(category == METAL_MAKER)
	{
		futureRequestedEnergy -= bt->unitList[def->id-1]->energyUpkeep;
	}
	else if(category == STATIONARY_JAMMER)
	{
		futureRequestedEnergy -= bt->units_static[def->id].efficiency[0];
	}
	else if(category == STATIONARY_RECON)
	{
		futureRequestedEnergy -= bt->units_static[def->id].efficiency[0];
	}
	else if(category == STATIONARY_DEF)
	{
		if(validSector)
			map->sector[x][y].RemoveDefence(unit_id);
	}
	
	// clear buildmap
	if(category == GROUND_FACTORY)
	{
		// remove future ressource demand now factory has been finished
		futureRequestedMetal -= bt->units_static[def->id].efficiency[0];
		futureRequestedEnergy -= bt->units_static[def->id].efficiency[1];

		// update buildmap of sector
		map->Pos2BuildMapPos(&build_pos, def);
		map->CheckRows(build_pos.x, build_pos.z, def->xsize, def->ysize, false, false);
		
		map->SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->ysize, 0);
		map->BlockCells(build_pos.x, build_pos.z - 8, def->xsize, 8, false, false);
		map->BlockCells(build_pos.x + def->xsize, build_pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, false, false);
		map->BlockCells(build_pos.x, build_pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, false, false);
	}
	else if(category == SEA_FACTORY)
	{
		// update buildmap of sector
		map->Pos2BuildMapPos(&build_pos, def);
		map->CheckRows(build_pos.x, build_pos.z, def->xsize, def->ysize, false, true);

		map->SetBuildMap(build_pos.x, build_pos.z, def->xsize, def->ysize, 4);
		map->BlockCells(build_pos.x, build_pos.z - 8, def->xsize, 8, false, true);
		map->BlockCells(build_pos.x + def->xsize, build_pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, false, true);
		map->BlockCells(build_pos.x, build_pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, false, true);
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

	for(list<int>::iterator i = bt->units_of_category[GROUND_FACTORY][ai->side-1].begin(); i != bt->units_of_category[GROUND_FACTORY][ai->side-1].end(); i++)
	{
		if(bt->units_dynamic[*i].builderAvailable)
		{
			my_rating = bt->GetFactoryRating(*i);

			if(my_rating > best_rating)
			{
				best_rating = my_rating;
				best_factory = *i;
			}
		}
	}

	// add sea factories
	if(ai->map->mapType > 1)
	{
		for(list<int>::iterator i = bt->units_of_category[SEA_FACTORY][ai->side-1].begin(); i != bt->units_of_category[SEA_FACTORY][ai->side-1].end(); i++)
		{
			if(bt->units_dynamic[*i].builderAvailable)
			{
				my_rating = bt->GetFactoryRating(*i);

				if(my_rating > best_rating)
				{
					best_rating = my_rating;
					best_factory = *i;
				}
			}
		}
	}

	if(best_factory)
	{
		++bt->units_dynamic[best_factory].requested;

		fprintf(ai->file, "%s requested\n", bt->unitList[best_factory-1]->humanName.c_str());

		for(list<int>::iterator j = bt->units_static[best_factory].canBuildList.begin(); j != bt->units_static[best_factory].canBuildList.end(); j++)
			bt->units_dynamic[*j].builderRequested = true;
	}
}


AAIGroup* AAIExecute::GetClosestGroupOfCategory(UnitCategory category, UnitType type, float3 pos, int importance)
{
	AAIGroup *best_group = 0;
	float best_rating = 0, my_rating;
	float3 group_pos; 

	for(list<AAIGroup*>::iterator group = ai->group_list[category].begin(); group != ai->group_list[category].end(); ++group)
	{
		if((*group)->group_type == type && !(*group)->attack)
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

void AAIExecute::DefendUnitVS(int unit, const UnitDef *def, UnitCategory category, float3 enemy_pos, int importance)
{
	if(unit < 0)
		return;

	float3 pos = cb->GetUnitPos(unit);

	int x = pos.x/map->xSectorSize;
	int y = pos.z/map->ySectorSize;

	if(x < 0 || y < 0 || x >= map->xSectors || y >= map->ySectors)
		return;


	AAIGroup *support = 0;

	bool enemy;

	// if enemy attacker is known, units will attack enemy instead of guarding damaged unit
	if(enemy_pos.x > 0)
		enemy = true;
	else
		enemy = false;

	Command guard_unit;
	guard_unit.id = CMD_GUARD;
	guard_unit.params.push_back(unit);

	Command attack_enemy;
	attack_enemy.id = CMD_FIGHT;
	attack_enemy.params.push_back(enemy_pos.x);
	attack_enemy.params.push_back(enemy_pos.y);
	attack_enemy.params.push_back(enemy_pos.z);
	
	// anti air mods have special behaviour
	if(cfg->AIR_ONLY_MOD)
	{
		support = GetClosestGroupOfCategory(AIR_ASSAULT, ASSAULT_UNIT, pos, 100);

		if(!support)
			support = GetClosestGroupOfCategory(HOVER_ASSAULT, ASSAULT_UNIT, pos, 100);

		if(!support)
			support = GetClosestGroupOfCategory(GROUND_ASSAULT, ASSAULT_UNIT, pos, 100);

		if(support)
		{
			if(enemy)
				support->GiveOrder(&attack_enemy, importance, DEFENDING);
			else
				support->GiveOrder(&guard_unit, importance, GUARDING);

			support->task = GROUP_DEFENDING;
		}
	}
	// normal mods
	else  
	{
		bool land, water;

		if(map->sector[x][y].water_ratio > 0.6)
		{
			land = false;
			water = true;
		}
		else
		{
			land = true;
			water = false;
		}

		// find possible defenders dependent of category of attacker

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
				support->GiveOrder(&guard_unit, importance, GUARDING);	
				support->task = GROUP_DEFENDING;
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
				if(enemy)
					support->GiveOrder(&attack_enemy, importance, DEFENDING);
				else
					support->GiveOrder(&guard_unit, importance, GUARDING);

				support->task = GROUP_DEFENDING;
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
				if(enemy)
					support->GiveOrder(&attack_enemy, importance, DEFENDING);
				else
					support->GiveOrder(&guard_unit, importance, GUARDING);

				support->task = GROUP_DEFENDING;
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
