#include "AAIUnitTable.h"

#include "AAI.h"

AAIUnitTable::AAIUnitTable(AAI *ai, AAIBuildTable *bt)
{
	this->ai = ai;
	this->bt = bt;
	this->cb = ai->cb;

	units = new AAIUnit[cfg->MAX_UNITS];

	// fill buildtable 
	for(int i = 0; i < cfg->MAX_UNITS; i++)
	{
		units[i].unit_id = -1;
		units[i].def_id = 0;
		units[i].group = 0;
		units[i].builder = 0;
		units[i].factory = 0;
	}

	cmdr = -1;
}

AAIUnitTable::~AAIUnitTable(void)
{
	// delete builders
	for(set<int>::iterator builder = builders.begin(); builder != builders.end(); ++builder)
	{
		delete units[*builder].builder;
	}

	// delete factories
	for(set<int>::iterator factory = factories.begin(); factory != factories.end(); ++factory)
	{
		delete units[*factory].factory;
	}

	delete [] units;
}


bool AAIUnitTable::AddUnit(int unit_id, int def_id, AAIGroup *group, AAIBuilder *builder, AAIFactory *factory)
{
	if(unit_id <= cfg->MAX_UNITS)
	{
		units[unit_id].unit_id = unit_id;
		units[unit_id].def_id = def_id;
		units[unit_id].group = group;
		units[unit_id].builder = builder;
		units[unit_id].factory = factory;
		return true;
	}
	else
	{
		fprintf(ai->file, "ERROR: AAIUnitTable::AddUnit() index %i out of range", unit_id);
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
		units[unit_id].builder = 0;
		units[unit_id].factory = 0;
	}
	else
	{
		fprintf(ai->file, "ERROR: AAIUnitTable::RemoveUnit() index %i out of range", unit_id);
	}
}

void AAIUnitTable::AddBuilder(int unit_id, int def_id)
{
	AAIBuilder *builder = new AAIBuilder(ai, unit_id, def_id);
	
	builders.insert(unit_id);
	units[unit_id].builder = builder;

	// set units buildable 
	if(bt->units_dynamic[def_id].active == 1)
	{
		for(list<int>::iterator unit = bt->units_static[def_id].canBuildList.begin(); unit != bt->units_static[def_id].canBuildList.end(); ++unit)
			bt->units_dynamic[*unit].builderAvailable = true;	
	}
}

void AAIUnitTable::RemoveBuilder(int unit_id, int def_id)
{
	// erase from builders list
	builders.erase(unit_id);

	--bt->units_dynamic[def_id].active;

	// clean up memory
	units[unit_id].builder->Killed();
	delete units[unit_id].builder;
	units[unit_id].builder = 0;
}

void AAIUnitTable::AddFactory(int unit_id, int def_id)
{
	// remove future ressource demand now factory has been finished
	ai->execute->futureRequestedMetal -= bt->units_static[def_id].efficiency[0];
	ai->execute->futureRequestedEnergy -= bt->units_static[def_id].efficiency[1];

	AAIFactory *factory = new AAIFactory(ai, unit_id, def_id);
	factories.insert(unit_id);
	units[unit_id].factory = factory;

	// set units buildable 
	if(bt->units_dynamic[def_id].active == 1)
	{
		for(list<int>::iterator unit = bt->units_static[def_id].canBuildList.begin(); unit != bt->units_static[def_id].canBuildList.end(); ++unit)
			bt->units_dynamic[*unit].builderAvailable = true;	
	}
}

void AAIUnitTable::RemoveFactory(int unit_id)
{
	factories.erase(unit_id);

	units[unit_id].factory->Killed();
	delete units[unit_id].factory;
	units[unit_id].factory = 0;
}

void AAIUnitTable::AddCommander(int unit_id, const UnitDef *def)
{
	// check if startng unit is a factory or builder
	if(def->canfly || def->movedata)
	{
		cmdr = unit_id;
		AAIBuilder *com = new AAIBuilder(ai, unit_id, def->id);

		units[cmdr].builder = com;
		builders.insert(unit_id);	
	}
	else	
	{
		cmdr = unit_id;
		AAIFactory *com = new AAIFactory(ai, unit_id, def->id);

		units[cmdr].factory = com;
		factories.insert(unit_id);
	}
}

void AAIUnitTable::RemoveCommander(int unit_id)
{
	if(units[unit_id].builder)
	{
		builders.erase(unit_id);
		units[unit_id].builder->Killed();
		
		delete units[unit_id].builder;	
		units[unit_id].builder = 0;	
	}
	else if(units[unit_id].factory)
	{
		factories.erase(unit_id);
		units[unit_id].factory->Killed();
											
		delete units[unit_id].factory;
		units[unit_id].factory = 0;
	}
			
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
	// delete mex from list
	extractors.erase(unit_id);
}

void AAIUnitTable::AddPowerPlant(int unit_id, int def_id)
{
	power_plants.insert(unit_id);

	float output = bt->units_static[def_id].efficiency[0];
			
	ai->execute->futureAvailableEnergy -= output;

	if(output > ai->brain->best_power_plant_output)
		ai->brain->best_power_plant_output = output; 
}

void AAIUnitTable::RemovePowerPlant(int unit_id)
{
	power_plants.erase(unit_id);
}

void AAIUnitTable::AddMetalMaker(int unit_id, int def_id)
{
	metal_makers.insert(unit_id);
	ai->execute->futureRequestedEnergy -= bt->unitList[def_id-1]->energyUpkeep;
}

void AAIUnitTable::RemoveMetalMaker(int unit_id)
{
	if(!cb->IsUnitActivated(unit_id))
		--ai->execute->disabledMMakers;

	metal_makers.erase(unit_id);
}

void AAIUnitTable::AddRecon(int unit_id, int def_id)
{
	recon.insert(unit_id);
	
	ai->execute->futureRequestedEnergy -= bt->units_static[def_id].efficiency[0];
}

void AAIUnitTable::RemoveRecon(int unit_id)
{
	recon.erase(unit_id);
}

void AAIUnitTable::AddJammer(int unit_id, int def_id)
{
	jammers.insert(unit_id);

	ai->execute->futureRequestedEnergy -= bt->units_static[def_id].efficiency[0];
}

void AAIUnitTable::RemoveJammer(int unit_id)
{
	jammers.erase(unit_id);
}

void AAIUnitTable::AddStationaryArty(int unit_id, int def_id)
{
	stationary_arty.insert(unit_id);
}

void AAIUnitTable::RemoveStationaryArty(int unit_id)
{
	stationary_arty.erase(unit_id);
}


AAIBuilder* AAIUnitTable::FindBuilder(int building, bool commander, int importance)
{	
	AAIBuilder *builder;

	// look for idle builder
	for(set<int>::iterator i = builders.begin(); i != builders.end(); ++i)
	{
		builder = units[*i].builder;

		// find unit that can directly build that building 
		if(builder->task != UNIT_KILLED && importance > builder->task_importance && bt->CanBuildUnit(builder->def_id, building))
		{
			// filter out commander (if not allowed)
			if(!commander)
			{
				if(!IsUnitCommander(builder->unit_id))
					return builder;	
			}
			else
				return builder;
		}
	}
	
	// no builder found 
	return 0;
}

AAIBuilder* AAIUnitTable::FindClosestBuilder(int building, float3 pos, bool commander, int importance)
{	
	float min_dist = 1000000, my_dist;
	AAIBuilder *best_builder = 0, *builder;
	float3 builder_pos;

	// look for idle builder
	for(set<int>::iterator i = builders.begin(); i != builders.end(); ++i)
	{
		builder = units[*i].builder;

		// find idle or assisting builder, who can build this building
		if(bt->CanBuildUnit(builder->def_id, building) && importance > builder->task_importance)
		{
			// filter out commander/non water builders
			if(!commander &&  bt->units_static[builder->def_id].category == COMMANDER)
				my_dist = 1000000;
			else if(builder->task == UNIT_KILLED)
				my_dist = 1000000;
			else
			{
				builder_pos = cb->GetUnitPos(builder->unit_id);

				if(pos.x > 0)
				{
					my_dist = sqrt(pow(builder_pos.x - pos.x, 2)+pow(builder_pos.z - pos.z, 2));
					
					if(my_dist / bt->unitList[builder->def_id-1]->speed > 0)
						my_dist /= bt->unitList[builder->def_id-1]->speed;
				}
				else
					my_dist = 1000000;	
			}
			
			if(my_dist < min_dist)
			{
				best_builder = builder;
				min_dist = my_dist;
			}
		}
	}
		
	// no builder found 
	return best_builder;
}

AAIBuilder* AAIUnitTable::FindAssistBuilder(float3 pos, int importance,  bool water, bool floater)
{
	AAIBuilder *best_builder = 0, *builder;
	float best_rating = 0, my_rating;
	float3 builder_pos;
	bool suitable;
	UnitCategory category;
	float temp;

	// find idle builder
	for(set<int>::iterator i = builders.begin(); i != builders.end(); ++i)
	{
		builder = units[*i].builder;

		if(builder->task != UNIT_KILLED && builder->task_importance < importance)
		{
			category = bt->units_static[builder->def_id].category;
			suitable = false;

			if(!water && category != SEA_BUILDER)
				suitable = true;
			else if(floater && (category == AIR_BUILDER || category == SEA_BUILDER || category == HOVER_BUILDER))
				suitable = true;
			else if(water && !floater && category == SEA_BUILDER)
				suitable = true;

			if(suitable)
			{
				builder_pos = cb->GetUnitPos(builder->unit_id);
				temp = pow(pos.x - builder_pos.x, 2) + pow(pos.z - builder_pos.z, 2);
				
				if(temp > 0)
					my_rating = builder->buildspeed / sqrt(temp);
				else
					my_rating = 10;
		
				if(my_rating > best_rating)
				{
					best_rating = my_rating;
					best_builder = builder;
				}
			}
		}
	}
		
	// no builder found 
	if(!best_builder)
	{
		bt->AddAssitantBuilder(water, floater, true);
	}

	return best_builder;
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
		if(bt->startUnits[s] == def_id)
			return true;
	}

	return false;
}
