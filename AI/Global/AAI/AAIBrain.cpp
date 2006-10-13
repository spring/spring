//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------

#include "AAIBrain.h"
#include "AAI.h"
#include "AAIMap.h"

AAIBrain::AAIBrain(AAI *ai)
{
	this->ai = ai;
	this->map = ai->map;
	cb = ai->cb;
	bt = ai->bt;
	execute = 0;
	freeBaseSpots = false;
	expandable = true;

	// initialize random numbers generator
	srand ( time(NULL) );

	max_distance = ai->map->xSectors + ai->map->ySectors - 2;
	sectors = new list<AAISector*>[max_distance];

	base_center = ZeroVector;

	max_units_spotted = new float[bt->combat_categories];
	attacked_by = new float[bt->combat_categories];
	defence_power_vs = new float[bt->combat_categories];

	for(int i = 0; i < bt->combat_categories; ++i)
	{
		max_units_spotted[i] = 0;
		attacked_by[i] = 0;
		defence_power_vs[i] = 0;
	}
}

AAIBrain::~AAIBrain(void)
{
	delete [] sectors;
	delete [] max_units_spotted;
	delete [] attacked_by;
	delete [] defence_power_vs;
}


AAISector* AAIBrain::GetAttackDest(bool land, bool water, AttackType type)
{
	float best_rating = 0, my_rating;
	AAISector *dest = 0, *sector;

	int side = ai->side-1;
	
	float ground = map->map_usefulness[0][side]/100.0f;
	float air = map->map_usefulness[1][side]/100.0f;
	float hover = map->map_usefulness[2][side]/100.0f;
	float sea = map->map_usefulness[3][side]/100.0f;
	float submarine = map->map_usefulness[4][side]/100.0f;

	// TODO: improve destination sector selection
	for(int x = 0; x < map->xSectors; x++)
	{
		for(int y = 0; y < map->ySectors; y++)
		{
			sector = &map->sector[x][y];

			if(sector->distance_to_base == 0 || sector->enemy_structures == 0)
					my_rating = 0;
			else if(type == BASE_ATTACK)
			{
				if(land && sector->water_ratio < 0.35)
				{
					my_rating = sector->enemy_structures / (2 * sector->GetThreatTo(ground, air, hover, sea, submarine) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f) + 1);
					my_rating /= (8 + sector->distance_to_base);
				}
				else if(water && sector->water_ratio > 0.65)
				{
					my_rating = sector->enemy_structures / (2 * sector->GetThreatTo(ground, air, hover, sea, submarine) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f) + 1);
					my_rating /= (8 + sector->distance_to_base);
				}
				else 
					my_rating = 0;
			}
			else if(type == OUTPOST_ATTACK)
			{
				if(land && sector->water_ratio < 0.35)
				{
					my_rating = 1 / (1 + pow(sector->GetThreatTo(ground, air, hover, sea, submarine), 2.0f) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f));
					my_rating /= (2 + sector->distance_to_base);
				}
				else if(water && sector->water_ratio > 0.65)
				{
					my_rating = 1 / (1 + pow(sector->GetThreatTo(ground, air, hover, sea, submarine), 2.0f) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f));
					my_rating /= (2 + sector->distance_to_base);
				}	
				else 
					my_rating = 0;	
			}

			if(my_rating > best_rating)
			{
				dest = sector;
				best_rating = my_rating;
			}
		}
	}

	return dest;
}

AAISector* AAIBrain::GetNextAttackDest(AAISector *current_sector, bool land, bool water)
{
	float best_rating = 0, my_rating, dist;
	AAISector *dest = 0, *sector;

	int side = ai->side-1;
	
	float ground = map->map_usefulness[0][side]/100.0f;
	float air = map->map_usefulness[1][side]/100.0f;
	float hover = map->map_usefulness[2][side]/100.0f;
	float sea = map->map_usefulness[3][side]/100.0f;
	float submarine = map->map_usefulness[4][side]/100.0f;

	// TODO: improve destination sector selection
	for(int x = 0; x < map->xSectors; x++)
	{
		for(int y = 0; y < map->ySectors; y++)
		{
			sector = &map->sector[x][y];

			if(sector->distance_to_base == 0 || sector->enemy_structures == 0)
					my_rating = 0;
			else 
			{
				if(land && sector->water_ratio < 0.35)
				{
					dist = sqrt( pow(sector->x - current_sector->x, 2) + pow(sector->y - current_sector->y , 2) );
					
					my_rating = 1 / (1 + pow(sector->GetThreatTo(ground, air, hover, sea, submarine), 2.0f) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f));
				
				}
				else if(water && sector->water_ratio > 0.65)
				{
					dist = sqrt( pow(sector->x - current_sector->x, 2) + pow(sector->y - current_sector->y , 2) );

					my_rating = 1 / (1 + pow(sector->GetThreatTo(ground, air, hover, sea, submarine), 2.0f) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f));
					my_rating /= (1 + dist);
				}	
				else 
					my_rating = 0;	
			}

			if(my_rating > best_rating)
			{
				dest = sector;
				best_rating = my_rating;
			}
		}
	}

	return dest;
}

AAISector* AAIBrain::GetNewScoutDest(int scout)
{
	// TODO: take scouts pos into account
	float my_rating, best_rating = 0;
	AAISector *scout_sector = 0, *sector;
	UnitCategory category;

	const UnitDef *def = cb->GetUnitDef(scout);
	
	if(def)
		 category = bt->units_static[def->id].category;
	else 
		return 0;

	for(int x = 0; x < map->xSectors; x++)
	{
		for(int y = 0; y < map->ySectors; y++)
		{
			sector = &map->sector[x][y];
			
			if(sector->distance_to_base > 0)
			{
				// land sector
				if(sector->water_ratio > 0.7)
				{
					if(category != GROUND_SCOUT)
					{
						my_rating = sector->importance[0] * sector->last_scout;
						++sector->last_scout;
					}
					else
						my_rating = 0;
				}
				// water sector
				else if(sector->water_ratio < 0.3)
				{
					if(category != SEA_SCOUT)
					{
						my_rating = sector->importance[0] * sector->last_scout;
						++sector->last_scout;
					}
					else
						my_rating = 0;
				}
				// land/water sector
				else
				{
					my_rating = sector->importance[0] * sector->last_scout;
					++sector->last_scout;
				}

				if(my_rating > best_rating)
				{
					best_rating = my_rating;
					scout_sector = sector;
				}
			}
		}
	}

	// set dest sector as visited
	if(scout_sector)
		scout_sector->last_scout = 1;
	
	return scout_sector;
}

bool AAIBrain::MetalForConstr(int unit, int workertime)
{
	// check index 
	if(unit >= bt->numOfUnits)
	{
		fprintf(ai->file, "ERROR: MetalForConstr(): index %i out of range, max units are: %i\n", unit, bt->numOfSides);
		return false;
	}

	int metal = (bt->unitList[unit-1]->buildTime/workertime) * (cb->GetMetalIncome()-(cb->GetMetalUsage()) + cb->GetMetal());
	int total_cost = bt->unitList[unit-1]->metalCost;

	if(!bt->units_dynamic[unit].builderAvailable)
		total_cost += bt->units_static[unit].builder_metal_cost;
	
	if(metal > total_cost)
		return true;
	
	return false;
}

bool AAIBrain::EnergyForConstr(int unit, int wokertime)
{
	// check index 
	if(unit >= bt->numOfUnits)
	{
		fprintf(ai->file, "ERROR: EnergyForConstr(): index %i out of range, max units are: %i\n", unit, bt->numOfSides);
		return false;
	}

	// check energy
	int energy =  bt->unitList[unit-1]->buildTime * (cb->GetEnergyIncome()-(cb->GetEnergyUsage()/2));

	return true;
	//return (energy > bt->unitList[unit-1]->energyCost);
}

bool AAIBrain::RessourcesForConstr(int unit, int wokertime)
{
	// check metal and energy
	/*if(MetalForConstr(unit) && EnergyForConstr(unit))
			return true;
	
	return false;*/
	return true;
}

void AAIBrain::AddSector(AAISector *sector)
{
	sectors[0].push_back(sector);
	sector->distance_to_base = 0;
}

void AAIBrain::DefendBuilding(int building, const UnitDef *def, int attacker)
{
	// determine what kind of building has been attacked
	// TODO: let ai learn about priority of defending different building categories
	float importance = 110;
		
	// very primitive: send forces to attacked building
	// TODO: optimize defence 
	float3 pos = cb->GetUnitPos(building);

	UnitType group_type = UNKNOWN_UNIT;

	Command c;
	c.id = CMD_PATROL;
	c.params.resize(3);
	c.params[0] = pos.x;
	c.params[1] = pos.y;
	c.params[2] = pos.z;

	if(attacker != -1)
	{
		const UnitDef *def = cb->GetUnitDef(attacker);

		if(def)
		{
			// call air support
			ai->af->CheckTarget(attacker, def);

			// call ground support
			if(bt->units_static[def->id].category == AIR_ASSAULT)
			{
				group_type = ANTI_AIR_UNIT;
			}
			else
			{
				group_type = ASSAULT_UNIT;
			}
		}
	}
	else
	{
		group_type = ASSAULT_UNIT;
	}

	if(group_type != UNKNOWN_UNIT)
	{
		for(list<AAIGroup*>::iterator group = ai->group_list[GROUND_ASSAULT].begin(); group != ai->group_list[GROUND_ASSAULT].end(); group++)
		{
			if((*group)->task_importance < importance && (*group)->group_type == group_type)
			{
				(*group)->GiveOrder(&c, importance, GUARDING);
				(*group)->task = GROUP_DEFENDING;
				return;
			}
		}
	}
}

void AAIBrain::DefendCommander(int attacker)
{
	float3 pos = cb->GetUnitPos(ai->ut->cmdr);
	float importance = 120;
	Command c;

	// evacuate cmdr
	/*if(ai->cmdr->task != BUILDING)
	{
		AAISector *sector = GetSafestSector();

		if(sector != 0)
		{
			pos = sector->GetCenter();

			if(pos.x > 0 && pos.z > 0)
			{
				pos.y = cb->GetElevation(pos.x, pos.z);
				execute->moveUnitTo(ai->cmdr->unit_id, &pos);
			}
		}
	}*/
}

void AAIBrain::UpdateBaseCenter()
{
	base_center = ZeroVector;

	for(list<AAISector*>::iterator sector = sectors[0].begin(); sector != sectors[0].end(); ++sector)
	{
		base_center.x += (0.5 + (*sector)->x) * map->xSectorSize;
		base_center.z += (0.5 + (*sector)->y) * map->ySectorSize;
	}

	base_center.x /= sectors[0].size();
	base_center.z /= sectors[0].size();
}

void AAIBrain::UpdateNeighbouringSectors()
{
	int x,y,neighbours;

	// delete old values
	for(x = 0; x < map->xSectors; ++x)
	{
		for(y = 0; y < map->ySectors; ++y)
		{
			if(map->sector[x][y].distance_to_base > 0)
				map->sector[x][y].distance_to_base = -1;
		}
	}

	for(int i = 1; i < max_distance; i++)
	{
		// delete old sectors
		sectors[i].clear();
		neighbours = 0;

		for(list<AAISector*>::iterator sector = sectors[i-1].begin(); sector != sectors[i-1].end(); ++sector)
		{
			x = (*sector)->x;
			y = (*sector)->y;

			// check left neighbour 
			if(x > 0 && map->sector[x-1][y].distance_to_base == -1)
			{
				map->sector[x-1][y].distance_to_base = i;
				sectors[i].push_back(&map->sector[x-1][y]);
				++neighbours;
			}
			// check right neighbour
			if(x < (ai->map->xSectors - 1) && ai->map->sector[x+1][y].distance_to_base == -1)
			{
				map->sector[x+1][y].distance_to_base = i;
				sectors[i].push_back(&map->sector[x+1][y]);
				++neighbours;
			}
			// check upper neighbour
			if(y > 0 && ai->map->sector[x][y-1].distance_to_base == -1) 
			{
				map->sector[x][y-1].distance_to_base = i;
				sectors[i].push_back(&map->sector[x][y-1]);
				++neighbours;
			}
			// check lower neighbour
			if(y < (ai->map->ySectors - 1) && ai->map->sector[x][y+1].distance_to_base == -1)
			{
				map->sector[x][y+1].distance_to_base = i;
				sectors[i].push_back(&map->sector[x][y+1]);
				++neighbours;
			}

			if(i == 1 && !neighbours)
				(*sector)->interior = true;
		}
	}

	// determine interior sectors
	for(list<AAISector*>::iterator sector = sectors[0].begin(); sector != sectors[0].end(); ++sector)
	{
	}

	fprintf(ai->file, "Base has now %i direct neighbouring sectors\n", sectors[1].size());

	// debug
	/*FILE *my_file = fopen("debug_map.txt", "w+");

	for(int y = 0; y < ai->map->ySectors; ++y)
	{
		for(int x = 0; x < ai->map->xSectors; ++x)
		{
			fprintf(my_file, "%+2i ", ai->map->sector[x][y].distance_to_base);
		}

		fprintf(my_file, "\n");
	}

	fclose(my_file);*/
}

bool AAIBrain::SectorInList(list<AAISector*> mylist, AAISector *sector)
{
	// check if sector already added to list
	for(list<AAISector*>::iterator t = mylist.begin(); t != mylist.end(); t++)
	{
		if(*t == sector)
			return true;
	}
	return false;
}

bool AAIBrain::ExpandBase(SectorType sectorType)
{
	// TODO: improve expansion algorithm
	//if(expandable)
	if(sectors[0].size() < cfg->MAX_BASE_SIZE)
	{
		// debug purposes:
		//fprintf(ai->file, "Found %i possible target sectors \n", neighbouring_sectors.size());

		// now targets should contain all neighbouring sectors that are not currently part of the base
		// only once; select the sector with most metalspots and least danger
		AAISector *best_sector = 0; 
		float best_rating  = 0, my_rating;
		int spots;
		float dist;

		for(list<AAISector*>::iterator t = sectors[1].begin(); t != sectors[1].end(); t++)
		{
			// dont expand if enemy structures in sector
			if((*t)->enemy_structures < 100)
			{
				// rate current sector
				spots = (*t)->GetNumberOfMetalSpots();
			
				my_rating = 2 + spots/2.0;
			
				// minmize distance between sectors
				dist = 0.1;
				for(list<AAISector*>::iterator sector = sectors[0].begin(); sector != sectors[0].end(); sector++)
					dist += 2.0 * sqrt(pow( (*t)->x - (*sector)->x , 2) + pow( (*t)->y - (*sector)->y , 2));
				

				if(sectorType = LAND_SECTOR)
				{
					// prefer flat sectors without water
					my_rating += ((*t)->flat_ratio - (*t)->water_ratio) * 8;
					my_rating /= dist;
				}
				else if(sectorType == WATER_SECTOR)
				{
					my_rating += 8 * (*t)->water_ratio;
					my_rating /= dist;
				}
				else
					my_rating = 0;

				// check for allied buildings (to prevent aai from expanding to sectors which are part of the base of another player)
				my_rating /= sqrt(10 + (*t)->allied_structures);

				if((*t)->allied_structures > 700)
					my_rating = 0;
			
				// choose higher rated sector
				if(my_rating > best_rating)
				{
					best_rating = my_rating;
					best_sector = *t;
				}
			}
		}

		if(best_sector)
		{
			// add this sector to base
			AddSector(best_sector);
			best_sector->SetBase(true);
			// debug purposes:
			if(sectorType == LAND_SECTOR)
				fprintf(ai->file, "\nAdding land sector %i,%i to base; base size: %i \n", best_sector->x, best_sector->y, sectors[0].size());
			else
				fprintf(ai->file, "\nAdding water sector %i,%i to base; base size: %i \n", best_sector->x, best_sector->y, sectors[0].size());

			// update neighbouring sectors
			UpdateNeighbouringSectors();
			UpdateBaseCenter();

			// check if further expansion possible
			if(sectors[0].size() == cfg->MAX_BASE_SIZE)
				expandable = false;

			freeBaseSpots = true;

			return true;
		}
	}
	
	return false;
}

void AAIBrain::UpdateMaxCombatUnitsSpotted(float *units_spotted)
{
	for(int i = 0; i < bt->combat_categories; ++i)
	{
		// decrease old values
		max_units_spotted[i] *= 0.995;
	
		// check for new max values
		if(units_spotted[i] > max_units_spotted[i])
			max_units_spotted[i] = units_spotted[i];
	}
}

void AAIBrain::UpdateAttackedByValues()
{
	for(int i = 0; i < bt->combat_categories; ++i)
	{
		attacked_by[i] *= 0.96;
	}
}

void AAIBrain::AttackedBy(int combat_category_id)
{
	attacked_by[combat_category_id] += 1;
}

void AAIBrain::UpdateDefenceCapabilities()
{
	for(int i = 0; i < bt->assault_categories.size(); ++i)
		defence_power_vs[i] = 0;
	
	// anti air power
	for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
	{
		for(list<AAIGroup*>::iterator group = ai->group_list[*category].begin(); group != ai->group_list[*category].end(); ++group)
		{
			if((*group)->group_type == ANTI_AIR_UNIT)
				defence_power_vs[1] += (*group)->GetPowerVS(1);
			else if((*group)->group_type == ASSAULT_UNIT)
			{
				if((*group)->category == GROUND_ASSAULT)
				{
					defence_power_vs[0] += (*group)->GetPowerVS(0);
					defence_power_vs[2] += (*group)->GetPowerVS(2);
				}
				/*else if((*group)->category == AIR_ASSAULT)
				{
					defence_power_vs[0] += (*group)->GetPowerVS(0);
					defence_power_vs[2] += (*group)->GetPowerVS(2);
					defence_power_vs[3] += (*group)->GetPowerVS(3);
				}*/
				else if((*group)->category == HOVER_ASSAULT)
				{
					defence_power_vs[0] += (*group)->GetPowerVS(0);
					defence_power_vs[2] += (*group)->GetPowerVS(2);
					defence_power_vs[3] += (*group)->GetPowerVS(3);
				}
				else if((*group)->category == SEA_ASSAULT)
				{
					defence_power_vs[2] += (*group)->GetPowerVS(2);
					defence_power_vs[3] += (*group)->GetPowerVS(3);
					defence_power_vs[4] += (*group)->GetPowerVS(4);
				}
				else if((*group)->category == SUBMARINE_ASSAULT)
				{
					defence_power_vs[3] += (*group)->GetPowerVS(3);
					defence_power_vs[4] += (*group)->GetPowerVS(4);
				}
			}
		}
	}

	// debug
	/*fprintf(ai->file, "Defence capabilities:\n");

	for(int i = 0; i < bt->assault_categories.size(); ++i)
		fprintf(ai->file, "%-20s %f\n" , bt->GetCategoryString2(bt->GetAssaultCategoryOfID(i)),defence_power_vs[i]);
	*/
}

float AAIBrain::Affordable()
{
	return 40.0 /(cb->GetMetalIncome() + 5.0);
}

void AAIBrain::BuildUnits()
{
	int side = ai->side-1;
	UnitCategory category;
	bool urgent = false;
	int k;

	float cost = 1 + Affordable()/12.0;

	float ground_eff = 0;
	float air_eff = 0;
	float hover_eff = 0;
	float sea_eff = 0;
	float stat_eff = 0;
	float submarine_eff = 0;

	int anti_air_urgency;
	int anti_sea_urgency;
	int anti_ground_urgency;
	int anti_hover_urgency;
	int anti_submarine_urgency;

	// todo: improve selection
	category = UNKNOWN;

	if(cfg->AIR_ONLY_MOD)
	{
		// determine effectiveness vs several other units
		anti_ground_urgency = (int)( 1 + (map->map_usefulness[0][side] + bt->mod_usefulness[0][side][0]) * (attacked_by[0] + 4) * (max_units_spotted[0] + 2) / (defence_power_vs[0] + 1));
		anti_air_urgency = (int)( 1 + (map->map_usefulness[1][side] + bt->mod_usefulness[1][side][0]) * (attacked_by[1] + 4) * (max_units_spotted[1] + 2) / (defence_power_vs[1] + 1));
		anti_hover_urgency = (int)( 1 + (map->map_usefulness[2][side] + bt->mod_usefulness[2][side][0]) * (attacked_by[2] + 4) * (max_units_spotted[2] + 2) / (defence_power_vs[2] + 1));
		anti_sea_urgency = (int) (1 + (map->map_usefulness[3][side] + bt->mod_usefulness[3][side][0]) * (attacked_by[3] + 4) * (max_units_spotted[3] + 2) / (defence_power_vs[3] + 1));
				
		for(int i = 0; i < execute->unitProductionRate; ++i)
		{
			ground_eff = 0;
			air_eff = 0;
			hover_eff = 0;
			sea_eff = 0;

			// build super units only in late game
			if(cost < 1.1 && rand()%16 == 1)
			{	
				category = SEA_ASSAULT;
			}
			else
			{
				if(rand()%3 == 1)
					category = GROUND_ASSAULT;
				else if(rand()%3 == 2)
					category = AIR_ASSAULT;
				else
					category = HOVER_ASSAULT;
			}

		
			k = rand()%(anti_ground_urgency + anti_air_urgency + anti_hover_urgency + anti_sea_urgency);

			if(k < anti_ground_urgency)
			{	
				ground_eff = 4;
			}
			else if(k < anti_ground_urgency + anti_air_urgency)
			{
				air_eff = 4;
			}
			else if(k < anti_ground_urgency + anti_air_urgency + anti_hover_urgency)
			{
				hover_eff = 4;
			}
			else
			{
				sea_eff = 4;
			}

			BuildUnitOfCategory(category, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, urgent);
		}
	}
	else
	{
		MapType mapType = map->mapType;

		float ground_usefulness = map->map_usefulness[0][side] + bt->mod_usefulness[0][side][mapType];
		float air_usefulness = map->map_usefulness[1][side] + bt->mod_usefulness[1][side][mapType];
		float hover_usefulness = map->map_usefulness[2][side] + bt->mod_usefulness[2][side][mapType];
		float sea_usefulness = map->map_usefulness[3][side] + bt->mod_usefulness[3][side][mapType];
		float submarine_usefulness = map->map_usefulness[4][side] + bt->mod_usefulness[4][side][mapType];

		// choose unit category dependend on map type
		if(mapType == LAND_MAP)
		{
			// determine effectiveness vs several other units
			anti_ground_urgency = (int)( 2 + ground_usefulness * (attacked_by[0] + 4) * (max_units_spotted[0] + 1) / (2 * defence_power_vs[0] + 1));
			anti_air_urgency = (int)( 2 + air_usefulness * (attacked_by[1] + 4) * (max_units_spotted[1] + 1) / (2 * defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 2 + hover_usefulness * (attacked_by[2] + 4) * (max_units_spotted[2] + 2) / (2 * defence_power_vs[2] + 1));
			
			for(int i = 0; i < execute->unitProductionRate; ++i)
			{
				ground_eff = 0;
				air_eff = 0;
				hover_eff = 0;
				stat_eff = 0;

				// determine unit type
				if(rand()%cfg->AIRCRAFT_RATE == 1)
				{
					category = AIR_ASSAULT;

					k = rand()%3;

					if(k == 0)
					{
						ground_eff = 4;
						hover_eff = 1;
					}
					else if(k == 1)
					{
						air_eff = 4;
					}
					else
					{
						stat_eff = 4;
					}
				}
				else
				{
					// decide between hover and ground based on experience
					if(rand()%((int)(ground_usefulness + hover_usefulness)) >= (int) ground_usefulness)
						category = HOVER_ASSAULT;
					else
						category = GROUND_ASSAULT;

					k = rand()%(anti_ground_urgency + anti_air_urgency + anti_hover_urgency);

					if(k < anti_ground_urgency)
					{	
						stat_eff = 2;
						ground_eff = 5;
						hover_eff = 1;
					}
					else if(k < anti_ground_urgency + anti_air_urgency)
					{
						air_eff = 4;

						if(anti_air_urgency > 1.5 * anti_ground_urgency)
							urgent = true;
					}
					else
					{
						ground_eff = 1;
						hover_eff = 4;
					}
				}

				BuildUnitOfCategory(category, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff,stat_eff, urgent);
			}

			// debug
			/*fprintf(ai->file, "Selecting units: %i %i\n", k , anti_ground_urgency + anti_air_urgency + anti_hover_urgency);
			fprintf(ai->file, "Selected: ground %f   air %f   hover %f\n", ground_eff, air_eff, hover_eff);
			fprintf(ai->file, "Ground assault: %f %f %f %i \n", defence_power_vs[0], attacked_by[0], max_units_spotted[0], anti_ground_urgency);
			fprintf(ai->file, "Air assault:    %f %f %f %i \n", defence_power_vs[1], attacked_by[1], max_units_spotted[1], anti_air_urgency);
			fprintf(ai->file, "Hover assault:  %f %f %f %i \n", defence_power_vs[2], attacked_by[2], max_units_spotted[1], anti_hover_urgency);*/
		}
		else if(mapType == LAND_WATER_MAP)
		{
			// determine effectiveness vs several other units
			anti_ground_urgency = (int)( 1 + ground_usefulness * (attacked_by[0] + 4) * (max_units_spotted[0] + 2) / (defence_power_vs[0] + 1));
			anti_air_urgency = (int)( 1 + air_usefulness * (attacked_by[1] + 4) * (max_units_spotted[1] + 2) / (defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 1 + hover_usefulness * (attacked_by[2] + 4) * (max_units_spotted[2] + 2) / (defence_power_vs[2] + 1));
			anti_sea_urgency = (int) (1 + sea_usefulness * (attacked_by[3] + 4) * (max_units_spotted[3] + 2) / (defence_power_vs[3] + 1));
			
			for(int i = 0; i < execute->unitProductionRate; ++i)
			{
				ground_eff = 0;
				air_eff = 0;
				hover_eff = 0;
				sea_eff = 0;
				submarine_eff = 0;
				stat_eff = 0;

				if(rand()%cfg->AIRCRAFT_RATE == 1)
				{
					category = AIR_ASSAULT;
					
					if(rand()%3 == 1)
					{
						ground_eff = 4;
						hover_eff = 1;
						sea_eff = 2;
					}
					else if(rand()%3 == 2)
					{
						air_eff = 4;
					}
					else
					{
						stat_eff = 4;
					}
				
				}
				else
				{
					// decide between hover and ground based on experience
					int random = rand()%((int)(ground_usefulness + hover_usefulness + sea_usefulness + submarine_usefulness));
					
					if(random < (int) ground_usefulness )
					{
						category = GROUND_ASSAULT;
						anti_submarine_urgency = 0;
					}
					else if(random < (int)(ground_usefulness + hover_usefulness))
					{
						category = HOVER_ASSAULT;
						anti_submarine_urgency = 0;
					}
					else if(random < (int)(ground_usefulness + hover_usefulness + sea_usefulness))
					{
						category = SEA_ASSAULT;
						anti_submarine_urgency = (int) (1 + submarine_usefulness * (attacked_by[4] + 2) * (max_units_spotted[4] + 2) / (defence_power_vs[4] + 1));
					}
					else
					{
						category = SUBMARINE_ASSAULT;
						anti_submarine_urgency = (int) (1 + submarine_usefulness * (attacked_by[4] + 2) * (max_units_spotted[4] + 2) / (defence_power_vs[4] + 1));
					}

					k = rand()%(anti_ground_urgency + anti_air_urgency + anti_hover_urgency + anti_sea_urgency + anti_submarine_urgency);

					if(k < anti_ground_urgency)
					{	
						stat_eff = 2;
						ground_eff = 5;
						hover_eff = 1;
					}
					else if(k < anti_ground_urgency + anti_air_urgency)
					{
						air_eff = 4;

						if(anti_air_urgency > anti_ground_urgency)
							urgent = true;
					}
					else if(k < anti_ground_urgency + anti_air_urgency + anti_hover_urgency)
					{
						ground_eff = 1;
						hover_eff = 4;
					}
					else if(k < anti_ground_urgency + anti_air_urgency + anti_hover_urgency + anti_sea_urgency)
					{
						hover_eff = 1;
						sea_eff = 4;
					}
					else
					{
						submarine_eff = 4;
						sea_eff = 1;
					}
				}
					
				BuildUnitOfCategory(category, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff,stat_eff, urgent);
			}
		}
		else if(mapType == WATER_MAP)
		{
			// determine effectiveness vs several other units
			anti_air_urgency = (int)( 1 + air_usefulness * (attacked_by[1] + 4) * (max_units_spotted[1] + 2) / (defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 1 + hover_usefulness * (attacked_by[2] + 4) * (max_units_spotted[2] + 2) / (defence_power_vs[2] + 1));
			anti_sea_urgency = (int) (1 + sea_usefulness * (attacked_by[3] + 4) * (max_units_spotted[3] + 2) / (defence_power_vs[3] + 1));
							
			for(int i = 0; i < execute->unitProductionRate; ++i)
			{	
				air_eff = 0;
				hover_eff = 0;
				sea_eff = 0;
				submarine_eff = 0;
				stat_eff = 0;

				if(rand()%cfg->AIRCRAFT_RATE == 1)
				{
					category = AIR_ASSAULT;
					anti_submarine_urgency = (int) (1 + submarine_usefulness * (attacked_by[4] + 2) * (max_units_spotted[4] + 2) / (defence_power_vs[4] + 1));
				
					
					if(rand()%3 == 1)
					{
						sea_eff = 4;
						hover_eff = 2;
					}
					else if(rand()%3 == 2)
					{
						air_eff = 4;
					}
					else
					{
						stat_eff = 4;
					}
				}
				else
				{
					// decide between hover and ground based on experience
					int random = rand()%((int)(hover_usefulness + sea_usefulness + submarine_usefulness));
					
					if(random < (int)(hover_usefulness))
					{
						category = HOVER_ASSAULT;
						anti_submarine_urgency = 0;
			
					}
					else if(random < (int)(hover_usefulness + sea_usefulness))
					{
						category = SEA_ASSAULT;
						anti_submarine_urgency = (int) (1 + submarine_usefulness * (attacked_by[4] + 2) * (max_units_spotted[4] + 2) / (defence_power_vs[4] + 1));
			
					}
					else
					{
						category = SUBMARINE_ASSAULT;
						anti_submarine_urgency = (int) (1 + submarine_usefulness * (attacked_by[4] + 2) * (max_units_spotted[4] + 2) / (defence_power_vs[4] + 1));
					}

					k = rand()%(anti_sea_urgency + anti_air_urgency + anti_hover_urgency + anti_submarine_urgency);

					if(k < anti_air_urgency)
					{	
						air_eff = 4;
	
						if(anti_air_urgency > anti_sea_urgency)
							urgent = true;
					}
					else if(k < anti_hover_urgency + anti_air_urgency)
					{
						sea_eff = 1;
						hover_eff = 5;
						stat_eff = 2;
					}
					else if(k < anti_hover_urgency + anti_air_urgency + anti_submarine_urgency)
					{
						submarine_eff = 4;
						sea_eff = 1;
					}
					else
					{
						hover_eff = 1;
						sea_eff = 5;
						stat_eff = 2;
					}
				}

				BuildUnitOfCategory(category, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff,stat_eff, urgent);
			}
		}
		else if(mapType == AIR_MAP)
		{
			category = AIR_ASSAULT;	

			if(rand()%3 == 1)
			{
				air_eff = 0.5;
				stat_eff = 4;
			}
			else
			{
				air_eff = 4;
				stat_eff = 0.5;
			}

			BuildUnitOfCategory(category, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff,stat_eff, urgent);
		}
	}
}

void AAIBrain::BuildUnitOfCategory(UnitCategory category, float cost, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, bool urgent)
{
	float speed = 0;
	float range = 0;

	if(category != UNKNOWN)
	{	
		// determine speed, range & eff 
		if(rand()%cfg->FAST_UNITS_RATE == 1)
		{
			if(rand()%2 == 1)
				speed = 1.5;
			else
				speed = 4;
		}
		else
			speed = 0.2;

		if(rand()%cfg->HIGH_RANGE_UNITS_RATE == 1)
		{
			if(rand()%2 == 1)
				range = 1;
			else
				range = 3;

		}
		else
			range = 0.2;

		execute->BuildUnit(category, speed, cost, range, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff,stat_eff, urgent);
	}
}
