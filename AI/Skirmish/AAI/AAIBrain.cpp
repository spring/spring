// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

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

	land_sectors = 0;
	water_sectors = 0;

	// initialize random numbers generator
	srand ( time(NULL) );

	max_distance = ai->map->xSectors + ai->map->ySectors - 2;
	sectors.resize(max_distance);

	base_center = ZeroVector;

	max_units_spotted.resize(bt->combat_categories);
	attacked_by.resize(bt->combat_categories);
	defence_power_vs.resize(bt->combat_categories);

	for(int i = 0; i < bt->combat_categories; ++i)
	{
		max_units_spotted[i] = 0;
		attacked_by[i] = 0;
		defence_power_vs[i] = 0;
	}
}

AAIBrain::~AAIBrain(void)
{
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
					dist = sqrt( pow((float)sector->x - current_sector->x, 2) + pow((float)sector->y - current_sector->y , 2) );
					
					my_rating = 1 / (1 + pow(sector->GetThreatTo(ground, air, hover, sea, submarine), 2.0f) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f));
				
				}
				else if(water && sector->water_ratio > 0.65)
				{
					dist = sqrt( pow((float)(sector->x - current_sector->x), 2) + pow((float)(sector->y - current_sector->y), 2) );

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

void AAIBrain::GetNewScoutDest(float3 *dest, int scout)
{
	*dest = ZeroVector;

	// TODO: take scouts pos into account
	float my_rating, best_rating = 0;
	AAISector *scout_sector = 0, *sector;

	const UnitDef *def = cb->GetUnitDef(scout);
	unsigned int scout_movement_type = bt->units_static[def->id].movement_type;

	float3 pos = cb->GetUnitPos(scout);

	// get continent 
	int continent = map->GetSmartContinentID(&pos, scout_movement_type);//map->GetContinentID(&pos);
	
	for(int x = 0; x < map->xSectors; ++x)
	{
		for(int y = 0; y < map->ySectors; ++y)
		{
			sector = &map->sector[x][y];
			
			if(sector->distance_to_base > 0 && scout_movement_type & sector->allowed_movement_types)
			{
				my_rating = sector->importance_this_game * sector->last_scout;
				++sector->last_scout;
				
				if(my_rating > best_rating)
				{
					// possible scout dest, try to find pos in sector
					pos = ZeroVector;

					sector->GetMovePos(&pos, scout_movement_type, continent); 
					
					if(pos.x > 0)
					{
						best_rating = my_rating;
						scout_sector = sector;
						*dest = pos;
					}
				}
			}
		}
	}

	// set dest sector as visited 
	if(dest->x > 0)
		scout_sector->last_scout = 1;
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
	if(sector->water_ratio < 0.4)
		++land_sectors;
	else if(sector->water_ratio < 0.6)
	{
		++land_sectors;
		++water_sectors;
	}
	else
		++water_sectors;

	sectors[0].push_back(sector);
	
	sector->SetBase(true);
}

void AAIBrain::RemoveSector(AAISector *sector)
{
	if(sector->water_ratio < 0.4)
		--land_sectors;
	else if(sector->water_ratio < 0.6)
	{
		--land_sectors;
		--water_sectors;
	}
	else
		--water_sectors;


	sectors[0].remove(sector);
	
	sector->SetBase(false);
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

	for(int i = 1; i < max_distance; ++i)
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

	//fprintf(ai->file, "Base has now %i direct neighbouring sectors\n", sectors[1].size());
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
	if(sectors[0].size() >= cfg->MAX_BASE_SIZE)
		return false;

	// now targets should contain all neighbouring sectors that are not currently part of the base
	// only once; select the sector with most metalspots and least danger
	AAISector *best_sector = 0; 
	float best_rating  = 0, my_rating;
	int spots;
	float dist;

	int max_search_dist = 1;

	// if aai is looking for a water sector to expand into ocean, allow bigger serach_dist
	if(sectorType == WATER_SECTOR &&  water_sectors == 0 && land_sectors > 1)
		max_search_dist = 2;

	for(int search_dist = 1; search_dist <= max_search_dist; ++search_dist)
	{
		for(list<AAISector*>::iterator t = sectors[search_dist].begin(); t != sectors[search_dist].end(); ++t)
		{
			// dont expand if enemy structures in sector && check for allied buildings 
			if((*t)->enemy_structures <= 0 && (*t)->allied_structures < 200 && map->team_sector_map[(*t)->x][(*t)->y] == -1)
			{
				// rate current sector
				spots = (*t)->GetNumberOfMetalSpots();
			
				my_rating = 2.0f + (float)spots / 2.0f;

				my_rating += 4.0f / (*t)->GetMapBorderDist();
			
				// minmize distance between sectors
				dist = 0.1f;

				for(list<AAISector*>::iterator sector = sectors[0].begin(); sector != sectors[0].end(); ++sector)
					dist += sqrt( pow( (float)((*t)->x - (*sector)->x) , 2) + pow( (float)((*t)->y - (*sector)->y) , 2)  );
				
				dist *= 3.0f;
				
				if(sectorType == LAND_SECTOR)
				{
					// prefer flat sectors without water
					my_rating += ((*t)->flat_ratio - (*t)->water_ratio) * 8.0f;
					my_rating /= dist;
				}
				else if(sectorType == WATER_SECTOR)
				{
					my_rating += 8.0f * (*t)->water_ratio;
					my_rating /= dist;

					// check for continent size (to prevent aai to expand into little ponds instead of big ocean)
					if((*t)->water_ratio > 0.3 && !(*t)->ConnectedToOcean())
						my_rating = 0;
				}
				else
					my_rating = 0;

			
				// choose higher rated sector
				if(my_rating > best_rating)
				{
					best_rating = my_rating;
					best_sector = *t;
				}
			}
		}
	}

	if(best_sector)
	{
		// add this sector to base
		AddSector(best_sector);

		// debug purposes:
		if(sectorType == LAND_SECTOR)
			fprintf(ai->file, "\nAdding land sector %i,%i to base; base size: %i \n\n", best_sector->x, best_sector->y, sectors[0].size());
		else
			fprintf(ai->file, "\nAdding water sector %i,%i to base; base size: %i \n\n", best_sector->x, best_sector->y, sectors[0].size());

		// update neighbouring sectors
		UpdateNeighbouringSectors();
		UpdateBaseCenter();

		// check if further expansion possible
		if(sectors[0].size() == cfg->MAX_BASE_SIZE)
			expandable = false;

		freeBaseSpots = true;

		return true;
	}

	
	return false;
}

void AAIBrain::UpdateMaxCombatUnitsSpotted(vector<float>& units_spotted)
{
	for(int i = 0; i < bt->combat_categories; ++i)
	{
		// decrease old values
		max_units_spotted[i] *= 0.996f;
	
		// check for new max values
		if(units_spotted[i] > max_units_spotted[i])
			max_units_spotted[i] = units_spotted[i];
	}
}

void AAIBrain::UpdateAttackedByValues()
{
	for(int i = 0; i < bt->combat_categories; ++i)
	{
		attacked_by[i] *= 0.96f;
	}
}

void AAIBrain::AttackedBy(int combat_category_id)
{
	// update counter for current game
	attacked_by[combat_category_id] += 1;

	// update counter for memory dependent on playtime
	bt->attacked_by_category[0][combat_category_id][GetGamePeriod()] += 1.0f;
}

void AAIBrain::UpdateDefenceCapabilities()
{
	for(int i = 0; i < bt->assault_categories.size(); ++i)
		defence_power_vs[i] = 0;

	if(cfg->AIR_ONLY_MOD)
	{
		for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
		{
			for(list<AAIGroup*>::iterator group = ai->group_list[*category].begin(); group != ai->group_list[*category].end(); ++group)
			{
				defence_power_vs[0] += (*group)->GetPowerVS(0);
				defence_power_vs[1] += (*group)->GetPowerVS(1);
				defence_power_vs[2] += (*group)->GetPowerVS(2);
				defence_power_vs[3] += (*group)->GetPowerVS(3);
			}
		}
	}
	else
	{	
		// anti air power
		for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
		{
			for(list<AAIGroup*>::iterator group = ai->group_list[*category].begin(); group != ai->group_list[*category].end(); ++group)
			{	
				if((*group)->group_unit_type == ASSAULT_UNIT)
				{
					if((*group)->category == GROUND_ASSAULT)
					{
						defence_power_vs[0] += (*group)->GetPowerVS(0);
						defence_power_vs[2] += (*group)->GetPowerVS(2);
					}
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
				else if((*group)->group_unit_type == ANTI_AIR_UNIT)
					defence_power_vs[1] += (*group)->GetPowerVS(1);
			}
		}
	}

	// debug
	/*fprintf(ai->file, "Defence capabilities:\n");

	for(int i = 0; i < bt->assault_categories.size(); ++i)
		fprintf(ai->file, "%-20s %f\n" , bt->GetCategoryString2(bt->GetAssaultCategoryOfID(i)),defence_power_vs[i]);
	*/
}

void AAIBrain::AddDefenceCapabilities(int def_id, UnitCategory category)
{
	if(cfg->AIR_ONLY_MOD)
	{
		defence_power_vs[0] += bt->units_static[def_id].efficiency[0];
		defence_power_vs[1] += bt->units_static[def_id].efficiency[1];
		defence_power_vs[2] += bt->units_static[def_id].efficiency[2];
		defence_power_vs[3] += bt->units_static[def_id].efficiency[3];
	}
	else
	{
		if(bt->GetUnitType(def_id) == ASSAULT_UNIT)
		{
			if(category == GROUND_ASSAULT)
			{
				defence_power_vs[0] += bt->units_static[def_id].efficiency[0];
				defence_power_vs[2] += bt->units_static[def_id].efficiency[2];
			}
			else if(category == HOVER_ASSAULT)
			{
				defence_power_vs[0] += bt->units_static[def_id].efficiency[0];
				defence_power_vs[2] += bt->units_static[def_id].efficiency[2];
				defence_power_vs[3] += bt->units_static[def_id].efficiency[3];
			}
			else if(category == SEA_ASSAULT)
			{
				defence_power_vs[2] += bt->units_static[def_id].efficiency[2];
				defence_power_vs[3] += bt->units_static[def_id].efficiency[3];
				defence_power_vs[4] += bt->units_static[def_id].efficiency[4];
			}
			else if(category == SUBMARINE_ASSAULT)
			{
				defence_power_vs[3] += bt->units_static[def_id].efficiency[3];
				defence_power_vs[4] += bt->units_static[def_id].efficiency[4];
			}
		}
		else if(bt->GetUnitType(def_id) == ANTI_AIR_UNIT)
			defence_power_vs[1] += bt->units_static[def_id].efficiency[1];
	}
}

void AAIBrain::SubtractDefenceCapabilities(int def_id, UnitCategory category)
{
}

float AAIBrain::Affordable()
{
	return 35.0 /(cb->GetMetalIncome() + 5.0);
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

	// determine elapsed game time
	int t = GetGamePeriod();

	// todo: improve selection
	category = UNKNOWN;

	MapType mapType = map->mapType;

	float ground_usefulness = map->map_usefulness[0][side] + bt->mod_usefulness[0][side][mapType];
	float air_usefulness = map->map_usefulness[1][side] + bt->mod_usefulness[1][side][mapType];
	float hover_usefulness = map->map_usefulness[2][side] + bt->mod_usefulness[2][side][mapType];
	float sea_usefulness = map->map_usefulness[3][side] + bt->mod_usefulness[3][side][mapType];
	float submarine_usefulness = map->map_usefulness[4][side] + bt->mod_usefulness[4][side][mapType];

	if(cfg->AIR_ONLY_MOD)
	{
		// determine effectiveness vs several other units
		anti_ground_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][0][t]) * ground_usefulness * (2.0f * attacked_by[0] + 1.0f) * (4.0f * max_units_spotted[0] + 0.2f) / (4.0f * defence_power_vs[0] + 1));
		anti_air_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][1][t]) * air_usefulness * (2.0f * attacked_by[1] + 1.0f) * (4.0f * max_units_spotted[1] + 0.2f) / (4.0f * defence_power_vs[1] + 1));
		anti_hover_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][2][t]) * hover_usefulness * (2.0f * attacked_by[2] + 1.0f) * (4.0f * max_units_spotted[2] + 0.2f) / (4.0f * defence_power_vs[2] + 1));
		anti_sea_urgency = (int) (2 + (0.05f + bt->attacked_by_category[1][3][t]) * sea_usefulness * (2.0f * attacked_by[3] + 1.0f) * (4.0f * max_units_spotted[3] + 0.2f) / (4.0f * defence_power_vs[3] + 1));
			
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
				if(rand()%1024 > 384)
					category = GROUND_ASSAULT;
				else if(rand()%1024 > 384)
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
		// choose unit category dependend on map type
		if(mapType == LAND_MAP)
		{
			// determine effectiveness vs several other units
			anti_ground_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][0][t]) * ground_usefulness * (2.0f * attacked_by[0] + 1.0f) * (4.0f * max_units_spotted[0] + 0.2f) / (4.0f * defence_power_vs[0] + 1));
			anti_air_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][1][t]) * air_usefulness * (2.0f * attacked_by[1] + 1.0f) * (4.0f * max_units_spotted[1] + 0.2f) / (4.0f * defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][2][t]) * hover_usefulness * (2.0f * attacked_by[2] + 1.0f) * (4.0f * max_units_spotted[2] + 0.2f) / (4.0f * defence_power_vs[2] + 1));
			
			for(int i = 0; i < execute->unitProductionRate; ++i)
			{
				ground_eff = 0;
				air_eff = 0;
				hover_eff = 0;
				stat_eff = 0;

				// determine unit type
				if(rand()%(cfg->AIRCRAFT_RATE * 100) < 100)
				{
					category = AIR_ASSAULT;

					k = rand()%1024;

					if(k < 384)
					{
						ground_eff = 4;
						hover_eff = 1;
					}
					else if(k < 640)
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
			anti_ground_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][0][t]) * ground_usefulness * (2.0f * attacked_by[0] + 1.0f) * (4.0f * max_units_spotted[0] + 0.2f) / (4.0f * defence_power_vs[0] + 1));
			anti_air_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][1][t]) * air_usefulness * (2.0f * attacked_by[1] + 1.0f) * (4.0f * max_units_spotted[1] + 0.2f) / (4.0f * defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][2][t]) * hover_usefulness * (2.0f * attacked_by[2] + 1.0f) * (4.0f * max_units_spotted[2] + 0.2f) / (4.0f * defence_power_vs[2] + 1));
			anti_sea_urgency = (int) (2 + (0.05f + bt->attacked_by_category[1][3][t]) * sea_usefulness * (2.0f * attacked_by[3] + 1.0f) * (4.0f * max_units_spotted[3] + 0.2f) / (4.0f * defence_power_vs[3] + 1));
			anti_submarine_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][4][t]) * submarine_usefulness * (2.0f * attacked_by[4] + 1.0f) * (4.0f * max_units_spotted[4] + 0.2f) / (4.0f * defence_power_vs[4] + 1));

			for(int i = 0; i < execute->unitProductionRate; ++i)
			{
				ground_eff = 0;
				air_eff = 0;
				hover_eff = 0;
				sea_eff = 0;
				submarine_eff = 0;
				stat_eff = 0;

				
				if(rand()%(cfg->AIRCRAFT_RATE * 100) < 100)
				{
					category = AIR_ASSAULT;
					
					if(rand()%1000 < 333)
					{
						ground_eff = 4;
						hover_eff = 1;
						sea_eff = 2;
					}
					else if(rand()%1000 < 333)
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
			anti_air_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][1][t]) * air_usefulness * (2.0f * attacked_by[1] + 1.0f) * (4.0f * max_units_spotted[1] + 0.2f) / (4.0f * defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][2][t]) * hover_usefulness * (2.0f * attacked_by[2] + 1.0f) * (4.0f * max_units_spotted[2] + 0.2f) / (4.0f * defence_power_vs[2] + 1));
			anti_sea_urgency = (int) (2 + (0.05f + bt->attacked_by_category[1][3][t]) * sea_usefulness * (2.0f * attacked_by[3] + 1.0f) * (4.0f * max_units_spotted[3] + 0.2f) / (4.0f * defence_power_vs[3] + 1));
			anti_submarine_urgency = (int)( 2 + (0.05f + bt->attacked_by_category[1][4][t]) * submarine_usefulness * (2.0f * attacked_by[4] + 1.0f) * (4.0f * max_units_spotted[4] + 0.2f) / (4.0f * defence_power_vs[4] + 1));
							
			for(int i = 0; i < execute->unitProductionRate; ++i)
			{	
				air_eff = 0;
				hover_eff = 0;
				sea_eff = 0;
				submarine_eff = 0;
				stat_eff = 0;

				if(rand()%(cfg->AIRCRAFT_RATE * 100) < 100)
				{
					category = AIR_ASSAULT;
					anti_submarine_urgency = (int) (1 + submarine_usefulness * (attacked_by[4] + 2) * (max_units_spotted[4] + 2) / (defence_power_vs[4] + 1));
				
					
					if(rand()%1000 < 333)
					{
						sea_eff = 4;
						hover_eff = 2;
					}
					else if(rand()%1000 < 333)
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
	float power = 2;
	float eff = 2;

	if(category != UNKNOWN)
	{	
		// determine speed, range & eff 
		if(rand()%cfg->FAST_UNITS_RATE == 1)
		{
			if(rand()%2 == 1)
				speed = 1;
			else
				speed = 2;
		}
		else
			speed = 0.1f;

		if(rand()%cfg->HIGH_RANGE_UNITS_RATE == 1)
		{
			int t = rand()%1000;

			if(t < 350)
				range = 0.75;
			else if(t == 700)
				range = 1.3f;
			else 
				t = 1.7f;
		}
		else
			range = 0.1f;

		if(rand()%3 == 1)
			power = 4;
		
		if(rand()%3 == 1)
			eff = 4;

		execute->BuildUnit(category, speed, cost, range, power, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, eff, urgent);
	}
}


int AAIBrain::GetGamePeriod()
{
	int tick = cb->GetCurrentFrame();
	
	if(tick < 18000)
		return 0;
	else if(tick < 36000)
		return 1;
	else if(tick < 72000)
		return 2;
	else
		return 3;
}

bool AAIBrain::IsSafeSector(AAISector *sector)
{
	// TODO: improve criteria
	if(sector->enemy_structures <= 0  && sector->lost_units[MOBILE_CONSTRUCTOR-COMMANDER]  < 1 && sector->threat <= 0)
		return true;
	else 
		return false;
}
