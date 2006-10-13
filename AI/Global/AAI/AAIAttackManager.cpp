//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------

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
}

AAIAttackManager::~AAIAttackManager(void)
{
	for(list<AAIAttack*>::iterator a = attacks.begin(); a != attacks.end(); ++a)
		delete (*a);
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
	if(attacks.size() > 0)
		a_type = OUTPOST_ATTACK;
	else 
		a_type = BASE_ATTACK;

	if(cfg->AIR_ONLY_MOD)
	{
		land = true;
		water = true;
	}
	else
	{
		if(map->mapType == LAND_MAP)
		{
			land = true;
			water = false;
		}
		else if(map->mapType == LAND_WATER_MAP)
		{
			land = true;
			water = true;
		}
		else if(map->mapType == WATER_MAP)
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
		if(cfg->AIR_ONLY_MOD)
		{
			list<AAIGroup*> combat_available;

			// get all combat groups suitable to attack
			for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
			{
				for(list<AAIGroup*>::iterator group = ai->group_list[*category].begin(); group != ai->group_list[*category].end(); ++group)
				{		
					if(!(*group)->attack && (*group)->SufficientAttackPower())
						combat_available.push_back(*group);
				}
			}

			// possible groups found
			if(!combat_available.empty())
			{
				// todo: check if enough att power
				AAIAttack *attack;
			
				try
				{
					attack = new AAIAttack(ai);
				}
				catch(...)
				{
					fprintf(ai->file, "Exception thrown when allocating memory for AAIAttack");
					return;
				}

				attacks.push_back(attack);

				attack->land = land;
				attack->water = water;

				// add combat groups
				for(list<AAIGroup*>::iterator group = combat_available.begin(); group != combat_available.end(); ++group)
					attack->AddGroup(*group);

				// start the attack
				attack->AttackSector(dest, a_type);
			}

			// clean up
			combat_available.clear();
		}
		else
		{
			list<AAIGroup*> combat_available;
			list<AAIGroup*> aa_available;

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

			// get all combat/aa groups suitable to attack
			for(list<UnitCategory>::iterator category = bt->assault_categories.begin(); category != bt->assault_categories.end(); ++category)
			{
				for(list<AAIGroup*>::iterator group = ai->group_list[*category].begin(); group != ai->group_list[*category].end(); ++group)
				{
					// check movement type first

					suitable = true;
	
					if(land && (*group)->move_type == SEA)
						suitable = false;

					if(water && (*group)->move_type == GROUND)
						suitable = false;

					if(suitable)
					{
						if((*group)->group_type == ASSAULT_UNIT)
						{
							if(!(*group)->attack && (*group)->SufficientAttackPower())
								combat_available.push_back(*group);
						}
						else if((*group)->group_type == ANTI_AIR_UNIT)
						{
							if(!(*group)->attack && (*group)->task == GROUP_IDLE)
								aa_available.push_back(*group);
						}
					}
				}
			}

			// possible groups found
			if(!combat_available.empty())
			{
				// todo: check if enough att power

				AAIAttack *attack;
			
				try
				{
				attack = new AAIAttack(ai);
				}
				catch(...)
				{
					fprintf(ai->file, "Exception thrown when allocating memory for AAIAttack");
					return;
				}

				attacks.push_back(attack);

				attack->land = land;
				attack->water = water;

				// add combat groups
				for(list<AAIGroup*>::iterator group = combat_available.begin(); group != combat_available.end(); ++group)
					attack->AddGroup(*group);
			
				// add some aa
				int aa_added = 0, max_aa;

				// check how much aa sensible
				if(brain->max_units_spotted[1] < 0.2)
					max_aa = 0;
				else 
					max_aa = 1;


				for(list<AAIGroup*>::iterator group = aa_available.begin(); group != aa_available.end(); ++group)
				{
					attack->AddGroup(*group);

					++aa_added;

					if(aa_added >= max_aa)
						break;
				}

				// start the attack
				attack->AttackSector(dest, a_type);

			}

			// clean up
			aa_available.clear();
			combat_available.clear();
		}
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

void AAIAttackManager::GetNextDest(AAIAttack *attack)
{
	// prevent command overflow
	if((cb->GetCurrentFrame() - attack->lastAttack) < 100)
		return;

	// get new target sector
	AAISector *dest = ai->brain->GetNextAttackDest(attack->dest, attack->land, attack->water);

	if(dest)
	{
		attack->AttackSector(dest, attack->type);
	}
	else  // end attack if now valid next target found
	{
		attack->StopAttack();
	}
}
