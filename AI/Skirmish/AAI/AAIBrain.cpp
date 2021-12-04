// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAI.h"
#include "AAIBrain.h"
#include "AAIBuildTable.h"
#include "AAIExecute.h"
#include "AAIUnitTable.h"
#include "AAIConfig.h"
#include "AAIMap.h"
#include "AAIGroup.h"
#include "AAISector.h"

#include "LegacyCpp/UnitDef.h"
using namespace springLegacyAI;

AAIBrain::AAIBrain(AAI *ai)
{
	this->ai = ai;
	freeBaseSpots = false;
	expandable = true;

	max_distance = ai->Getmap()->xSectors + ai->Getmap()->ySectors - 2;
	sectors.resize(max_distance);

	base_center = ZeroVector;

	max_combat_units_spotted.resize(AAIBuildTable::ass_categories, 0);
	attacked_by.resize(AAIBuildTable::combat_categories, 0);
	defence_power_vs.resize(AAIBuildTable::ass_categories, 0);

	enemy_pressure_estimation = 0;
}

AAIBrain::~AAIBrain(void)
{
}


AAISector* AAIBrain::GetAttackDest(bool land, bool water)
{
	float best_rating = 0.0f, my_rating = 0.0f;
	AAISector *dest = 0, *sector;

	//int side = ai->Getside()-1;

	float ground =  1.0f;
	float air = 1.0f;
	float hover = 1.0f;
	float sea = 1.0f;
	float submarine = 1.0f;

	float def_power;

	// TODO: improve destination sector selection
	for(int x = 0; x < ai->Getmap()->xSectors; ++x)
	{
		for(int y = 0; y < ai->Getmap()->ySectors; ++y)
		{
			sector = &ai->Getmap()->sector[x][y];

			if(sector->distance_to_base == 0 || sector->enemy_structures == 0)
					my_rating = 0;
			else
			{
				if(land && sector->water_ratio < 0.4)
				{
					def_power = sector->GetEnemyDefencePower(ground, air, hover, sea, submarine);

					if(def_power) {
						my_rating = sector->enemy_structures / sector->GetEnemyDefencePower(ground, air, hover, sea, submarine);
					} else {
						my_rating = sector->enemy_structures / (2 * sector->GetEnemyDefencePower(ground, air, hover, sea, submarine) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f) + 1);
					}
					my_rating /= (8 + sector->distance_to_base);
				}
				else if(water && sector->water_ratio > 0.6)
				{
					my_rating = sector->enemy_structures / (2 * sector->GetEnemyDefencePower(ground, air, hover, sea, submarine) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f) + 1);
					my_rating /= (8 + sector->distance_to_base);
				}
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

	//int side = ai->Getside()-1;

	float ground = 1.0f;
	float air = 1.0f;
	float hover = 1.0f;
	float sea = 1.0f;
	float submarine = 1.0f;

	// TODO: improve destination sector selection
	for(int x = 0; x < ai->Getmap()->xSectors; x++)
	{
		for(int y = 0; y < ai->Getmap()->ySectors; y++)
		{
			sector = &ai->Getmap()->sector[x][y];

			if(sector->distance_to_base == 0 || sector->enemy_structures < 0.001f)
					my_rating = 0;
			else
			{
				if(land && sector->water_ratio < 0.35)
				{
					dist = sqrt( pow((float)sector->x - current_sector->x, 2) + pow((float)sector->y - current_sector->y , 2) );

					my_rating = 1.0f / (1.0f + pow(sector->GetEnemyDefencePower(ground, air, hover, sea, submarine), 2.0f) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f));
					my_rating /= (1.0f + dist);

				}
				else if(water && sector->water_ratio > 0.65)
				{
					dist = sqrt( pow((float)(sector->x - current_sector->x), 2) + pow((float)(sector->y - current_sector->y), 2) );

					my_rating = 1.0f / (1.0f + pow(sector->GetEnemyDefencePower(ground, air, hover, sea, submarine), 2.0f) + pow(sector->GetLostUnits(ground, air, hover, sea, submarine) + 1, 1.5f));
					my_rating /= (1.0f + dist);
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

	const UnitDef *def = ai->Getcb()->GetUnitDef(scout);
	unsigned int scout_movement_type = ai->Getbt()->units_static[def->id].movement_type;

	float3 pos = ai->Getcb()->GetUnitPos(scout);

	// get continent
	int continent = ai->Getmap()->GetSmartContinentID(&pos, scout_movement_type);//ai->Getmap()->GetContinentID(&pos);

	for(int x = 0; x < ai->Getmap()->xSectors; ++x)
	{
		for(int y = 0; y < ai->Getmap()->ySectors; ++y)
		{
			sector = &ai->Getmap()->sector[x][y];

			if(sector->distance_to_base > 0 && scout_movement_type & sector->allowed_movement_types)
			{
				if(enemy_pressure_estimation > 0.01f && sector->distance_to_base < 2)
					my_rating = sector->importance_this_game * sector->last_scout * (1.0f + sector->enemy_combat_units[5]);
				else
					my_rating = sector->importance_this_game * sector->last_scout;

				++sector->last_scout;

				if(my_rating > best_rating)
				{
					// possible scout dest, try to find pos in sector
					pos = ZeroVector;

					sector->GetMovePosOnContinent(&pos, scout_movement_type, continent);

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

	int metal = (ai->Getbt()->GetUnitDef(unit).buildTime/workertime) * (ai->Getcb()->GetMetalIncome()-(ai->Getcb()->GetMetalUsage()) + ai->Getcb()->GetMetal());
	int total_cost = ai->Getbt()->GetUnitDef(unit).metalCost;

	if(metal > total_cost)
		return true;

	return false;
}

bool AAIBrain::EnergyForConstr(int unit, int /*wokertime*/)
{

	// check energy
//	int energy =  ai->Getbt()->unitList[unit-1]->buildTime * (ai->Getcb()->GetEnergyIncome()-(ai->Getcb()->GetEnergyUsage()/2));

	// TODO: FIXME: add code here

	return true;
}

bool AAIBrain::RessourcesForConstr(int /*unit*/, int /*wokertime*/)
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

	sector->SetBase(true);

	// update base land/water ratio
	baseLandRatio = 0;
	baseWaterRatio = 0;

	for(list<AAISector*>::iterator s = sectors[0].begin(); s != sectors[0].end(); ++s)
	{
		baseLandRatio += (*s)->GetFlatRatio();
		baseWaterRatio += (*s)->GetWaterRatio();
	}

	baseLandRatio /= (float)sectors[0].size();
	baseWaterRatio /= (float)sectors[0].size();
}

void AAIBrain::RemoveSector(AAISector *sector)
{
	sectors[0].remove(sector);

	sector->SetBase(false);

	// update base land/water ratio
	baseLandRatio = 0;
	baseWaterRatio = 0;

	if(sectors[0].size() > 0)
	{
		for(list<AAISector*>::iterator s = sectors[0].begin(); s != sectors[0].end(); ++s)
		{
			baseLandRatio += (*s)->GetFlatRatio();
			baseWaterRatio += (*s)->GetWaterRatio();
		}

		baseLandRatio /= (float)sectors[0].size();
		baseWaterRatio /= (float)sectors[0].size();
	}
}


void AAIBrain::DefendCommander(int /*attacker*/)
{
//	float3 pos = ai->Getcb()->GetUnitPos(ai->Getut()->cmdr);
	//float importance = 120;
	Command c;

	// evacuate cmdr
	// TODO: FIXME: check/fix?/implement me?
	/*if(ai->cmdr->task != BUILDING)
	{
		AAISector *sector = GetSafestSector();

		if(sector != 0)
		{
			pos = sector->GetCenter();

			if(pos.x > 0 && pos.z > 0)
			{
				pos.y = ai->Getcb()->GetElevation(pos.x, pos.z);
				ai->Getexecute()->MoveUnitTo(ai->cmdr->unit_id, &pos);
			}
		}
	}*/
}

void AAIBrain::UpdateBaseCenter()
{
	base_center = ZeroVector;

	for(list<AAISector*>::iterator sector = sectors[0].begin(); sector != sectors[0].end(); ++sector)
	{
		base_center.x += (0.5 + (*sector)->x) * ai->Getmap()->xSectorSize;
		base_center.z += (0.5 + (*sector)->y) * ai->Getmap()->ySectorSize;
	}

	base_center.x /= sectors[0].size();
	base_center.z /= sectors[0].size();
}

void AAIBrain::UpdateNeighbouringSectors()
{
	int x,y,neighbours;

	// delete old values
	for(x = 0; x < ai->Getmap()->xSectors; ++x)
	{
		for(y = 0; y < ai->Getmap()->ySectors; ++y)
		{
			if(ai->Getmap()->sector[x][y].distance_to_base > 0)
				ai->Getmap()->sector[x][y].distance_to_base = -1;
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
			if(x > 0 && ai->Getmap()->sector[x-1][y].distance_to_base == -1)
			{
				ai->Getmap()->sector[x-1][y].distance_to_base = i;
				sectors[i].push_back(&ai->Getmap()->sector[x-1][y]);
				++neighbours;
			}
			// check right neighbour
			if(x < (ai->Getmap()->xSectors - 1) && ai->Getmap()->sector[x+1][y].distance_to_base == -1)
			{
				ai->Getmap()->sector[x+1][y].distance_to_base = i;
				sectors[i].push_back(&ai->Getmap()->sector[x+1][y]);
				++neighbours;
			}
			// check upper neighbour
			if(y > 0 && ai->Getmap()->sector[x][y-1].distance_to_base == -1)
			{
				ai->Getmap()->sector[x][y-1].distance_to_base = i;
				sectors[i].push_back(&ai->Getmap()->sector[x][y-1]);
				++neighbours;
			}
			// check lower neighbour
			if(y < (ai->Getmap()->ySectors - 1) && ai->Getmap()->sector[x][y+1].distance_to_base == -1)
			{
				ai->Getmap()->sector[x][y+1].distance_to_base = i;
				sectors[i].push_back(&ai->Getmap()->sector[x][y+1]);
				++neighbours;
			}

			if(i == 1 && !neighbours)
				(*sector)->interior = true;
		}
	}

	//ai->Log("Base has now %i direct neighbouring sectors\n", sectors[1].size());
}

bool AAIBrain::SectorInList(list<AAISector*> mylist, AAISector *sector)
{
	// check if sector already added to list
	for(list<AAISector*>::iterator t = mylist.begin(); t != mylist.end(); ++t)
	{
		if(*t == sector)
			return true;
	}
	return false;
}

float AAIBrain::GetBaseBuildspaceRatio(unsigned int building_move_type)
{
	if(building_move_type & MOVE_TYPE_STATIC_LAND)
	{
		if(baseLandRatio > 0.1f)
			return baseLandRatio;
		else
			return 0;
	}
	else if(building_move_type & MOVE_TYPE_STATIC_WATER)
	{
		if(baseWaterRatio > 0.1f)
			return baseWaterRatio;
		else
			return 0;
	}
	else
		return 1.0f;
}

bool AAIBrain::CommanderAllowedForConstructionAt(AAISector *sector, float3 *pos)
{
	// commander is always allowed in base
	if(sector->distance_to_base <= 0)
		return true;
	// allow construction close to base for small bases
	else if(sectors[0].size() < 3 && sector->distance_to_base <= 1)
		return true;
	// allow construction on islands close to base on water maps
	else if(ai->Getmap()->map_type == WATER_MAP && ai->Getcb()->GetElevation(pos->x, pos->z) >= 0 && sector->distance_to_base <= 3)
		return true;
	else
		return false;
}

bool AAIBrain::MexConstructionAllowedInSector(AAISector *sector)
{
	return sector->freeMetalSpots && IsSafeSector(sector) && (ai->Getmap()->team_sector_map[sector->x][sector->y] == -1 || ai->Getmap()->team_sector_map[sector->x][sector->y] == ai->Getcb()->GetMyAllyTeam());
}

bool AAIBrain::ExpandBase(SectorType sectorType)
{
	if(sectors[0].size() >= cfg->MAX_BASE_SIZE)
		return false;

	// now targets should contain all neighbouring sectors that are not currently part of the base
	// only once; select the sector with most metalspots and least danger
	AAISector *best_sector = NULL;
	float best_rating  = 0, my_rating;
	int spots;
	float dist;

	int max_search_dist = 1;

	// if aai is looking for a water sector to expand into ocean, allow greater search_dist
	if(sectorType == WATER_SECTOR &&  baseWaterRatio < 0.1)
		max_search_dist = 3;

	for(int search_dist = 1; search_dist <= max_search_dist; ++search_dist)
	{
		for(list<AAISector*>::iterator t = sectors[search_dist].begin(); t != sectors[search_dist].end(); ++t)
		{
			// dont expand if enemy structures in sector && check for allied buildings
			if(IsSafeSector(*t) && (*t)->allied_structures < 3 && ai->Getmap()->team_sector_map[(*t)->x][(*t)->y] == -1)
			{
				// rate current sector
				spots = (*t)->GetNumberOfMetalSpots();

				my_rating = 1.0f + (float)spots;

				if(sectorType == LAND_SECTOR)
					// prefer flat sectors without water
					my_rating += ((*t)->flat_ratio - (*t)->water_ratio) * 16.0f;
				else if(sectorType == WATER_SECTOR)
				{
					// check for continent size (to prevent aai to expand into little ponds instead of big ocean)
					if((*t)->water_ratio > 0.1 &&  (*t)->ConnectedToOcean())
						my_rating += 8.0f * (*t)->water_ratio;
					else
						my_rating = 0;
				}
				else // LAND_WATER_SECTOR
					my_rating += ((*t)->flat_ratio + (*t)->water_ratio) * 8.0f;

				// minmize distance between sectors
				dist = 0.1f;

				for(list<AAISector*>::iterator sector = sectors[0].begin(); sector != sectors[0].end(); ++sector) {
					dist += fastmath::apxsqrt( ((*t)->x - (*sector)->x) * ((*t)->x - (*sector)->x) + ((*t)->y - (*sector)->y) * ((*t)->y - (*sector)->y) );
				}

				float3 center = (*t)->GetCenter();
				my_rating /= (dist * fastmath::apxsqrt(ai->Getmap()->GetEdgeDistance( &center )) );

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
		{
			ai->Log("\nAdding land sector %i,%i to base; base size: " _STPF_, best_sector->x, best_sector->y, sectors[0].size());
			ai->Log("\nNew land : water ratio within base: %f : %f\n\n", baseLandRatio, baseWaterRatio);
		}
		else
		{
			ai->Log("\nAdding water sector %i,%i to base; base size: " _STPF_, best_sector->x, best_sector->y, sectors[0].size());
			ai->Log("\nNew land : water ratio within base: %f : %f\n\n", baseLandRatio, baseWaterRatio);
		}

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

void AAIBrain::UpdateMaxCombatUnitsSpotted(vector<unsigned short>& units_spotted)
{
	for(int i = 0; i < AAIBuildTable::ass_categories; ++i)
	{
		// decrease old values
		max_combat_units_spotted[i] *= 0.996f;

		// check for new max values
		if((float)units_spotted[i] > max_combat_units_spotted[i])
			max_combat_units_spotted[i] = (float)units_spotted[i];
	}
}

void AAIBrain::UpdateAttackedByValues()
{
	for(int i = 0; i < AAIBuildTable::ass_categories; ++i)
	{
		attacked_by[i] *= 0.96f;
	}
}

void AAIBrain::AttackedBy(int combat_category_id)
{
	// update counter for current game
	attacked_by[combat_category_id] += 1;

	// update counter for memory dependent on playtime
	ai->Getbt()->attacked_by_category_current[GetGamePeriod()][combat_category_id] += 1.0f;
}

void AAIBrain::UpdateDefenceCapabilities()
{
	for(int i = 0; i < ai->Getbt()->assault_categories.size(); ++i)
		defence_power_vs[i] = 0;
	fill(defence_power_vs.begin(), defence_power_vs.end(), 0.0f);

	if(cfg->AIR_ONLY_MOD)
	{
		for(list<UnitCategory>::iterator category = ai->Getbt()->assault_categories.begin(); category != ai->Getbt()->assault_categories.end(); ++category)
		{
			for(list<AAIGroup*>::iterator group = ai->Getgroup_list()[*category].begin(); group != ai->Getgroup_list()[*category].end(); ++group)
			{
				defence_power_vs[0] += (*group)->GetCombatPowerVsCategory(0);
				defence_power_vs[1] += (*group)->GetCombatPowerVsCategory(1);
				defence_power_vs[2] += (*group)->GetCombatPowerVsCategory(2);
				defence_power_vs[3] += (*group)->GetCombatPowerVsCategory(3);
			}
		}
	}
	else
	{
		// anti air power
		for(list<UnitCategory>::iterator category = ai->Getbt()->assault_categories.begin(); category != ai->Getbt()->assault_categories.end(); ++category)
		{
			for(list<AAIGroup*>::iterator group = ai->Getgroup_list()[*category].begin(); group != ai->Getgroup_list()[*category].end(); ++group)
			{
				if((*group)->group_unit_type == ASSAULT_UNIT)
				{
					if((*group)->category == GROUND_ASSAULT)
					{
						defence_power_vs[0] += (*group)->GetCombatPowerVsCategory(0);
						defence_power_vs[2] += (*group)->GetCombatPowerVsCategory(2);
					}
					else if((*group)->category == HOVER_ASSAULT)
					{
						defence_power_vs[0] += (*group)->GetCombatPowerVsCategory(0);
						defence_power_vs[2] += (*group)->GetCombatPowerVsCategory(2);
						defence_power_vs[3] += (*group)->GetCombatPowerVsCategory(3);
					}
					else if((*group)->category == SEA_ASSAULT)
					{
						defence_power_vs[2] += (*group)->GetCombatPowerVsCategory(2);
						defence_power_vs[3] += (*group)->GetCombatPowerVsCategory(3);
						defence_power_vs[4] += (*group)->GetCombatPowerVsCategory(4);
					}
					else if((*group)->category == SUBMARINE_ASSAULT)
					{
						defence_power_vs[3] += (*group)->GetCombatPowerVsCategory(3);
						defence_power_vs[4] += (*group)->GetCombatPowerVsCategory(4);
					}
				}
				else if((*group)->group_unit_type == ANTI_AIR_UNIT)
					defence_power_vs[1] += (*group)->GetCombatPowerVsCategory(1);
			}
		}
	}

	// debug
	/*ai->Log("Defence capabilities:\n");

	for(int i = 0; i < ai->Getbt()->assault_categories.size(); ++i)
		ai->Log("%-20s %f\n" , ai->Getbt()->GetCategoryString2(ai->Getbt()->GetAssaultCategoryOfID(i)),defence_power_vs[i]);
	*/
}

void AAIBrain::AddDefenceCapabilities(int def_id, UnitCategory category)
{
	if(cfg->AIR_ONLY_MOD)
	{
		defence_power_vs[0] += ai->Getbt()->units_static[def_id].efficiency[0];
		defence_power_vs[1] += ai->Getbt()->units_static[def_id].efficiency[1];
		defence_power_vs[2] += ai->Getbt()->units_static[def_id].efficiency[2];
		defence_power_vs[3] += ai->Getbt()->units_static[def_id].efficiency[3];
	}
	else
	{
		if(ai->Getbt()->GetUnitType(def_id) == ASSAULT_UNIT)
		{
			if(category == GROUND_ASSAULT)
			{
				defence_power_vs[0] += ai->Getbt()->units_static[def_id].efficiency[0];
				defence_power_vs[2] += ai->Getbt()->units_static[def_id].efficiency[2];
			}
			else if(category == HOVER_ASSAULT)
			{
				defence_power_vs[0] += ai->Getbt()->units_static[def_id].efficiency[0];
				defence_power_vs[2] += ai->Getbt()->units_static[def_id].efficiency[2];
				defence_power_vs[3] += ai->Getbt()->units_static[def_id].efficiency[3];
			}
			else if(category == SEA_ASSAULT)
			{
				defence_power_vs[2] += ai->Getbt()->units_static[def_id].efficiency[2];
				defence_power_vs[3] += ai->Getbt()->units_static[def_id].efficiency[3];
				defence_power_vs[4] += ai->Getbt()->units_static[def_id].efficiency[4];
			}
			else if(category == SUBMARINE_ASSAULT)
			{
				defence_power_vs[3] += ai->Getbt()->units_static[def_id].efficiency[3];
				defence_power_vs[4] += ai->Getbt()->units_static[def_id].efficiency[4];
			}
		}
		else if(ai->Getbt()->GetUnitType(def_id) == ANTI_AIR_UNIT)
			defence_power_vs[1] += ai->Getbt()->units_static[def_id].efficiency[1];
	}
}

/*
void AAIBrain::SubtractDefenceCapabilities(int def_id, UnitCategory category)
{
}
*/

float AAIBrain::Affordable()
{
	return 25.0f /(ai->Getcb()->GetMetalIncome() + 5.0f);
}

void AAIBrain::BuildUnits()
{
	//int side = ai->Getside()-1;
	bool urgent = false;
	int k;
	unsigned int allowed_move_type = 0;

	float cost = 1.0f + Affordable()/12.0f;

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
	int game_period = GetGamePeriod();

	float ground = GetAttacksBy(0, game_period);
	float air = GetAttacksBy(0, game_period);
	float hover = GetAttacksBy(0, game_period);
	float sea = GetAttacksBy(0, game_period);
	float submarine = GetAttacksBy(0, game_period);

	if(cfg->AIR_ONLY_MOD)
	{
		// determine effectiveness vs several other units
		anti_ground_urgency = (int)( 2 + (0.05f + ground) * (2.0f * attacked_by[0] + 1.0f) * (4.0f * max_combat_units_spotted[0] + 0.2f) / (4.0f * defence_power_vs[0] + 1));
		anti_air_urgency = (int)( 2 + (0.05f + air) * (2.0f * attacked_by[1] + 1.0f) * (4.0f * max_combat_units_spotted[1] + 0.2f) / (4.0f * defence_power_vs[1] + 1));
		anti_hover_urgency = (int)( 2 + (0.05f + hover) * (2.0f * attacked_by[2] + 1.0f) * (4.0f * max_combat_units_spotted[2] + 0.2f) / (4.0f * defence_power_vs[2] + 1));
		anti_sea_urgency = (int) (2 + (0.05f + sea) * (2.0f * attacked_by[3] + 1.0f) * (4.0f * max_combat_units_spotted[3] + 0.2f) / (4.0f * defence_power_vs[3] + 1));

		for(int i = 0; i < ai->Getexecute()->unitProductionRate; ++i)
		{
			ground_eff = 0;
			air_eff = 0;
			hover_eff = 0;
			sea_eff = 0;

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

			BuildUnitOfMovementType(MOVE_TYPE_AIR, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, urgent);
		}
	}
	else
	{
		// choose unit category dependend on map type
		if(ai->Getmap()->map_type == LAND_MAP)
		{
			// determine effectiveness vs several other units
			anti_ground_urgency = (int)( 2 + (0.1f + ground) * (attacked_by[0] + 1.0f) * (4.0f * max_combat_units_spotted[0] + 0.2f) / (4.0f * defence_power_vs[0] + 1));
			anti_air_urgency = (int)( 2 + (0.1f + air) * (attacked_by[1] + 1.0f) * (4.0f * max_combat_units_spotted[1] + 0.2f) / (4.0f * defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 2 + (0.1f + hover) * (attacked_by[2] + 1.0f) * (4.0f * max_combat_units_spotted[2] + 0.2f) / (4.0f * defence_power_vs[2] + 1));

			for(int i = 0; i < ai->Getexecute()->unitProductionRate; ++i)
			{
				ground_eff = 0;
				air_eff = 0;
				hover_eff = 0;
				stat_eff = 0;

				// determine unit type
				if(rand()%(cfg->AIRCRAFT_RATE * 100) < 100)
				{
					allowed_move_type |= MOVE_TYPE_AIR;

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
					allowed_move_type |= MOVE_TYPE_GROUND;
					allowed_move_type |= MOVE_TYPE_HOVER;

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

						if(anti_air_urgency > 2.0f * anti_ground_urgency)
							urgent = true;
					}
					else
					{
						ground_eff = 1;
						hover_eff = 4;
					}
				}

				BuildUnitOfMovementType(allowed_move_type, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, urgent);
			}
		}
		else if(ai->Getmap()->map_type == LAND_WATER_MAP)
		{
			// determine effectiveness vs several other units
			anti_ground_urgency = (int)( 2 + (0.1f + ground) * (2.0f * attacked_by[0] + 1.0f) * (4.0f * max_combat_units_spotted[0] + 0.2f) / (4.0f * defence_power_vs[0] + 1));
			anti_air_urgency = (int)( 2 + (0.1f + air) * (2.0f * attacked_by[1] + 1.0f) * (4.0f * max_combat_units_spotted[1] + 0.2f) / (4.0f * defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 2 + (0.1f + hover) * (2.0f * attacked_by[2] + 1.0f) * (4.0f * max_combat_units_spotted[2] + 0.2f) / (4.0f * defence_power_vs[2] + 1));
			anti_sea_urgency = (int) (2 + (0.1f + sea) * (2.0f * attacked_by[3] + 1.0f) * (4.0f * max_combat_units_spotted[3] + 0.2f) / (4.0f * defence_power_vs[3] + 1));
			anti_submarine_urgency = (int)( 2 + (0.1f + submarine) * (2.0f * attacked_by[4] + 1.0f) * (4.0f * max_combat_units_spotted[4] + 0.2f) / (4.0f * defence_power_vs[4] + 1));

			for(int i = 0; i < ai->Getexecute()->unitProductionRate; ++i)
			{
				ground_eff = 0;
				air_eff = 0;
				hover_eff = 0;
				sea_eff = 0;
				submarine_eff = 0;
				stat_eff = 0;


				if(rand()%(cfg->AIRCRAFT_RATE * 100) < 100)
				{
					allowed_move_type |= MOVE_TYPE_AIR;

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
					allowed_move_type |= MOVE_TYPE_HOVER;

					k = rand()%(anti_ground_urgency + anti_air_urgency + anti_hover_urgency + anti_sea_urgency + anti_submarine_urgency);

					if(k < anti_ground_urgency)
					{
						stat_eff = 2;
						ground_eff = 5;
						hover_eff = 1;
						allowed_move_type |= MOVE_TYPE_GROUND;
					}
					else if(k < anti_ground_urgency + anti_air_urgency)
					{
						allowed_move_type |= MOVE_TYPE_GROUND;
						allowed_move_type |= MOVE_TYPE_SEA;

						air_eff = 4;

						if(anti_air_urgency > 2.0f * anti_ground_urgency)
							urgent = true;
					}
					else if(k < anti_ground_urgency + anti_air_urgency + anti_hover_urgency)
					{
						allowed_move_type |= MOVE_TYPE_GROUND;
						allowed_move_type |= MOVE_TYPE_SEA;

						ground_eff = 1;
						hover_eff = 4;
					}
					else if(k < anti_ground_urgency + anti_air_urgency + anti_hover_urgency + anti_sea_urgency)
					{
						allowed_move_type |= MOVE_TYPE_SEA;
						hover_eff = 1;
						sea_eff = 4;
						submarine_eff = 1;
					}
					else
					{
						allowed_move_type |= MOVE_TYPE_SEA;
						submarine_eff = 4;
						sea_eff = 1;
					}
				}

				BuildUnitOfMovementType(allowed_move_type, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff,stat_eff, urgent);
			}
		}
		else if(ai->Getmap()->map_type == WATER_MAP)
		{
			// determine effectiveness vs several other units
			anti_air_urgency = (int)( 2 + (0.1f + air) * (2.0f * attacked_by[1] + 1.0f) * (4.0f * max_combat_units_spotted[1] + 0.2f) / (4.0f * defence_power_vs[1] + 1));
			anti_hover_urgency = (int)( 2 + (0.1f + hover) * (2.0f * attacked_by[2] + 1.0f) * (4.0f * max_combat_units_spotted[2] + 0.2f) / (4.0f * defence_power_vs[2] + 1));
			anti_sea_urgency = (int) (2 + (0.1f + sea) * (2.0f * attacked_by[3] + 1.0f) * (4.0f * max_combat_units_spotted[3] + 0.2f) / (4.0f * defence_power_vs[3] + 1));
			anti_submarine_urgency = (int)( 2 + (0.1f + submarine) * (2.0f * attacked_by[4] + 1.0f) * (4.0f * max_combat_units_spotted[4] + 0.2f) / (4.0f * defence_power_vs[4] + 1));

			for(int i = 0; i < ai->Getexecute()->unitProductionRate; ++i)
			{
				air_eff = 0;
				hover_eff = 0;
				sea_eff = 0;
				submarine_eff = 0;
				stat_eff = 0;

				if(rand()%(cfg->AIRCRAFT_RATE * 100) < 100)
				{
					allowed_move_type |= MOVE_TYPE_AIR;

					if(rand()%1000 < 333)
					{
						sea_eff = 4;
						hover_eff = 2;
					}
					else if(rand()%1000 < 333)
						air_eff = 4;
					else
						stat_eff = 4;
				}
				else
				{
					allowed_move_type |= MOVE_TYPE_HOVER;
					allowed_move_type |= MOVE_TYPE_SEA;

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
						submarine_eff = 1;
					}
				}

				BuildUnitOfMovementType(allowed_move_type, cost, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff,stat_eff, urgent);
			}
		}
	}
}

void AAIBrain::BuildUnitOfMovementType(unsigned int allowed_move_type, float cost, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, bool urgent)
{
	float speed = 0;
	float range = 0;
	float power = 2;
	float eff = 2;

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
		const int t = rand()%1000;

		if(t < 350)
			range = 0.75;
		else if(t == 700)
			range = 1.3f;
		else
			range = 1.7f;
	}
	else
		range = 0.1f;

	if(rand()%3 == 1)
		power = 4;

	if(rand()%3 == 1)
		eff = 4;

	// start selection
	int unit = 0, ground = 0, hover = 0;

	if(allowed_move_type & MOVE_TYPE_AIR)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			unit = ai->Getbt()->GetRandomUnit(ai->Getbt()->units_of_category[AIR_ASSAULT][ai->Getside()-1]);
		else
		{
			unit = ai->Getbt()->GetAirAssault(ai->Getside(), power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 9, false);

			if(unit && ai->Getbt()->units_dynamic[unit].constructorsAvailable + ai->Getbt()->units_dynamic[unit].constructorsRequested <= 0)
			{
				ai->Getbt()->BuildFactoryFor(unit);
				unit = ai->Getbt()->GetAirAssault(ai->Getside(), power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 9, true);
			}
		}
	}

	if(allowed_move_type & MOVE_TYPE_GROUND)
	{
		// choose random unit (to learn more)
		if(rand()%cfg->LEARN_RATE == 1)
			ground = ai->Getbt()->GetRandomUnit(ai->Getbt()->units_of_category[GROUND_ASSAULT][ai->Getside()-1]);
		else
		{
			ground = ai->Getbt()->GetGroundAssault(ai->Getside(), power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 15, false);

			if(ground && ai->Getbt()->units_dynamic[ground].constructorsAvailable + ai->Getbt()->units_dynamic[ground].constructorsRequested <= 0)
			{
				ai->Getbt()->BuildFactoryFor(ground);
				ground = ai->Getbt()->GetGroundAssault(ai->Getside(), power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 15, true);
			}
		}
	}

	if(allowed_move_type & MOVE_TYPE_HOVER)
	{
		if(rand()%cfg->LEARN_RATE == 1)
			hover = ai->Getbt()->GetRandomUnit(ai->Getbt()->units_of_category[HOVER_ASSAULT][ai->Getside()-1]);
		else
		{
			hover = ai->Getbt()->GetHoverAssault(ai->Getside(), power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 9, false);

			if(hover && ai->Getbt()->units_dynamic[hover].constructorsAvailable + ai->Getbt()->units_dynamic[hover].constructorsRequested <= 0)
			{
				ai->Getbt()->BuildFactoryFor(hover);
				hover = ai->Getbt()->GetHoverAssault(ai->Getside(), power, ground_eff, air_eff, hover_eff, sea_eff, stat_eff, eff, speed, range, cost, 9, true);
			}
		}
	}

	if(allowed_move_type & MOVE_TYPE_SEA)
	{
		int ship, submarine;

		if(rand()%cfg->LEARN_RATE == 1)
		{
			ship = ai->Getbt()->GetRandomUnit(ai->Getbt()->units_of_category[SEA_ASSAULT][ai->Getside()-1]);
			submarine = ai->Getbt()->GetRandomUnit(ai->Getbt()->units_of_category[SUBMARINE_ASSAULT][ai->Getside()-1]);
		}
		else
		{
			ship = ai->Getbt()->GetSeaAssault(ai->Getside(), power, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, eff, speed, range, cost, 9, false);

			if(ship && ai->Getbt()->units_dynamic[ship].constructorsAvailable + ai->Getbt()->units_dynamic[ship].constructorsRequested <= 0)
			{
				ai->Getbt()->BuildFactoryFor(ship);
				ship = ai->Getbt()->GetSeaAssault(ai->Getside(), power, ground_eff, air_eff, hover_eff, sea_eff, submarine_eff, stat_eff, eff, speed, range, cost, 9, false);
			}

			submarine = ai->Getbt()->GetSubmarineAssault(ai->Getside(), power, sea_eff, submarine_eff, stat_eff, eff, speed, range, cost, 9, false);

			if(submarine && ai->Getbt()->units_dynamic[submarine].constructorsAvailable + ai->Getbt()->units_dynamic[submarine].constructorsRequested <= 0)
			{
				ai->Getbt()->BuildFactoryFor(submarine);
				submarine = ai->Getbt()->GetSubmarineAssault(ai->Getside(), power, sea_eff, submarine_eff, stat_eff, eff, speed, range, cost, 9, false);
			}
		}

		// determine better unit for desired purpose
		if(ship)
		{
			if(submarine)
				unit = ai->Getbt()->DetermineBetterUnit(ship, submarine, 0, air_eff, hover_eff, sea_eff, submarine_eff, speed, range, cost);
			else
				unit = ship;
		}
		else if(submarine)
			unit = submarine;
	}

	// ground and hover assault
	if(ground)
	{
		if(hover)
			unit = ai->Getbt()->DetermineBetterUnit(ground, hover, ground_eff, air_eff, hover_eff, sea_eff, 0, speed, range, cost);
		else
			unit = ground;
	}
	// hover and sea
	else if(hover)
	{
		if(unit)
			unit =  ai->Getbt()->DetermineBetterUnit(unit, hover, 0, air_eff, hover_eff, sea_eff, submarine_eff, speed, range, cost);
		else
			unit = hover;
	}

	if(unit)
	{
		if(ai->Getbt()->units_dynamic[unit].constructorsAvailable > 0)
		{
			if(ai->Getbt()->units_static[unit].cost < cfg->MAX_COST_LIGHT_ASSAULT * ai->Getbt()->max_cost[ai->Getbt()->units_static[unit].category][ai->Getside()-1])
			{
				if(ai->Getexecute()->AddUnitToBuildqueue(unit, 3, urgent))
				{
					ai->Getbt()->units_dynamic[unit].requested += 3;
					ai->Getut()->UnitRequested(ai->Getbt()->units_static[unit].category, 3);
				}
			}
			else if(ai->Getbt()->units_static[unit].cost < cfg->MAX_COST_MEDIUM_ASSAULT * ai->Getbt()->max_cost[ai->Getbt()->units_static[unit].category][ai->Getside()-1])
			{
				if(ai->Getexecute()->AddUnitToBuildqueue(unit, 2, urgent))
					ai->Getbt()->units_dynamic[unit].requested += 2;
					ai->Getut()->UnitRequested(ai->Getbt()->units_static[unit].category, 2);
			}
			else
			{
				if(ai->Getexecute()->AddUnitToBuildqueue(unit, 1, urgent))
					ai->Getbt()->units_dynamic[unit].requested += 1;
					ai->Getut()->UnitRequested(ai->Getbt()->units_static[unit].category);
			}
		}
		else if(ai->Getbt()->units_dynamic[unit].constructorsRequested <= 0)
			ai->Getbt()->BuildFactoryFor(unit);
	}
}


int AAIBrain::GetGamePeriod()
{
	int tick = ai->Getcb()->GetCurrentFrame();

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
	return (sector->lost_units[MOBILE_CONSTRUCTOR-COMMANDER] < 0.5
		&& sector->enemy_combat_units[5] < 0.1 && sector->enemy_structures < 0.01
		&& sector->enemies_on_radar == 0);
}

float AAIBrain::GetAttacksBy(int combat_category, int game_period)
{
	return (ai->Getbt()->attacked_by_category_current[game_period][combat_category] + ai->Getbt()->attacked_by_category_learned[ai->Getmap()->map_type][game_period][combat_category]) / 2.0f;
}

void AAIBrain::UpdatePressureByEnemy()
{
	enemy_pressure_estimation = 0;

	// check base and neighbouring sectors for enemies
	for(list<AAISector*>::iterator s = sectors[0].begin(); s != sectors[0].end(); ++s)
		enemy_pressure_estimation += 0.1f * (*s)->enemy_combat_units[5];

	for(list<AAISector*>::iterator s = sectors[1].begin(); s != sectors[1].end(); ++s)
		enemy_pressure_estimation += 0.1f * (*s)->enemy_combat_units[5];

	if(enemy_pressure_estimation > 1.0f)
		enemy_pressure_estimation = 1.0f;
}
