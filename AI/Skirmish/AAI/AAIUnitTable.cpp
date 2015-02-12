// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include "AAI.h"
#include "AAIUnitTable.h"
#include "AAIExecute.h"
#include "AAIConstructor.h"
#include "AAIBuildTable.h"
#include "AAIAirForceManager.h"
#include "AAIConfig.h"
#include "AAIMap.h"
#include "AAIGroup.h"
#include "AAIConstructor.h"

#include "LegacyCpp/UnitDef.h"
using namespace springLegacyAI;


AAIUnitTable::AAIUnitTable(AAI *ai)
{
	this->ai = ai;

	units.resize(cfg->MAX_UNITS);

	// fill buildtable
	for(int i = 0; i < cfg->MAX_UNITS; ++i)
	{
		units[i].unit_id = -1;
		units[i].def_id = 0;
		units[i].group = 0;
		units[i].cons = 0;
		units[i].status = UNIT_KILLED;
		units[i].last_order = 0;
	}

	for(int i = 0; i <= MOBILE_CONSTRUCTOR; ++i)
	{
		activeUnits[i] = 0;
		futureUnits[i] = 0;
		requestedUnits[i] = 0;
	}

	activeBuilders = futureBuilders = 0;
	activeFactories = futureFactories = 0;

	cmdr = -1;
}

AAIUnitTable::~AAIUnitTable(void)
{
	// delete constructors
	for(set<int>::iterator cons = constructors.begin(); cons != constructors.end(); ++cons)
	{
		delete units[*cons].cons;
	}
}


bool AAIUnitTable::AddUnit(int unit_id, int def_id, AAIGroup *group, AAIConstructor *cons)
{
	if(unit_id <= cfg->MAX_UNITS)
	{
		// clear possible enemies that are still listed (since they had been killed outside of los)
		if(units[unit_id].status == ENEMY_UNIT)
		{
			if(units[unit_id].group)
				units[unit_id].group->TargetUnitKilled();
		}
		else if(units[unit_id].status == BOMB_TARGET)
		{
			ai->Getaf()->RemoveTarget(unit_id);

			if(units[unit_id].group)
				units[unit_id].group->TargetUnitKilled();
		}

		units[unit_id].unit_id = unit_id;
		units[unit_id].def_id = def_id;
		units[unit_id].group = group;
		units[unit_id].cons = cons;
		units[unit_id].status = UNIT_IDLE;
		return true;
	}
	else
	{
		ai->Log("ERROR: AAIUnitTable::AddUnit() index %i out of range", unit_id);
		return false;
	}
}

void AAIUnitTable::RemoveUnit(int unit_id)
{
	if(unit_id <= cfg->MAX_UNITS)
	{
		units[unit_id].unit_id = -1;
		units[unit_id].def_id = 0;
		units[unit_id].group = 0;
		units[unit_id].cons = 0;
		units[unit_id].status = UNIT_KILLED;
	}
	else
	{
		ai->Log("ERROR: AAIUnitTable::RemoveUnit() index %i out of range", unit_id);
	}
}

void AAIUnitTable::AddConstructor(int unit_id, int def_id)
{
	bool factory;
	bool builder;
	bool assister;

	if(ai->Getbt()->units_static[def_id].unit_type & UNIT_TYPE_FACTORY)
		factory = true;
	else
		factory = false;

	if(ai->Getbt()->units_static[def_id].unit_type & UNIT_TYPE_BUILDER)
		builder = true;
	else
		builder = false;

	if(ai->Getbt()->units_static[def_id].unit_type & UNIT_TYPE_ASSISTER)
		assister = true;
	else
		assister = false;

	AAIConstructor *cons = new AAIConstructor(ai, unit_id, def_id, factory, builder, assister);

	constructors.insert(unit_id);
	units[unit_id].cons = cons;

	// increase/decrease number of available/requested builders for all buildoptions of the builder
	for(list<int>::iterator unit = ai->Getbt()->units_static[def_id].canBuildList.begin();  unit != ai->Getbt()->units_static[def_id].canBuildList.end(); ++unit)
	{
		ai->Getbt()->units_dynamic[*unit].constructorsAvailable += 1;
		ai->Getbt()->units_dynamic[*unit].constructorsRequested -= 1;
	}

	if(builder)
	{
		--futureBuilders;
		++activeBuilders;
	}

	if(factory && ai->Getbt()->IsStatic(def_id))
	{
		--futureFactories;
		++activeFactories;

		// remove future ressource demand now factory has been finished
		ai->Getexecute()->futureRequestedMetal -= ai->Getbt()->units_static[def_id].efficiency[0];
		ai->Getexecute()->futureRequestedEnergy -= ai->Getbt()->units_static[def_id].efficiency[1];
	}
}

void AAIUnitTable::RemoveConstructor(int unit_id, int def_id)
{
	if(units[unit_id].cons->builder)
		activeBuilders -= 1;

	if(units[unit_id].cons->factory && ai->Getbt()->IsStatic(def_id))
		activeFactories -= 1;

	// decrease number of available builders for all buildoptions of the builder
	for(list<int>::iterator unit = ai->Getbt()->units_static[def_id].canBuildList.begin();  unit != ai->Getbt()->units_static[def_id].canBuildList.end(); ++unit)
		ai->Getbt()->units_dynamic[*unit].constructorsAvailable -= 1;

	// erase from builders list
	constructors.erase(unit_id);

	// clean up memory
	units[unit_id].cons->Killed();
	delete units[unit_id].cons;
	units[unit_id].cons = 0;
}

void AAIUnitTable::AddCommander(int unit_id, int def_id)
{
	bool factory;
	bool builder;
	bool assister;

	if(ai->Getbt()->units_static[def_id].unit_type & UNIT_TYPE_FACTORY)
		factory = true;
	else
		factory = false;

	if(ai->Getbt()->units_static[def_id].unit_type & UNIT_TYPE_BUILDER)
		builder = true;
	else
		builder = false;

	if(ai->Getbt()->units_static[def_id].unit_type & UNIT_TYPE_ASSISTER)
		assister = true;
	else
		assister = false;

	AAIConstructor *cons = new AAIConstructor(ai, unit_id, def_id, factory, builder, assister);
	constructors.insert(unit_id);
	units[unit_id].cons = cons;

	cmdr = unit_id;

	// increase number of builders for all buildoptions of the commander
	for(list<int>::iterator unit = ai->Getbt()->units_static[def_id].canBuildList.begin();  unit != ai->Getbt()->units_static[def_id].canBuildList.end(); ++unit)
		++ai->Getbt()->units_dynamic[*unit].constructorsAvailable;

}

void AAIUnitTable::RemoveCommander(int unit_id, int def_id)
{
	// decrease number of builders for all buildoptions of the commander
	for(list<int>::iterator unit = ai->Getbt()->units_static[def_id].canBuildList.begin();  unit != ai->Getbt()->units_static[def_id].canBuildList.end(); ++unit)
		--ai->Getbt()->units_dynamic[*unit].constructorsAvailable;

	// erase from builders list
	constructors.erase(unit_id);

	// clean up memory
	units[unit_id].cons->Killed();
	delete units[unit_id].cons;
	units[unit_id].cons = 0;

	// commander has been destroyed, set pointer to zero
	if(unit_id == cmdr)
		cmdr = -1;
}

void AAIUnitTable::AddExtractor(int unit_id)
{
	extractors.insert(unit_id);
}

void AAIUnitTable::RemoveExtractor(int unit_id)
{
	extractors.erase(unit_id);
}

void AAIUnitTable::AddScout(int unit_id)
{
	scouts.insert(unit_id);
}

void AAIUnitTable::RemoveScout(int unit_id)
{
	scouts.erase(unit_id);
}

void AAIUnitTable::AddPowerPlant(int unit_id, int def_id)
{
	power_plants.insert(unit_id);
	ai->Getexecute()->futureAvailableEnergy -= ai->Getbt()->units_static[def_id].efficiency[0];
}

void AAIUnitTable::RemovePowerPlant(int unit_id)
{
	power_plants.erase(unit_id);
}

void AAIUnitTable::AddMetalMaker(int unit_id, int def_id)
{
	metal_makers.insert(unit_id);
	ai->Getexecute()->futureRequestedEnergy -= ai->Getbt()->GetUnitDef(def_id).energyUpkeep;
}

void AAIUnitTable::RemoveMetalMaker(int unit_id)
{
	if(!ai->Getcb()->IsUnitActivated(unit_id))
		--ai->Getexecute()->disabledMMakers;

	metal_makers.erase(unit_id);
}

void AAIUnitTable::AddRecon(int unit_id, int def_id)
{
	recon.insert(unit_id);

	ai->Getexecute()->futureRequestedEnergy -= ai->Getbt()->units_static[def_id].efficiency[0];
}

void AAIUnitTable::RemoveRecon(int unit_id)
{
	recon.erase(unit_id);
}

void AAIUnitTable::AddJammer(int unit_id, int def_id)
{
	jammers.insert(unit_id);

	ai->Getexecute()->futureRequestedEnergy -= ai->Getbt()->units_static[def_id].efficiency[0];
}

void AAIUnitTable::RemoveJammer(int unit_id)
{
	jammers.erase(unit_id);
}

void AAIUnitTable::AddStationaryArty(int unit_id, int /*def_id*/)
{
	stationary_arty.insert(unit_id);
}

void AAIUnitTable::RemoveStationaryArty(int unit_id)
{
	stationary_arty.erase(unit_id);
}

AAIConstructor* AAIUnitTable::FindBuilder(int building, bool commander)
{
	//ai->Log("constructor for %s\n", ai->Getbt()->GetCategoryString(building));
	AAIConstructor *builder;

	// look for idle builder
	for(set<int>::iterator i = constructors.begin(); i != constructors.end(); ++i)
	{
		// check all builders
		if(units[*i].cons->builder)
		{
			builder = units[*i].cons;

			// find unit that can directly build that building
			if(builder->task != BUILDING && ai->Getbt()->CanBuildUnit(builder->def_id, building))
			{
				//if(ai->Getbt()->units_static[building].category == STATIONARY_JAMMER)
				//	ai->Log("%s can build %s\n", ai->Getbt()->GetUnitDef(builder->def_id-1).humanName.c_str(), ai->Getbt()->GetUnitDef(building-1).humanName.c_str());

				// filter out commander (if not allowed)
				if(! (!commander &&  ai->Getbt()->IsCommander(builder->def_id)) )
					return builder;
			}
		}
	}

	// no builder found
	return 0;
}

AAIConstructor* AAIUnitTable::FindClosestBuilder(int building, float3 *pos, bool commander, float *min_dist)
{
	float my_dist;
	AAIConstructor *best_builder = 0, *builder;
	float3 builder_pos;
	bool suitable;

	int continent = ai->Getmap()->GetContinentID(pos);
	*min_dist = 100000.0f;

	// look for idle builder
	for(set<int>::iterator i = constructors.begin(); i != constructors.end(); ++i)
	{
		// check all builders
		if(units[*i].cons->builder)
		{
			builder = units[*i].cons;

			// find idle or assisting builder, who can build this building
			if(  builder->task != BUILDING && ai->Getbt()->CanBuildUnit(builder->def_id, building))
			{
				builder_pos = ai->Getcb()->GetUnitPos(builder->unit_id);

				// check continent if necessary
				if(ai->Getbt()->units_static[builder->def_id].movement_type & MOVE_TYPE_CONTINENT_BOUND)
				{
					if(ai->Getmap()->GetContinentID(&builder_pos) == continent)
						suitable = true;
					else
						suitable = false;
				}
				else
					suitable = true;

				// filter out commander
				if(suitable && ( commander || !ai->Getbt()->IsCommander(builder->def_id) ) )
				{
					my_dist = fastmath::apxsqrt( (builder_pos.x - pos->x) * (builder_pos.x - pos->x) + (builder_pos.z - pos->z) * (builder_pos.z - pos->z) );

					if(ai->Getbt()->GetUnitDef(builder->def_id).speed > 0)
						my_dist /= ai->Getbt()->GetUnitDef(builder->def_id).speed;

					if(my_dist < *min_dist)
					{
						best_builder = builder;
						*min_dist = my_dist;
					}
				}
			}
		}
	}

	return best_builder;
}

AAIConstructor* AAIUnitTable::FindClosestAssistant(float3 pos, int /*importance*/, bool commander)
{
	AAIConstructor *best_assistant = 0, *assistant;
	float best_rating = 0, my_rating, dist;
	float3 assistant_pos;
	bool suitable;

	int continent = ai->Getmap()->GetContinentID(&pos);

	// find idle builder
	for(set<int>::iterator i = constructors.begin(); i != constructors.end(); ++i)
	{
		// check all assisters
		if(units[*i].cons->assistant)
		{
			assistant = units[*i].cons;

			// find idle assister
			if(assistant->task == UNIT_IDLE)
			{
				assistant_pos = ai->Getcb()->GetUnitPos(assistant->unit_id);

				// check continent if necessary
				if(ai->Getbt()->units_static[assistant->def_id].movement_type & MOVE_TYPE_CONTINENT_BOUND)
				{
					if(ai->Getmap()->GetContinentID(&assistant_pos) == continent)
						suitable = true;
					else
						suitable = false;
				}
				else
					suitable = true;

				// filter out commander
				if(suitable && ( commander || !ai->Getbt()->IsCommander(assistant->def_id) ) )
				{
					dist = (pos.x - assistant_pos.x) * (pos.x - assistant_pos.x) + (pos.z - assistant_pos.z) * (pos.z - assistant_pos.z);

					if(dist > 0)
						my_rating = assistant->buildspeed / fastmath::apxsqrt(dist);
					else
						my_rating = 1;

					if(my_rating > best_rating)
					{
						best_rating = my_rating;
						best_assistant = assistant;
					}
				}
			}
		}
	}

	// no assister found -> request one
	if(!best_assistant)
	{
		unsigned int allowed_movement_types = 22;

		if(ai->Getcb()->GetElevation(pos.x, pos.z) < 0)
			allowed_movement_types |= MOVE_TYPE_SEA;
		else
			allowed_movement_types |= MOVE_TYPE_GROUND;

		ai->Getbt()->AddAssistant(allowed_movement_types, true);
	}

	return best_assistant;
}

bool AAIUnitTable::IsUnitCommander(int unit_id)
{
	if(cmdr != -1)
		return false;
	else if(cmdr == unit_id)
		return true;
	else
		return false;
}

bool AAIUnitTable::IsDefCommander(int def_id)
{
	for(int s = 0; s < cfg->SIDES; ++s)
	{
		if(ai->Getbt()->startUnits[s] == def_id)
			return true;
	}

	return false;
}

void AAIUnitTable::EnemyKilled(int unit)
{
	if(units[unit].status == BOMB_TARGET)
		ai->Getaf()->RemoveTarget(unit);


	if(units[unit].group)
		units[unit].group->TargetUnitKilled();

	RemoveUnit(unit);
}

void AAIUnitTable::AssignGroupToEnemy(int unit, AAIGroup *group)
{
	units[unit].unit_id = unit;
	units[unit].group = group;
	units[unit].status = ENEMY_UNIT;
}


void AAIUnitTable::SetUnitStatus(int unit, UnitTask status)
{
	units[unit].status = status;
}

bool AAIUnitTable::IsBuilder(int unit_id)
{
	if(units[unit_id].cons && units[unit_id].cons->builder)
		return true;
	else
		return false;
}

void AAIUnitTable::ActiveUnitKilled(UnitCategory category)
{
	--activeUnits[category];
}

void AAIUnitTable::FutureUnitKilled(UnitCategory category)
{
	--futureUnits[category];
}

void AAIUnitTable::UnitCreated(UnitCategory category)
{
	--requestedUnits[category];
	++futureUnits[category];
}

void AAIUnitTable::UnitFinished(UnitCategory category)
{
	--futureUnits[category];
	++activeUnits[category];
}

void AAIUnitTable::UnitRequestFailed(UnitCategory category)
{
	--requestedUnits[category];
}

void AAIUnitTable::UnitRequested(UnitCategory category, int number)
{
	requestedUnits[category] += number;
}
