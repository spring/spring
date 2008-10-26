// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAIAttackManager.h"
#include "AAI.h"
#include "AAIBrain.h"
#include "AAIBuildTable.h"
#include "AAIGroup.h"
#include "AAIMap.h"
#include "AAIAttack.h"

AAIAttackManager::AAIAttackManager(AAI *ai, IAICallback *cb, AAIBuildTable *bt)
{
	this->ai = ai;
	brain = ai->brain;
	this->bt = bt;
	this->cb = cb;
	this->map = ai->map;

//	available_combat_cat = new int[bt->combat_categories];
	available_combat_cat.resize(bt->combat_categories);
}

AAIAttackManager::~AAIAttackManager(void)
{
	for(list<AAIAttack*>::iterator a = attacks.begin(); a != attacks.end(); ++a)
		delete (*a);

//	delete [] available_combat_cat;
}


void AAIAttackManager::Update()
{
	for(list<AAIAttack*>::iterator a = attacks.begin(); a != attacks.end(); ++a)
	{
		// drop failed attacks
		if((*a)->Failed())
		{
			(*a)->StopAttack();

			delete (*a);
			attacks.erase(a);

			break;
		}

		// check if sector cleared
		if((*a)->dest)
		{
			if((*a)->dest->enemy_structures <= 0)
				GetNextDest(*a);
		}
	}

	if(attacks.size() < cfg->MAX_ATTACKS)
	{
		LaunchAttack();
	}
}

void AAIAttackManager::LaunchAttack()
{
	AAISector *dest;
	AttackType a_type;
	bool suitable, land, water;

	// determine attack sector

	// todo: improve decision when to attack base or outposts
	a_type = OUTPOST_ATTACK;
	
	if(cfg->AIR_ONLY_MOD)
	{
		land = true;
		water = true;
	}
	else
	{
		if(map->map_type == LAND_MAP)
		{
			land = true;
			water = false;
		}
		else if(map->map_type == LAND_WATER_MAP)
		{
			land = true;
			water = true;
		}
		else if(map->map_type == WATER_MAP)
		{
			land = false;
			water = true;
		}
		else
		{
			land = true;
			water = false;
		}
	}
	
	// get target sector
	dest = ai->brain->GetAttackDest(land, water, a_type);

	if(dest)
	{
		// get all available combat/aa/arty groups for attack
		set<AAIGroup*> combat_available;
		set<AAIGroup*> aa_available;

		if(cfg->AIR_ONLY_MOD)
		{
			for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
			{
				for(list<AAIGroup*>::iterator group = ai->group_list[*category].begin(); group != ai->group_list[*category].end(); ++group)
				{		
					if(!(*group)->attack && (*group)->SufficientAttackPower())
						combat_available.insert(*group);
				}
			}
		}
		else	// non air only mods must take movement type into account
		{
			// todo: improve by checking how to reach that sector
			if(dest->water_ratio > 0.65)
			{
				land = false;
				water = true;
			}
			else 
			{
				water = false;
				land = true;
			}

			for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
			{
				for(list<AAIGroup*>::iterator group = ai->group_list[*category].begin(); group != ai->group_list[*category].end(); ++group)
				{
					// check movement type first
					suitable = true;
	
					if(land && (*group)->group_movement_type & MOVE_TYPE_SEA)
						suitable = false;

					if(water && (*group)->group_movement_type & MOVE_TYPE_GROUND)
						suitable = false;

					if(suitable && !(*group)->attack && (*group)->task == GROUP_IDLE )
					{
						if((*group)->group_unit_type == ASSAULT_UNIT && (*group)->SufficientAttackPower())
							combat_available.insert(*group);
						else if((*group)->group_unit_type == ANTI_AIR_UNIT)
							aa_available.insert(*group);
					}
				}
			}
		}

		if((combat_available.size() > 0 && SufficientAttackPowerVS(dest, &combat_available, 2)) ||  combat_available.size() > 10)
		{
			AAIAttack *attack;
			
			attack = new AAIAttack(ai);
			attacks.push_back(attack);

			attack->land = land;
			attack->water = water;

			// add combat groups
			for(set<AAIGroup*>::iterator group = combat_available.begin(); group != combat_available.end(); ++group)
				attack->AddGroup(*group);
			
			// add antiair defence
			if(!aa_available.empty())
			{
				int aa_added = 0, max_aa;

				// check how much aa sensible
				if(brain->max_units_spotted[1] < 0.2)
					max_aa = 0;
				else 
					max_aa = 1;


				for(set<AAIGroup*>::iterator group = aa_available.begin(); group != aa_available.end(); ++group)
				{
					attack->AddGroup(*group);

					++aa_added;

					if(aa_added >= max_aa)
						break;
				}
			}

			// rally attacking groups
			//this->RallyGroups(attack);

			// start the attack
			attack->AttackSector(dest, a_type);
		}

		// clean up
		aa_available.clear();
		combat_available.clear();
	}
}

void AAIAttackManager::StopAttack(AAIAttack *attack)
{
	for(list<AAIAttack*>::iterator a = attacks.begin(); a != attacks.end(); ++a)
	{
		if(*a == attack)
		{
			(*a)->StopAttack();

			delete (*a);
			attacks.erase(a);

			break;
		}
	}
}

void AAIAttackManager::CheckAttack(AAIAttack *attack)
{
	// prevent command overflow
	if((cb->GetCurrentFrame() - attack->lastAttack) < 30)
		return;

	// drop failed attacks
	if(attack->Failed())
	{
		// delete attack from list
		for(list<AAIAttack*>::iterator a = attacks.begin(); a != attacks.end(); ++a)
		{
			if(*a == attack)
			{
				attacks.erase(a);

				attack->StopAttack();
				delete attack;

				break;
			}
		}	
	}
}

void AAIAttackManager::GetNextDest(AAIAttack *attack)
{
	// prevent command overflow
	if((cb->GetCurrentFrame() - attack->lastAttack) < 60)
		return;

	// get new target sector
	AAISector *dest = ai->brain->GetNextAttackDest(attack->dest, attack->land, attack->water);

	//fprintf(ai->file, "Getting next dest\n");
	if(dest && SufficientAttackPowerVS(dest, &(attack->combat_groups), 2))
		attack->AttackSector(dest, attack->type);
	else
		attack->StopAttack();	
}

bool AAIAttackManager::SufficientAttackPowerVS(AAISector *dest, set<AAIGroup*> *combat_groups, float aggressiveness)
{
	if(dest && combat_groups->size() > 0)
	{
		// check attack power
		float attack_power = 0.5;
		float sector_defence = 0;
		int total_units = 1;

		// store ammount and category of involved groups;
		for(int i = 0; i < bt->combat_categories; ++i)
			available_combat_cat[i] = 0;

		// get total att power
		for(set<AAIGroup*>::iterator group = combat_groups->begin(); group != combat_groups->end(); ++group)
		{	
			attack_power += (*group)->GetPowerVS(5);
			available_combat_cat[(*group)->combat_category] += (*group)->size;
			total_units += (*group)->size;
		} 

		attack_power += (float)total_units * 0.2f;

		//  get expected def power
		for(int i = 0; i < bt->ass_categories; ++i)
			sector_defence += dest->stat_combat_power[i] * (float)available_combat_cat[i];

		sector_defence /= (float)total_units; 

		//fprintf(ai->file, "Checking attack power - att power / def power %f %f\n", aggressiveness * attack_power, sector_defence);
				
		if(aggressiveness * attack_power >= sector_defence)
			return true;
	}

	return false;	
}

bool AAIAttackManager::SufficientCombatPowerAt(AAISector *dest, set<AAIGroup*> *combat_groups, float aggressiveness)
{
	if(dest && combat_groups->size() > 0)
	{
		// store ammount and category of involved groups;
		double my_power = 0, enemy_power = 0;
		int total_units = 0, cat;

		// reset counter
		for(int i = 0; i < bt->combat_categories; ++i)
			available_combat_cat[i] = 0;

		// get total att power 
		for(int i = 0; i < bt->combat_categories; ++i)
		{
			cat = (int) bt->GetAssaultCategoryOfID(i);

			// skip if no enemy units of that category present
			if(dest->enemyUnitsOfType[cat] > 0)
			{
				// filter out air in normal mods
				if(i != 1 || cfg->AIR_ONLY_MOD)
				{
					for(set<AAIGroup*>::iterator group = combat_groups->begin(); group != combat_groups->end(); ++group)
						my_power += (*group)->GetPowerVS(i) * dest->enemyUnitsOfType[cat];

					total_units +=  dest->enemyUnitsOfType[cat];
				}
			}
		} 

		// skip if no enemy units found
		if(total_units == 0)
			return true;

		my_power += (float) total_units * 0.20f;

		my_power /= (float)total_units;

		// get ammount of involved groups per category
		total_units = 1;

		for(set<AAIGroup*>::iterator group = combat_groups->begin(); group != combat_groups->end(); ++group)
		{
			available_combat_cat[(*group)->combat_category] += (*group)->size;
			total_units += (*group)->size;
		}

		// get total enemy power
		for(int i = 0; i < bt->combat_categories; ++i)
			enemy_power += dest->GetAreaCombatPowerVs(i, 0.25) * (float)available_combat_cat[i];

		enemy_power /= (float)total_units;

		//fprintf(ai->file, "Checking combat power - att power / def power %f %f\n", aggressiveness * my_power, enemy_power);

		if(aggressiveness * my_power >= enemy_power)
			return true;
	}

	return false;
}

bool AAIAttackManager::SufficientDefencePowerAt(AAISector *dest, float aggressiveness)
{
	if(dest)
	{
		// store ammount and category of involved groups;
		double my_power = 0, enemy_power = 0;
		int cat;
		float temp, enemies = 0;

		// get defence power
		for(int i = 0; i < bt->combat_categories; ++i)
		{
			cat = (int) bt->GetAssaultCategoryOfID(i);

			// only check if enemies of that category present
			if(dest->enemyUnitsOfType[cat] > 0)
			{
				temp = 0;
				enemies += dest->enemyUnitsOfType[cat];

				for(list<AAIDefence>::iterator def = dest->defences.begin(); def != dest->defences.end(); ++def)
					temp += bt->units_static[def->def_id].efficiency[i];
				
				my_power += temp * dest->enemyUnitsOfType[cat];
			}
		}

		if(enemies > 0)
			my_power /= enemies;
	
		// get enemy attack power vs buildings
		enemy_power = dest->GetAreaCombatPowerVs(5, 0.5);

		//fprintf(ai->file, "Checking defence power - att power / def power %f %f\n", aggressiveness * my_power, enemy_power);

		if(aggressiveness * my_power >= enemy_power)
			return true;
	}

	return false;
}

void AAIAttackManager::RallyGroups(AAIAttack *attack)
{
}
