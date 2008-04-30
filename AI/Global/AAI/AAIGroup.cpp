// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIGroup.h"
#include "AAI.h"
#include "AAIBuildTable.h"
#include "AAIAttack.h"

AAIGroup::AAIGroup(IAICallback *cb, AAI *ai, const UnitDef *def, UnitType unit_type)
{
	this->cb = cb;
	this->ai = ai;
	this->bt = ai->bt;

	attack = 0;

	category = bt->units_static[def->id].category;
	combat_category = bt->GetIDOfAssaultCategory(category);

	// set unit type of group
	group_unit_type = unit_type;

	// set movement type of group (filter out add. movement info like underwater, floater, etc.)
	group_movement_type = bt->units_static[def->id].movement_type & MOVE_TYPE_UNIT;

	// now we know type and category, determine max group size
	if(cfg->AIR_ONLY_MOD)
	{
		maxSize = cfg->MAX_AIR_GROUP_SIZE;
	}
	else if(group_unit_type == ANTI_AIR_UNIT)
			maxSize = cfg->MAX_ANTI_AIR_GROUP_SIZE;	
	else
	{
		if(category >= GROUND_ARTY && category <= HOVER_ARTY)
			maxSize = cfg->MAX_ARTY_GROUP_SIZE;
		else if(category == AIR_ASSAULT)
			maxSize = cfg->MAX_AIR_GROUP_SIZE;
		else if(category == SEA_ASSAULT)
			maxSize = cfg->MAX_NAVAL_GROUP_SIZE;
		else if(category == SUBMARINE_ASSAULT)
			maxSize = cfg->MAX_SUBMARINE_GROUP_SIZE;
		else
			maxSize = cfg->MAX_GROUP_SIZE;
	}

	size = 0;

	task_importance = 0;
	task = GROUP_IDLE;

	lastCommand.id = CMD_STOP;
	lastCommand.params.resize(3);

	target_sector = 0;

	rally_point = ai->execute->GetRallyPoint(category, 1, 1, 10);

	// get speed group (if necessary)
	if(cfg->AIR_ONLY_MOD)
	{
		if(category == AIR_ASSAULT)
		{
			speed_group = floor((bt->unitList[def->id-1]->speed - bt->min_speed[1][ai->side-1])/bt->group_speed[1][ai->side-1]);
		}
		else
			speed_group = 0;
	}
	else
	{
		if(category == GROUND_ASSAULT)
			speed_group = floor((bt->unitList[def->id-1]->speed - bt->min_speed[0][ai->side-1])/bt->group_speed[0][ai->side-1]);
		else if(category == SEA_ASSAULT)
			speed_group = floor((bt->unitList[def->id-1]->speed - bt->min_speed[3][ai->side-1])/bt->group_speed[3][ai->side-1]);
		else
			speed_group = 0;
	}

	avg_speed = bt->unitList[def->id-1]->speed;
	//fprintf(ai->file, "Creating new group - max size: %i move type: %i speed group: %i\n", maxSize, (int)move_type, speed_group);
	//fprintf(ai->file, "speedgroup: %i,  min: %f,  avg: %f,  this: %f\n", speed_group, val1, val2, bt->unitList[def->id-1]->speed);
}

AAIGroup::~AAIGroup(void)
{
	if(attack)
		attack->RemoveGroup(this);

	attack = 0;

	units.clear();
}

bool AAIGroup::AddUnit(int unit_id, int def_id, UnitType type)
{
	// check the type match && current size
	if(type == group_unit_type && units.size() < maxSize && !attack && (task == GROUP_IDLE || task == GROUP_RETREATING))
	{
		// check if speed group is matching
		if(cfg->AIR_ONLY_MOD)
		{
			if(category == AIR_ASSAULT && speed_group != floor((bt->unitList[def_id-1]->speed - bt->min_speed[1][ai->side-1])/bt->group_speed[1][ai->side-1]))
				return false;
		}
		else
		{
			if(category == GROUND_ASSAULT && speed_group != floor((bt->unitList[def_id-1]->speed - bt->min_speed[0][ai->side-1])/bt->group_speed[0][ai->side-1]))
				return false;
			
			if(category == SEA_ASSAULT && speed_group != floor((bt->unitList[def_id-1]->speed - bt->min_speed[3][ai->side-1])/bt->group_speed[3][ai->side-1]))
				return false;
		}

		units.push_back(int2(unit_id, def_id));

		size++;

		// send unit to rally point of the group
		if(rally_point.x > 0)
		{
			Command c;
			c.id = CMD_MOVE;
			c.params.resize(3);
			c.params[0] = rally_point.x;
			c.params[1] = rally_point.y;
			c.params[2] = rally_point.z;

			if(category != AIR_ASSAULT)
				c.options |= SHIFT_KEY;

			cb->GiveOrder(unit_id, &c);
		}

		return true;
	}
	return false;
}

bool AAIGroup::RemoveUnit(int unit, int attacker)
{
	// look for unit with that id
	for(list<int2>::iterator i = units.begin(); i != units.end(); i++)
	{
		if(i->x == unit)
		{
			units.erase(i);
			--size;

			// remove from list of attacks groups if too less units
			if(attack)
			{
				if(group_unit_type == ASSAULT_UNIT)
				{
					// todo: improve criteria
					if(size < 2)
						attack->RemoveGroup(this);	
				}
				else if(group_unit_type == ANTI_AIR_UNIT)
				{
					if(size < 1)
						attack->RemoveGroup(this);
				}
			}	

			// check if attacking still sensible
			if(attack)
				ai->am->CheckAttack(attack);

			if(attacker)
			{
				const UnitDef *def = cb->GetUnitDef(attacker);

				if(def && !cfg->AIR_ONLY_MOD)
				{
					UnitCategory category = bt->units_static[def->id].category;

					if(category == STATIONARY_DEF)
						ai->af->CheckTarget(attacker, def);
					else if(category == GROUND_ASSAULT && bt->units_static[def->id].efficiency[0] > cfg->MIN_AIR_SUPPORT_EFFICIENCY)
						ai->af->CheckTarget(attacker, def);
					else if(category == SEA_ASSAULT && bt->units_static[def->id].efficiency[3] > cfg->MIN_AIR_SUPPORT_EFFICIENCY)
						ai->af->CheckTarget(attacker, def);
					else if(category == HOVER_ASSAULT && bt->units_static[def->id].efficiency[2] > cfg->MIN_AIR_SUPPORT_EFFICIENCY)
						ai->af->CheckTarget(attacker, def);
					else if(category == AIR_ASSAULT)
					{
						// get a random unit of the group
						int unit = GetRandomUnit();

						if(unit)
							ai->execute->DefendUnitVS(unit, cb->GetUnitDef(unit), category, NULL, 110);
					}
				}
			}

			return true;
		}
	}

	// unit not found
	return false;
}

void AAIGroup::GiveOrder(Command *c, float importance, UnitTask task)
{
	task_importance = importance;

	for(list<int2>::iterator i = units.begin(); i != units.end(); ++i)
	{
		cb->GiveOrder(i->x, c);
		ai->ut->SetUnitStatus(i->x, task);
	}
}

void AAIGroup::Update()
{
	task_importance *= 0.96f;

	// attacking groups recheck target
	if(task == GROUP_ATTACKING && target_sector)
	{
		if(target_sector->enemy_structures == 0)
		{
			task = GROUP_IDLE;
			target_sector = 0;
		}
	}

	// idle empty groups so they can be filled with units again...
	if(units.empty())
	{
		target_sector = 0;
		task = GROUP_IDLE;
	}

	// check fall back of long range units
	if(task == GROUP_ATTACKING)
	{
		float range;
		float3 pos;
		Command c;

		c.id = CMD_MOVE;
		c.params.resize(3);

		for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
		{
			range = bt->units_static[unit->y].range;

			if(range > cfg->MIN_FALLBACK_RANGE)
			{
				ai->execute->GetFallBackPos(&pos, unit->x, range);

				if(pos.x > 0)
				{
					c.params[0] = pos.x;
					c.params[1] = cb->GetElevation(pos.x, pos.z);
					c.params[2] = pos.z;

					cb->GiveOrder(unit->x, &c);
				}
			}
		}
	}
}

float AAIGroup::GetPowerVS(int assault_cat_id)
{
	float power = 0;

	for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
		power += bt->units_static[unit->y].efficiency[assault_cat_id];
	
	return power;
}

float3 AAIGroup::GetGroupPos()
{
	if(!units.empty())
	{
		return cb->GetUnitPos(units.begin()->x);
	}
	else 
		return ZeroVector;
}

void AAIGroup::TargetUnitKilled()
{
	// behaviour of normal mods
	if(!cfg->AIR_ONLY_MOD)
	{
		// air groups retreat to rally point
		if(category == AIR_ASSAULT)
		{	
			Command c;
			c.id = CMD_MOVE;
			c.params.push_back(rally_point.x);
			c.params.push_back(rally_point.y);
			c.params.push_back(rally_point.z);

			GiveOrder(&c, 90, MOVING);
		}
	}
}

void AAIGroup::AttackSector(AAISector *dest, float importance)
{
	float3 pos;
	Command c;
	c.id = CMD_FIGHT;
	c.params.resize(3);

	// get position of the group
	pos = GetGroupPos();

	int group_x = pos.x/ai->map->xSectorSize;
	int group_y = pos.z/ai->map->ySectorSize;
				
	c.params[0] = (dest->left + dest->right)/2;
	c.params[2] = (dest->bottom + dest->top)/2;
							
	// choose location that way that attacking units must cross the entire sector
	if(dest->x > group_x)
		c.params[0] = (dest->left + 7 * dest->right)/8;
	else if(dest->x < group_x)
		c.params[0] = (7 * dest->left + dest->right)/8;
	else
		c.params[0] = (dest->left + dest->right)/2;

	if(dest->y > group_y)
		c.params[2] = (7 * dest->bottom + dest->top)/8;
	else if(dest->y < group_y)
		c.params[2] = (dest->bottom + 7 * dest->top)/8;
	else
		c.params[2] = (dest->bottom + dest->top)/2;

	c.params[1] = cb->GetElevation(c.params[0], c.params[2]);

	// move group to that sector
	GiveOrder(&c, importance + 8, UNIT_ATTACKING);
	
	target_sector = dest;
	task = GROUP_ATTACKING;			
}

void AAIGroup::Defend(int unit, float3 *enemy_pos, int importance)
{
	Command cmd;

	if(enemy_pos)
	{
		cmd.id = CMD_FIGHT;
		cmd.params.push_back(enemy_pos->x);
		cmd.params.push_back(enemy_pos->y);
		cmd.params.push_back(enemy_pos->z);

		GiveOrder(&cmd, importance, DEFENDING);

		target_sector = ai->map->GetSectorOfPos(enemy_pos);
	}
	else
	{
		cmd.id = CMD_GUARD;
		cmd.params.push_back(unit);
			
		GiveOrder(&cmd, importance, GUARDING);

		float3 pos = cb->GetUnitPos(unit);

		target_sector = ai->map->GetSectorOfPos(&pos);
	}

	task = GROUP_DEFENDING;
}

void AAIGroup::Retreat(float3 *pos)
{
	this->task = GROUP_RETREATING;

	Command c;
	c.id = CMD_MOVE;
	c.params.push_back(pos->x);
	c.params.push_back(pos->y);
	c.params.push_back(pos->z);

	GiveOrder(&c, 105, MOVING);

	// set new dest sector
	target_sector = ai->map->GetSectorOfPos(pos);
}

int AAIGroup::GetRandomUnit()
{
	if(units.empty())
		return -1;
	else
		return units.begin()->x;
}

bool AAIGroup::SufficientAttackPower()
{
	/*if(units.size() >= maxSize)
		return true;
	else if(units.size() > 0)
	{
		// group not full, check combat eff. of units
		float avg_combat_power = 0;

		for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
			avg_combat_power += bt->units_static[unit->y].efficiency[5];

		avg_combat_power /= units.size();

		if(units.size() > maxSize/2.0f && avg_combat_power > bt->avg_eff[bt->GetIDOfAssaultCategory(category)][5])
			return true;
		else if(units.size() >= maxSize/3.0f &&  avg_combat_power > 1.5 * bt->avg_eff[bt->GetIDOfAssaultCategory(category)][5])
			return true;
	}
	
	return false;*/

	if(group_unit_type == ASSAULT_UNIT)
	{
		if(size > maxSize/3)
			return true;
	}
	else
	{
		if(size > maxSize/2)
			return true;
	}

	return false;
}

void AAIGroup::UnitIdle(int unit)
{
	// special behaviour of aircraft in not air only mods
	if(category == AIR_ASSAULT && task != GROUP_IDLE && !cfg->AIR_ONLY_MOD)
	{
		Command c;
		c.id = CMD_MOVE;
		c.params.push_back(rally_point.x);
		c.params.push_back(rally_point.y);
		c.params.push_back(rally_point.z);

		GiveOrder(&c, 100, MOVING);

		task = GROUP_IDLE;
	}
	// behaviour of all other categories
	else if(attack)
	{
		//check if idle unit is in target sector
		float3 pos = cb->GetUnitPos(unit);

		AAISector *temp = ai->map->GetSectorOfPos(&pos);

		if(temp == target_sector || !target_sector)
		{
			// combat groups 
			if(group_unit_type == ASSAULT_UNIT && attack->dest->enemy_structures <= 0)
			{
				ai->am->GetNextDest(attack);
				return;
			}
			// unit the aa groups was guarding has been killed
			else if(group_unit_type == ANTI_AIR_UNIT)
			{
				if(!attack->combat_groups.empty())
				{
					int unit = (*attack->combat_groups.begin())->GetRandomUnit();

					if(unit >= 0)
					{
						Command c;
						c.id = CMD_GUARD;
						c.params.push_back(unit);

						GiveOrder(&c, 110, GUARDING);
					}
				}
				else
					attack->StopAttack();
			}
		}
	}
	else if(task == GROUP_RETREATING)
	{
		//check if retreating units is in target sector
		float3 pos = cb->GetUnitPos(unit);

		AAISector *temp = ai->map->GetSectorOfPos(&pos);

		if(temp == target_sector || !target_sector)
			task = GROUP_IDLE;
	}
	else if(task == GROUP_DEFENDING)
	{
		//check if retreating units is in target sector
		float3 pos = cb->GetUnitPos(unit);

		AAISector *temp = ai->map->GetSectorOfPos(&pos);

		if(temp == target_sector || !target_sector)
			task = GROUP_IDLE;
	}
}

void AAIGroup::BombTarget(int target_id, float3 *target_pos)
{
	Command c;
	c.id = CMD_ATTACK;
	c.params.push_back(target_pos->x);
	c.params.push_back(target_pos->y);
	c.params.push_back(target_pos->z);

	GiveOrder(&c, 110, UNIT_ATTACKING);

	ai->ut->AssignGroupToEnemy(target_id, this);

	task = GROUP_BOMBING;
}

void AAIGroup::DefendAirSpace(float3 *pos)
{
	Command c;
	c.id = CMD_PATROL;
	c.params.push_back(pos->x);
	c.params.push_back(pos->y);
	c.params.push_back(pos->z);

	GiveOrder(&c, 110, UNIT_ATTACKING);

	task = GROUP_PATROLING;
}

void AAIGroup::AirRaidUnit(int unit_id)
{
	Command c;
	c.id = CMD_ATTACK;
	c.params.push_back(unit_id);

	GiveOrder(&c, 110, UNIT_ATTACKING);

	ai->ut->AssignGroupToEnemy(unit_id, this);

	task = GROUP_ATTACKING;
}
