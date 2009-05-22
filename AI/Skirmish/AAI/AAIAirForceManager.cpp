// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIAirForceManager.h"

#include "AAI.h"
#include "AAIBuildTable.h"
#include "AAIGroup.h"
#include "AAIMap.h"

AAIAirForceManager::AAIAirForceManager(AAI *ai, IAICallback *cb, AAIBuildTable *bt)
{
	this->ai = ai;
	this->bt = bt;
	this->cb = cb;
	this->map = ai->map;

	my_team = cb->GetMyTeam();
	num_of_targets = 0;

	targets.resize(cfg->MAX_AIR_TARGETS);

	for(int i = 0; i < cfg->MAX_AIR_TARGETS; ++i)
		targets[i].unit_id = -1;

	air_groups = &ai->group_list[AIR_ASSAULT];
}

AAIAirForceManager::~AAIAirForceManager(void)
{

}

void AAIAirForceManager::CheckTarget(int unit_id, const UnitDef *def)
{
	// do not attack own units
	if(my_team != cb->GetUnitTeam(unit_id))
	{
		float3 pos = cb->GetUnitPos(unit_id);

		// calculate in which sector unit is located
		int x = pos.x/map->xSectorSize;
		int y = pos.z/map->ySectorSize;

		// check if unit is within the map
		if(x >= 0 && x < map->xSectors && y >= 0 && y < map->ySectors)
		{
			// check for anti air defences if low on units
			if(map->sector[x][y].lost_units[AIR_ASSAULT-COMMANDER] >= cfg->MAX_AIR_GROUP_SIZE && ai->group_list[AIR_ASSAULT].size() < 5)
				return;

			AAIGroup *group;
			int max_groups;

			UnitCategory category = bt->units_static[def->id].category;

			if(bt->unitList[def->id-1]->health > 8000)
				max_groups = 3;
			else if(bt->unitList[def->id-1]->health > 4000)
				max_groups = 2;
			else
				max_groups = 1;

			for(int i = 0; i < max_groups; ++i)
			{
				if(category == AIR_ASSAULT)
				{
					group = GetAirGroup(100.0, ANTI_AIR_UNIT);

					if(group)
						group->DefendAirSpace(&pos);
				}
				else if(category <= METAL_MAKER)
				{
					group = GetAirGroup(100.0, BOMBER_UNIT);

					if(group)
						group->BombTarget(unit_id, &pos);
				}
				else
				{
					group = GetAirGroup(100.0, ASSAULT_UNIT);

					if(group)
						group->AirRaidUnit(unit_id);
				}
			}
		}
	}
}

void AAIAirForceManager::CheckBombTarget(int unit_id, int def_id)
{
	// dont continue if target list already full
	if(num_of_targets >= cfg->MAX_AIR_TARGETS)
		return;

	// do not add own units or units that ar already on target list
	if(my_team != cb->GetUnitTeam(unit_id) && !IsTarget(unit_id))
	{
		float3 pos = cb->GetUnitPos(unit_id);

		// calculate in which sector unit is located
		int x = pos.x/map->xSectorSize;
		int y = pos.z/map->ySectorSize;

		// check if unit is within the map
		if(x >= 0 && x < map->xSectors && y >= 0 && y < map->ySectors)
		{
			AddTarget(unit_id, def_id);
		}
	}
}

void AAIAirForceManager::AddTarget(int unit_id, int def_id)
{
	for(int i = 0; i < cfg->MAX_AIR_TARGETS; ++i)
	{
		if(targets[i].unit_id == -1)
		{
			ai->cb->SendTextMsg("Target added...", 0);

			targets[i].pos = cb->GetUnitPos(unit_id);
			targets[i].def_id = def_id;
			targets[i].cost = bt->units_static[def_id].cost;
			targets[i].health = cb->GetUnitHealth(unit_id);
			targets[i].category = bt->units_static[def_id].category;

			ai->ut->units[unit_id].status = BOMB_TARGET;

			++num_of_targets;

			return;
		}
	}

	// could not add target, randomly overwrite one of the existing targets
	/*i = rand()%cfg->MAX_AIR_TARGETS;
	targets[i].pos.x = pos.x;
	targets[i].pos.z = pos.z;
	targets[i].def_id = def_id;
	targets[i].cost = cost;
	targets[i].health = health;
	targets[i].category = category;*/
}

void AAIAirForceManager::RemoveTarget(int unit_id)
{
	for(int i = 0; i < cfg->MAX_AIR_TARGETS; ++i)
	{
		if(targets[i].unit_id == unit_id)
		{
			ai->cb->SendTextMsg("Target removed...", 0);

			targets[i].unit_id = -1;

			ai->ut->units[unit_id].status = ENEMY_UNIT;

			--num_of_targets;

			return;
		}
	}
}

void AAIAirForceManager::BombBestUnit(float cost, float danger)
{
	float best = 0, current;
	int best_target = -1;
	int x, y, i;

	for(i = 0; i < cfg->MAX_AIR_TARGETS; ++i)
	{
		if(targets[i].unit_id != -1)
		{
			x = targets[i].pos.x/map->xSectorSize;
			y = targets[i].pos.z/map->ySectorSize;

			current = pow(targets[i].cost, cost) / (1.0f + map->sector[x][y].enemy_stat_combat_power[1] * danger) * targets[i].health / bt->unitList[targets[i].def_id-1]->health ;

			if(current > best)
			{
				best = current;
				best_target = i;
			}
		}
	}

	if(best_target != -1)
	{
		AAIGroup *group = GetAirGroup(100.0, BOMBER_UNIT);

		if(group)
		{
			//ai->cb->SendTextMsg("Bombing...", 0);
			group->BombTarget(targets[i].unit_id, &targets[i].pos);

			targets[i].unit_id = -1;
			--num_of_targets;
		}
	}
}

AAIGroup* AAIAirForceManager::GetAirGroup(float importance, UnitType group_type)
{
	if(cfg->AIR_ONLY_MOD)
	{
		for(list<AAIGroup*>::iterator group = air_groups->begin(); group != air_groups->end(); ++group)
		{
			if((*group)->task_importance < importance && group_type == (*group)->group_unit_type  && (*group)->units.size() > (*group)->maxSize/2)
				return *group;
		}
	}
	else
	{
		for(list<AAIGroup*>::iterator group = air_groups->begin(); group != air_groups->end(); ++group)
		{
			if((*group)->task_importance < importance && group_type == (*group)->group_unit_type && (*group)->units.size() >= (*group)->maxSize)
				return *group;
		}
	}

	return 0;
}

bool AAIAirForceManager::IsTarget(int unit_id)
{
	for(int i = 0; i < cfg->MAX_AIR_TARGETS; ++i)
	{
		if(targets[i].unit_id == unit_id)
			return true;
	}

	return false;
}
