#include "AAIFactory.h"
#include "AAI.h"
#include "AAIExecute.h"

AAIFactory::AAIFactory(AAI *ai, int unit_id, int def_id)
{
	this->ai = ai;
	this->bt = ai->bt;
	busy = false;

	this->def_id = def_id;
	this->unit_id = unit_id;

	building_def_id = 0;

	buildque = ai->execute->GetBuildqueOfFactory(def_id);

	my_pos = ai->cb->GetUnitPos(unit_id);
}

AAIFactory::~AAIFactory(void)
{
}

void AAIFactory::Update()
{
	if(!isBusy(unit_id))
	{
		if(!buildque->empty())
		{
			int def_id = (*buildque->begin());
			UnitCategory cat = ai->bt->units_static[def_id].category;

			if(ai->bt->IsBuilder(cat) || ai->bt->IsScout(cat))
			{
				//debug 
				/*fprintf(ai->file, "\nBuilding scout:\n");
				for(list<int>::iterator unit = buildque->begin(); unit != buildque->end(); ++unit)
				{
					fprintf(ai->file, "%s ", ai->bt->unitList[*unit-1]->humanName.c_str());
				}
				fprintf(ai->file, "\n");*/

				// give build order
				Command c;
				c.id = -def_id;
				ai->cb->GiveOrder(unit_id, &c);

				++ai->futureUnits[cat];
				--ai->bt->units_dynamic[def_id].requested;

				// remove unit from wishlist
				buildque->pop_front();

				//debug 
				/*for(list<int>::iterator unit = buildque->begin(); unit != buildque->end(); ++unit)
				{
					fprintf(ai->file, "%s ", ai->bt->unitList[*unit-1]->humanName.c_str());
				}
				fprintf(ai->file, "\n \n");*/
			}
			else if(ai->cb->GetMetal() > 50 || ai->bt->units_static[def_id].cost < ai->bt->avg_cost[ai->bt->units_static[def_id].category][ai->side-1])
			{
				// give build order
				Command c;
				c.id = -def_id;
				ai->cb->GiveOrder(unit_id, &c);

				++ai->futureUnits[cat];

				// remove unit from wishlist
				buildque->pop_front();
			}
		}
	}

	CheckAssistance();
}

// returns true if factory is busy
bool AAIFactory::isBusy(int factory)
{
	const deque<Command> *commands = ai->cb->GetCurrentUnitCommands(factory);

	if(commands->empty())
		return false;
	return true;
}

void AAIFactory::Killed()
{
	ReleaseAllAssistants();
}

void AAIFactory::ReleaseAllAssistants()
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

void AAIFactory::StopAllAssistants()
{
	AAIBuilder *builder;

	for(set<int>::iterator i = assistants.begin(); i != assistants.end(); ++i)
	{
		builder = ai->ut->units[*i].builder;

		if(builder)
			builder->StopAssistant();
	}
		
	assistants.clear();
}

void AAIFactory::RemoveAssitant(int unit_id)
{
	assistants.erase(unit_id);
}

void AAIFactory::CheckAssistance()
{
	// check if another factory of that type needed
	if(buildque->size() > cfg->MAX_BUILDQUE_SIZE-2 && assistants.size() >= cfg->MAX_ASSISTANTS)
	{
		if(ai->bt->units_dynamic[def_id].requested < cfg->MAX_FACTORIES_PER_TYPE)
		{
			++ai->bt->units_dynamic[def_id].requested;

			if(ai->execute->urgency[ai->bt->units_static[def_id].category] < 0.3)
				ai->execute->urgency[ai->bt->units_static[def_id].category] = 0.3;
		}
	}
	// check if support needed
	else if(assistants.size() < cfg->MAX_ASSISTANTS)
	{
		bool assist = false;

		if(buildque->size() > cfg->MAX_BUILDQUE_SIZE-2)
			assist = true;
		else if(building_def_id  && (bt->unitList[building_def_id]->buildTime/(30.0f * bt->unitList[def_id]->buildSpeed) > cfg->MIN_ASSISTANCE_BUILDTIME)) 
			assist = true;
		
		if(assist)
		{
			AAIBuilder* assistant; 

			// call idle builder
			if(bt->units_static[def_id].category == SEA_FACTORY)
			{
				if(ai->bt->unitList[def_id-1]->floater)
					assistant = ai->ut->FindAssistBuilder(my_pos, 5, true, true);
				else
					assistant = ai->ut->FindAssistBuilder(my_pos, 5, true, false);
			}
			else
				assistant = ai->ut->FindAssistBuilder(my_pos, 5, false, false);

			if(assistant)
			{
				assistants.insert(assistant->unit_id);
				assistant->AssistFactory(unit_id);
			}
		}
	}
	// check if assistants are needed anymore
	else if(!assistants.empty() && buildque->size() <  cfg->MAX_BUILDQUE_SIZE/3.0)
	{
		StopAllAssistants();
	}
}

double AAIFactory::GetMyQueBuildtime()
{
	double buildtime = 0;

	for(list<int>::iterator unit = buildque->begin(); unit != buildque->end(); ++unit)
		buildtime += bt->unitList[(*unit)-1]->buildTime;

	return buildtime;
}
