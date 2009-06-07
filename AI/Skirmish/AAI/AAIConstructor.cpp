
#include "aidef.h"
#include "AAI.h"
#include "AAIConstructor.h"
#include "AAIBuildTask.h"

AAIConstructor::AAIConstructor(AAI *ai, int unit_id, int def_id, bool factory, bool builder, bool assistant)
{
	this->ai = ai;
	cb = ai->cb;
	bt = ai->bt;

	this->unit_id = unit_id;
	this->def_id = def_id;
	buildspeed = ai->bt->unitList[def_id-1]->buildSpeed;

	construction_unit_id = -1;
	construction_def_id = 0;
	construction_category = UNKNOWN;

	assistance = -1;
	build_task = 0;
	order_tick = 0;

	task = UNIT_IDLE;

	build_pos = ZeroVector;

	this->factory = factory;
	this->builder = builder;
	this->assistant = assistant;

	buildque = ai->execute->GetBuildqueueOfFactory(def_id);
}

AAIConstructor::~AAIConstructor(void)
{
}

// returns true if factory is busy
bool AAIConstructor::IsBusy()
{
	const CCommandQueue *commands = ai->cb->GetCurrentUnitCommands(unit_id);

	if(commands->empty())
		return false;
	else
		return true;
}

void AAIConstructor::Idle()
{
	//char c[80];
	//SNPRINTF(c, 80, "%s is idle", bt->unitList[def_id-1]->humanName.c_str());
	//cb->SendTextMsg(c, 0);

	if(builder)
	{
		if(task == BUILDING)
		{
			if(construction_unit_id == -1)
			{
				ai->bt->units_dynamic[construction_def_id].active -= 1;
				ai->ut->UnitRequestFailed(construction_category);

				// clear up buildmap etc. (make sure conctructor wanted to build a building and not a unit)
				if(bt->units_static[construction_def_id].category <= METAL_MAKER)
					ai->execute->ConstructionFailed(build_pos, construction_def_id);

				// free builder
				ConstructionFinished();
			}
		}
		else if(task != UNIT_KILLED)
		{
			task = UNIT_IDLE;
			assistance = -1;

			ReleaseAllAssistants();
		}
	}

	if(factory)
	{
		ConstructionFinished();

		Update();
	}
}

void AAIConstructor::Update()
{
	if(factory)
	{
		if(!construction_def_id && !buildque->empty())
		{
			int def_id = (*buildque->begin());
			UnitCategory cat = ai->bt->units_static[def_id].category;

			if(ai->bt->IsBuilder(def_id) || cat == SCOUT || ai->cb->GetMetal() > 50
				|| ai->bt->units_static[def_id].cost < ai->bt->avg_cost[ai->bt->units_static[def_id].category][ai->side-1])
			{
				// check if mobile or stationary builder
				if(bt->IsStatic(this->def_id))
				{
					// give build order
					Command c;
					c.id = -def_id;
					ai->cb->GiveOrder(unit_id, &c);
					construction_def_id = def_id;
					task = BUILDING;

					//if(bt->IsFactory(def_id))
					//	++ai->futureFactories;

					buildque->pop_front();
				}
				else
				{
					// find buildpos for the unit
					float3 pos = ai->execute->GetUnitBuildsite(unit_id, def_id);

					if(pos.x > 0)
					{
						// give build order
						Command c;
						c.id = -def_id;
						c.params.resize(3);
						c.params[0] = pos.x;
						c.params[1] = pos.y;
						c.params[2] = pos.z;

						ai->cb->GiveOrder(unit_id, &c);
						construction_def_id = def_id;
						task = BUILDING;

						++ai->ut->futureUnits[cat];

						buildque->pop_front();
					}
				}

				return;
			}
		}

		CheckAssistance();
	}

	if(builder)
	{
		if(task == BUILDING)
		{
			// if building has begun, check for possible assisters
			if(construction_unit_id >= 0)
				CheckAssistance();
			// if building has not yet begun, check if something unexpected happened (buildsite blocked)
			else
			{
				if(!IsBusy() && construction_unit_id == -1)
					ConstructionFailed();
			}
		}
		/*else if(task == UNIT_IDLE)
		{
			float3 pos = cb->GetUnitPos(unit_id);

			if(pos.x > 0)
			{
				int x = pos.x/ai->map->xSectorSize;
				int y = pos.z/ai->map->ySectorSize;

				if(x >= 0 && y >= 0 && x < ai->map->xSectors && y < ai->map->ySectors)
				{
					if(ai->map->sector[x][y].distance_to_base < 2)
					{
						pos = ai->map->sector[x][y].GetCenter();

						Command c;
						const UnitDef *def;

						def = cb->GetUnitDef(unit_id);
						// can this thing resurrect? If so, maybe we should raise the corpses instead of consuming them?
						if(def->canResurrect)
						{
							if(rand()%2 == 1)
								c.id = CMD_RESURRECT;
							else
								c.id = CMD_RECLAIM;
						}
						else
							c.id = CMD_RECLAIM;

						c.params.resize(4);
						c.params[0] = pos.x;
						c.params[1] = cb->GetElevation(pos.x, pos.z);
						c.params[2] = pos.z;
						c.params[3] = 500.0;

						//cb->GiveOrder(unit_id, &c);
						task = RECLAIMING;
						ai->execute->GiveOrder(&c, unit_id, "Builder::Reclaming");
					}
				}
			}
		}
		*/
	}
}

void AAIConstructor::CheckAssistance()
{
	if(factory)
	{
		// check if another factory of that type needed
		if(buildque->size() >= cfg->MAX_BUILDQUE_SIZE - 2 && assistants.size() >= cfg->MAX_ASSISTANTS-2)
		{
			if(ai->bt->units_dynamic[def_id].active + ai->bt->units_dynamic[def_id].requested + ai->bt->units_dynamic[def_id].under_construction  < cfg->MAX_FACTORIES_PER_TYPE)
			{
				ai->bt->units_dynamic[def_id].requested += 1;

				if(ai->execute->urgency[STATIONARY_CONSTRUCTOR] < 1.5f)
					ai->execute->urgency[STATIONARY_CONSTRUCTOR] = 1.5f;

				for(list<int>::iterator j = bt->units_static[def_id].canBuildList.begin(); j != bt->units_static[def_id].canBuildList.end(); ++j)
					bt->units_dynamic[*j].constructorsRequested += 1;
			}
		}

		// check if support needed
		if(assistants.size() < cfg->MAX_ASSISTANTS)
		{
			bool assist = false;

			if(buildque->size() > 2)
				assist = true;
			else if(construction_def_id && (bt->unitList[construction_def_id-1]->buildTime/(30.0f * bt->unitList[def_id-1]->buildSpeed) > cfg->MIN_ASSISTANCE_BUILDTIME))
				assist = true;

			if(assist)
			{
				AAIConstructor* assistant = ai->ut->FindClosestAssistant(ai->cb->GetUnitPos(unit_id), 5, true);

				if(assistant)
				{
					assistants.insert(assistant->unit_id);
					assistant->AssistConstruction(unit_id);
				}
			}
		}
		// check if assistants are needed anymore
		else if(!assistants.empty() && buildque->empty() && !construction_def_id)
		{
			//cb->SendTextMsg("factory releasing assistants",0);
			ReleaseAllAssistants();
		}

	}

	if(builder && build_task)
	{
		// prevent assisting when low on ressources
		if(ai->execute->averageMetalSurplus < 0.1)
		{
			if(construction_category == METAL_MAKER)
			{
				if(ai->execute->averageEnergySurplus < 0.5 * ai->bt->unitList[construction_def_id-1]->energyUpkeep)
					return;
			}
			else if(construction_category != EXTRACTOR && construction_category != POWER_PLANT)
				return;
		}

		float buildtime = ai->bt->unitList[construction_def_id-1]->buildTime / ai->bt->unitList[def_id-1]->buildSpeed;

		if(buildtime > cfg->MIN_ASSISTANCE_BUILDTIME && assistants.size() < cfg->MAX_ASSISTANTS)
		{
			// com only allowed if buildpos is inside the base
			bool commander = false;

			int x = build_pos.x / ai->map->xSectorSize;
			int y = build_pos.z / ai->map->ySectorSize;

			if(x >= 0 && y >= 0 && x < ai->map->xSectors && y < ai->map->ySectors)
			{
				if(ai->map->sector[x][y].distance_to_base == 0)
					commander = true;
			}

			AAIConstructor* assistant = ai->ut->FindClosestAssistant(build_pos, 5, commander);

			if(assistant)
			{
				assistants.insert(assistant->unit_id);
				assistant->AssistConstruction(unit_id, construction_unit_id);
			}
		}
	}
}


double AAIConstructor::GetMyQueBuildtime()
{
	double buildtime = 0;

	for(list<int>::iterator unit = buildque->begin(); unit != buildque->end(); ++unit)
		buildtime += bt->unitList[(*unit)-1]->buildTime;

	return buildtime;
}

void AAIConstructor::GiveReclaimOrder(int unit_id)
{
	if(assistance >= 0)
	{
		ai->ut->units[assistance].cons->RemoveAssitant(unit_id);

		assistance = -1;
	}

	task = RECLAIMING;

	Command c;
	c.id = CMD_RECLAIM;
	c.params.push_back(unit_id);
	//cb->GiveOrder(this->unit_id, &c);
	ai->execute->GiveOrder(&c, this->unit_id, "Builder::GiveRelaimOrder");
}


void AAIConstructor::GiveConstructionOrder(int id_building, float3 pos, bool water)
{
	// get def and final position
	const UnitDef *def = ai->bt->unitList[id_building-1];

	// give order if building can be placed at the desired position (position lies within a valid sector)
	if(ai->execute->InitBuildingAt(def, &pos, water))
	{
		order_tick = cb->GetCurrentFrame();

		// check if builder was previously assisting other builders/factories
		if(assistance >= 0)
		{
			ai->ut->units[assistance].cons->RemoveAssitant(unit_id);
			assistance = -1;
		}

		// set building as current task and order construction
		build_pos = pos;
		construction_def_id = id_building;
		task = BUILDING;
		construction_category = bt->units_static[id_building].category;

		// order builder to construct building
		Command c;
		c.id = - id_building;
		c.params.resize(3);
		c.params[0] = build_pos.x;
		c.params[1] = build_pos.y;
		c.params[2] = build_pos.z;

		cb->GiveOrder(unit_id, &c);

		// increase number of active units of that type/category
		bt->units_dynamic[def->id].requested += 1;

		ai->ut->UnitRequested(construction_category);

		if(bt->IsFactory(id_building))
			ai->ut->futureFactories += 1;
	}
}

void AAIConstructor::AssistConstruction(int constructor, int target_unit)
{
	if(target_unit == -1)
	{
		Command c;
		// Check if the target can be assisted at all. If not, try to repair it instead
		const UnitDef *def;
		def = cb->GetUnitDef(constructor);
		if(def->canBeAssisted)
		{
			c.id = CMD_GUARD;
		} else
		{
			c.id = CMD_REPAIR;
		}
		c.params.push_back(constructor);
		//cb->GiveOrder(unit_id, &c);
		ai->execute->GiveOrder(&c, unit_id, "Builder::Assist");

		task = ASSISTING;
		assistance = constructor;
	}
	else
	{
		Command c;
		c.id = CMD_REPAIR;
		c.params.push_back(target_unit);
		//cb->GiveOrder(unit_id, &c);
		ai->execute->GiveOrder(&c, unit_id, "Builder::Assist");

		task = ASSISTING;
		assistance = constructor;
	}
}

void AAIConstructor::TakeOverConstruction(AAIBuildTask *build_task)
{
	if(assistance >= 0)
	{
		ai->ut->units[assistance].cons->RemoveAssitant(unit_id);
		assistance = -1;
	}

	order_tick = build_task->order_tick;

	construction_unit_id = build_task->unit_id;
	construction_def_id = build_task->def_id;
	construction_category = ai->bt->units_static[construction_def_id].category;
	build_pos = build_task->build_pos;

	Command c;
	c.id = CMD_REPAIR;
	c.params.push_back(build_task->unit_id);

	task = BUILDING;
	cb->GiveOrder(unit_id, &c);
}

void AAIConstructor::ConstructionFailed()
{
	--bt->units_dynamic[construction_def_id].requested;
	ai->ut->UnitRequestFailed(construction_category);

	// clear up buildmap etc.
	if(bt->units_static[construction_def_id].category <= METAL_MAKER)
		ai->execute->ConstructionFailed(build_pos, construction_def_id);

	// tells the builder construction has finished
	ConstructionFinished();
}

void AAIConstructor::ConstructionFinished()
{
	task = UNIT_IDLE;

	build_pos = ZeroVector;
	construction_def_id = 0;
	construction_unit_id = -1;
	construction_category = UNKNOWN;

	build_task = 0;

	// release assisters
	ReleaseAllAssistants();
}

void AAIConstructor::ReleaseAllAssistants()
{
	// release assisters
	for(set<int>::iterator i = assistants.begin(); i != assistants.end(); ++i)
	{
		 if(ai->ut->units[*i].cons)
			 ai->ut->units[*i].cons->StopAssisting();
	}

	assistants.clear();
}

void AAIConstructor::StopAssisting()
{
	task = UNIT_IDLE;
	assistance = -1;

	Command c;
	c.id = CMD_STOP;
	//cb->GiveOrder(unit_id, &c);
	ai->execute->GiveOrder(&c, unit_id, "Builder::StopAssisting");
}
void AAIConstructor::RemoveAssitant(int unit_id)
{
	assistants.erase(unit_id);
}
void AAIConstructor::Killed()
{
	if(builder)
	{
		// when builder was killed on the way to the buildsite, inform ai that construction
		// of building hasnt been started
		if(task == BUILDING)
		{
			//if buildling has not begun yet, decrease some values
			if(construction_unit_id == -1)
			{
				// killed on the way to the buildsite
				ai->map->UnitKilledAt(&build_pos, MOBILE_CONSTRUCTOR);

				// clear up buildmap etc.
				ConstructionFailed();
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
			ai->ut->units[assistance].cons->RemoveAssitant(unit_id);
		}
	}

	ReleaseAllAssistants();
	task = UNIT_KILLED;
}

void AAIConstructor::Retreat(UnitCategory attacked_by)
{
	if(task != UNIT_KILLED)
	{
		float3 pos = cb->GetUnitPos(unit_id);

		int x = pos.x / ai->map->xSectorSize;
		int y = pos.z / ai->map->ySectorSize;

		// attacked by scout
		if(attacked_by == SCOUT)
		{
			// dont flee from scouts in your own base
			if(x >= 0 && y >= 0 && x < ai->map->xSectors && y < ai->map->ySectors)
			{
				// builder is within base
				if(ai->map->sector[x][y].distance_to_base == 0)
					return;
				// dont flee outside of the base if health is > 50%
				else
				{
					if(cb->GetUnitHealth(unit_id) > ai->bt->unitList[def_id-1]->health / 2.0)
						return;
				}
			}
		}
		else
		{
			if(x >= 0 && y >= 0 && x < ai->map->xSectors && y < ai->map->ySectors)
			{
				// builder is within base
				if(ai->map->sector[x][y].distance_to_base == 0)
					return;
			}
		}


		// get safe position
		pos = ai->execute->GetSafePos(def_id, pos);

		if(pos.x > 0)
		{
			Command c;
			c.id = CMD_MOVE;
			c.params.push_back(pos.x);
			c.params.push_back(cb->GetElevation(pos.x, pos.z));
			c.params.push_back(pos.z);

			ai->execute->GiveOrder(&c, unit_id, "BuilderRetreat");
			//cb->GiveOrder(unit_id, &c);
		}
	}
}
