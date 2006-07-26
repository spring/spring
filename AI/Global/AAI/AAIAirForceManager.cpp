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

	targets = new AAIAirTarget[cfg->MAX_AIR_TARGETS];

	for(int i = 0; i < cfg->MAX_AIR_TARGETS; i++)
		targets[i].category = UNKNOWN;

	air_groups = &ai->group_list[AIR_ASSAULT];
}

AAIAirForceManager::~AAIAirForceManager(void)
{
	delete [] targets;
}

void AAIAirForceManager::CheckTarget(int unit, const UnitDef *def)
{
	// do not attack own units
	if(cb->GetMyTeam() != cb->GetUnitTeam(unit))
	{
		float3 pos = cb->GetUnitPos(unit);

		// calculate in which sector unit is located
		int x = pos.x/map->xSectorSize;
		int y = pos.z/map->ySectorSize;

		// check if unit is within the map
		if(x > map->xSectors && x < 0 && y < 0 && y > map->ySectors)
			return;

		// check for anti air defences
		if(map->sector[x][y].lost_units[AIR_ASSAULT] > 1.4)
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
				group = GetAirGroup(100.0, ANTI_AIR_UNIT);
			else if(category == STATIONARY_DEF)
				group = GetAirGroup(100.0, BOMBER_UNIT);
			else
				// todo: change to gunship 
				group = GetAirGroup(100.0, BOMBER_UNIT);

			if(group)
			{
				if(bt->units_static[def->id].category <= METAL_MAKER)
				{
					//cb->SendTextMsg("Air support on the way",0);
					group->BombTarget(unit, 110);
				}
				else
					group->AirRaidTarget(unit, 110);
			}
		}
	}
}

void AAIAirForceManager::CheckTarget(AAIAirTarget *target)
{
	//
	/*if(bt->units_static[target->def_id].efficiency[1] < 2.3)
	{
		AAIGroup *group = GetAirGroup(100.0);

		if(group)
		{
			group->BombTarget(target->pos, 120);
		}
	}*/
}

void AAIAirForceManager::AddTarget(float3 pos, int def_id, float cost, float health, UnitCategory category)
{
	if(pos.x <= 0 || pos.z <= 0)
		return;

	int i;

	for(i = 0; i < cfg->MAX_AIR_TARGETS; i++)
	{
		if(targets[i].category == UNKNOWN)
		{
			targets[i].pos.x = pos.x;
			targets[i].pos.z = pos.z;
			targets[i].def_id = def_id;
			targets[i].cost = cost;
			targets[i].health = health;
			targets[i].category = category;

			/*char c[100];
			sprintf(c, "Target: %f %f", targets[i].pos.x, targets[i].pos.z);
			cb->SendTextMsg(c ,0);*/

			return;
		}
	}

	// could not add target, randomly overwrite one of the existing targets
	i = rand()%cfg->MAX_AIR_TARGETS;
	targets[i].pos.x = pos.x;
	targets[i].pos.z = pos.z;
	targets[i].def_id = def_id;
	targets[i].cost = cost;
	targets[i].health = health;
	targets[i].category = category;

	/*char c[100];
	sprintf(c, "Target: %f %f", targets[i].pos.x, targets[i].pos.z);
	cb->SendTextMsg(c ,0);*/
}

void AAIAirForceManager::BombUnitsOfCategory(UnitCategory category)
{
	/*for(int i = 0; i < cfg->MAX_AIR_TARGETS; i++)
	{
		if(targets[i].category == category)
		{
			AAIGroup *group = GetAirGroup(100.0);

			if(group)
			{
				group->BombTarget(targets[i].pos, 115);
				targets[i].category = UNKNOWN;
			}
			
			return;
		}
	}*/
}

void AAIAirForceManager::BombBestUnit(float cost, float danger)
{
	float best = 0, current;
	int best_target = -1;
	int x, y, i;

	for(i = 0; i < cfg->MAX_AIR_TARGETS; i++)
	{
		if(targets[i].category != UNKNOWN)
		{
			x = targets[i].pos.x/map->xSectorSize;
			y = targets[i].pos.z/map->ySectorSize;

			current = targets[i].cost * cost / (targets[i].health + 1 + map->sector[x][y].threat_against[1] * danger);

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
			/*char c[100];
			sprintf(c, "Target: %f %f", targets[i].pos.x, targets[i].pos.z);
			cb->SendTextMsg(c ,0);*/
			//group->BombTarget(targets[i].pos, 110);
			targets[i].category = UNKNOWN;
		}
	}
}

AAIGroup* AAIAirForceManager::GetAirGroup(float importance, UnitType group_type)
{
	for(list<AAIGroup*>::iterator group = air_groups->begin(); group != air_groups->end(); ++group)
	{
		if((*group)->task_importance < importance && group_type == (*group)->group_type)
		{
			if(cfg->AIR_ONLY_MOD && (*group)->units.size() > cfg->MAX_GROUP_SIZE/2)
				return *group;
			else if((*group)->units.size() > cfg->MAX_GROUP_SIZE/4)
				return *group;
		}
	}

	return 0;
}
