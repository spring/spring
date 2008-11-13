//-------------------------------------------------------------------------
// JCAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "BaseAIDef.h"
#include "BaseAIObjects.h"
#include "AI_Config.h"
#include "TaskManager.h"
#include "BuildTable.h"
#include "BuildMap.h"

#include "ResourceUnitHandler.h"

#include "DebugWindow.h"


float FeatureReclaimDist (const UnitDef* def) {
	BuildTable::UDef* cd=buildTable.GetCachedDef (def->id);
	if (cd->IsBuilding()) {
        if (cd->IsBuilder()) return SQUARE_SIZE * 12; // factories
		else return SQUARE_SIZE * (def->xsize+def->zsize)/4;
	} else return SQUARE_SIZE * 6;
}

static float UnitTypeBuildSpeed(const UnitDef* def) {
	float hp=def->health;
	if (def->isCommander) hp=400;
	return def->buildSpeed * (sqrt(hp)+100) * (sqrt(def->speed)+10) / 1800000;
}

/*
Todo list:

- calculate threatProximity for the gameinfo map
- make force groups split up when units are stuck
- support/defense building
- resource excess resulting in temporary production increase
- make force units defend while building the group
- support geothermals
*/

/*
buildmap:
	buildings are marked by FindTaskBuildPos,
	and unmarked in UnitDestroyed()
		- when the unit is destroyed
		- when it's lead builder was destroyed but the unit was not yet created
	and by CheckBuildError():
		- when the unit order can't be found in the command list
		- when the builder timed out and a new command is given
*/

using namespace std;
using namespace boost;

bool TaskManagerConfig::Load(CfgList *sidecfg)
{
	InitialOrders = dynamic_cast<CfgBuildOptions*>(sidecfg->GetValue("InitialBuildOrders"));
	if (InitialOrders) InitialOrders->InitIDs();

	BuildSpeedPerMetalIncome = sidecfg->GetNumeric ("BuildSpeedPerMetalIncome", 0.05);
	logPrintf ("BuildSpeedPerMetalIncome: %f\n", BuildSpeedPerMetalIncome);

	return true;
}




TaskManager::TaskManager (CGlobals *g) : aiHandler(g)
{
	if (!config.Load (g->sidecfg))
		throw "Failed to load build handler configuration";

	initialBuildOrderTask = 0;
	jobFinderIterator = 0;
	initialBuildOrdersFinished=false;
}


TaskManager::~TaskManager ()
{}

aiUnit* TaskManager::Init (int id)
{
	// Add the commander to the builder list
	BuildUnit *b = new BuildUnit;
	b->owner = this;
	b->def = globals->cb->GetUnitDef (id);
	b->id = id;

	builders.add (b);

	// allocate space for the builder types
	currentBuilders.resize (buildTable.numDefs);
	currentUnitBuilds.resize (buildTable.numDefs);

	InitBuilderTypes (globals->cb->GetUnitDef (id));

	// initialize initial build orders
	if (config.InitialOrders) 
		initialBuildOrderState.resize (config.InitialOrders->builds.size(), 0);

	return b;
}

void TaskManager::InitBuilderTypes (const UnitDef* commdr)
{
	for (int a=0;a<buildTable.numDefs;a++)
	{
		BuildTable::UDef *d = &buildTable.deflist [a];

		// Builder?
		if (!d->IsBuilder() || d->IsShip())
			continue;

		// Can it be build directly or indirectly?
		BuildPathNode& bpn = buildTable.GetBuildPath (commdr->id, a);

		if (bpn.id >= 0)
			builderTypes.push_back (a + 1);
	}
}


void TaskManager::AddTask (aiTask *task)
{
	tasks.add(task);
	task->Initialize (globals);
	task->startFrame=globals->cb->GetCurrentFrame();

	ChatDebugPrintf (globals->cb, "Added task: %s", task->GetDebugName (globals).c_str());
}


//TODO: Put builder resources in the comparision too
const UnitDef* TaskManager::FindBestBuilderType (const UnitDef* constr, BuildUnit **builder)
{
	vector<int>& bl = buildTable.buildby [constr->id-1];

	int best=-1;
	int bs;

	for (int a=0;a<bl.size();a++)
	{
		int v = currentBuilders[bl[a]];

        if (currentBuilders[bl[a]] < 0)
		{
			if (builder) 
				*builder=FindInactiveBuilderForTask (constr);

			return buildTable.GetDef (bl[a]+1);
		}

		v=abs(v);

		if (best < 0 || bs<v)
		{
			best = bl[a];
			bs = v;
		}
	}

	return best >= 0 ? buildTable.GetDef (best+1) : 0;
}

void TaskManager::FindNewJob (BuildUnit *u)
{
	BuildTable::UDef *cd = buildTable.GetCachedDef (u->def->id);

	if (cd->IsBuilding ()) {
		// factory - find a task that is not yet initialized
		for (int a=0;a<tasks.size();a++) {
			aiTask *t = tasks[a];
			BuildTask *bt = dynamic_cast<BuildTask*>(t);

			if (bt && bt->isAllocated && !bt->depends && bt->constructors.empty () && 
				!bt->IsBuilding() && buildTable.UnitCanBuild (u->def,bt->def))
			{
				u->AddTask (t);
				return;
			}
		}
	} else {
        // mobile builder
		aiTask *best=0;
		float bestScore,score;

		for (int a=0;a<tasks.size();a++) {
			if (tasks[a]->CalculateScore (u, globals, score) && (!best || score > bestScore))
			{
				bestScore=score;
				best=tasks[a];
			}
		}

		if (best) {
			u->AddTask (best);
		}
	}
}

void TaskManager::OrderNewBuilder()
{
	float bestScore;
	int best=0;

	for (int a=0;a<builderTypes.size();a++)	{
		assert (builderTypes[a]);
		BuildTable::UDef *cd = buildTable.GetCachedDef (builderTypes[a]);

		if (cd->IsBuilding())
			continue;

		// Find the best unit we can use to build this builder
		const vector<int>& bb = *cd->buildby;
		int numBuilders=0;

		for (int i=0;i<bb.size();i++)
		{
			int n = currentBuilders [bb[i]];
			numBuilders += abs(n) + (n<0)?2:0; // an inactive builder equals 2 extra active builders
		}

		float score = std::min((1+numBuilders),3) * sqrtf(cd->numBuildOptions) * cd->buildSpeed / globals->resourceManager->ResourceValue(cd->cost);
		if (!best || score > bestScore)
		{
			best = builderTypes[a];
			bestScore = score;
		}
	}

	if (best)
	{
		BuildTask *t = new BuildTask (buildTable.GetDef (best));
		AddTask (t);
		ChatDebugPrintf (globals->cb, "Added task to increase build speed: %s\n", t->def->name.c_str());
	}
}

// Is there a builder that can build this unit
bool TaskManager::CanBuildUnit (const UnitDef *def)
{
	BuildTable::UDef* cd = buildTable.GetCachedDef (def->id);
	const vector<int>& bb = *cd->buildby;

	for (int a=0;a<bb.size();a++) {
		if (currentBuilders[bb[a]])
			return true;
	}
	return false;
}


// TODO: Fix this, heuristic is crap
BuildUnit* TaskManager::FindInactiveBuilderForTask (const UnitDef *def)
{
	int best = -1;
	float best_score;

	for (int a=0;a<inactive.size();a++)
	{
		BuildUnit *b = inactive[a];

		// It's already set
		if (!b->tasks.empty ())
			continue; 

		if (!buildTable.UnitCanBuild (b->def, def))
			continue;

		BuildPathNode& node = buildTable.GetBuildPath (b->def, def);

		float score = globals->resourceManager->ResourceValue (node.res.energy, node.res.metal) + node.res.buildtime;
		if (best<0 || best_score > score)
		{
			best_score = score;
			best=a;
		}
	}

	if (best >= 0)
		return inactive [best];

	return 0;
}


void TaskManager::RemoveUnitBlocking(const UnitDef* def, const float3& pos)
{
	if (!buildTable.GetCachedDef (def->id)->IsBuilding ())
		return;

	globals->buildmap->Mark (def, pos, false);
}


void TaskManager::Update()
{
	float totalBuildSpeed = 0.0f;

	// clear the current set of unit builds
	fill(currentUnitBuilds.begin(), currentUnitBuilds.end(), 0);

	// clear builder type counts
	fill(currentBuilders.begin(), currentBuilders.end(), 0);

	ResourceManager *rm = globals->resourceManager;
	rm->totalBuildPower = ResourceInfo();

	// put all the empty builders in the inactive list
	for (ptrvec<BuildUnit>::iterator i = builders.begin();i != builders.end(); i++)
	{
		int nb = currentBuilders[i->def->id-1];

		BuildTable::UDef *cd=buildTable.GetCachedDef(i->def->id);
		if (!cd->IsBuilding ()){
			float s= UnitTypeBuildSpeed(i->def);
			totalBuildSpeed += s;
		}

		if (nb < 0) nb--;
		else nb++;

		if (i->tasks.empty())
		{
			inactive.push_back (&*i);
			if (nb > 0) nb=-nb;
		} 
		else {
			ResourceInfo res = i->GetResourceUse ();
			aiTask *t = i->tasks.front();
			rm->totalBuildPower += res;
		}

		currentBuilders [i->def->id-1] = nb;
	}

	// update tasks and remove finished tasks
	int tIndex = 0;
	while (tIndex < tasks.size())
	{
		aiTask *t = tasks[tIndex];

		if (!t->Update (globals))
		{
			// delete the task
			tasks.del (t);
			continue;
		}

		if (dynamic_cast<BuildTask*>(t)) {
			BuildTask *bt=(BuildTask*)t;
			if (bt->IsBuilder() && !bt->IsBuilding ()) // the build speed of the unfinished builders is also counted, 
			{	                // so the OrderNewBuilder() is stable
				float s=UnitTypeBuildSpeed (bt->def);
				totalBuildSpeed += s;
			}
			currentUnitBuilds[bt->def->id - 1] ++;
		}
		tIndex ++;
	}

	rm->UpdateBuildMultiplier ();

	if (!config.InitialOrders || !DoInitialBuildOrders ())
	{
		for (int type=0;type<NUM_TASK_TYPES;type++) 
			BalanceResourceUsage (type);

		if (totalBuildSpeed < rm->averageProd.metal * config.BuildSpeedPerMetalIncome)
			OrderNewBuilder ();
	}

	stat.totalBuildSpeed=totalBuildSpeed;
	stat.minBuildSpeed=rm->averageProd.metal * config.BuildSpeedPerMetalIncome;

	for (int a=0;a<builders.size();a++) {
		BuildUnit *u = builders[a];

		if (u->activeTask)// && u->activeTask->lead == u)
			CheckBuildError (u);

		if (!u->tasks.empty())
		{
			// Make the front task the active task again
			if (u->tasks.front() != u->activeTask) {
				u->activeTask = u->tasks.front();
				u->activeTask->CommandBuilder (u, globals);
			}
		}
	}

	// perform FindNewJob on inactive builders
	if (jobFinderIterator >= builders.size())
		jobFinderIterator = 0;

	int num = 1+builders.size()/8;
	for (;jobFinderIterator<builders.size() && num>0;jobFinderIterator++)
		if (builders[jobFinderIterator]->tasks.empty())
		{
			FindNewJob (builders[jobFinderIterator]);
			num--;
		}

	// inactive is not valid outside Update()
	inactive.clear ();
}


bool TaskManager::DoInitialBuildOrders ()
{
    // initial build orders completed?
	assert (config.InitialOrders);

	if (initialBuildOrderTask)
		return true;

	for (int a=0;a<initialBuildOrderState.size();a++) {
		int &state = initialBuildOrderState[a];
		if (state < config.InitialOrders->builds[a]->count) {
			int type=-1;
			int id=config.InitialOrders->builds[a]->def;
			BuildTable::UDef *cd = buildTable.GetCachedDef(id);

			CfgList* info = config.InitialOrders->builds[a]->info;
			const char *handler = info ? info->GetLiteral ("Handler") : 0;
			if (handler) {
				int h;
				for (h=0;h<NUM_TASK_TYPES;h++) {
					if(!STRCASECMP(handler, ResourceManager::handlerStr[h])) {
						type=h;
						break;
					}
				}
				
				if (h == BTF_Force) {
					ChatMsgPrintf (globals->cb, "Error: No force units possible in the initial build orders.\n");
					state=config.InitialOrders->builds[a]->count;
					return false;
				}
			} else {
				if(!cd->IsBuilder())  {
					ChatMsgPrintf (globals->cb, "Error in initial buildorder list: unit %s is not a builder\n", cd->name.c_str());
					state=config.InitialOrders->builds[a]->count;
					return false;
				}
			}

			BuildTask *t = initialBuildOrderTask = new BuildTask(buildTable.GetDef(id));
			t->resourceType = type;
			t->destHandler = (type>=0) ? globals->taskFactories [type] : 0;
			AddTask (t);

			ResourceManager *rm=globals->resourceManager;
			rm->TakeResources (rm->ResourceValue (t->def->energyCost, t->def->metalCost));
			t->isAllocated=true;

			state ++;
			return true;
		}
	}

	// all initial build tasks are done
	if (!initialBuildOrdersFinished) {
		ChatDebugPrintf (globals->cb,"Initial build orders finished.");

		globals->resourceManager->DistributeResources ();
		initialBuildOrdersFinished=true;
	}
	return false;
}

// Check for a timeout in the task handling by the builder
// - Is the command actually in the unit's command que?
// - Does the builder move when it's not yet on the building location?
void TaskManager::CheckBuildError (BuildUnit *builder)
{}


aiUnit* TaskManager::UnitCreated (int id)
{
	float bestDis;
	BuildTask *task=0;
	aiUnit *unit = 0;

	float3 pos = globals->cb->GetUnitPos (id);
	const UnitDef* def = globals->cb->GetUnitDef (id);

	for (int a=0;a<tasks.size();a++) {
		BuildTask *bt = dynamic_cast<BuildTask*>(tasks[a]);
        if (bt && !bt->unit && bt->lead && bt->def == def)
		{
			float d = pos.distance2D (bt->pos);
			if (!task || d < bestDis) {
				task = bt;
				bestDis = d;
			}
		}
	}

	if (!task || bestDis > 200)
	{
		// TODO: Find a way to deal with these outcasts...
		return new aiUnit;
	}

	if (task->IsBuilder())
	{
		BuildUnit *bu = new BuildUnit;
		bu->owner = this;
		task->unit = bu;
		bu->id = id;
		bu->def = task->def;

		task->unit = bu;
		task->AddDeathDependence (bu);
		logPrintf ("New builder created. %s\n", task->def->name.c_str());
		return bu;
	}

	assert (task->destHandler);
	unit = task->destHandler->CreateUnit (id,task);
	unit->owner = this;
	task->AddDeathDependence (unit);
	task->unit = unit;
	return unit;
}


void TaskManager::UnitDestroyed (aiUnit *unit)
{
	RemoveUnitBlocking (unit->def, globals->cb->GetUnitPos (unit->id));

	if (unit->flags & UNIT_FINISHED)
	{
		BuildUnit *bu = dynamic_cast<BuildUnit*>(unit);

		logPrintf ("UnitDestroyed: Builder %s removed (%p)\n", unit->def->name.c_str(), unit);
		if (bu->activeTask) bu->activeTask->BuilderDestroyed (bu);

		assert(bu);
		builders.del (bu);
	}
	else
	{
		assert (unit->owner);
		
		unit->owner->UnfinishedUnitDestroyed (unit);
		delete unit;
	}
}


void TaskManager::FinishConstructedUnit(BuildTask *t)
{
	if (t->IsBuilder()) {
		BuildUnit* bu = (BuildUnit *)t->unit;
		logPrintf ("Builder %s finished: now %d builders\n",bu->def->name.c_str(), builders.size()+1);
		builders.add (bu);
	}
	else {
		t->unit->owner = t->destHandler;
		t->destHandler->UnitFinished (t->unit);
		logPrintf ("Build Task %s completed for %s\n", t->def->name.c_str(), t->destHandler->GetName ());
	}

	if (t==initialBuildOrderTask)
		initialBuildOrderTask=0;

	if (t->IsBuilder ())
		currentBuilders [t->def->id-1] = -abs(currentBuilders [t->def->id-1])-1;
}

void TaskManager::UnitMoveFailed(aiUnit *unit)
{
	BuildUnit *u = dynamic_cast <BuildUnit *> (unit);

	assert (u);

	if (u) {
		if (u->activeTask) {
			u->activeTask->retries ++;
			u->activeTask->BuilderMoveFailed (u,globals);

			if (u->activeTask->retries == aiTask::MaxRetries) {
				tasks.del (u->activeTask);
				assert (u->activeTask==0);
			}
		}

		ReclaimFeaturesTask *rft=new ReclaimFeaturesTask;

		rft->pos = globals->cb->GetUnitPos (unit->id);
		rft->range = 50;

		AddTask (rft);

		// remove the current task, and put it on this new task
		if (u->activeTask)
			u->RemoveTask (u->activeTask);
		u->AddTask (rft);
	}
}


/* Orders new tasks until all resources have been allocated for the current task type */
void TaskManager::BalanceResourceUsage (int type)
{
	TaskFactory *h = globals->taskFactories [type];
	ResourceManager *rm = globals->resourceManager;

	BuildTask *unalloc = 0;
	for (int a=0;a<tasks.size();a++){
		BuildTask *t = dynamic_cast <BuildTask*>(tasks[a]);
		if (t && t->resourceType == type) {
			if (!t->isAllocated)
			{
				unalloc = t;
				break;
			}
		}
	}

	if (unalloc) {
		unalloc->isAllocated = rm->AllocateForTask (unalloc->def->energyCost, unalloc->def->metalCost, type);
	} else {
		int size = h->activeTasks.size();
		if (size >= rm->config.MaxTasks [type])
			return;

        BuildTask* task = h->GetNewBuildTask();
		if (task) {
			task->resourceType = type;

			if (!task->destHandler)
				task->destHandler = h;

			task->isAllocated = rm->AllocateForTask (task->def->energyCost, task->def->metalCost, type);
			AddTask (task);
		} else {
			// give the resources to other handlers
			ResourceInfo curIncome (globals->cb->GetEnergyIncome (), globals->cb->GetMetalIncome ());
			rm->SpillResources (curIncome * (1/30.0f),type);
		}
	}
}

void TaskManager::ChatMsg (const char *msg, int player)
{
	if (!STRCASECMP(msg, ".tasks"))
	{
		for (int a=0;a<tasks.size();a++ )
			ChatMsgPrintf (globals->cb, "%d Task: %s", tasks[a]->startFrame, tasks[a]->GetDebugName(globals).c_str());
	}
	if (!STRCASECMP(msg, ".builders"))
	{
		for (int a=0;a<builders.size();a++)
		{
			ChatMsgPrintf (globals->cb, "Builder %s: task: %s", builders[a]->def->name.c_str(), 
				builders[a]->activeTask ? builders[a]->activeTask->GetDebugName (globals).c_str() : "no task");
		}
	}
}

// -----------------------------------------
// BuildUnit
// -----------------------------------------


BuildUnit::BuildUnit()
{
	index=0; 
	activeTask = 0;
	isDefending=false;
}

BuildUnit::~BuildUnit()
{}

void BuildUnit::UnitFinished ()
{}

void BuildUnit::DependentDied (aiObject *obj)
{
	if (obj == activeTask)
		activeTask = 0;

	vector<aiTask*>::iterator i = remove_if (tasks.begin(),tasks.end(), bind(equal_to<aiObject*>(), _1, obj));
	assert (i != tasks.end());
	tasks.erase (i, tasks.end());
}

ResourceInfo BuildUnit::GetResourceUse ()
{
	if (tasks.empty())
		return ResourceInfo (0.0f,0.0f);

	BuildTask *t = dynamic_cast<BuildTask*>(tasks.front());
	if (t) {
		float speed = def->buildSpeed / t->def->buildTime;
		float metal = speed * t->def->metalCost;
		float energy = speed * t->def->energyCost;

		return ResourceInfo (energy, metal);
	}
	return ResourceInfo();
}


void BuildUnit::RemoveTask (aiTask *task)
{
	task->DependentDied (this);
	task->DeleteDeathDependence (this);

	DependentDied (task);
	DeleteDeathDependence (task);

	task->buildSpeed -= def->buildSpeed;
	
	if (activeTask == task)
		activeTask=0;
}

void BuildUnit::AddTask (aiTask *task)
{
	tasks.push_back (task);
	AddDeathDependence (task);

	task->constructors.push_back (this);
	task->AddDeathDependence (this);

	task->buildSpeed += def->buildSpeed;

	logPrintf ("Added task to %s\n", def->name.c_str());
}

