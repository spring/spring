#include "AAIBuilder.h"

#include "AAI.h"
#include "AAIBuildTask.h"

AAIBuilder::AAIBuilder(AAI *ai, int unit_id, int def_id)
{
	this->unit_id = unit_id;
	this->ai = ai;
	this->def_id = def_id;
	cb = ai->cb;
	buildspeed = ai->bt->unitList[def_id-1]->buildSpeed;

	building_def_id = 0;
	building_unit_id = -1;
	task = IDLE;
	task_importance = 0;

	assistance = -1;
	build_task = 0;
	order_tick = 0;

	build_pos = ZeroVector;
}

AAIBuilder::~AAIBuilder(void)
{
}

void AAIBuilder::Killed()
{
	ReleaseAllAssistants();

	// when builder was killed on the way to the buildsite, inform ai that construction 
	// of building hasnt been started
	if(task == BUILDING)
	{
		//if buildling has not begun yet, decrease some values
		if(building_unit_id == -1)
		{
			--ai->bt->units_dynamic[building_def_id].active;
			--ai->futureUnits[building_category];

			// clear up buildmap etc.
			ai->execute->ConstructionFailed(-1, build_pos, building_def_id);
		}
		// building has begun
		else
		{
			if(build_task)
				build_task->BuilderDestroyed();	
		}
	}
	else if(task == ASSISTING)
	{
		if(ai->ut->units[assistance].factory)
			ai->ut->units[assistance].factory->RemoveAssitant(unit_id);
		else if(ai->ut->units[assistance].builder)
			ai->ut->units[assistance].builder->RemoveAssitant(unit_id);
	}

	task = UNIT_KILLED;
	task_importance = 100;
}

void AAIBuilder::ConstructionFailed()
{
	// tells the builder construction has finished
	BuildingFinished();
}

void AAIBuilder::Update()
{
	if(task == BUILDING)
	{
		// if building has begun, check for possible assisters
		if(building_unit_id >= 0) 
			CheckAssistance();
		// if building has not yet begun, check if something unexpected happened (buildsite blocked)
		else
		{
			const deque<Command> *commands = cb->GetCurrentUnitCommands(unit_id);

			if(commands->empty())
			{
				--ai->bt->units_dynamic[building_def_id].active;
				--ai->futureUnits[building_category];
	
				// decrease number of units of that category in the target sector
				ai->execute->ConstructionFailed(-1, build_pos, building_def_id);

				// free builder
				BuildingFinished();
			}
		}
	}
	else if(task == IDLE)
	{
		float3 pos = cb->GetUnitPos(unit_id);

		if(pos.x > 0)
		{
			int x = pos.x/ai->map->xSectorSize;
			int y = pos.z/ai->map->ySectorSize;

			if(x >= 0 && y >= 0 && x < ai->map->xSectors && y < ai->map->ySectors)
			{
				//cb->SendTextMsg("reclaiming",0);
				pos = ai->map->sector[x][y].GetCenter();

				Command c;
				c.id = CMD_RECLAIM;
				c.params.resize(4);
				c.params[0] = pos.x;
				c.params[1] = cb->GetElevation(pos.x, pos.z);
				c.params[2] = pos.z;
				c.params[3] = 1000.0;

				cb->GiveOrder(unit_id, &c);
			}
		}
	}
}

void AAIBuilder::GiveBuildOrder(int id_building, float3 pos, bool water)
{
	// get def and final position
	const UnitDef *def = ai->bt->unitList[id_building-1];
	ai->map->Pos2FinalBuildPos(&pos, def);
	build_pos = pos;

	// determine target sector
	int x = build_pos.x/ai->map->xSectorSize;
	int y = build_pos.z/ai->map->ySectorSize;

	// drop bad sectors (should only happen when defending mexes at the edge of the map)
	if(x < 0 || y < 0 || x >= ai->map->xSectors || y >= ai->map->ySectors)
		return;

	order_tick = cb->GetCurrentFrame();
	task_importance = 10;

	// check if builder was previously assisting other builders/factories
	if(assistance >= 0)
	{
		if(ai->ut->units[assistance].factory)
			ai->ut->units[assistance].factory->RemoveAssitant(unit_id);
		else if(ai->ut->units[assistance].builder)
			ai->ut->units[assistance].builder->RemoveAssitant(unit_id);

		assistance = -1;
	}

	// set building as current task and order construction
	building_def_id = id_building;
	task = BUILDING;
	building_category = ai->bt->units_static[id_building].category;

	// order builder to construct building
	Command c;
	c.id = - id_building;
	c.params.resize(3);
	c.params[0] = build_pos.x;
	c.params[1] = build_pos.y;
	c.params[2] = build_pos.z;

	cb->GiveOrder(unit_id, &c);

	// increase number of active units of that type/category
	++ai->bt->units_dynamic[def->id].active;
	++ai->futureUnits[building_category];

	// increase number of units of that category in the target sector
	++ai->map->sector[x][y].unitsOfType[building_category];
	ai->map->sector[x][y].own_structures += ai->bt->units_static[building_def_id].cost;

	// update buildmap of sector
	ai->map->Pos2BuildMapPos(&pos, def);

	// factory
	if(building_category == GROUND_FACTORY) 
	{
		ai->map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 1);
		ai->map->BlockCells(pos.x, pos.z - 8, def->xsize, 8, true, false);
		ai->map->BlockCells(pos.x + def->xsize, pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, true, false);
		ai->map->BlockCells(pos.x, pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, true, false);
	}
	else if(building_category == SEA_FACTORY)
	{
		ai->map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 5);
		ai->map->BlockCells(pos.x, pos.z - 8, def->xsize, 8, true, true);
		ai->map->BlockCells(pos.x + def->xsize, pos.z - 8, cfg->X_SPACE, def->ysize + 1.5 * cfg->Y_SPACE, true, true);
		ai->map->BlockCells(pos.x, pos.z + def->ysize, def->xsize, 1.5 * cfg->Y_SPACE - 8, true, true);
	}
	// normal building
	else
	{
		if(water)
			ai->map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 5);
		else
			ai->map->SetBuildMap(pos.x, pos.z, def->xsize, def->ysize, 1);
	}
	
	// prevent ai from building too many things in a row
	ai->map->CheckRows(pos.x, pos.z, def->xsize, def->ysize, true, water);
}

void AAIBuilder::TakeOverConstruction(AAIBuildTask *task)
{
	if(assistance >= 0)
	{
		if(ai->ut->units[assistance].factory)
			ai->ut->units[assistance].factory->RemoveAssitant(unit_id);
		else if(ai->ut->units[assistance].builder)
			ai->ut->units[assistance].builder->RemoveAssitant(unit_id);

		assistance = -1;
	}

	this->task = BUILDING;
	task_importance = 10;
	order_tick = task->order_tick;

	building_unit_id = task->unit_id;
	building_def_id = task->def_id;
	building_category = ai->bt->units_static[building_def_id].category;
	build_pos = task->build_pos;
	
	Command c;
	c.id = CMD_REPAIR;
	c.params.push_back(task->unit_id);

	cb->GiveOrder(unit_id, &c);
}

void AAIBuilder::BuildingFinished()
{
	task = IDLE;

	task_importance = 0;
	build_pos = ZeroVector;
	building_def_id = 0;
	building_unit_id = -1; 
	building_category = UNKNOWN;

	build_task = 0;

	// release assisters
	ReleaseAllAssistants();
}

void AAIBuilder::AssistConstruction(int builder, int target_unit, int importance)
{
	task = ASSISTING;
	assistance = builder;
	task_importance = importance;

	Command c;
	c.id = CMD_REPAIR;
	c.params.push_back(target_unit);
	cb->GiveOrder(unit_id, &c);
}

void AAIBuilder::AssistFactory(int factory, int importance)
{
	task = ASSISTING;
	assistance = factory;
	task_importance = importance;

	Command c;
	c.id = CMD_GUARD;
	c.params.push_back(factory);
	cb->GiveOrder(unit_id, &c);
}

void AAIBuilder::ReleaseAssistant()
{
	task = IDLE;
	assistance = -1;
	task_importance = 0;
}

void AAIBuilder::ReleaseAllAssistants()
{
	// release assisters
	AAIBuilder *builder;

	for(set<int>::iterator i = assistants.begin(); i != assistants.end(); ++i)
	{
		builder = ai->ut->units[*i].builder;

		if(builder)
			builder->ReleaseAssistant();
	}

	assistants.clear();
}


void AAIBuilder::StopAssistant()
{
	//if(task != UNIT_KILLED && unit_id > 0 && unit_id < cfg->MAX_UNITS  && cb!=NULL && cb->GetUnitDef(unit_id)!=0)
	
	ReleaseAssistant();

	Command c;
	c.id = CMD_STOP;
	cb->GiveOrder(unit_id, &c);
}

void AAIBuilder::RemoveAssitant(int unit_id)
{
	assistants.erase(unit_id);
}

void AAIBuilder::CheckAssistance()
{
	// prevent assisting when low on ressources
	if(ai->execute->averageMetalSurplus < 0.1)
	{	
		if(building_category == METAL_MAKER)
		{
			if(ai->execute->averageEnergySurplus < 0.7 * ai->bt->unitList[building_def_id-1]->energyUpkeep)
				return;
		}
		else if(building_category != EXTRACTOR && building_category != POWER_PLANT)
			return;
	}

	float buildtime = ai->bt->unitList[building_def_id-1]->buildTime / ai->bt->unitList[def_id-1]->buildSpeed;

	if(buildtime > cfg->MIN_ASSISTANCE_BUILDTIME && assistants.size() < cfg->MAX_ASSISTANTS)
	{
		AAIBuilder* assistant; 

		// call idle builder
		if(ai->bt->unitList[building_def_id-1]->minWaterDepth > 0)
			assistant = ai->ut->FindAssistBuilder(build_pos, 5, true, ai->bt->unitList[building_def_id-1]->floater);
		else
			assistant = ai->ut->FindAssistBuilder(build_pos, 5, false, false);

		if(assistant)
		{
			assistants.insert(assistant->unit_id);
			assistant->AssistConstruction(unit_id, building_unit_id);
		}
	}
}

void AAIBuilder::GiveReclaimOrder(int unit_id)
{
	if(assistance >= 0)
	{
		if(ai->ut->units[assistance].factory)
			ai->ut->units[assistance].factory->RemoveAssitant(unit_id);
		else if(ai->ut->units[assistance].builder)
			ai->ut->units[assistance].builder->RemoveAssitant(unit_id);

		assistance = -1;
	}
	
	task = RECLAIMING;
	task_importance = 10;

	Command c;
	c.id = CMD_RECLAIM;
	c.params.push_back(unit_id);
	cb->GiveOrder(this->unit_id, &c);
}

void AAIBuilder::Idle()
{
	if(task == BUILDING)
	{
		if(building_unit_id == -1)
		{
			--ai->bt->units_dynamic[building_def_id].active;
			--ai->futureUnits[building_category];

			// clear up buildmap etc.
			ai->execute->ConstructionFailed(-1, build_pos, building_def_id);

			// free builder
			BuildingFinished();
		}
	}
	else if(task != UNIT_KILLED)
	{
		task = IDLE;
		task_importance = 0;
		assistance = -1;

		ReleaseAllAssistants();
	}
}