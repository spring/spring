// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#include <set>

#include "AAI.h"
#include "AAIConstructor.h"
#include "AAIBuildTask.h"
#include "AAIExecute.h"
#include "AAIBuildTable.h"
#include "AAIUnitTable.h"
#include "AAIConfig.h"
#include "AAIMap.h"
#include "AAISector.h"

#include "LegacyCpp/UnitDef.h"
#include "LegacyCpp/CommandQueue.h"
using namespace springLegacyAI;


AAIConstructor::AAIConstructor(AAI *ai, int unit_id, int def_id, bool factory, bool builder, bool assistant)
{
	this->ai = ai;

	this->unit_id = unit_id;
	this->def_id = def_id;
	buildspeed = ai->Getbt()->GetUnitDef(def_id).buildSpeed;

	construction_unit_id = -1;
	construction_def_id = -1;
	construction_category = UNKNOWN;

	assistance = -1;
	build_task = 0;
	order_tick = 0;

	task = UNIT_IDLE;

	build_pos = ZeroVector;

	this->factory = factory;
	this->builder = builder;
	this->assistant = assistant;

	buildque = ai->Getexecute()->GetBuildqueueOfFactory(def_id);
}

AAIConstructor::~AAIConstructor(void)
{
}

// returns true if factory is busy
bool AAIConstructor::IsBusy()
{
	const CCommandQueue *commands = ai->Getcb()->GetCurrentUnitCommands(unit_id);

	if(commands->empty())
		return false;
	else
		return true;
}

void AAIConstructor::Idle()
{
	//ai->LogConsole("%s is idle", ai->Getbt()->GetUnitDef(def_id-1).humanName.c_str());

	if(builder)
	{
		if(task == BUILDING)
		{
			if(!ai->Getbt()->IsValidUnitDefID(construction_unit_id))
			{
				//ai->Getbt()->units_dynamic[construction_def_id].active -= 1;
				//assert(ai->Getbt()->units_dynamic[construction_def_id].active >= 0);
				ai->Getut()->UnitRequestFailed(construction_category);

				// clear up buildmap etc. (make sure conctructor wanted to build a building and not a unit)
				if(ai->Getbt()->units_static[construction_def_id].category <= METAL_MAKER)
					ai->Getexecute()->ConstructionFailed(build_pos, construction_def_id);

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
	if(factory && buildque != nullptr)
	{
		if(!ai->Getbt()->IsValidUnitDefID(construction_def_id) && !buildque->empty())
		{
			int def_id = (*buildque->begin());
			UnitCategory cat = ai->Getbt()->units_static[def_id].category;

			if(ai->Getbt()->IsBuilder(def_id) || cat == SCOUT || ai->Getcb()->GetMetal() > 50
				|| ai->Getbt()->units_static[def_id].cost < ai->Getbt()->avg_cost[ai->Getbt()->units_static[def_id].category][ai->Getside()-1])
			{
				// check if mobile or stationary builder
				if(ai->Getbt()->IsStatic(this->def_id))
				{
					// give build order
					Command c;
					c.id = -def_id;
					ai->Getcb()->GiveOrder(unit_id, &c);
					construction_def_id = def_id;
					assert(ai->Getbt()->IsValidUnitDefID(def_id));
					task = BUILDING;

					//if(ai->Getbt()->IsFactory(def_id))
					//	++ai->futureFactories;

					buildque->pop_front();
				}
				else
				{
					// find buildpos for the unit
					float3 pos = ai->Getexecute()->GetUnitBuildsite(unit_id, def_id);

					if(pos.x > 0)
					{
						// give build order
						Command c;
						c.id = -def_id;
						c.params.resize(3);
						c.params[0] = pos.x;
						c.params[1] = pos.y;
						c.params[2] = pos.z;

						ai->Getcb()->GiveOrder(unit_id, &c);
						construction_def_id = def_id;
						assert(ai->Getbt()->IsValidUnitDefID(def_id));
						task = BUILDING;

						++ai->Getut()->futureUnits[cat];

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
				if(!IsBusy() && !ai->Getbt()->IsValidUnitDefID(construction_unit_id))
					ConstructionFailed();
			}
		}
		/*else if(task == UNIT_IDLE)
		{
			float3 pos = ai->Getcb()->GetUnitPos(unit_id);

			if(pos.x > 0)
			{
				int x = pos.x/ai->Getmap()->xSectorSize;
				int y = pos.z/ai->Getmap()->ySectorSize;

				if(x >= 0 && y >= 0 && x < ai->Getmap()->xSectors && y < ai->Getmap()->ySectors)
				{
					if(ai->Getmap()->sector[x][y].distance_to_base < 2)
					{
						pos = ai->Getmap()->sector[x][y].GetCenter();

						Command c;
						const UnitDef *def;

						def = ai->Getcb()->GetUnitDef(unit_id);
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
						c.params[1] = ai->Getcb()->GetElevation(pos.x, pos.z);
						c.params[2] = pos.z;
						c.params[3] = 500.0;

						//ai->Getcb()->GiveOrder(unit_id, &c);
						task = RECLAIMING;
						ai->Getexecute()->GiveOrder(&c, unit_id, "Builder::Reclaming");
					}
				}
			}
		}
		*/
	}
}

void AAIConstructor::CheckAssistance()
{
	if(factory && (buildque != nullptr))
	{
		// check if another factory of that type needed
		if(buildque->size() >= cfg->MAX_BUILDQUE_SIZE - 2 && assistants.size() >= cfg->MAX_ASSISTANTS-2)
		{
			if(ai->Getbt()->units_dynamic[def_id].active + ai->Getbt()->units_dynamic[def_id].requested + ai->Getbt()->units_dynamic[def_id].under_construction  < cfg->MAX_FACTORIES_PER_TYPE)
			{
				ai->Getbt()->units_dynamic[def_id].requested += 1;

				if(ai->Getexecute()->urgency[STATIONARY_CONSTRUCTOR] < 1.5f)
					ai->Getexecute()->urgency[STATIONARY_CONSTRUCTOR] = 1.5f;

				for(list<int>::iterator j = ai->Getbt()->units_static[def_id].canBuildList.begin(); j != ai->Getbt()->units_static[def_id].canBuildList.end(); ++j)
					ai->Getbt()->units_dynamic[*j].constructorsRequested += 1;
			}
		}

		// check if support needed
		if(assistants.size() < cfg->MAX_ASSISTANTS)
		{
			bool assist = false;


			if(buildque->size() > 2)
				assist = true;
			else if(ai->Getbt()->IsValidUnitDefID(construction_def_id)) {
				float buildtime = 1e6;
				if (buildspeed > 0) {
					//FIXME why use *1/30 here? below there is exactly the same code w/o it, so what's the correct one?
					buildtime = ai->Getbt()->GetUnitDef(construction_def_id).buildTime / (30.0f * buildspeed);
				}

				if (buildtime > cfg->MIN_ASSISTANCE_BUILDTIME)
					assist = true;
			}

			if(assist)
			{
				AAIConstructor* assistant = ai->Getut()->FindClosestAssistant(ai->Getcb()->GetUnitPos(unit_id), 5, true);

				if(assistant)
				{
					assistants.insert(assistant->unit_id);
					assistant->AssistConstruction(unit_id);
				}
			}
		}
		// check if assistants are needed anymore
		else if(!assistants.empty() && buildque->empty() && !ai->Getbt()->IsValidUnitDefID(construction_def_id))
		{
			//ai->LogConsole("factory releasing assistants");
			ReleaseAllAssistants();
		}

	}

	if(builder && build_task)
	{
		// prevent assisting when low on ressources
		if(ai->Getexecute()->averageMetalSurplus < 0.1)
		{
			if(construction_category == METAL_MAKER)
			{
				if(ai->Getexecute()->averageEnergySurplus < 0.5 * ai->Getbt()->GetUnitDef(construction_def_id).energyUpkeep)
					return;
			}
			else if(construction_category != EXTRACTOR && construction_category != POWER_PLANT)
				return;
		}

		float buildtime = 1e6;
		if (buildspeed > 0) {
			buildtime = ai->Getbt()->GetUnitDef(construction_def_id).buildTime / buildspeed;
		}

		if((buildtime > cfg->MIN_ASSISTANCE_BUILDTIME) && (assistants.size() < cfg->MAX_ASSISTANTS))
		{
			// com only allowed if buildpos is inside the base
			bool commander = false;

			int x = build_pos.x / ai->Getmap()->xSectorSize;
			int y = build_pos.z / ai->Getmap()->ySectorSize;

			if(x >= 0 && y >= 0 && x < ai->Getmap()->xSectors && y < ai->Getmap()->ySectors)
			{
				if(ai->Getmap()->sector[x][y].distance_to_base == 0)
					commander = true;
			}

			AAIConstructor* assistant = ai->Getut()->FindClosestAssistant(build_pos, 5, commander);

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
	if (buildque == nullptr)
		return 0;

	double buildtime = 0;
	for(list<int>::iterator unit = buildque->begin(); unit != buildque->end(); ++unit)
		buildtime += ai->Getbt()->GetUnitDef((*unit)).buildTime;

	return buildtime;
}

void AAIConstructor::GiveReclaimOrder(int unit_id)
{
	if(assistance >= 0)
	{
		ai->Getut()->units[assistance].cons->RemoveAssitant(unit_id);

		assistance = -1;
	}

	task = RECLAIMING;

	Command c;
	c.id = CMD_RECLAIM;
	c.params.push_back(unit_id);
	//ai->Getcb()->GiveOrder(this->unit_id, &c);
	ai->Getexecute()->GiveOrder(&c, this->unit_id, "Builder::GiveRelaimOrder");
}


void AAIConstructor::GiveConstructionOrder(int id_building, float3 pos, bool water)
{
	// get def and final position
	const UnitDef *def = &ai->Getbt()->GetUnitDef(id_building);

	// give order if building can be placed at the desired position (position lies within a valid sector)
	if(ai->Getexecute()->InitBuildingAt(def, &pos, water))
	{
		order_tick = ai->Getcb()->GetCurrentFrame();

		// check if builder was previously assisting other builders/factories
		if(assistance >= 0)
		{
			ai->Getut()->units[assistance].cons->RemoveAssitant(unit_id);
			assistance = -1;
		}

		// set building as current task and order construction
		build_pos = pos;
		construction_def_id = id_building;
		assert(ai->Getbt()->IsValidUnitDefID(id_building));
		task = BUILDING;
		construction_category = ai->Getbt()->units_static[id_building].category;

		// order builder to construct building
		Command c;
		c.id = - id_building;
		c.params.resize(3);
		c.params[0] = build_pos.x;
		c.params[1] = build_pos.y;
		c.params[2] = build_pos.z;

		ai->Getcb()->GiveOrder(unit_id, &c);

		// increase number of active units of that type/category
		ai->Getbt()->units_dynamic[def->id].requested += 1;

		ai->Getut()->UnitRequested(construction_category);

		if(ai->Getbt()->IsFactory(id_building))
			ai->Getut()->futureFactories += 1;
	}
}

void AAIConstructor::AssistConstruction(int constructor, int target_unit)
{
	if(target_unit == -1)
	{
		Command c;
		// Check if the target can be assisted at all. If not, try to repair it instead
		const UnitDef *def;
		def = ai->Getcb()->GetUnitDef(constructor);
		if(def->canBeAssisted)
		{
			c.id = CMD_GUARD;
		} else
		{
			c.id = CMD_REPAIR;
		}
		c.params.push_back(constructor);
		//ai->Getcb()->GiveOrder(unit_id, &c);
		ai->Getexecute()->GiveOrder(&c, unit_id, "Builder::Assist");

		task = ASSISTING;
		assistance = constructor;
	}
	else
	{
		Command c;
		c.id = CMD_REPAIR;
		c.params.push_back(target_unit);
		//ai->Getcb()->GiveOrder(unit_id, &c);
		ai->Getexecute()->GiveOrder(&c, unit_id, "Builder::Assist");

		task = ASSISTING;
		assistance = constructor;
	}
}

void AAIConstructor::TakeOverConstruction(AAIBuildTask *build_task)
{
	if(assistance >= 0)
	{
		ai->Getut()->units[assistance].cons->RemoveAssitant(unit_id);
		assistance = -1;
	}

	order_tick = build_task->order_tick;

	construction_unit_id = build_task->unit_id;
	construction_def_id = build_task->def_id;
	assert(ai->Getbt()->IsValidUnitDefID(construction_def_id));
	construction_category = ai->Getbt()->units_static[construction_def_id].category;
	build_pos = build_task->build_pos;

	Command c;
	c.id = CMD_REPAIR;
	c.params.push_back(build_task->unit_id);

	task = BUILDING;
	ai->Getcb()->GiveOrder(unit_id, &c);
}

void AAIConstructor::ConstructionFailed()
{
	--ai->Getbt()->units_dynamic[construction_def_id].requested;
	ai->Getut()->UnitRequestFailed(construction_category);

	// clear up buildmap etc.
	if(ai->Getbt()->units_static[construction_def_id].category <= METAL_MAKER)
		ai->Getexecute()->ConstructionFailed(build_pos, construction_def_id);

	// tells the builder construction has finished
	ConstructionFinished();
}

void AAIConstructor::ConstructionFinished()
{
	task = UNIT_IDLE;

	build_pos = ZeroVector;
	construction_def_id = -1;
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
		 if(ai->Getut()->units[*i].cons)
			 ai->Getut()->units[*i].cons->StopAssisting();
	}

	assistants.clear();
}

void AAIConstructor::StopAssisting()
{
	task = UNIT_IDLE;
	assistance = -1;

	Command c;
	c.id = CMD_STOP;
	//ai->Getcb()->GiveOrder(unit_id, &c);
	ai->Getexecute()->GiveOrder(&c, unit_id, "Builder::StopAssisting");
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
				ai->Getmap()->UnitKilledAt(&build_pos, MOBILE_CONSTRUCTOR);

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
			ai->Getut()->units[assistance].cons->RemoveAssitant(unit_id);
		}
	}

	ReleaseAllAssistants();
	task = UNIT_KILLED;
}

void AAIConstructor::Retreat(UnitCategory attacked_by)
{
	if(task != UNIT_KILLED)
	{
		float3 pos = ai->Getcb()->GetUnitPos(unit_id);

		int x = pos.x / ai->Getmap()->xSectorSize;
		int y = pos.z / ai->Getmap()->ySectorSize;

		// attacked by scout
		if(attacked_by == SCOUT)
		{
			// dont flee from scouts in your own base
			if(x >= 0 && y >= 0 && x < ai->Getmap()->xSectors && y < ai->Getmap()->ySectors)
			{
				// builder is within base
				if(ai->Getmap()->sector[x][y].distance_to_base == 0)
					return;
				// dont flee outside of the base if health is > 50%
				else
				{
					if(ai->Getcb()->GetUnitHealth(unit_id) > ai->Getbt()->GetUnitDef(def_id).health / 2.0)
						return;
				}
			}
		}
		else
		{
			if(x >= 0 && y >= 0 && x < ai->Getmap()->xSectors && y < ai->Getmap()->ySectors)
			{
				// builder is within base
				if(ai->Getmap()->sector[x][y].distance_to_base == 0)
					return;
			}
		}


		// get safe position
		pos = ai->Getexecute()->GetSafePos(def_id, pos);

		if(pos.x > 0)
		{
			Command c;
			c.id = CMD_MOVE;
			c.params.push_back(pos.x);
			c.params.push_back(ai->Getcb()->GetElevation(pos.x, pos.z));
			c.params.push_back(pos.z);

			ai->Getexecute()->GiveOrder(&c, unit_id, "BuilderRetreat");
			//ai->Getcb()->GiveOrder(unit_id, &c);
		}
	}
}
