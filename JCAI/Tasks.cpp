#include "BaseAIDef.h"
#include "BaseAIObjects.h"
#include "AI_Config.h"
#include "TaskManager.h"
#include "BuildTable.h"
#include "InfoMap.h"
#include "BuildMap.h"
#include "MetalSpotMap.h"

// -----------------------------------------
// TaskFactory
// -----------------------------------------

void TaskFactory::AddTask (aiTask *t)
{
	AddDeathDependence (t);
	activeTasks.push_back (t);
}

void TaskFactory::DependentDied (aiObject *obj)
{
	vector<aiTask*>::iterator i = remove_if (activeTasks.begin(),activeTasks.end(), bind(equal_to<aiObject*>(),_1,obj));
	assert(i != activeTasks.end());
	activeTasks.erase (i,activeTasks.end());
}


// -----------------------------------------
// aiTask
// -----------------------------------------

void aiTask::DependentDied (aiObject *obj)
{
	if (obj == depends)
		depends = 0;
	else {
		vector<BuildUnit*>::iterator i = remove_if (constructors.begin(),constructors.end(), bind(equal_to<aiObject*>(), _1, obj));
		assert (i != constructors.end());
		constructors.erase (i, constructors.end());
	}
}

void aiTask::GiveConstructorCommand (IAICallback *cb, Command *cmd)
{
	for (int a=0;a<constructors.size();a++) {
		if (constructors[a]->activeTask==this) 
			cb->GiveOrder(constructors[a]->id, cmd);
	}
}


// -----------------------------------------
// ReclaimUnitTask
// -----------------------------------------

void ReclaimUnitTask::SetReclaimTarget (aiUnit *u)
{
	if (target)
		DeleteDeathDependence (target);

	target = u;
	if (u) AddDeathDependence (u);
}

void ReclaimUnitTask::DependentDied (aiObject *obj)
{
	if (obj == target)
		target = 0;

	aiTask::DependentDied (obj);
}

bool ReclaimUnitTask::Update(CGlobals *g)
{
	// destroy the task when the unit is destroyed
	return target != 0;
}


bool ReclaimUnitTask::CalculateScore (BuildUnit *unit, CGlobals *g, float &score)
{
	return false;
}


void ReclaimUnitTask::CommandBuilder (BuildUnit *builder, CGlobals *g)
{
	if (target) {
		logPrintf ("Reclaiming unit %s\n", target->def->name.c_str());

		Command cmd;
		cmd.id = CMD_RECLAIM;
		cmd.params.push_back (target->id);

		g->cb->GiveOrder (builder->id, &cmd);
	}
}


string ReclaimUnitTask::GetDebugName (CGlobals *g)
{
	char tmp[60];
	SNPRINTF (tmp, 60, "reclaim unit %d (%s)", target ? target->id : 0, target ? target->def->name.c_str() : "no target set");
	return tmp;
}



// -----------------------------------------
// RepairTask
// -----------------------------------------


void RepairTask::SetRepairTarget (aiUnit *u)
{
	if (target) DeleteDeathDependence (target);

	target = u;
	if (u) AddDeathDependence (u);
}


bool RepairTask::Update(CGlobals *g)
{
	if (target && !constructors.empty()) {
		// see if it's done with repairing
		return ! (g->cb->GetUnitHealth (target->id) >= g->cb->GetUnitMaxHealth (target->id));
	} 
	return false;
}

void RepairTask::DependentDied (aiObject *obj)
{
	if (obj == target)
		target = 0;

	aiTask::DependentDied (obj);
}

void RepairTask::CommandBuilder (BuildUnit *u, CGlobals *g)
{
	Command cmd;
	cmd.id = CMD_REPAIR;
	cmd.params.push_back (target->id);

	g->cb->GiveOrder(u->id,&cmd);
}

bool RepairTask::CalculateScore (BuildUnit *bu, CGlobals *g, float& score)
{
	return false;
}

string RepairTask::GetDebugName (CGlobals *g)
{
	char tmp[60];
	SNPRINTF (tmp, 60, "repair unit %d (%s)", target ? target->id : 0, target ? target->def->name.c_str() : "no target set");
	return tmp;
}


// -----------------------------------------
// ReclaimFeaturesTask
// -----------------------------------------

bool ReclaimFeaturesTask::Update (CGlobals *g)
{
	return g->cb->GetFeatures (idBuffer, IDBUF_SIZE, pos, range);
}

bool ReclaimFeaturesTask::CalculateScore (BuildUnit *bu, CGlobals *g, float& score)
{
	return false;
}

void ReclaimFeaturesTask::CommandBuilder (BuildUnit *builder, CGlobals *g)
{
	Command cmd;
	cmd.id = CMD_RECLAIM;

	cmd.params.resize(4);
	cmd.params[0]=pos.x;
	cmd.params[1]=pos.y;
	cmd.params[2]=pos.z;
	cmd.params[3]=range;;

	g->cb->GiveOrder(builder->id, &cmd);
}

string ReclaimFeaturesTask::GetDebugName (CGlobals *g)
{
	char tmp[60];
	SNPRINTF (tmp, 60, "reclaim at (%d,%d) with range %f", (int)pos.x, (int)pos.y, range);
	return tmp;
}

// -----------------------------------------
// BuildTask
// -----------------------------------------

BuildTask::BuildTask(const UnitDef* unitDef)
{
	def = unitDef;
	isAllocated=false;
	lead=0;
	spotID=-1;
	unit=0;
	resourceType=-1;
	destHandler=0;
	pos.x=-1.0f;
}

void BuildTask::Initialize (CGlobals *g)
{
	if (resourceType>=0) {
		TaskFactory *tf = g->taskFactories [resourceType];
		tf->AddTask (this);
		isAllocated = false;
	} else {
		g->resourceManager->TakeResources (g->resourceManager->ResourceValue (def->energyCost,def->metalCost));
		isAllocated=true;
	}

	BuildTable::UDef *cd = buildTable.GetCachedDef (def->id);
	if (cd->IsBuilding ())
	{
		flags |= BT_BUILDING;
		
		// Find a build position
		SetupBuildLocation (g);
	}

	if (cd->IsBuilder ())
		MarkBuilder ();

	g->taskManager->currentUnitBuilds[def->id-1] ++;
}

// default version finds a build position in a safe sector that is not filled.
bool BuildTask::InitBuildPos (CGlobals *g)
{
	BuildTable::UDef* cd = buildTable.GetCachedDef (def->id);

	// Find a sector to build in
	float3 st (g->map->baseCenter.x, 0.0f, g->map->baseCenter.y);
	int2 sector = g->map->FindSafeBuildingSector (st, aiConfig.safeSectorRadius, cd->IsBuilder () ? BLD_FACTORY : 1);

	if (sector.x < 0)
		return false;

	// find a position for the unit
	float3 tpos = float3(sector.x+0.5f,0.0f,sector.y+0.5f)*g->map->mblocksize;
	if(!g->buildmap->FindBuildPosition (def, tpos, pos))
		return false;

	return true;
}

int BuildTask::GetBuildSpace(CGlobals *g)
{
	return def->buildOptions.empty() ? 1 : BLD_FACTORY;
}


bool BuildTask::Update (CGlobals *g)
{
	if (unit && unit->flags & UNIT_FINISHED)
	{
		g->taskManager->FinishConstructedUnit(this);

		// make sure the assisting builders stop guarding the lead builder
		Command c;
		c.id=CMD_STOP;
		GiveConstructorCommand (g->cb,&c);

		return false; // task can be deleted
	}

	// unit will be build once the dependency task is finished, and there is a builder that can build this task
	if (depends || !constructors.empty() || g->taskManager->CanBuildUnit (def))
		return true;

	// There is no builder that can start the task
	const UnitDef* bestBuilder = g->taskManager->FindBestBuilderType (def, 0);
	if (bestBuilder)
	{ // no active builder either, a new builder has to be made
		// see if this order for the new builder is already in the tasks,
		// in that case there is no need to build another one.
		if (g->taskManager->currentUnitBuilds[bestBuilder->id-1])
			return true;

		BuildTask *nb = new BuildTask (bestBuilder);
		nb->resourceType = resourceType;
		depends = nb;
		AddDeathDependence (nb);

		g->taskManager->AddTask (nb);
	}
	return true;
}

void BuildTask::DependentDied (aiObject *obj)
{
	if (obj == unit) {
		unit = 0;
		return;
	}

	if (obj == lead)
		lead = 0;
	aiTask::DependentDied (obj);
}



void BuildTask::InitializeLeadBuilder (CGlobals *g)
{
	BuildUnit *leadc = 0;// lead builder candidate

	assert (!constructors.empty());
	assert (!lead);

	// Find a new lead builder
	for (int a=0;a<constructors.size();a++)
	{
		BuildUnit *b = constructors [a];
		if (constructors[a]->tasks.front () != this)
			continue;

		if (buildTable.UnitCanBuild (b->def, def))
		{
			leadc = b;
			break;
		}
	}

	if (!leadc) // No candidate for lead builder?
	{
		for (int a=0;a<constructors.size();a++)
		{
			BuildUnit *u = constructors[a];
			DeleteDeathDependence (u);

			u->DependentDied (this);
			u->DeleteDeathDependence (this);
		}
		constructors.clear();
		buildSpeed = 0.0f;
		return;
	}

	float3 builderPos = g->cb->GetUnitPos (leadc->id);

	Command c;
	BuildTable::UDef* cd = buildTable.GetCachedDef (def->id);
	if (cd->IsBuilding())
	{
		if (def->extractsMetal && spotID>=0)
		{
			MetalSpot *spot = g->metalmap->GetSpot (spotID);
			if (spot->unit)
			{
				assert (spot->unit->def->extractsMetal < def->extractsMetal);

				// create a reclaim task and make this depends on the reclaim task
				ReclaimUnitTask *rt = new ReclaimUnitTask;
				g->taskManager->AddTask (rt);

				AddDeathDependence (rt);
				depends = rt;

				rt->SetReclaimTarget (spot->unit);
				return;
			}
		}

		if (pos.x < 0.0f) {
			if (!SetupBuildLocation (g))
				return;
		}

		c.params.resize(3);
		c.params[0]=pos.x;
		c.params[1]=pos.y;
		c.params[2]=pos.z;

		if (aiConfig.debug)
			g->cb->DrawUnit (def->name.c_str(), pos, 0.0f, 800, g->cb->GetMyTeam(), true, true);
	} else
		pos = builderPos;

	c.id = -def->id;
	g->cb->GiveOrder (leadc->id, &c);
	lead = leadc;
}

void BuildTask::CommandBuilder (BuildUnit *builder, CGlobals *g)
{
	if (unit) {
		// make the builder repair the unfinished unit
		Command c;
		c.id = CMD_REPAIR;
		c.params.push_back (unit->id);

		g->cb->GiveOrder (builder->id, &c);
	}
	else
	{
		if (!lead)
			InitializeLeadBuilder (g);

		if (lead && builder != lead)
		{
			Command c;
			c.id = CMD_GUARD;
			c.params.push_back (lead->id);
			g->cb->GiveOrder(builder->id, &c);
		}
	}
}

bool BuildTask::SetupBuildLocation (CGlobals *g)
{
	if (!InitBuildPos (g))
		return false;

	// Mark the unit on the buildmap
	if (pos.x>=0.0f)
		g->buildmap->Mark (def, pos, true);

	MapInfo *mi = g->map->GetMapInfo (pos);
	mi->buildstate += GetBuildSpace(g);

	return true;
}


void BuildTask::BuilderDestroyed (BuildUnit *builder, CGlobals *g)
{
	if (builder == lead && !unit) {
		// Mark the unit on the buildmap
		assert (pos.x>=0.0f);
		g->buildmap->Mark (def, pos, false);

		MapInfo *mi = g->map->GetMapInfo (pos);
		mi->buildstate -= GetBuildSpace(g);
	}
}

bool BuildTask::CalculateScore (BuildUnit *bu, CGlobals *g, float& score)
{
	if (depends || !isAllocated)
		return false;

	float3 builderPos = g->cb->GetUnitPos(bu->id);
	const UnitDef *builderDef = bu->def;

	float hp = unit ? g->cb->GetUnitHealth(unit->id) : 0.0f;
	float buildTimeLeft = def->buildTime*((def->health-hp)/def->health);

	//can we build this directly or is there someone who can start it for us
	bool canBuildThis=buildTable.UnitCanBuild (builderDef, def) ;
	if (canBuildThis || lead)
	{
		float buildTime=buildTimeLeft/(buildSpeed+builderDef->buildSpeed);
		float distance=0.0f;
		if (pos.x>=0) distance = (pos-builderPos).Length(); // position has been estimated for metal extractors
		else distance = 40 * SQUARE_SIZE; // assume a constant distance
		float moveTime=max(0.01f,(distance-150)/g->cb->GetUnitSpeed (bu->id)*30);
		float travelMod=buildTime/(buildTime+moveTime);			//units prefer stuff with low travel time compared to build time
		float finishMod=def->buildTime/(buildTimeLeft+def->buildTime*0.1f);			//units prefer stuff that is nearly finished
		float canBuildThisMod=canBuildThis?1.5f:1;								//units prefer to do stuff they have in their build options (less risk of guarded unit dying etc)
		float ageMod=2000+g->cb->GetCurrentFrame()-startFrame;
		float buildersMod=1.0f/max(int(constructors.size()),1);
		score = finishMod*canBuildThisMod*travelMod*ageMod*buildersMod;
		return true;
	}

	return false;
}

void BuildTask::BuilderMoveFailed (BuildUnit *builder, CGlobals *g)
{
	if (retries == MaxRetries && !unit) {
		// choose another position
		SetupBuildLocation (g);
	}
}

string BuildTask::GetDebugName(CGlobals *g)
{
	string r = string("build ") + def->name;
	if (resourceType >= 0)  {
		r += " TaskFactory:";
		r += g->taskFactories [resourceType]->GetName();
	}
	return r;
}

