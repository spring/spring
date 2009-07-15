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

AAIAttackManager::AAIAttackManager(AAI *ai, IAICallback *cb, AAIBuildTable *bt, int continents)
{
	this->ai = ai;
	brain = ai->brain;
	this->bt = bt;
	this->cb = cb;
	this->map = ai->map;

	available_combat_cat.resize(bt->ass_categories);

	available_combat_groups_continent.resize(continents);
	available_aa_groups_continent.resize(continents);

	attack_power_continent.resize(continents, vector<float>(bt->combat_categories) );
	attack_power_global.resize(bt->combat_categories);
}

AAIAttackManager::~AAIAttackManager(void)
{
	for(list<AAIAttack*>::iterator a = attacks.begin(); a != attacks.end(); ++a)
		delete (*a);

	attacks.clear();
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
	//////////////////////////////////////////////////////////////////////////////////////////////
	// get all available combat/aa/arty groups for attack
	//////////////////////////////////////////////////////////////////////////////////////////////
	int total_combat_groups = 0;

	for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
	{
		for(list<AAIGroup*>::iterator group = ai->group_list[*category].begin(); group != ai->group_list[*category].end(); ++group)
		{
			if( (*group)->AvailableForAttack() )
			{
				if((*group)->group_movement_type & MOVE_TYPE_CONTINENT_BOUND)
				{
					if((*group)->group_unit_type == ASSAULT_UNIT)
					{
						available_combat_groups_continent[(*group)->continent].push_back(*group);
						++total_combat_groups;
					}
					else
						available_aa_groups_continent[(*group)->continent].push_back(*group);
				}
				else
				{
					if((*group)->group_unit_type == ASSAULT_UNIT)
					{
						available_combat_groups_global.push_back(*group);
						++total_combat_groups;
					}
					else
						available_aa_groups_global.push_back(*group);
				}
			}
		}
	}

	// stop planing an attack if there are no combat groups available at the moment
	if(total_combat_groups == 0)
		return;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// calculate max attack power for each continent
	//////////////////////////////////////////////////////////////////////////////////////////////
	fill(attack_power_global.begin(), attack_power_global.end(), 0.0f);

	for(list<AAIGroup*>::iterator group = available_combat_groups_global.begin(); group != available_combat_groups_global.end(); ++group)
		(*group)->GetCombatPower( &attack_power_global );

	for(size_t continent = 0; continent < available_combat_groups_continent.size(); ++continent)
	{
		fill(attack_power_continent[continent].begin(), attack_power_continent[continent].end(), 0.0f);

		for(list<AAIGroup*>::iterator group = available_combat_groups_continent[continent].begin(); group != available_combat_groups_continent[continent].end(); ++group)
			(*group)->GetCombatPower( &(attack_power_continent[continent]) );
	}

	// determine max lost units
	float max_lost_units = 0.0f, lost_units;

	for(int x = 0; x < map->xSectors; ++x)
	{
		for(int y = 0; y < map->ySectors; ++y)
		{
			lost_units = map->sector[x][y].GetLostUnits(1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

			if(lost_units > max_lost_units)
				max_lost_units = lost_units;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// determine attack sector
	//////////////////////////////////////////////////////////////////////////////////////////////
	float def_power, att_power;
	float best_rating = 0, my_rating;
	AAISector *dest = 0, *sector;

	for(int x = 0; x < map->xSectors; ++x)
	{
		for(int y = 0; y < map->ySectors; ++y)
		{
			sector = &map->sector[x][y];

			if(sector->distance_to_base == 0 || sector->enemy_structures < 0.0001)
				my_rating = 0;
			else
			{
				if(ai->map->continents[sector->continent].water)
				{
					def_power = sector->GetEnemyDefencePower(0.0f, 0.0f, 0.5f, 1.0f, 1.0f) + 0.01;
					att_power = attack_power_global[5] + attack_power_continent[sector->continent][5];
				}
				else
				{
					def_power = sector->GetEnemyDefencePower(1.0f, 0.0f, 0.5f, 0.0f, 0.0f) + 0.01;
					att_power = attack_power_global[5] + attack_power_continent[sector->continent][5];
				}

				my_rating = (1.0f - sector->GetLostUnits(1.0f, 1.0f, 1.0f, 1.0f, 1.0f) / max_lost_units) * sector->enemy_structures * att_power / ( def_power * (float)(2 + sector->distance_to_base) );

				//if(SufficientAttackPowerVS(dest, &combat_available, 2))
			}

			if(my_rating > best_rating)
			{
				dest = sector;
				best_rating = my_rating;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// order attack
	//////////////////////////////////////////////////////////////////////////////////////////////

	if(dest)
	{
		AAIAttack *attack = new AAIAttack(ai);
		attacks.push_back(attack);

		// add combat groups
		for(list<AAIGroup*>::iterator group = available_combat_groups_continent[dest->continent].begin(); group != available_combat_groups_continent[dest->continent].end(); ++group)
			attack->AddGroup(*group);

		for(list<AAIGroup*>::iterator group = available_combat_groups_global.begin(); group != available_combat_groups_global.end(); ++group)
			attack->AddGroup(*group);

		// add anti-air defence
		int aa_added = 0, max_aa;

		// check how much aa sensible
		if(brain->max_combat_units_spotted[1] < 0.2)
			max_aa = 0;
		else
			max_aa = 1;

		for(list<AAIGroup*>::iterator group = available_aa_groups_continent[dest->continent].begin(); group != available_aa_groups_continent[dest->continent].end(); ++group)
		{
			if(aa_added >= max_aa)
				break;

			attack->AddGroup(*group);
			++aa_added;
		}

		for(list<AAIGroup*>::iterator group = available_aa_groups_global.begin(); group != available_aa_groups_global.end(); ++group)
		{
			if(aa_added >= max_aa)
				break;

			attack->AddGroup(*group);
			++aa_added;
		}

		// rally attacking groups
		//RallyGroups(attack);

		// start the attack
		attack->AttackSector(dest);
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
		attack->AttackSector(dest);
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
		fill(available_combat_cat.begin(), available_combat_cat.end(), 0);

		// get total att power
		for(set<AAIGroup*>::iterator group = combat_groups->begin(); group != combat_groups->end(); ++group)
		{
			attack_power += (*group)->GetCombatPowerVsCategory(5);
			available_combat_cat[(*group)->combat_category] += (*group)->size;
			total_units += (*group)->size;
		}

		attack_power += (float)total_units * 0.2f;

		//  get expected def power
		for(int i = 0; i < bt->ass_categories; ++i)
			sector_defence += dest->enemy_stat_combat_power[i] * (float)available_combat_cat[i];

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
		int total_units = 0;

		// reset counter
		for(int i = 0; i < bt->ass_categories; ++i)
			available_combat_cat[i] = 0;

		// get total att power
		for(int i = 0; i < bt->ass_categories; ++i)
		{
			// skip if no enemy units of that category present
			if(dest->enemy_combat_units[i] > 0)
			{
				// filter out air in normal mods
				if(i != 1 || cfg->AIR_ONLY_MOD)
				{
					for(set<AAIGroup*>::iterator group = combat_groups->begin(); group != combat_groups->end(); ++group)
						my_power += (*group)->GetCombatPowerVsCategory(i) * dest->enemy_combat_units[i];

					total_units +=  dest->enemy_combat_units[i];
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
		for(int i = 0; i < bt->ass_categories; ++i)
			enemy_power += dest->GetEnemyAreaCombatPowerVs(i, 0.25) * (float)available_combat_cat[i];

		enemy_power /= (float)total_units;

		//fprintf(ai->file, "Checking combat power - att power / def power %f %f\n", aggressiveness * my_power, enemy_power);

		if(aggressiveness * my_power >= enemy_power)
			return true;
	}

	return false;
}

bool AAIAttackManager::SufficientDefencePowerAt(AAISector *dest, float aggressiveness)
{
	// store ammount and category of involved groups;
	double my_power = 0, enemy_power = 0;
	float enemies = 0;

	// get defence power
	for(int i = 0; i < bt->ass_categories; ++i)
	{
		// only check if enemies of that category present
		if(dest->enemy_combat_units[i] > 0)
		{
			enemies += dest->enemy_combat_units[i];
			my_power += dest->my_stat_combat_power[i] * dest->enemy_combat_units[i];
		}
	}

	if(enemies > 0)
	{
		my_power /= enemies;

		if(aggressiveness * my_power >= dest->GetEnemyAreaCombatPowerVs(5, 0.5))
			return true;
	}

	return false;
}

void AAIAttackManager::RallyGroups(AAIAttack *attack)
{
}
