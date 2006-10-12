//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------

#include "AAIBuildTask.h"
#include "AAI.h"
#include "AAIBuilder.h"

AAIBuildTask::AAIBuildTask(AAI *ai, int unit_id, int def_id, float3 pos, int tick)
{
	this->ai = ai;
	this->unit_id = unit_id;
	this->def_id = def_id;

	order_tick = tick;

	builder_id = -1;

	build_pos = pos;
}

AAIBuildTask::~AAIBuildTask(void)
{
}

void AAIBuildTask::BuilderDestroyed()
{
	builder_id = -1;

	// look for new builder 
	AAIBuilder *new_builder;

	if(ai->bt->unitList[def_id-1]->minWaterDepth <= 0)
		new_builder = ai->ut->FindAssistBuilder(build_pos, 10, false, false);
	else
		new_builder = ai->ut->FindAssistBuilder(build_pos, 10, false, ai->bt->unitList[def_id-1]->floater);

	if(new_builder)
	{
		new_builder->TakeOverConstruction(this);
		builder_id = new_builder->unit_id;
	}
}

void AAIBuildTask::BuildingDestroyed()
{
	// cleanup buildmap etc.
	ai->execute->ConstructionFailed(unit_id, build_pos, def_id);
	
	// tell builder to stop construction (and release assisters) (if still alive)
	if(builder_id >= 0)
	{
		AAIBuilder *builder = ai->ut->units[builder_id].builder;

		if(builder)
			builder->BuildingFinished();
	}
}
