#include "AAIGroup.h"

#include "AAI.h"
#include "AAIBuildTable.h"

AAIGroup::AAIGroup(IAICallback *cb, AAI *ai, const UnitDef *def, UnitType unit_type)
{
	this->cb = cb;
	this->ai = ai;
	this->bt = ai->bt;

	category = bt->units_static[def->id].category;

	// get type of group
	group_type = unit_type;

	// debug
	/*if(group_type == ANTI_AIR_UNIT)
		cb->SendTextMsg("new anti air group",0);
	else if(group_type == ASSAULT_UNIT)
		cb->SendTextMsg("new assault group",0);
	else if(group_type == BOMBER_UNIT)
		cb->SendTextMsg("new bomber group",0);*/

	// now we know type and category, determine max group size
	if(cfg->AIR_ONLY_MOD)
	{
		maxSize = cfg->MAX_AIR_GROUP_SIZE;
	}
	else
	{
		if(category == MOBILE_ARTY)
			maxSize = cfg->MAX_ARTY_GROUP_SIZE;
		else if(group_type == ANTI_AIR_UNIT)
			maxSize = cfg->MAX_ANTI_AIR_GROUP_SIZE;		
		else
		{
			if(category == AIR_ASSAULT)
				maxSize = cfg->MAX_AIR_GROUP_SIZE;
			else if(category == SEA_ASSAULT)
				maxSize = cfg->MAX_NAVAL_GROUP_SIZE;
			else if(category == SUBMARINE_ASSAULT)
				maxSize = cfg->MAX_SUBMARINE_GROUP_SIZE;
			else
				maxSize = cfg->MAX_GROUP_SIZE;
		}
	}

	size = 0;

	task_importance = 0;
	task = GROUP_IDLE;

	lastCommand.id = CMD_STOP;
	lastCommand.params.resize(3);

	target_sector = 0;

	rally_point = ai->execute->GetRallyPoint(category);

	float val1 = 0;
	float val2 = 0;

	// get speed group (if necessary)
	if(cfg->AIR_ONLY_MOD)
	{
		if(category == AIR_ASSAULT)
		{
			speed_group = floor((bt->unitList[def->id-1]->speed - bt->max_value[AIR_ASSAULT][ai->side-1])/bt->avg_value[AIR_ASSAULT][ai->side-1]);
		}
		else
			speed_group = 0;
	}
	else
	{
		if(category == GROUND_ASSAULT)
		{
			val1 = bt->max_value[GROUND_ASSAULT][ai->side-1];
			val2 = bt->avg_value[GROUND_ASSAULT][ai->side-1];
			speed_group = floor((bt->unitList[def->id-1]->speed - bt->max_value[GROUND_ASSAULT][ai->side-1])/bt->avg_value[GROUND_ASSAULT][ai->side-1]);
		}
		else if(category == SEA_ASSAULT)
		{
			val1 = bt->max_value[SEA_ASSAULT][ai->side-1];
			val2 = bt->avg_value[SEA_ASSAULT][ai->side-1];
			speed_group = floor((bt->unitList[def->id-1]->speed - val1)/val2);

			//speed_group = floor((bt->unitList[def->id-1]->speed - bt->max_value[SEA_ASSAULT][ai->side-1])/bt->avg_value[SEA_ASSAULT][ai->side-1]);
		}
		else
			speed_group = 0;
	}

	avg_speed = bt->unitList[def->id-1]->speed;
	//fprintf(ai->file, "speedgroup: %i,  min: %f,  avg: %f,  this: %f\n", speed_group, val1, val2, bt->unitList[def->id-1]->speed);
}

AAIGroup::~AAIGroup(void)
{
	units.clear();
}

bool AAIGroup::AddUnit(int unit_id, int def_id, UnitType type)
{
	// check the type match && current size
	if(type == group_type && units.size() < maxSize && task != GROUP_ATTACKING)
	{
		// check if speed group is matching// get speed group (if necessary)
		
		if(cfg->AIR_ONLY_MOD)
		{
			int my_speed_group = floor((bt->unitList[def_id-1]->speed - bt->max_value[AIR_ASSAULT][ai->side-1])/bt->avg_value[AIR_ASSAULT][ai->side-1]);
		
			if(category == AIR_ASSAULT && speed_group != my_speed_group)
				return false;
		}
		else
		{

			if(category == GROUND_ASSAULT && speed_group != floor((bt->unitList[def_id-1]->speed - bt->max_value[GROUND_ASSAULT][ai->side-1])/bt->avg_value[GROUND_ASSAULT][ai->side-1]))
				return false;
			
			if(category == SEA_ASSAULT && speed_group != floor((bt->unitList[def_id-1]->speed - bt->max_value[SEA_ASSAULT][ai->side-1])/bt->avg_value[SEA_ASSAULT][ai->side-1]))
				return false;
		}

		units.push_back(int2(unit_id, def_id));

		size++;

		// send unit to rally point of the group
		if(rally_point.x > 0)
		{
			float3 pos = rally_point;

			pos.x += ai->map->xSectorSize * ((float(rand()%5))/20.0);
			pos.z += ai->map->ySectorSize * ((float(rand()%5))/20.0);
			
			Command c;
			c.id = CMD_MOVE;
			c.params.resize(3);
			c.params[0] = pos.x;
			c.params[1] = pos.y;
			c.params[2] = pos.z;

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
	UnitCategory category;

	// look for unit with that id
	for(list<int2>::iterator i = units.begin(); i != units.end(); i++)
	{
		if(i->x == unit)
		{
			units.erase(i);
			size--;

			if(attacker)
			{
				const UnitDef *def = cb->GetUnitDef(attacker);

				if(def && !cfg->AIR_ONLY_MOD)
				{
					category = bt->units_static[def->id].category;

					if(category == STATIONARY_DEF)
						ai->af->CheckTarget(attacker, def);
					else if((category == GROUND_ASSAULT || category == SEA_ASSAULT) && bt->units_static[def->id].efficiency[0] > cfg->MIN_AIR_SUPPORT_EFFICIENCY)
						ai->af->CheckTarget(attacker, def);
					else if(category == COMMANDER)
						ai->af->CheckTarget(attacker, def);
					else if(category == AIR_ASSAULT)
					{
						ai->af->CheckTarget(attacker, def);
					}
				}
			}

			return true;
		}
	}

	// unit not found
	return false;
}

void AAIGroup::GiveOrder(Command *c, float importance)
{
	task_importance = importance;

	for(list<int2>::iterator i = units.begin(); i != units.end(); ++i)
	{
		cb->GiveOrder(i->x, c);
	}
}

void AAIGroup::Update()
{
	task_importance *= 0.96;

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
}

float AAIGroup::GetPowerVS(int assault_cat_id)
{
	float power = 0;

	for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
		power += bt->units_static[unit->y].efficiency[assault_cat_id];
	
	return power;
}

void AAIGroup::BombTarget(float3 pos, float importance)
{
	Command c;
	c.id = CMD_ATTACK;
	c.params.resize(3);
	c.params[0] = pos.x;
	c.params[1] = cb->GetElevation(pos.x, pos.z);
	c.params[2] = pos.z;

	if(pos.x <= 0 || pos.z <= 0)
	{
		/*cb->SendTextMsg("invalid bomb pos",0);
		char c[80];
		sprintf(c, "pos: %f %f", pos.x, pos.z);
		cb->SendTextMsg(c, 0);*/
		return;
	}
		
	// get all bombers and order them to attack target
	for(list<int2>::iterator unit = units.begin(); unit != units.end(); unit++)
	{
		//if(bt->units[unit->y].stationary_efficiency >= bt->units[unit->y].air_efficiency)
		cb->GiveOrder(unit->x, &c);	
	}

	task_importance = importance;
}

void AAIGroup::BombTarget(int unit, float importance)
{
	Command c;
	c.id = CMD_ATTACK;
	c.params.resize(1);
	c.params[0] = unit;

	// get all bombers and order them to attack target
	for(list<int2>::iterator unit = units.begin(); unit != units.end(); unit++)
	{
		//if(bt->units[unit->y].stationary_efficiency >= bt->units[unit->y].air_efficiency)
		cb->GiveOrder(unit->x, &c);	
	}

	task_importance = importance;
}

void AAIGroup::AirRaidTarget(int unit, float importance)
{
	Command c;
	c.id = CMD_ATTACK;
	c.params.resize(1);
	c.params[0] = unit;

	// get all assault bombers and order them to attack target
	for(list<int2>::iterator unit = units.begin(); unit != units.end(); unit++)
	{
		//if(bt->units[unit->y].ground_efficiency >= bt->units[unit->y].air_efficiency)
		cb->GiveOrder(unit->x, &c);	
	}

	task_importance = importance;

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
