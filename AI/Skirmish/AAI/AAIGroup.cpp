// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIGroup.h"

#include "AAI.h"
#include "AAIBuildTable.h"
#include "AAIAttack.h"
#include "AAIExecute.h"
#include "AAIAttackManager.h"
#include "AAIAirForceManager.h"
#include "AAIUnitTable.h"
#include "AAIConfig.h"
#include "AAIMap.h"
#include "AAISector.h"


#include "LegacyCpp/UnitDef.h"
using namespace springLegacyAI;


AAIGroup::AAIGroup(AAI *ai, const UnitDef *def, UnitType unit_type, int continent_id):
	rally_point(ZeroVector)
{
	this->ai = ai;

	attack = 0;

	category = ai->Getbt()->units_static[def->id].category;
	combat_category = ai->Getbt()->GetIDOfAssaultCategory(category);

	// set unit type of group
	group_unit_type = unit_type;

	// set movement type of group (filter out add. movement info like underwater, floater, etc.)
	group_movement_type = ai->Getbt()->units_static[def->id].movement_type & MOVE_TYPE_UNIT;

	continent = continent_id;

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

	lastCommand = Command(CMD_STOP);
	lastCommandFrame = 0;

	target_sector = 0;

	// get a rally point
	GetNewRallyPoint();

	// get speed group (if necessary)
	if(cfg->AIR_ONLY_MOD)
	{
		if(category == AIR_ASSAULT)
		{
			speed_group = floor((ai->Getbt()->GetUnitDef(def->id).speed - ai->Getbt()->min_speed[1][ai->Getside()-1])/ai->Getbt()->group_speed[1][ai->Getside()-1]);
		}
		else
			speed_group = 0;
	}
	else
	{
		if(category == GROUND_ASSAULT)
			speed_group = floor((ai->Getbt()->GetUnitDef(def->id).speed - ai->Getbt()->min_speed[0][ai->Getside()-1])/ai->Getbt()->group_speed[0][ai->Getside()-1]);
		else if(category == SEA_ASSAULT)
			speed_group = floor((ai->Getbt()->GetUnitDef(def->id).speed - ai->Getbt()->min_speed[3][ai->Getside()-1])/ai->Getbt()->group_speed[3][ai->Getside()-1]);
		else
			speed_group = 0;
	}

	avg_speed = ai->Getbt()->GetUnitDef(def->id).speed;

	//ai->Log("Creating new group - max size: %i   move type: %i   speed group: %i   continent: %i\n", maxSize, group_movement_type, speed_group, continent);
}

AAIGroup::~AAIGroup(void)
{
	if(attack)
	{
		attack->RemoveGroup(this);
		attack = 0;
	}

	units.clear();

	if(rally_point.x > 0)
	{
		AAISector *sector = ai->Getmap()->GetSectorOfPos(&rally_point);

		--sector->rally_points;
	}
}

bool AAIGroup::AddUnit(int unit_id, int def_id, UnitType type, int continent_id)
{
	// for continent bound units: check if unit is on the same continent as the group
	if(continent_id == -1 || continent_id == continent)
	{
		//check if type match && current size
		if(type == group_unit_type && units.size() < maxSize && !attack && (task == GROUP_IDLE || task == GROUP_RETREATING))
		{
			// check if speed group is matching
			if(cfg->AIR_ONLY_MOD)
			{
				if(category == AIR_ASSAULT && speed_group != floor((ai->Getbt()->GetUnitDef(def_id).speed - ai->Getbt()->min_speed[1][ai->Getside()-1])/ai->Getbt()->group_speed[1][ai->Getside()-1]))
					return false;
			}
			else
			{
				if(category == GROUND_ASSAULT && speed_group != floor((ai->Getbt()->GetUnitDef(def_id).speed - ai->Getbt()->min_speed[0][ai->Getside()-1])/ai->Getbt()->group_speed[0][ai->Getside()-1]))
					return false;

				if(category == SEA_ASSAULT && speed_group != floor((ai->Getbt()->GetUnitDef(def_id).speed - ai->Getbt()->min_speed[3][ai->Getside()-1])/ai->Getbt()->group_speed[3][ai->Getside()-1]))
					return false;
			}

			units.push_back(int2(unit_id, def_id));

			++size;

			// send unit to rally point of the group
			if(rally_point.x > 0)
			{
				Command c(CMD_MOVE);
				c.PushPos(rally_point);

				if(category != AIR_ASSAULT)
					c.SetOpts(c.GetOpts() | SHIFT_KEY);

				//ai->Getcb()->GiveOrder(unit_id, &c);
				ai->Getexecute()->GiveOrder(&c, unit_id, "Group::AddUnit");
			}

			return true;
		}
	}

	return false;
}

bool AAIGroup::RemoveUnit(int unit, int attacker)
{
	// look for unit with that id
	for(list<int2>::iterator i = units.begin(); i != units.end(); ++i)
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
					{
						attack->RemoveGroup(this);
						attack = 0;
					}
				}
				else if(group_unit_type == ANTI_AIR_UNIT)
				{
					if(size < 1)
					{
						attack->RemoveGroup(this);
						attack = 0;
					}
				}
			}

			// check if attacking still sensible
			if(attack)
				ai->Getam()->CheckAttack(attack);

			if(attacker)
			{
				const UnitDef *def = ai->Getcb()->GetUnitDef(attacker);

				if(def && !cfg->AIR_ONLY_MOD)
				{
					UnitCategory category = ai->Getbt()->units_static[def->id].category;

					if(category == STATIONARY_DEF)
						ai->Getaf()->CheckTarget(attacker, def);
					else if(category == GROUND_ASSAULT && ai->Getbt()->units_static[def->id].efficiency[0] > cfg->MIN_AIR_SUPPORT_EFFICIENCY)
						ai->Getaf()->CheckTarget(attacker, def);
					else if(category == SEA_ASSAULT && ai->Getbt()->units_static[def->id].efficiency[3] > cfg->MIN_AIR_SUPPORT_EFFICIENCY)
						ai->Getaf()->CheckTarget(attacker, def);
					else if(category == HOVER_ASSAULT && ai->Getbt()->units_static[def->id].efficiency[2] > cfg->MIN_AIR_SUPPORT_EFFICIENCY)
						ai->Getaf()->CheckTarget(attacker, def);
					else if(category == AIR_ASSAULT)
					{
						float3 enemy_pos = ai->Getcb()->GetUnitPos(attacker);

						// get a random unit of the group
						int unit = GetRandomUnit();

						if(unit)
							ai->Getexecute()->DefendUnitVS(unit, ai->Getbt()->units_static[def->id].movement_type, &enemy_pos, 100);
					}
				}
			}

			return true;
		}
	}

	// unit not found
	return false;
}

void AAIGroup::GiveOrder(Command *c, float importance, UnitTask task, const char *owner)
{
	lastCommandFrame = ai->Getcb()->GetCurrentFrame();

	task_importance = importance;

	for(list<int2>::iterator i = units.begin(); i != units.end(); ++i)
	{
		//ai->Getcb()->GiveOrder(i->x, c);
		ai->Getexecute()->GiveOrder(c, i->x, owner);
		ai->Getut()->SetUnitStatus(i->x, task);
	}
}

void AAIGroup::Update()
{
	task_importance *= 0.97f;

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
		Command c(CMD_MOVE);

		for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
		{
			range = ai->Getbt()->units_static[unit->y].range;

			if(range > cfg->MIN_FALLBACK_RANGE)
			{
				ai->Getexecute()->GetFallBackPos(&pos, unit->x, range);

				if(pos.x > 0)
				{
					c = Command(CMD_MOVE);
					c.PushParam(pos.x);
					c.PushParam(ai->Getcb()->GetElevation(pos.x, pos.z));
					c.PushParam(pos.z);

					//ai->Getcb()->GiveOrder(unit->x, &c);
					ai->Getexecute()->GiveOrder(&c, unit->x, "GroupFallBack");
				}
			}
		}
	}
}

float AAIGroup::GetCombatPowerVsCategory(int assault_cat_id)
{
	float power = 0;

	for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
		power += ai->Getbt()->units_static[unit->y].efficiency[assault_cat_id];

	return power;
}

void AAIGroup::GetCombatPower(vector<float> *combat_power)
{
	for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
	{
		for(int cat = 0; cat < AAIBuildTable::combat_categories; ++cat)
			(*combat_power)[cat] += ai->Getbt()->units_static[unit->y].efficiency[cat];
	}
}

float3 AAIGroup::GetGroupPos()
{
	if(!units.empty())
	{
		return ai->Getcb()->GetUnitPos(units.begin()->x);
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
			Command c(CMD_MOVE);
			c.PushPos(rally_point);

			GiveOrder(&c, 90, MOVING, "Group::TargetUnitKilled");
		}
	}
}

void AAIGroup::AttackSector(AAISector *dest, float importance)
{
	float3 pos;
	Command c(CMD_FIGHT);

	// get position of the group
	c.PushPos(pos = GetGroupPos());

	int group_x = pos.x/ai->Getmap()->xSectorSize;
	int group_y = pos.z/ai->Getmap()->ySectorSize;

	c.SetParam(0, (dest->left + dest->right)/2);
	c.SetParam(2, (dest->bottom + dest->top)/2);

	// choose location that way that attacking units must cross the entire sector
	if(dest->x > group_x)
		c.SetParam(0, (dest->left + 7 * dest->right)/8);
	else if(dest->x < group_x)
		c.SetParam(0, (7 * dest->left + dest->right)/8);
	else
		c.SetParam(0, (dest->left + dest->right)/2);

	if(dest->y > group_y)
		c.SetParam(2, (7 * dest->bottom + dest->top)/8);
	else if(dest->y < group_y)
		c.SetParam(2, (dest->bottom + 7 * dest->top)/8);
	else
		c.SetParam(2, (dest->bottom + dest->top)/2);

	c.SetParam(1, ai->Getcb()->GetElevation(c.GetParam(0), c.GetParam(2)));

	// move group to that sector
	GiveOrder(&c, importance + 8, UNIT_ATTACKING, "Group::AttackSector");

	target_sector = dest;
	task = GROUP_ATTACKING;
}

void AAIGroup::Defend(int unit, float3 *enemy_pos, int importance)
{
	Command cmd((enemy_pos != nullptr)? CMD_FIGHT: CMD_GUARD);

	if(enemy_pos)
	{
		cmd.PushPos(*enemy_pos);

		GiveOrder(&cmd, importance, DEFENDING, "Group::Defend");

		target_sector = ai->Getmap()->GetSectorOfPos(enemy_pos);
	}
	else
	{
		cmd.PushParam(unit);

		GiveOrder(&cmd, importance, GUARDING, "Group::Defend");

		float3 pos = ai->Getcb()->GetUnitPos(unit);

		target_sector = ai->Getmap()->GetSectorOfPos(&pos);
	}

	task = GROUP_DEFENDING;
}

void AAIGroup::Retreat(float3 *pos)
{
	this->task = GROUP_RETREATING;

	Command c(CMD_MOVE);
	c.PushPos(*pos);

	GiveOrder(&c, 105, MOVING, "Group::Retreat");

	// set new dest sector
	target_sector = ai->Getmap()->GetSectorOfPos(pos);
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
	/*else if(units.size() > 0)
	{
		// group not full, check combat eff. of units
		float avg_combat_power = 0;

		for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
			avg_combat_power += ai->Getbt()->units_static[unit->y].efficiency[5];

		avg_combat_power /= units.size();

		if(units.size() > maxSize/2.0f && avg_combat_power > ai->Getbt()->avg_eff[ai->Getbt()->GetIDOfAssaultCategory(category)][5])
			return true;
		else if(units.size() >= maxSize/3.0f &&  avg_combat_power > 1.5 * ai->Getbt()->avg_eff[ai->Getbt()->GetIDOfAssaultCategory(category)][5])
			return true;
	}

	return false;*/

	if(units.size() >= maxSize - 1)
		return true;

	if(group_unit_type == ASSAULT_UNIT)
	{
		float avg_combat_power = 0;

		if(category == GROUND_ASSAULT)
		{
			float ground = 1.0f;
			float hover = 0.2f;

			for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
				avg_combat_power += ground * ai->Getbt()->units_static[unit->y].efficiency[0] + hover * ai->Getbt()->units_static[unit->y].efficiency[2];

			if( avg_combat_power > (ground * ai->Getbt()->avg_eff[ai->Getside()-1][0][0] + hover * ai->Getbt()->avg_eff[ai->Getside()-1][0][2]) * (float)units.size() )
				return true;
		}
		else if(category == HOVER_ASSAULT)
		{
			float ground = 1.0f;
			float hover = 0.2f;
			float sea = 1.0f;

			for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
				avg_combat_power += ground * ai->Getbt()->units_static[unit->y].efficiency[0] + hover * ai->Getbt()->units_static[unit->y].efficiency[2] + sea * ai->Getbt()->units_static[unit->y].efficiency[3];

			if( avg_combat_power > (ground * ai->Getbt()->avg_eff[ai->Getside()-1][2][0] + hover * ai->Getbt()->avg_eff[ai->Getside()-1][2][2] + sea * ai->Getbt()->avg_eff[ai->Getside()-1][2][3]) * (float)units.size() )
				return true;
		}
		else if(category == SEA_ASSAULT)
		{
			float hover = 0.3f;
			float sea = 1.0f;
			float submarine = 0.8f;

			for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
				avg_combat_power += hover * ai->Getbt()->units_static[unit->y].efficiency[2] + sea * ai->Getbt()->units_static[unit->y].efficiency[3] + submarine * ai->Getbt()->units_static[unit->y].efficiency[4];

			if( avg_combat_power > (hover * ai->Getbt()->avg_eff[ai->Getside()-1][3][2] + sea * ai->Getbt()->avg_eff[ai->Getside()-1][3][3] + submarine * ai->Getbt()->avg_eff[ai->Getside()-1][3][4]) * (float)units.size() )
				return true;
		}
		else if(category == SUBMARINE_ASSAULT)
		{
			float sea = 1.0f;
			float submarine = 0.8f;

			for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
				avg_combat_power += sea * ai->Getbt()->units_static[unit->y].efficiency[3] + submarine * ai->Getbt()->units_static[unit->y].efficiency[4];

			if( avg_combat_power > (sea * ai->Getbt()->avg_eff[ai->Getside()-1][4][3] + submarine * ai->Getbt()->avg_eff[ai->Getside()-1][4][4]) * (float)units.size() )
				return true;
		}
	}
	else
	{
		float avg_combat_power = 0;

		for(list<int2>::iterator unit = units.begin(); unit != units.end(); ++unit)
			avg_combat_power += ai->Getbt()->units_static[unit->y].efficiency[1];

		if(avg_combat_power > ai->Getbt()->avg_eff[ai->Getside()-1][category][1] * (float)units.size())
			return true;
	}

	return false;
}

bool AAIGroup::AvailableForAttack()
{
	if(!attack && task == GROUP_IDLE)
	{
		if(group_unit_type == ASSAULT_UNIT)
		{
			if(SufficientAttackPower())
				return true;
			else
				return false;
		}
		else
			return true;
	}
	else
		return false;
}

void AAIGroup::UnitIdle(int unit)
{
	if(ai->Getcb()->GetCurrentFrame() - lastCommandFrame < 10)
		return;

	// special behaviour of aircraft in non air only mods
	if(category == AIR_ASSAULT && task != GROUP_IDLE && !cfg->AIR_ONLY_MOD)
	{
		Command c(CMD_MOVE);
		c.PushPos(rally_point);

		GiveOrder(&c, 100, MOVING, "Group::Idle_a");

		task = GROUP_IDLE;
	}
	// behaviour of all other categories
	else if(attack)
	{
		//check if idle unit is in target sector
		float3 pos = ai->Getcb()->GetUnitPos(unit);

		AAISector *temp = ai->Getmap()->GetSectorOfPos(&pos);

		if(temp == target_sector || !target_sector)
		{
			// combat groups
			if(group_unit_type == ASSAULT_UNIT && attack->dest->enemy_structures <= 0)
			{
				ai->Getam()->GetNextDest(attack);
				return;
			}
			// unit the aa group was guarding has been killed
			else if(group_unit_type == ANTI_AIR_UNIT)
			{
				if(!attack->combat_groups.empty())
				{
					int unit = (*attack->combat_groups.begin())->GetRandomUnit();

					if(unit >= 0)
					{
						Command c(CMD_GUARD);
						c.PushParam(unit);

						GiveOrder(&c, 110, GUARDING, "Group::Idle_b");
					}
				}
				else
					attack->StopAttack();
			}
		}
		else
		{
			// idle assault units are ordered to attack the current target sector
			if(group_unit_type == ASSAULT_UNIT)
			{
				Command c(CMD_FIGHT);

				// get position of the group
				c.PushPos(pos = ai->Getcb()->GetUnitPos(unit));

				int pos_x = pos.x/ai->Getmap()->xSectorSize;
				int pos_y = pos.z/ai->Getmap()->ySectorSize;

				c.SetParam(0, (target_sector->left + target_sector->right)/2);
				c.SetParam(2, (target_sector->bottom + target_sector->top)/2);

				// choose location that way that attacking units must cross the entire sector
				if(target_sector->x > pos_x)
					c.SetParam(0, (target_sector->left + 7 * target_sector->right)/8);
				else if(target_sector->x < pos_x)
					c.SetParam(0, (7 * target_sector->left + target_sector->right)/8);
				else
					c.SetParam(0, (target_sector->left + target_sector->right)/2);

				if(target_sector->y > pos_y)
					c.SetParam(2, (7 * target_sector->bottom + target_sector->top)/8);
				else if(target_sector->y < pos_y)
					c.SetParam(2, (target_sector->bottom + 7 * target_sector->top)/8);
				else
					c.SetParam(2, (target_sector->bottom + target_sector->top)/2);

				c.SetParam(1, ai->Getcb()->GetElevation(c.GetParam(0), c.GetParam(2)));

				// move group to that sector
				GiveOrder(&c, 110, UNIT_ATTACKING, "Group::Idle_c");
			}
		}
	}
	else if(task == GROUP_RETREATING)
	{
		//check if retreating units is in target sector
		float3 pos = ai->Getcb()->GetUnitPos(unit);

		AAISector *temp = ai->Getmap()->GetSectorOfPos(&pos);

		if(temp == target_sector || !target_sector)
			task = GROUP_IDLE;
	}
	else if(task == GROUP_DEFENDING)
	{
		//check if retreating units is in target sector
		float3 pos = ai->Getcb()->GetUnitPos(unit);

		AAISector *temp = ai->Getmap()->GetSectorOfPos(&pos);

		if(temp == target_sector || !target_sector)
			task = GROUP_IDLE;
	}
}

void AAIGroup::BombTarget(int target_id, float3 *target_pos)
{
	Command c(CMD_ATTACK);
	c.PushPos(*target_pos);

	GiveOrder(&c, 110, UNIT_ATTACKING, "Group::BombTarget");

	ai->Getut()->AssignGroupToEnemy(target_id, this);

	task = GROUP_BOMBING;
}

void AAIGroup::DefendAirSpace(float3 *pos)
{
	Command c(CMD_PATROL);
	c.PushPos(*pos);

	GiveOrder(&c, 110, UNIT_ATTACKING, "Group::DefendAirSpace");

	task = GROUP_PATROLING;
}

void AAIGroup::AirRaidUnit(int unit_id)
{
	Command c(CMD_ATTACK);
	c.PushParam(unit_id);

	GiveOrder(&c, 110, UNIT_ATTACKING, "Group::AirRaidUnit");

	ai->Getut()->AssignGroupToEnemy(unit_id, this);

	task = GROUP_ATTACKING;
}

void AAIGroup::UpdateRallyPoint()
{
	AAISector *sector = ai->Getmap()->GetSectorOfPos(&rally_point);

	// check if rally point lies within base (e.g. AAI has expanded its base after rally point had been set)
	if(sector->distance_to_base <= 0)
		GetNewRallyPoint();

	// check if rally point is blocked by building


}

void AAIGroup::GetNewRallyPoint()
{
	AAISector *sector;

	// delete old rally point (if there is any)
	if(rally_point.x > 0)
	{
		sector = ai->Getmap()->GetSectorOfPos(&rally_point);

		--sector->rally_points;
	}

	rally_point = ai->Getexecute()->GetRallyPoint(group_movement_type, continent, 1, 1);

	if(rally_point.x > 0)
	{
		//add new rally point to sector
		sector = ai->Getmap()->GetSectorOfPos(&rally_point);
		++sector->rally_points;

		// send idle groups to new rally point
		if(task == GROUP_IDLE)
		{
			Command c(CMD_MOVE);
			c.PushPos(rally_point);

			GiveOrder(&c, 90, HEADING_TO_RALLYPOINT, "Group::RallyPoint");
		}
	}
}
