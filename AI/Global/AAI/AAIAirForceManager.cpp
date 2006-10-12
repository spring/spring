//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------

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
		if(x >= 0 && x < map->xSectors && y >= 0 && y < map->ySectors)
		{
			// check for anti air defences
			if(map->sector[x][y].lost_units[AIR_ASSAULT] > 3)
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
				else if(category <= METAL_MAKER)
				{
					group = GetAirGroup(100.0, BOMBER_UNIT);

					if(!group)
						group = GetAirGroup(100.0, ASSAULT_UNIT);
				}
				else
					group = GetAirGroup(100.0, ASSAULT_UNIT);

				if(group)
				{
					if(group->group_type == BOMBER_UNIT || category <= METAL_MAKER)
					{
						Command c;
						c.id = CMD_ATTACK;
						c.params.push_back(pos.x);
						c.params.push_back(pos.y);
						c.params.push_back(pos.z);

						group->GiveOrder(&c, 110, UNIT_ATTACKING);

						group->task_importance = 110;

						// add enemy to target list
						ai->ut->AssignGroupToEnemy(unit, group);
					
					}	
					else if(group->group_type == ASSAULT_UNIT)
					{
						Command c;
						c.id = CMD_PATROL;
						c.params.push_back(pos.x);
						c.params.push_back(pos.y);
						c.params.push_back(pos.z);

						group->GiveOrder(&c, 110, UNIT_ATTACKING);

						group->task_importance = 110;

						// add enemy to target list
						ai->ut->AssignGroupToEnemy(unit, group);
					}
				}
			}
		}
	}
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

void AAIAirForceManager::BombBestUnit(float cost, float danger)
{
	/*float best = 0, current;
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
			char c[100];
			sprintf(c, "Target: %f %f", targets[i].pos.x, targets[i].pos.z);
			cb->SendTextMsg(c ,0);
			//group->BombTarget(targets[i].pos, 110);
			targets[i].category = UNKNOWN;
		}
	}*/
}

AAIGroup* AAIAirForceManager::GetAirGroup(float importance, UnitType group_type)
{
	if(cfg->AIR_ONLY_MOD)
	{
		for(list<AAIGroup*>::iterator group = air_groups->begin(); group != air_groups->end(); ++group)
		{
			if((*group)->task_importance < importance && group_type == (*group)->group_type  && (*group)->units.size() > (*group)->maxSize/2)
				return *group;
		}
	}
	else
	{
		
		for(list<AAIGroup*>::iterator group = air_groups->begin(); group != air_groups->end(); ++group)
		{
			if((*group)->task_importance < importance && group_type == (*group)->group_type && (*group)->units.size() >= (*group)->maxSize)
				return *group;
		}
	}

	return 0;
}
